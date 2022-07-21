#include <iostream>
#include <deque>
#include <functional>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <random>

//thread pool
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

ThreadPool::ThreadPool(unsigned int n)
    : busy()
    , processed()
    , stop()
{
    for (unsigned int i=0; i<n; ++i)
        workers.emplace_back(std::bind(&ThreadPool::thread_proc, this));
}

ThreadPool::~ThreadPool()
{
    // set stop-condition
    std::unique_lock<std::mutex> latch(queue_mutex);
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
        std::unique_lock<std::mutex> latch(queue_mutex);
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

// generic function push
template<class F>
void ThreadPool::enqueue(F&& f)
{
    std::unique_lock<std::mutex> lock(queue_mutex);
    tasks.emplace_back(std::forward<F>(f));
    cv_task.notify_one();
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
    std::random_device rd;
    std::mt19937 rng(rd());

    // build a vector of random numbers
    std::vector<int> data;
    data.reserve(100000);
    std::generate_n(std::back_inserter(data), data.capacity(), [&](){ return rng(); });
    std::sort(data.begin(), data.end(), std::greater<int>());
}

int main()
{
    ThreadPool tp;

    // run five batches of 100 items
    for (int x=0; x<5; ++x)
    {
        // queue 100 work tasks
        for (int i=0; i<100; ++i)
            tp.enqueue(work_proc);

        tp.waitFinished();
        std::cout << tp.getProcessed() << '\n';
    }

    // destructor will close down thread pool
    return EXIT_SUCCESS;
}