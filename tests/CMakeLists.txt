include_directories(../common 
                    ../mesh
                    ../mesh/bearer
                    ../mesh/provision
                    ../mesh/inc
                    ../mesh/gap
                    ../platform
                    ../)
set(COMMON_SRC_LIST
    ../platform/linux/meshx_mem.c
    ../platform/linux/meshx_system.c
    ../platform/linux/meshx_timer.c
    ../platform/linux/meshx_misc.c
    ../common/meshx_trace.c
    ../common/meshx_assert.c
    ../common/meshx_list.c
    ../mesh/gap/meshx_gap.c
    ../mesh/bearer/meshx_bearer.c
    ../mesh/bearer/meshx_bearer_adv.c
    ../mesh/bearer/meshx_bearer_gatt.c
    ../mesh/network/meshx_network.c
    ../mesh/network/meshx_network_interface.c
    ../mesh/provision/meshx_pb_adv.c
    ../mesh/provision/meshx_provision.c)

set(DEV_SRC_LIST
    ../platform/linux/meshx_gap_wrapper_dev.c)

set(PROV_SRC_LIST
    ../platform/linux/meshx_gap_wrapper_prov.c)


find_package(Threads REQUIRED)

#test tassert
add_executable(meshx_provisioner meshx_provisioner.c
               ${COMMON_SRC_LIST} ${PROV_SRC_LIST})
target_link_libraries(meshx_provisioner pthread)

add_executable(meshx_device meshx_device.c
               ${COMMON_SRC_LIST} ${DEV_SRC_LIST})
target_link_libraries(meshx_device pthread)

