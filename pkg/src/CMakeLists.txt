cmake_minimum_required (VERSION 2.8)

project (Beetle)

# Handle the build version if it's set
set(BUILD_NUMBER "$ENV{BUILD_NUMBER}")
if(BUILD_NUMBER)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DBUILD_NUMBER=\\\"${BUILD_NUMBER}\\\"")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DBUILD_NUMBER=\\\"local\\\"")
endif()

add_executable(beetle
    babygatt_central.c
    babygatt_common.c
    babygatt_peripheral.c
    beetle.c
    build_info.h
    central.c
    command.c
    connlist.c
    devicelist.c
    evloop.c
    hci_beetle.c
    log.c
    peripheral.c
    utils.c
)

target_link_libraries(beetle ${BLUETOOTH_LIB_DIR}/libbluetooth.a ${SYSTEM_LIB_DIR}/librt.so.1 ${SYSTEM_LIB_DIR}/libc.so.6 ${SYSTEM_LIB_DIR}/${LD_SO})

add_custom_command(
  OUTPUT build_info.h
  COMMAND ./make-build-info.sh
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
