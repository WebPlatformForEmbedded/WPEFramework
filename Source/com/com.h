#pragma once

#include "Administrator.h"
#include "Communicator.h"
#include "IRPCIterator.h"
#include "IRemoteLinker.h"
#include "IStringIterator.h"
#include "ITracing.h"
#include "IUnknown.h"
#include "IValueIterator.h"
#include "RemoteLinker.h"
#include "Ids.h"
#include "Messages.h"

#ifdef __WINDOWS__
#pragma comment(lib, "com.lib")
#endif
