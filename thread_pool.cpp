#include "thread_pool.h"
#include <functional>
#include <mutex>
using namespace std;


ThreadPool::ThreadPool(unsigned int n)
    : busy()
    , processed()
    , stop()
{
    for (unsigned int i=0; i<n; ++i)
        workers.emplace_back(bind(&ThreadPool::thread_proc, this));
}

ThreadPool::~ThreadPool()
{
    // set stop-condition
    unique_lock<mutex> latch(queue_mutex);
    stop = true;
    cv_task.notify_all();
    latch.unlock();

    // all threads terminate, then we're done.
    for (auto& t : workers)
        t.join();
}

void ThreadPool::thread_proc()
{
    while (true)
    {
        unique_lock<mutex> latch(queue_mutex);
        cv_task.wait(latch, [this](){ return stop || !tasks.empty(); });
        if (!tasks.empty())
        {
            // got work. set busy.
            ++busy;

            // pull from queue
            auto fn = tasks.front();
            tasks.pop_front();

            // release lock. run async
            latch.unlock();

            // run function outside context
            fn();
            ++processed;

            latch.lock();
            --busy;
            cv_finished.notify_one();
        }
        else if (stop)
        {
            break;
        }
    }
}


// waits until the queue is empty.
void ThreadPool::waitFinished()
{
    std::unique_lock<std::mutex> lock(queue_mutex);
    cv_finished.wait(lock, [this](){ return tasks.empty() && (busy == 0); });
}

// a cpu-busy task.
void work_proc()
{
    random_device rd;
    mt19937 rng(rd());

    // build a vector of random numbers
    vector<int> data;
    data.reserve(100000);
    generate_n(back_inserter(data), data.capacity(), [&](){ return rng(); });
    sort(data.begin(), data.end(), greater<int>());
}


int test_thread_pool()
{
    ThreadPool tp;

    // run five batches of 100 items
    for (int x=0; x<5; ++x)
    {
        // queue 100 work tasks
        for (int i=0; i<100; ++i)
            tp.enqueue(work_proc);

        tp.waitFinished();
        cout << tp.getProcessed() << '\n';
    }

    // destructor will close down thread pool
    return EXIT_SUCCESS;
}
