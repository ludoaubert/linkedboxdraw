#ifndef _THREAD_POOL_
#define _THREAD_POOL_

#include <vector>
#include <deque>
#include <thread>
#include <condition_variable>
#include <mutex>


class ThreadPool
{
public:
    ThreadPool(unsigned int n = std::thread::hardware_concurrency());

    template<class F> void enqueue(F&& f);
    void waitFinished();
    ~ThreadPool();

    unsigned int getProcessed() const { return processed; }

private:
    std::vector< std::thread > workers;
    std::deque< std::function<void()> > tasks;
    std::mutex queue_mutex;
    std::condition_variable cv_task;
    std::condition_variable cv_finished;
    std::atomic_uint processed;
    unsigned int busy;
    bool stop;

    void thread_proc();
};


// generic function push
template<class F>
void ThreadPool::enqueue(F&& f)
{
    std::unique_lock<std::mutex> lock(queue_mutex);
    tasks.emplace_back(std::forward<F>(f));
    cv_task.notify_one();
}

int test_thread_pool();


#endif