#include "ThreadPool .h"

ThreadPool::ThreadPool(size_t minThreads, size_t maxThreads)
        : minThreads(minThreads), maxThreads(maxThreads), stopPool(false), activeThreads(0) {

    logConfig();
    for (size_t i = 0; i < minThreads; ++i) {
        workers.emplace_back(&ThreadPool::worker, this);
    }

    std::thread(&ThreadPool::managePool, this).detach();
}

void ThreadPool::enqueue(const std::function<void()>& task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        tasks.push(task);
    }
    condition.notify_one();
}

void ThreadPool::stop() {
    stopPool = true;
    condition.notify_all();
    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::worker() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] { return !tasks.empty() || stopPool; });

            if (stopPool && tasks.empty()) {
                return;
            }

            task = tasks.front();
            tasks.pop();
        }
        activeThreads++;
        task();
        activeThreads--;
    }
}

void ThreadPool::managePool() {
    while (!stopPool) {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        size_t currentThreads = workers.size();
        size_t tasksSize = tasks.size();

        if (tasksSize > currentThreads && currentThreads < maxThreads) {
            size_t threadsToAdd = std::min(tasksSize - currentThreads, maxThreads - currentThreads);
            for (size_t i = 0; i < threadsToAdd; ++i) {
                workers.emplace_back(&ThreadPool::worker, this);
            }
            std::cout << "Added " << threadsToAdd << " threads. Total threads: " << workers.size() << std::endl;
        } else if (tasksSize < currentThreads && currentThreads > minThreads) {
            size_t threadsToRemove = std::min(static_cast<size_t>(currentThreads - minThreads), currentThreads - tasksSize);
            try {
                workers.erase(workers.end() - threadsToRemove, workers.end());
                std::cout << "Removed " << threadsToRemove << " threads. Total threads: " << workers.size() << std::endl;
            } catch (const std::exception& ex) {
                std::cerr << "Exception caught while erasing threads: " << ex.what() << std::endl;
                // Handle the exception as needed, potentially logging or rethrowing
            } catch (...) {
                std::cerr << "Unknown exception caught while erasing threads." << std::endl;
                // Handle the unknown exception case
            }
        }

        if (currentThreads < minThreads) {
            size_t threadsToAdd = minThreads - currentThreads;
            for (size_t i = 0; i < threadsToAdd; ++i) {
                workers.emplace_back(&ThreadPool::worker, this);
            }
            std::cout << "Added " << threadsToAdd << " threads to maintain minimum. Total threads: " << workers.size() << std::endl;
        }
    }
}

void ThreadPool:: logConfig() {
//    spdlog::set_level(spdlog::level::debug); // Set global log level to debug
//    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
//    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("thread_pool.log", true);
//    spdlog::logger logger("multi_sink", {console_sink, file_sink});
//    spdlog::set_default_logger(std::make_shared<spdlog::logger>(logger));
}

ThreadPool::~ThreadPool() {
    stop();
}
