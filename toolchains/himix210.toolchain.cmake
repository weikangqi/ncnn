# set cross-compiled system type, it's better not use the type which cmake cannot recognized.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
# when hislicon SDK was installed, toolchain was installed in the path as below: 
set(CMAKE_C_COMPILER /opt/linux/x86-arm/aarch64-mix210-linux/bin/aarch64-mix210-linux-gcc)
set(CMAKE_CXX_COMPILER /opt/linux/x86-arm/aarch64-mix210-linux/bin/aarch64-mix210-linux-g++)

# set searching rules for cross-compiler
if(NOT CMAKE_FIND_ROOT_PATH_MODE_PROGRAM)
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
endif()
if(NOT CMAKE_FIND_ROOT_PATH_MODE_LIBRARY)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
endif()
if(NOT CMAKE_FIND_ROOT_PATH_MODE_INCLUDE)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()
if(NOT CMAKE_FIND_ROOT_PATH_MODE_PACKAGE)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
endif()

# set ${CMAKE_C_FLAGS} and ${CMAKE_CXX_FLAGS}flag for cross-compiled process
set(CROSS_COMPILATION_ARM aarch64-mix210)
set(CROSS_COMPILATION_ARCHITECTURE aarch64)

# set g++ param
set(CMAKE_CXX_FLAGS "-march=armv8-a ${CMAKE_CXX_FLAGS}")

# cache flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "c flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "c++ flags")
