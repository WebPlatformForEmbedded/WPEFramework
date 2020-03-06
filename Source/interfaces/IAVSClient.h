#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IAVSClient : virtual public Core::IUnknown {
        enum { ID = ID_AVSCLIENT };

        virtual ~IAVSClient(){};

        virtual bool Initialize(WPEFramework::PluginHost::IShell* service, const std::string& alexaClientConfig,
            const std::string& smartScreenConfig, const std::string& pathToInputFolder, const std::string& audiosource,
            const bool enableKWD, const std::string& logLevel) = 0;
        virtual bool Deinitialize() = 0;
        virtual uint32_t Execute(uint8_t command) = 0;
        virtual void StateChange(PluginHost::IShell* audiosource) = 0;
    };

} // Exchange
} // WPEFramework
