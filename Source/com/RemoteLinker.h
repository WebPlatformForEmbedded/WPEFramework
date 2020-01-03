#pragma once

#include "Module.h"
#include "IRemoteLinker.h"
#include "Communicator.h"

namespace WPEFramework {
namespace RPC {

    class RemoteLinker : virtual public RPC::IRemoteLinker {
    public:
        virtual ~RemoteLinker();
        
        uint32_t Link(const string& remoteAddress, const uint32_t interface, const uint32_t exchangeId) override;
        uint32_t Unlink() override;

    private:
        Core::ProxyType<RPC::CommunicatorClient> _remoteServer;
    };

}
}