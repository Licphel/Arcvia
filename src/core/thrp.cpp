#include "core/thrp.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include "core/log.h"

namespace arc {

struct thread_pool_impl_ {
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool stop_{false};

    static thread_pool_impl_& instance() {
        static thread_pool_impl_ pool;
        return pool;
    }

    void submit(std::function<void()> f) {
        {
            std::unique_lock lk(mtx_);
            if (stop_) [[unlikely]]
                print_throw(log_level::fatal, "thread pool is already shutdown!");
            tasks_.emplace(std::move(f));
        }
        cv_.notify_one();
    }

    void shutdown() {
        std::unique_lock lk(mtx_);
        stop_ = true;
        lk.unlock();
        cv_.notify_all();
        for (std::thread& t : threads_) t.join();
        threads_.clear();
    }

    ~thread_pool_impl_() {
        shutdown();
    }

    thread_pool_impl_() {
        size_t n = ARC_THREAD_POOL_KERNELS > 0 ? ARC_THREAD_POOL_KERNELS : 1;
        for (size_t i = 0; i < n; ++i) threads_.emplace_back([this] { worker(); });
    }

    void worker() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock lk(mtx_);
                cv_.wait(lk, [this] { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) return;
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            try {
                task();
            } catch (...) {
            }
        }
    }
};

void thread_pool::execute(std::function<void()> f) { thread_pool_impl_::instance().submit(std::move(f)); }
void thread_pool::shutdown() { thread_pool_impl_::instance().shutdown(); }

}  // namespace arc