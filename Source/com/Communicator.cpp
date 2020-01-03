#include "Communicator.h"

#include <limits>
#include <memory>
#include "interfaces/IRemoteInvocation.h"


namespace WPEFramework {
namespace RPC {

    class ProcessShutdown;

    static constexpr uint32_t DestructionStackSize = 64 * 1024;
    static Core::ProxyPoolType<RPC::AnnounceMessage> AnnounceMessageFactory(2);
    static Core::TimerType<ProcessShutdown>& _destructor = Core::SingletonType<Core::TimerType<ProcessShutdown>>::Instance(DestructionStackSize, "ProcessDestructor");

    class ClosingInfo {
    public:
        ClosingInfo& operator=(const ClosingInfo& RHS) = delete;
        ClosingInfo(const ClosingInfo& copy) = delete;

        virtual ~ClosingInfo() = default;

    protected:
        ClosingInfo() = default;

    public:
        virtual uint32_t AttemptClose(const uint8_t iteration) = 0; //shoud return 0 if no more iterations needed
    };

    class ProcessShutdown {
    public:
        template <class IMPLEMENTATION, typename... Args>
        static void Start(Args... args)
        {

            std::unique_ptr<IMPLEMENTATION> handler(new IMPLEMENTATION(args...));

            uint32_t nextinterval = handler->AttemptClose(0);

            if (nextinterval != 0) {
                _destructor.Schedule(Core::Time::Now().Add(nextinterval), ProcessShutdown(std::move(handler)));
            }
        }

    public:
        ProcessShutdown& operator=(const ProcessShutdown& RHS) = delete;
        ProcessShutdown(const ProcessShutdown& copy) = delete;

        ProcessShutdown(ProcessShutdown&& rhs)
            : _handler(std::move(rhs._handler))
            , _cycle(rhs._cycle)
        {
        }

        explicit ProcessShutdown(std::unique_ptr<ClosingInfo>&& handler)
            : _handler(std::move(handler))
            , _cycle(1)
        {
        }

        ~ProcessShutdown() = default;

    public:
        uint64_t Timed(const uint64_t scheduledTime)
        {
            uint64_t result = 0;

            uint32_t nextinterval = _handler->AttemptClose(_cycle++);

            if (nextinterval != 0) {
                result = Core::Time(scheduledTime).Add(nextinterval).Ticks();
            } else {
                ASSERT(_cycle != std::numeric_limits<uint8_t>::max()); //too many attempts trying to kill the process
            }

            return (result);
        }

    private:
        std::unique_ptr<ClosingInfo> _handler;
        uint8_t _cycle;
    };

    class LocalClosingInfo : public ClosingInfo {
    public:
        LocalClosingInfo& operator=(const LocalClosingInfo& RHS) = delete;
        LocalClosingInfo(const LocalClosingInfo& copy) = delete;

        virtual ~LocalClosingInfo() = default;

    private:
        friend class ProcessShutdown;

        explicit LocalClosingInfo(const uint32_t pid)
            : ClosingInfo()
            , _process(pid)
        {
        }

    protected:
        uint32_t AttemptClose(const uint8_t iteration) override
        {
            uint32_t nextinterval = 0;
            if (_process.IsActive() != false) {
                switch (iteration) {
                case 0:
                    _process.Kill(false);
                    nextinterval = 10000;
                    break;
                case 1:
                    _process.Kill(true);
                    nextinterval = 4000;
                    break;
                default:
                    // This should not happen. This is a very stubbern process. Can be killed.
                    ASSERT(false);
                    break;
                }
            }
            return nextinterval;
        }

    private:
        Core::Process _process;
    };


#ifdef PROCESSCONTAINERS_ENABLED

    class ContainerClosingInfo : public ClosingInfo {
    public:
        ContainerClosingInfo& operator=(const ContainerClosingInfo& RHS) = delete;
        ContainerClosingInfo(const ContainerClosingInfo& copy) = delete;

        virtual ~ContainerClosingInfo()
        {
            _container->Release();
        }

    private:
        friend class ProcessShutdown;

        explicit ContainerClosingInfo(ProcessContainers::IContainerAdministrator::IContainer* container)
            : ClosingInfo()
            , _process(static_cast<uint32_t>(container->Pid()))
            , _container(container)
        {
            container->AddRef();
        }

    protected:
        uint32_t AttemptClose(const uint8_t iteration) override
        {
            uint32_t nextinterval = 0;
            if ((_process.Id() != 0 && _process.IsActive() == true) || (_process.Id() == 0 && _container->IsRunning() == true)) {
                switch (iteration) {
                case 0: {
                    if (_process.Id() != 0) {
                        _process.Kill(false);
                    } else {
                        _container->Stop(0);
                    }
                }
                    nextinterval = 10000;
                    break;
                case 1: {
                    if (_process.Id() != 0) {
                        _process.Kill(true);
                        nextinterval = 4000;
                    } else {
                        ASSERT(false);
                        nextinterval = 0;
                    }
                } break;
                case 2:
                    _container->Stop(0);
                    nextinterval = 5000;
                    break;
                default:
                    // This should not happen. This is a very stubbern process. Can be killed.
                    ASSERT(false);
                    break;
                }
            }
            return nextinterval;
        }

    private:
        Core::Process _process;
        ProcessContainers::IContainerAdministrator::IContainer* _container;
    };

#endif

    /* static */ std::atomic<uint32_t> Communicator::RemoteConnection::_sequenceId(1);

    static void LoadProxyStubs(const string& pathName)
    {
        static std::list<Core::Library> processProxyStubs;

        Core::Directory index(pathName.c_str(), _T("*.so"));

        while (index.Next() == true) {
            // Check if this ProxySTub file is already loaded in this process space..
            std::list<Core::Library>::const_iterator loop(processProxyStubs.begin());
            while ((loop != processProxyStubs.end()) && (loop->Name() != index.Current())) {
                loop++;
            }

            if (loop == processProxyStubs.end()) {
                Core::Library library(index.Current().c_str());

                if (library.IsLoaded() == true) {
                    processProxyStubs.push_back(library);
                }
            }
        }
    }

    /* virtual */ uint32_t Communicator::RemoteConnection::Id() const
    {
        return (_id);
    }

    /* virtual */ void* Communicator::RemoteConnection::QueryInterface(const uint32_t id)
    {
        if (id == IRemoteConnection::ID) {
            AddRef();
            return (static_cast<IRemoteConnection*>(this));
        } else if (id == Core::IUnknown::ID) {
            AddRef();
            return (static_cast<Core::IUnknown*>(this));
        }
        return (nullptr);
    }

    /* virtual */ void* Communicator::RemoteConnection::Aquire(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version)
    {
        void* result(nullptr);

        if (_channel.IsValid() == true) {
            Core::ProxyType<RPC::AnnounceMessage> message(AnnounceMessageFactory.Element());

            TRACE_L1("Aquiring object through RPC: %s, 0x%04X [%s]", className.c_str(), interfaceId, RemoteId().c_str());

            message->Parameters().Set(_id, className, interfaceId, version);

            uint32_t feedback = _channel->Invoke(message, waitTime);

            if (feedback == Core::ERROR_NONE) {
                RPC::instanceId_t instanceId = message->Response().InstanceId();

                if (instanceId != RPC::EmptyInstance) {
                    // From what is returned, we need to create a proxy
                    ProxyStub::UnknownProxy* instance = RPC::Administrator::Instance().ProxyInstance(Core::ProxyType<Core::IPCChannel>(_channel), instanceId, interfaceId, true, interfaceId, false);
                    result = (instance != nullptr ? instance->QueryInterface(interfaceId) : nullptr);
                }
            }
        }

        return (result);
    }

    /* virtual */ void Communicator::RemoteConnection::Terminate()
    {
        Close();
    }

    /* virtual */ string Communicator::RemoteConnection::RemoteId() const
    {
        return _channel->Source().RemoteId();
    }

    /* virtual */ string Communicator::RemoteConnection::LocalId() const
    {
        return _channel->Source().LocalId();
    }

    /* virtual */ Communicator::RemoteConnection::Type Communicator::RemoteConnection::ConnectionType() const
    {
        return Local;
    }

    /* virtual */ uint32_t Communicator::RemoteConnection::ProcessId() const
    {
        return _processId;
    }

    /* virtual */  string Communicator::MonitorableRemoteProcess::Callsign() const 
    {
        return _callsign;
    }

    /* virtual */  void* Communicator::MonitorableRemoteProcess::QueryInterface(const uint32_t id) 
    {
        if (id == IRemoteConnection::IProcess::ID) {
            AddRef();
            return (static_cast<IRemoteConnection::IProcess*>(this));
        } 

        return RemoteProcess::QueryInterface(id);
    }

    /* virtual */ void Communicator::LocalRemoteProcess::Terminate()
    {
        // Do not yet call the close on the connection, the otherside might close down decently and release all opened interfaces..
        // Just submit our selves for destruction !!!!

        // Time to shoot the application, it will trigger a close by definition of the channel, if it is still standing..
        if (_id != 0) {
            ProcessShutdown::Start<LocalClosingInfo>(_id);
            _id = 0;
        }
    }

    uint32_t Communicator::LocalRemoteProcess::ProcessId() const
    {
        return _id;
    }

#ifdef PROCESSCONTAINERS_ENABLED

    void Communicator::ContainerRemoteProcess::Terminate()
    {
        ASSERT(_container != nullptr);
        if (_container != nullptr) {
            ProcessShutdown::Start<ContainerClosingInfo>(_container);
        }
    }

#endif

#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif

#ifdef REMOTEINVOCATION_ENABLED
    class DirectIPCServer : public Core::IIPCServer {
    public: 
        void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& message) {
            // This class should only by used for invoking remote methods!
            ASSERT(message->Label() == InvokeMessage::Id());

            Job(source, message, nullptr).Dispatch();
        }
    };

    // Helper template for calling functions of IRemoteInvocation on remote device
    // Syntax friendly to inlining argument function
    template<typename FuncType>
    void RemoteInvocationCall(const string& remoteAddress, FuncType&& processFunction) 
    {
        auto _engine = Core::ProxyType<DirectIPCServer>::Create();

        if (_engine.IsValid()) {
            auto _client = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId(remoteAddress.c_str()), Core::ProxyType<Core::IIPCServer>(_engine));

            if (_client.IsValid()) {
                Exchange::IRemoteInvocation* _remote = _client->Open<Exchange::IRemoteInvocation>(_T("RemoteInvocation"), ~0, 2000);

                if (_remote != nullptr) {
                    
                    processFunction(_remote);           

                    _remote->Release();
                } else {
                    TRACE_L1("Could not open remote communicaiton with service on address %s", remoteAddress.c_str());
                }

                _client.Release();
            } else {
                TRACE_L1("Failed to create CommunicatorClient while trying to remote launch on address %s", remoteAddress.c_str());
            }

            _engine.Release();
        } else {
            TRACE_L1("Failed to create InvokeServer while trying to remote launch on address %s", remoteAddress.c_str());
        }
    }

    Communicator::RemoteHost::RemoteHost(const string& callsign, const string& remoteAddress)
        : MonitorableRemoteProcess(callsign)
        , _connectionId(0)
        , _remoteAddress(remoteAddress)
    {
        
    }

    void Communicator::RemoteHost::Launch(const Object& instance, const Config& config) 
    {
        auto remoteCall = [this, &instance, &config] (Exchange::IRemoteInvocation* remote) {
            uint32_t loggingSettings = (Logging::LoggingType<Logging::Startup>::IsEnabled() ? 0x01 : 0) | (Logging::LoggingType<Logging::Shutdown>::IsEnabled() ? 0x02 : 0) | (Logging::LoggingType<Logging::Notification>::IsEnabled() ? 0x04 : 0);

            uint16_t port = Core::NodeId(config.Connector().c_str()).PortNumber();

            remote->LinkByCallsign(port, instance.Interface(), Id(), instance.Callsign());
        };

        RemoteInvocationCall(instance.RemoteAddress(), remoteCall);
    }

    class RemoteHostTerminator : public Core::IDispatch {
    public: 
        RemoteHostTerminator(string remoteAddress, uint32_t connectionId)
            : _remoteAddress(remoteAddress)
            , _connectionId(connectionId)
        {
        }

        void Dispatch() override {
            uint32_t id = _connectionId;

            RemoteInvocationCall(_remoteAddress, [id] (Exchange::IRemoteInvocation* remote) {
                remote->Unlink(id);
            });
        }
    private:
        string _remoteAddress;
        uint32_t _connectionId;
    };

    void Communicator::RemoteHost::Terminate() 
    {
        auto job = Core::ProxyType<RemoteHostTerminator>::Create(_remoteAddress, Id());
        Core::WorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(job));
    }
#endif

    Communicator::Communicator(const Core::NodeId& node, const string& proxyStubPath)
        : _connectionMap(*this)
        , _ipcServer(node, _connectionMap, proxyStubPath)
    {
        if (proxyStubPath.empty() == false) {
            RPC::LoadProxyStubs(proxyStubPath);
        }
        // These are the elements we are expecting to receive over the IPC channels.
        _ipcServer.CreateFactory<AnnounceMessage>(1);
        _ipcServer.CreateFactory<InvokeMessage>(3);
    }

    Communicator::Communicator(
        const Core::NodeId& node,
        const string& proxyStubPath,
        const Core::ProxyType<Core::IIPCServer>& handler)
        : _connectionMap(*this)
        , _ipcServer(node, _connectionMap, proxyStubPath, handler)
    {
        if (proxyStubPath.empty() == false) {
            RPC::LoadProxyStubs(proxyStubPath);
        }
        // These are the elements we are expecting to receive over the IPC channels.
        _ipcServer.CreateFactory<AnnounceMessage>(1);
        _ipcServer.CreateFactory<InvokeMessage>(3);
    }

    /* virtual */ Communicator::~Communicator()
    {
        // Make sure any closed channel is cleared before we start validating the end result :-)
        _ipcServer.Cleanup();

        // Close all communication paths...
        _ipcServer.Close(Core::infinite);

        // Warn but we need to clos up existing connections..
        _connectionMap.Destroy();
    }

    CommunicatorClient::CommunicatorClient(
        const Core::NodeId& remoteNode)
        : Core::IPCChannelClientType<Core::Void, false, true>(remoteNode, CommunicationBufferSize)
        , _announceMessage(Core::ProxyType<RPC::AnnounceMessage>::Create())
        , _announceEvent(false, true)
        , _handler(this)
    {
        CreateFactory<RPC::AnnounceMessage>(1);
        CreateFactory<RPC::InvokeMessage>(2);

        Register(RPC::InvokeMessage::Id(), Core::ProxyType<Core::IIPCServer>(Core::ProxyType<InvokeHandlerImplementation>::Create()));
        Register(RPC::AnnounceMessage::Id(), Core::ProxyType<Core::IIPCServer>(Core::ProxyType<AnnounceHandlerImplementation>::Create(this)));
    }

    CommunicatorClient::CommunicatorClient(
        const Core::NodeId& remoteNode,
        const Core::ProxyType<Core::IIPCServer>& handler)
        : Core::IPCChannelClientType<Core::Void, false, true>(remoteNode, CommunicationBufferSize)
        , _announceMessage(Core::ProxyType<RPC::AnnounceMessage>::Create())
        , _announceEvent(false, true)
        , _handler(this)
    {
        CreateFactory<RPC::AnnounceMessage>(1);
        CreateFactory<RPC::InvokeMessage>(2);

        BaseClass::Register(RPC::InvokeMessage::Id(), handler);
        BaseClass::Register(RPC::AnnounceMessage::Id(), handler);
    }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif

    CommunicatorClient::~CommunicatorClient()
    {
        BaseClass::Close(Core::infinite);

        BaseClass::Unregister(RPC::InvokeMessage::Id());
        BaseClass::Unregister(RPC::AnnounceMessage::Id());

        DestroyFactory<RPC::InvokeMessage>();
        DestroyFactory<RPC::AnnounceMessage>();
    }

    uint32_t CommunicatorClient::Open(const uint32_t waitTime)
    {
        ASSERT(BaseClass::IsOpen() == false);
        _announceEvent.ResetEvent();

        //do not set announce parameters, we do not know what side will offer the interface
        _announceMessage->Parameters().Set(Core::ProcessInfo().Id());

        uint32_t result = BaseClass::Open(waitTime);

        if ((result == Core::ERROR_NONE) && (_announceEvent.Lock(waitTime) != Core::ERROR_NONE)) {
            result = Core::ERROR_OPENING_FAILED;
        }

        return (result);
    }

    uint32_t CommunicatorClient::Open(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version)
    {
        ASSERT(BaseClass::IsOpen() == false);
        _announceEvent.ResetEvent();

        _announceMessage->Parameters().Set(Core::ProcessInfo().Id(), className, interfaceId, version);

        uint32_t result = BaseClass::Open(waitTime);

        if ((result == Core::ERROR_NONE) && (_announceEvent.Lock(waitTime) != Core::ERROR_NONE)) {
            result = Core::ERROR_OPENING_FAILED;
        }

        return (result);
    }

    uint32_t CommunicatorClient::Open(const uint32_t waitTime, const uint32_t interfaceId, void* implementation, const uint32_t exchangeId)
    {
        ASSERT(BaseClass::IsOpen() == false);
        _announceEvent.ResetEvent();

        // TODO: Check if we can register object before channel is valid?
        Core::ProxyType<Core::IPCChannel> refChannel(*this);
        RPC::instanceId_t instanceId = RPC::Administrator::Instance().RegisterInterface(refChannel, implementation, interfaceId);

        _announceMessage->Parameters().Set(Core::ProcessInfo().Id(), interfaceId, instanceId, exchangeId);

        uint32_t result = BaseClass::Open(waitTime);

        if ((result == Core::ERROR_NONE) && (_announceEvent.Lock(waitTime) != Core::ERROR_NONE)) {
            result = Core::ERROR_OPENING_FAILED;
        }

        return (result);
    }

    uint32_t CommunicatorClient::Close(const uint32_t waitTime)
    {
        return (BaseClass::Close(waitTime));
    }

    /* virtual */ void CommunicatorClient::StateChange()
    {
        BaseClass::StateChange();

        if (BaseClass::Source().IsOpen()) {
            TRACE_L1("Invoking the Announce message to the server. %d", __LINE__);
            uint32_t result = Invoke<RPC::AnnounceMessage>(_announceMessage, this);

            if (result != Core::ERROR_NONE) {
                TRACE_L1("Error during invoke of AnnounceMessage: %d", result);
            } 
        } else {
            TRACE_L1("Connection to the server is down");
        }
    }

    /* virtual */ void CommunicatorClient::Dispatch(Core::IIPC& element)
    {
        // Message delivered and responded on....
        RPC::AnnounceMessage* announceMessage = static_cast<RPC::AnnounceMessage*>(&element);

        ASSERT(dynamic_cast<RPC::AnnounceMessage*>(&element) != nullptr);

        if (announceMessage->Response().IsSet() == true) {
            // Is result of an announce message, contains default trace categories in JSON format.
            string jsonDefaultCategories(announceMessage->Response().TraceCategories());

            if (jsonDefaultCategories.empty() == false) {
                Trace::TraceUnit::Instance().Defaults(jsonDefaultCategories);
            }

            string proxyStubPath(announceMessage->Response().ProxyStubPath());
            if (proxyStubPath.empty() == false) {
                // Also load the ProxyStubs before we do anything else
                RPC::LoadProxyStubs(proxyStubPath);
            }
        }

        // Set event so WaitForCompletion() can continue.
        _announceEvent.SetEvent();
    }
}
}
