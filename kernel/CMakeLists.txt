add_executable(otrix_kernel kmain.c kthread.c)
target_link_libraries(otrix_kernel otrix_dev otrix_arch)
target_include_directories(otrix_kernel PUBLIC include/)

install(TARGETS otrix_kernel RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
