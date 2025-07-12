#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

using namespace std::chrono_literals;

class TaskQueue
{
  public:
    TaskQueue()
    {
        _worker = std::thread(&TaskQueue::_processTasks, this);
    }

    ~TaskQueue()
    {
        {
            std::lock_guard<std::mutex> lock(_mtx);
            _shutdown = true;
        }
        _cv.notify_all(); // Notify worker to exit if waiting
        if(_worker.joinable()) { _worker.join(); }
    }

    void pushTask(std::function<void()> task)
    {
        {
            std::lock_guard<std::mutex> lock(_mtx);
            _tasks.push(std::move(task));
        }
        _cv.notify_one();
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(_mtx);
        std::cout << "Clearing task queue...\n";
        while(!_tasks.empty())
        {
            _tasks.pop();
        }
        // The worker will just wake up, see the empty queue, and continue waiting.
    }

  private:
    std::queue<std::function<void()>> _tasks;
    std::mutex _mtx;
    std::condition_variable _cv;
    std::thread _worker;
    bool _shutdown{false};

    void _processTasks()
    {
        while(true)
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(_mtx);
                // Wait until there is a task or shutdown signal
                // return false will cause the thread to sleep until notified
                _cv.wait(lock, [&] { return !_tasks.empty() || _shutdown; });

                if(_shutdown)
                {
                    std::cout << "Worker thread shutting down...\n";
                    break;
                }

                task = std::move(_tasks.front());
                _tasks.pop();
            }

            std::cout << "Start running task...\n";
            task(); // safe to call
        }
    }
};

// Example functions
void greet()
{
    std::cout << "Starting greet...\n";
    std::this_thread::sleep_for(3s);
    std::cout << "Hello (no arg)\n";
}

void greetWithName(const std::string& name)
{
    std::cout << "Starting greet with " << name << "...\n";
    std::this_thread::sleep_for(4s);
    std::cout << "Hello, " << name << "\n";
}

int main()
{
    TaskQueue queue;

    std::cout << "Pushing Task 1\n";
    queue.pushTask(greet); // Type A

    std::this_thread::sleep_for(1s);
    std::cout << "Pushing Task 2\n";
    queue.pushTask([] { std::cout << "Lambda (no arg)\n"; }); // Type A

    size_t i{3};
    for(auto& name : {"Alice", "Bob", "Charlie"})
    {
        std::this_thread::sleep_for(1s);
        std::cout << "Pushing Task " << i << "\n";
        queue.pushTask([name] { greetWithName(name); }); // Type B wrapped
        i++;
    }

    queue.clear(); // Clear the queue

    std::cout << "Pushing Task " << i << "\n";
    queue.pushTask(
        []
        {
            std::cout << "Starting doing some work...\n";
            std::this_thread::sleep_for(6s);
            std::cout << "Completed the work\n";
        });
    i++;

    for(auto& name : {"David", "Edward", "Frank"})
    {
        std::this_thread::sleep_for(1s);
        std::cout << "Pushing Task " << i << "\n";
        queue.pushTask([name] { greetWithName(name); });
        i++;
    }

    std::this_thread::sleep_for(10s);
    return 0;
}
