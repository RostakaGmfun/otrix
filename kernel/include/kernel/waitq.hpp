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

    // Context for blocked thread
    struct waitq_item
    {
        intrusive_list wq_node;
        int ret;
        kthread *thread;
    };

    intrusive_list *wq_;
};

} // namespace otrix
