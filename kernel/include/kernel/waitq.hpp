#pragma once

#include "common/list.h"

namespace otrix
{

class waitq
{
public:
    waitq();
    waitq(const waitq &other) = delete;
    waitq &operator=(const waitq &other) = delete;
    ~waitq();

    void wait();
    void notify_one();
    void notify_all();

private:
    intrusive_list *wq_;
};

} // namespace otrix
