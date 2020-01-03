#pragma once

#include "Module.h"
 
namespace WPEFramework {
namespace Exchange {

    struct IRemoteInvocation : virtual public Core::IUnknown {

        enum { ID = ID_REMOTEINVOCATION };

        virtual ~IRemoteInvocation() {}

        virtual uint32_t LinkByCallsign(const uint16_t port, const uint32_t interfaceId, const uint32_t exchangeId, const string& callsign) = 0;
        virtual uint32_t Unlink(const uint32_t exchangeId) = 0;
    };
}
}