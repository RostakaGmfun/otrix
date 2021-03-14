add_library(otrix_net ipv4.cpp net_task.cpp arp.cpp icmp.cpp)
target_include_directories(otrix_net PUBLIC include)
target_link_libraries(otrix_net otrix_dev otrix_common otrix_kernel)
