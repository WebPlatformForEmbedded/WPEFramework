#include "RemoteLinker.h"

namespace WPEFramework {
namespace RPC {

    RemoteLinker::~RemoteLinker()
    {
        Unlink();
    }

    uint32_t RemoteLinker::Link(const string& remoteAddress, const uint32_t interface, const uint32_t exchangeId)
    {
        uint32_t result = Core::ERROR_GENERAL;

        auto remoteInvokeServer = Core::ProxyType<RPC::WorkerPoolIPCServer>::Create();
        _remoteServer = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId(remoteAddress.c_str()), Core::ProxyType<Core::IIPCServer>(remoteInvokeServer));

        if (remoteInvokeServer.IsValid()) {
            remoteInvokeServer->Announcements(_remoteServer->Announcement());

            void* requestedInterface = QueryInterface(interface);

            // Now try to create direct connection!
            result = _remoteServer->Open((RPC::CommunicationTimeOut != Core::infinite ? 2 * RPC::CommunicationTimeOut : RPC::CommunicationTimeOut), interface, requestedInterface, exchangeId);

            // QueryInterface added reference for ourselves, now we need to derefenece it
            Release();
        }
        
        return result;
    }

    uint32_t RemoteLinker::Unlink() 
    {
        if (_remoteServer.IsValid()) {
            _remoteServer->Close(2000);
            _remoteServer.Release();
        }

        ASSERT(_remoteServer.IsValid() == false);

        return Core::ERROR_NONE;
    }
}
}