# Raspberry cross compiler setup stuff
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)
SET(CMAKE_C_COMPILER /usr/bin/arm-linux-gnueabi-gcc)
SET(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabi-gcc)
SET(CMAKE_FIND_ROOT_PATH $ENV{HOME}/raspberrypi/rootfs)
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
SET(BLUETOOTH_LIB_DIR $ENV{HOME}/armel/rootfs/usr/lib/arm-linux-gnueabi)
SET(SYSTEM_LIB_DIR $ENV{HOME}/armel/rootfs/lib/arm-linux-gnueabi)
SET(LD_SO ld-linux.so.3)
# I shouldn't have to do this but for some reason it's not picking up the directories from the above lines
include_directories($ENV{HOME}/raspberrypi/rootfs/usr/include)
