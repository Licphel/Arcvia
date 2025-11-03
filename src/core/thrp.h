#pragma once

#include <functional>

#define ARC_THREAD_POOL_KERNELS 4

namespace arc {

namespace thread_pool {

void execute(std::function<void()> f);
// ensure all tasks to be done.
void shutdown();

};  // namespace thread_pool

}  // namespace arc