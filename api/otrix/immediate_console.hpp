#pragma once

#include <cstddef>

namespace otrix
{

class immediate_console
{
public:

    immediate_console() = delete;

    static void init();

    static void print(const char *format, ...);

private:
    static bool inited_;
    static constexpr size_t buffer_size_ = 2048;
    static char message_buffer_[buffer_size_];
};

} // namespace otrix
