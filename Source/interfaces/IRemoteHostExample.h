#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IRemoteHostExample : virtual public Core::IUnknown {

        enum { ID = ID_REMOTEHOSTEXAMPLE };

        struct ITimeListener : virtual public Core::IUnknown
        {
            enum { ID = ID_REMOTEHOSTEXAMPLE_NOTIFICATION };

            virtual ~ITimeListener() {};
            virtual uint32_t TimeUpdate(string time) = 0;
        };

        virtual ~IRemoteHostExample() {}

        virtual uint32_t Initialize(PluginHost::IShell* service) = 0;
        virtual uint32_t Greet(const string& message, string& response /* @out */) = 0;

        virtual uint32_t SubscribeTimeUpdates(ITimeListener* listener) = 0;
        virtual uint32_t UnsubscribeTimeUpdates(ITimeListener* listener) = 0;
    };
}
}
