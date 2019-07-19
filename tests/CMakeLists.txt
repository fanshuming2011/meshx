include_directories(../common 
                    ../mesh
                    ../mesh/bearer
                    ../mesh/provision
                    ../mesh/inc
                    ../mesh/gap
                    ../mesh/common
                    ../mesh/security
                    ../cmd
                    ../mesh/beacon
                    ../platform
                    ../platform/linux
                    ../thirdpart/aes_cmac
                    ../thirdpart/micro-ecc
                    ../thirdpart/tiny-AES-c
                    ../thirdpart/mbedtls
                    ../thirdpart/micro-ecc
                    ../)
set(COMMON_SRC_LIST
    ../thirdpart/aes_cmac/aes_cmac.c
    ../thirdpart/tiny-AES-c/aes.c
    ../thirdpart/mbedtls/aes_ccm.c
    ../thirdpart/micro-ecc/uECC.c
    ../meshx.c
    ../platform/linux/meshx_mem.c
    ../platform/linux/meshx_system.c
    ../platform/linux/meshx_timer.c
    ../platform/linux/meshx_misc.c
    ../platform/linux/meshx_tty.c
    ../platform/linux/msg_queue.c
    ../platform/linux/meshx_trace_io.c
    ../platform/linux/meshx_security_wrapper.c
    ../common/meshx_trace.c
    ../common/meshx_assert.c
    ../common/meshx_list.c
    ../common/meshx_3gpp_crc.c
    ../common/meshx_cmd.c
    ../common/meshx_errno.c
    ../common/meshx_lib.c
    ../common/meshx_io.c
    ../cmd/meshx_cmd_base.c
    ../cmd/meshx_cmd_common.c
    ../mesh/common/meshx_node.c
    ../mesh/common/meshx_async.c
    ../mesh/common/meshx_notify.c
    ../mesh/security/meshx_security.c
    ../mesh/gap/meshx_gap.c
    ../mesh/bearer/meshx_bearer.c
    ../mesh/bearer/meshx_bearer_adv.c
    ../mesh/bearer/meshx_bearer_gatt.c
    ../mesh/network/meshx_network.c
    ../mesh/network/meshx_network_interface.c
    ../mesh/provision/meshx_pb_adv.c
    ../mesh/provision/meshx_provision.c
    ../mesh/beacon/meshx_beacon.c)

set(DEV_SRC_LIST
    ../platform/linux/meshx_gap_wrapper_dev.c
    device_cmd.c)

set(PROV_SRC_LIST
    ../platform/linux/meshx_gap_wrapper_prov.c
    ../cmd/meshx_cmd_prov.c
    provisioner_cmd.c)


find_package(Threads REQUIRED)

#test tassert
add_executable(meshx_provisioner meshx_provisioner.c
               ${COMMON_SRC_LIST} ${PROV_SRC_LIST})
target_link_libraries(meshx_provisioner pthread rt)

add_executable(meshx_device meshx_device.c
               ${COMMON_SRC_LIST} ${DEV_SRC_LIST})
target_link_libraries(meshx_device pthread rt)

