#include "kernel/semaphore.hpp"
#include "arch/asm.h"

namespace otrix
{

semaphore::semaphore(int max_count, int initial_count): count_(initial_count), max_count_(max_count)
{

}

semaphore::~semaphore()
{
    waiting_queue_.notify_all();
}

bool semaphore::take(uint64_t timeout_ms)
{
    bool ret = false;
    auto flags = arch_irq_save();
    if (0 == count_) {
        if (timeout_ms > 0) {
            ret = waiting_queue_.wait(timeout_ms);
        }
    } else {
        count_--;
        ret = true;
    }
    arch_irq_restore(flags);
    return ret;
}

void semaphore::give()
{
    auto flags = arch_irq_save();

    if (count_ == max_count_) {
        arch_irq_restore(flags);
        return;
    }

    if (0 == count_ && !waiting_queue_.empty()) {
        waiting_queue_.notify_one();
    } else {
        count_++;
    }

    arch_irq_restore(flags);
}

} // namespace otrix
