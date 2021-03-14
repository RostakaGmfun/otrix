add_library(otrix_kmem kmem.cpp)
target_link_libraries(otrix_kmem otrix_common)
target_include_directories(otrix_kmem PUBLIC include)

if(BUILD_HOST_TESTS)
add_executable(kmem_test test/test_runner.c test/kmem_test.c)
target_include_directories(kmem_test PUBLIC include)
target_link_libraries(kmem_test unity otrix_kmem)
add_test(NAME kmem_test COMMAND kmem_test)
else()
add_library(otrix_kernel kmain.cpp kthread.cpp waitq.cpp semaphore.cpp msgq.cpp)
target_link_libraries(otrix_kernel otrix_arch otrix_kmem)
target_include_directories(otrix_kernel PUBLIC include)

add_executable(otrix otrix.cpp newlib_stubs.cpp cpp_stubs.cpp)
target_link_libraries(otrix otrix_kernel otrix_dev otrix_net)

install(TARGETS otrix RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
endif()
