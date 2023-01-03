#include "global.h"

#ifdef __vita__
#include <stdio.h>
#include <vitasdk.h>
int _newlib_heap_size_user = 300 * 1024 * 1024;
#endif

#if !defined(__SWITCH__) && !defined(__vita__)
#ifdef __WIIU__
void _assert(const char* exp, const char* file, s32 line) {
#else
void __assert(const char* exp, const char* file, s32 line) {
#endif
    char msg[256];

    osSyncPrintf("Assertion failed: %s, file %s, line %d, thread %d\n", exp, file, line, osGetThreadId(NULL));
    sprintf(msg, "ASSERT: %s:%d(%d)", file, line, osGetThreadId(NULL));
    Fault_AddHungupAndCrashImpl(msg, exp);
}
#endif