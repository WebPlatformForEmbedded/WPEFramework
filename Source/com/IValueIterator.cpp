#include "IValueIterator.h"
#include "Communicator.h"
#include "IUnknown.h"

namespace WPEFramework {
namespace ProxyStub {

    // -------------------------------------------------------------------------------------------
    // STUB
    // -------------------------------------------------------------------------------------------

    ProxyStub::MethodHandler ValueIteratorStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            uint32_t result = 0;

            // virtual bool Next(string& text) = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            bool valid = RPC::Administrator::GetImplementation<RPC::IValueIterator>(message->Parameters().InstanceId())->Next(result);

            response.Boolean(valid);

            if (valid == true) {
                response.Number(result);
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            uint32_t result = 0;

            // virtual bool Previous() = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            bool valid = RPC::Administrator::GetImplementation<RPC::IValueIterator>(message->Parameters().InstanceId())->Previous(result);

            response.Boolean(valid);

            if (valid == true) {
                response.Number(result);
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual void Reset(const uint32_t position)
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());

            uint32_t position(parameters.Number<uint32_t>());

            RPC::Administrator::GetImplementation<RPC::IValueIterator>(message->Parameters().InstanceId())->Reset(position);
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual bool IsValid() const = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Boolean(RPC::Administrator::GetImplementation<RPC::IValueIterator>(message->Parameters().InstanceId())->IsValid());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual uint32_t Count() const = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Number<uint32_t>(RPC::Administrator::GetImplementation<RPC::IValueIterator>(message->Parameters().InstanceId())->Count());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual string Current() const = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Number(RPC::Administrator::GetImplementation<RPC::IValueIterator>(message->Parameters().InstanceId())->Current());
        },
        nullptr
    };

    typedef ProxyStub::UnknownStubType<RPC::IValueIterator, ValueIteratorStubMethods> ValueIteratorStub;

    // -------------------------------------------------------------------------------------------
    // PROXY
    // -------------------------------------------------------------------------------------------
    class ValueIteratorProxy : public UnknownProxyType<RPC::IValueIterator> {
    public:
        ValueIteratorProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instanceId_t instanceId, const bool otherSideInformed)
            : BaseClass(channel, instanceId, otherSideInformed)
        {
            TRACE_L1("Constructed ValueIteratorProxy: %p", this);
        }
        virtual ~ValueIteratorProxy()
        {
            TRACE_L1("Destructed ValueIteratorProxy: %p", this);
        }

    public:
        virtual bool Next(uint32_t& result) override
        {
            bool valid = false;
            IPCMessage newMessage(BaseClass::Message(0));

            if (Invoke(newMessage) == Core::ERROR_NONE) {

                RPC::Data::Frame::Reader reader = newMessage->Response().Reader();
                valid = reader.Boolean();

                if (valid == true) {
                    result = reader.Number<uint32_t>();
                }
            }

            return (valid);
        }
        virtual bool Previous(uint32_t& result) override
        {
            bool valid = false;
            IPCMessage newMessage(BaseClass::Message(1));

            if (Invoke(newMessage) == Core::ERROR_NONE) {

                RPC::Data::Frame::Reader reader = newMessage->Response().Reader();
                valid = reader.Boolean();

                if (valid == true) {
                    result = reader.Number<uint32_t>();
                }
            }

            return (valid);
        }
        virtual void Reset(const uint32_t position) override
        {
            IPCMessage newMessage(BaseClass::Message(2));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number(position);

            Invoke(newMessage);
        }
        virtual bool IsValid() const override
        {
            bool valid = false;
            IPCMessage newMessage(BaseClass::Message(3));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                valid = newMessage->Response().Reader().Boolean();
            }
            return (valid);
        }
        virtual uint32_t Count() const override
        {
            uint32_t result = 0;
            IPCMessage newMessage(BaseClass::Message(4));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Number<uint32_t>();
            }

            return (result);
        }
        virtual uint32_t Current() const override
        {
            uint32_t result = ~0;
            IPCMessage newMessage(BaseClass::Message(5));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Number<uint32_t>();
            }

            return (result);
        }
    };

    // -------------------------------------------------------------------------------------------
    // Registration
    // -------------------------------------------------------------------------------------------

    namespace {

        class RPCInstantiation {
        public:
            RPCInstantiation()
            {
                RPC::Administrator::Instance().Announce<RPC::IValueIterator, ValueIteratorProxy, ValueIteratorStub>();
            }
            ~RPCInstantiation()
            {
            }

        } RPCRegistration;
    }
}
}
