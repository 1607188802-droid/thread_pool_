/**************** threadpool.cpp ****************/
#include <iomanip>

ThreadPool::ThreadPool(size_t threads, bool enableMonitoring,
    chrono::seconds interval)
    : stop(false), enableMonitor(enableMonitoring), monitorInterval(interval) {

    // 创建工作线程
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this, i] {
            while (true) {
                function<void()> task;
                {
                    unique_lock<mutex> lock(this->queueMutex);

                    // 更新活跃线程数（开始等待）
                    stats.activeThreads++;

                    this->condition.wait(lock, [this] {
                        return this->stop || !this->tasks.empty();
                        });

                    // 更新活跃线程数（结束等待）
                    stats.activeThreads--;

                    if (this->stop && this->tasks.empty())
                        return;

                    task = move(this->tasks.front());
                    this->tasks.pop();

                    // 更新等待队列长度
                    stats.pendingTasks = tasks.size();
                }

                // 执行任务并计时
                auto taskStart = chrono::steady_clock::now();
                task();
                auto taskEnd = chrono::steady_clock::now();

                // 更新统计信息
                stats.completedTasks++;

                // 可以在这里记录任务执行时间（需要扩展统计结构）
                auto duration = chrono::duration_cast<chrono::milliseconds>
                    (taskEnd - taskStart);
            }
            });
    }

    // 启动监控线程（如果启用）
    if (enableMonitor) {
        startMonitoring(interval);
    }
}

void ThreadPool::enqueue(function<void()> task) {
    {
        unique_lock<mutex> lock(queueMutex);
        tasks.emplace([this, task] {
            // 包装原任务，添加统计功能
            task();
            });
        stats.totalTasks++;
        stats.pendingTasks = tasks.size();
    }
    condition.notify_one();
}

void ThreadPool::startMonitoring(chrono::seconds interval) {
    if (monitorThread.joinable()) {
        stopMonitoring();
    }

    enableMonitor = true;
    monitorInterval = interval;

    monitorThread = thread([this] {
        while (enableMonitor) {
            this_thread::sleep_for(monitorInterval);
            if (enableMonitor) {
                printStats();
            }
        }
        });
}

void ThreadPool::stopMonitoring() {
    enableMonitor = false;
    if (monitorThread.joinable()) {
        monitorThread.join();
    }
}

void ThreadPool::printStats() const {
    cout << fixed << setprecision(2);
    cout << "\n=== 线程池统计信息 ===" << endl;
    cout << "运行时间: " << getUptime() << " 秒" << endl;
    cout << "总任务数: " << stats.totalTasks << endl;
    cout << "已完成: " << stats.completedTasks << endl;
    cout << "等待中: " << stats.pendingTasks << endl;
    cout << "活跃线程: " << stats.activeThreads << "/"
        << workers.size() << endl;
    cout << "吞吐量: " << (stats.completedTasks / getUptime())
        << " 任务/秒" << endl;
    cout << "===================\n" << endl;
}

double ThreadPool::getUptime() const {
    auto now = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>
        (now - stats.startTime);
    return duration.count() / 1000.0;
}

double ThreadPool::getAverageTaskTime() const {
    // 简化实现，实际需要记录总执行时间
    return 0.0;
}

ThreadPool::~ThreadPool() {
    // 停止监控线程
    stopMonitoring();

    // 停止工作线程
    {
        unique_lock<mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();

    for (thread& worker : workers)
        worker.join();
}