add_library(otrix_dev serial.c pci.c immediate_console.cpp acpi.cpp)
target_include_directories(otrix_dev PUBLIC include/ ../api)
target_link_libraries(otrix_dev otrix_arch otrix_common)
target_compile_options(otrix_dev PUBLIC ${KERNEL_C_FLAGS})
