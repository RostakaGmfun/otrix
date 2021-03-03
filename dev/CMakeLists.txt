add_library(otrix_dev serial.c pci.cpp immediate_console.cpp acpi.cpp virtio.cpp virtio_net.cpp virtio_console.cpp)
target_include_directories(otrix_dev PUBLIC include/ ../api)
target_link_libraries(otrix_dev otrix_arch otrix_common otrix_net otrix_kernel)
target_compile_options(otrix_dev PUBLIC ${KERNEL_C_FLAGS})
