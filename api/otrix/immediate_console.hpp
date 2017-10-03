#ifndef OTRIX_IMMEDIATE_CONSOLE_HPP
#define OTRIX_IMMEDIATE_CONSOLE_HPP

namespace otrix
{

class immediate_console
{
public:

    immediate_console() = delete;

    static void init();

    static void print(const char *str);

private:
    static bool inited_;
};

} // namespace otrix

#endif // OTRIX_IMMEDIATE_CONSOLE_HPP
