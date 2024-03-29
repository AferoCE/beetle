# Raspberry cross compiler setup stuff
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)
SET(CMAKE_C_COMPILER $ENV{HOME}/raspberrypi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER $ENV{HOME}/raspberrypi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-g++)
SET(CMAKE_FIND_ROOT_PATH $ENV{HOME}/raspberrypi/rootfs)
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
SET(BLUETOOTH_LIB_DIR $ENV{HOME}/raspberrypi/rootfs/usr/lib/arm-linux-gnueabihf)
SET(SYSTEM_LIB_DIR $ENV{HOME}/raspberrypi/rootfs/lib/arm-linux-gnueabihf)
SET(LD_SO ld-linux-armhf.so.3)
# I shouldn't have to do this but for some reason it's not picking up the directories from the above lines
include_directories($ENV{HOME}/raspberrypi/rootfs/usr/include $ENV{HOME}/raspberrypi/rootfs/usr/include/arm-linux-gnueabihf)
