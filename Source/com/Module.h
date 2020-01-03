#ifndef __MODULE_COM_H__
#define __MODULE_COM_H__

#ifndef MODULE_NAME
#define MODULE_NAME COM
#endif

#include "../core/core.h"
#include "../tracing/tracing.h"
#include "../com/com.h"

#ifdef __WINDOWS__
#undef EXTERNAL
#ifdef COM_EXPORTS
#define EXTERNAL EXTERNAL_EXPORT
#else
#define EXTERNAL EXTERNAL_IMPORT
#endif
#else
#define EXTERNAL
#endif

#endif // __MODULE_COM_H__
