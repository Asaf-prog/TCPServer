#ifndef COMPUTERNETWORKEX3_THREADPOOL_H
#define COMPUTERNETWORKEX3_THREADPOOL_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <functional>
//#include <spdlog/spdlog.h>
//#include <spdlog/sinks/basic_file_sink.h>
//#include <spdlog/sinks/stdout_color_sinks.h>


#pragma comment(lib, "Ws2_32.lib")

struct ThreadPool {
    ThreadPool(size_t minThreads, size_t maxThreads);
    ~ThreadPool();

    void enqueue(const std::function<void()>& task);
    void stop();

private:
    void worker();
    void managePool();
    void logConfig();

    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stopPool;
    size_t minThreads;
    size_t maxThreads;
    std::atomic<size_t> activeThreads;
    std::mutex poolMutex;
};

#endif //COMPUTERNETWORKEX3_THREADPOOL_H
