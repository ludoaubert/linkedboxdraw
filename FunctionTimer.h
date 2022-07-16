#ifndef FUNCTION_TIMER
#define FUNCTION_TIMER

#include <stdio.h>

extern FILE *log_file;

void log(const char* s);

struct FunctionTimerImpl;

struct FunctionTimer
{
    FunctionTimer(const char *func_name);
    ~FunctionTimer();

    FunctionTimerImpl *impl;

    static int MAX_NESTING ;
};


struct TestFunctionTimerImpl;

struct TestFunctionTimer
{
    TestFunctionTimer(const char *func_name);
    ~TestFunctionTimer();

    TestFunctionTimerImpl *impl;
};



#endif
