# This toolchain file is for cross-compiling targetting
#     the SheevaPlug wall-wart.

# It assumes that you've installed the rootfs in
#     /usr/local/sheeva/ or /opt/sheeva/

# Build with 
#  mkdir build; cd build
#  cmake -DCMAKE_TOOLCHAIN_FILE=../cmakemodules/Toolchain-sheeva.cmake ..
#  make

SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)

SET(CMAKE_C_COMPILER   arm-none-linux-gnueabi-gcc)
SET(CMAKE_CXX_COMPILER arm-none-linux-gnueabi-g++)

SET(CMAKE_FIND_ROOT_PATH  /opt/sheeva/ /usr/local/sheeva/)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

