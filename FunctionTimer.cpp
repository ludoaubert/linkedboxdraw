#include "FunctionTimer.h"
#include <stdio.h>
#include <chrono>
#include <time.h>
#include <string.h>
using namespace std::chrono;


FILE *log_file=nullptr;

void log(const char* s)
{
    if (log_file == nullptr)
        return;
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    char buff[100];
    strftime(buff, sizeof buff, "%T", gmtime(&ts.tv_sec));
    fprintf(log_file, "%s.%09ld %s\n", buff, ts.tv_nsec, s);
    fflush(log_file);
}

struct FunctionTimerImpl
{
    FunctionTimerImpl(const char* func_name) :
      m_func_name(func_name)
    {
        t1 = high_resolution_clock::now();
        count++;
    }

    ~FunctionTimerImpl()
    {
        count--;
        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
        printf("%.*s%s() took %f seconds.\n", count, "\t\t\t\t\t\t\t\t\t\t", m_func_name, time_span.count());
        //fflush(log_file);
    }

    static int count ;
    const char* m_func_name;
    high_resolution_clock::time_point t1;
};

int FunctionTimerImpl::count = 0;



FunctionTimer::FunctionTimer(const char *func_name)
{
    impl = new FunctionTimerImpl(func_name);
}
FunctionTimer::~FunctionTimer()
{
    delete impl;
}


struct TestFunctionTimerImpl
{
    TestFunctionTimerImpl(const char* func_name):
      m_func_name(func_name)
    {
        printf("%s()\n", m_func_name);
        t1 = high_resolution_clock::now();
    }

    ~TestFunctionTimerImpl()
    {
        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
        printf("%s() took %f seconds.\n", m_func_name, time_span.count());
    }

    const char* m_func_name;
    high_resolution_clock::time_point t1;
};


TestFunctionTimer::TestFunctionTimer(const char *func_name)
{
    impl = new TestFunctionTimerImpl(func_name);
}
TestFunctionTimer::~TestFunctionTimer()
{
    delete impl;
}

