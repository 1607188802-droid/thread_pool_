/**************** main.cpp ****************/
#include <random>

int main() {
    // 创建带监控的线程池，4个线程，每秒输出统计信息
    ThreadPool pool(4, true, chrono::seconds(1));

    // 随机数生成器
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(100, 500);

    cout << "开始提交任务..." << endl;

    // 提交20个模拟不同负载的任务
    for (int i = 0; i < 20; i++) {
        pool.enqueue([i, &dis, &gen] {
            // 模拟不同耗时的任务
            int workTime = dis(gen);
            this_thread::sleep_for(chrono::milliseconds(workTime));

            cout << "任务 " << i << " 完成 (耗时: " << workTime
                << "ms, 线程: " << this_thread::get_id() << ")" << endl;
            });

        // 模拟任务提交间隔
        this_thread::sleep_for(chrono::milliseconds(50));
    }

    // 等待所有任务完成
    this_thread::sleep_for(chrono::seconds(5));

    // 手动输出最终统计
    cout << "\n=== 最终统计 ===" << endl;
    pool.printStats();

    return 0;
}