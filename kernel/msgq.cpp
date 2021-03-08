#include "kernel/msgq.hpp"
#include "arch/asm.h"

#include <cstring>

namespace otrix
{

msgq::msgq(size_t size, size_t msg_size): read_p_(0), write_p_(0), size_(size),
                                          msg_size_(msg_size), sem_(size, 0)
{
    storage_ = new uint8_t[size_ * msg_size_];
    memset(storage_, 0, msg_size_ * size_);
}

msgq::~msgq()
{
    delete [] storage_;
}

bool msgq::read(void *msg, uint64_t timeout_ms)
{
    if (!sem_.take(timeout_ms)) {
        return false;
    }

    auto flags = arch_irq_save();
    // read elem
    const uint8_t *elem_ptr = storage_ + read_p_ * msg_size_;
    read_p_ = (read_p_ + 1) % size_;
    memcpy(msg, elem_ptr, msg_size_);
    arch_irq_restore(flags);

    return true;
}

bool msgq::write(const void *msg)
{
    auto flags = arch_irq_save();
    if (write_p_ == read_p_ && sem_.count() != 0) {
        arch_irq_restore(flags);
        return false;
    }

    memcpy(storage_ + write_p_ * msg_size_, msg, msg_size_);
    write_p_ = (write_p_ + 1) % size_;

    sem_.give();
    arch_irq_restore(flags);

    return true;
}

} // namespace otrix
