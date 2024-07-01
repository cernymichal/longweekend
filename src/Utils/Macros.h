#pragma once

#ifdef _DEBUG

#define DEBUG(x) x
#define NODEBUG(x)

#else

#define DEBUG(x)
#define NODEBUG(x) x

#endif
