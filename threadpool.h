/**************** threadpool.h ****************/
#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <chrono>
using namespace std;

// 线程池统计信息结构体
struct ThreadPoolStats {
    atomic<size_t> totalTasks;           // 总提交任务数
    atomic<size_t> completedTasks;        // 已完成任务数
    atomic<size_t> pendingTasks;          // 等待中任务数
    atomic<size_t> activeThreads;         // 活跃线程数
    chrono::steady_clock::time_point startTime;  // 线程池启动时间

    ThreadPoolStats() : totalTasks(0), completedTasks(0),
        pendingTasks(0), activeThreads(0) {
        startTime = chrono::steady_clock::now();
    }
};

class ThreadPool {
private:
    vector<thread> workers;                  // 工作线程
    queue<function<void()>> tasks;           // 任务队列
    mutex queueMutex;                        // 队列互斥锁
    condition_variable condition;            // 条件变量
    bool stop;                                // 停止标志

    // 监控与统计相关
    ThreadPoolStats stats;                    // 统计信息
    mutex statsMutex;                         // 统计信息互斥锁
    thread monitorThread;                      // 监控线程
    bool enableMonitor;                        // 是否启用监控
    chrono::seconds monitorInterval;           // 监控间隔

public:
    ThreadPool(size_t threads, bool enableMonitoring = false,
        chrono::seconds interval = chrono::seconds(1));
    void enqueue(function<void()> task);

    // 统计信息获取接口
    size_t getTotalTasks() const { return stats.totalTasks; }
    size_t getCompletedTasks() const { return stats.completedTasks; }
    size_t getPendingTasks() const { return stats.pendingTasks; }
    size_t getActiveThreads() const { return stats.activeThreads; }
    double getAverageTaskTime() const;         // 获取平均任务执行时间
    double getUptime() const;                   // 获取线程池运行时间

    // 监控控制
    void startMonitoring(chrono::seconds interval = chrono::seconds(1));
    void stopMonitoring();
    void printStats() const;                    // 打印统计信息

    ~ThreadPool();
};