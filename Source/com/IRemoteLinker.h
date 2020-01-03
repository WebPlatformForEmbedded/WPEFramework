#pragma once

#include "Module.h"
#include "IUnknown.h"


namespace WPEFramework {
namespace RPC {

    struct IRemoteLinker : virtual public Core::IUnknown {

        enum { ID = ID_REMOTELINKER };

        virtual uint32_t Link(const string& remoteAddress, const uint32_t interface, const uint32_t exchangeId) = 0;
        virtual uint32_t Unlink() = 0;
    };
}
}
