#pragma once

#ifdef _DEBUG

#define DEBUG_ONLY(x) x
#define NODEBUG_ONLY(x)

#else

#define DEBUG_ONLY(x)
#define NODEBUG_ONLY(x) x

#endif
