#include <cstdio>

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

extern "C" DLLEXPORT void printi(long long val)
{
    printf("%lld\n", val);
}
