
option (WITH_LOAD_FW "Include loading firmware example" OFF)

add_subdirectory (echo)
add_subdirectory (rpmsg_sample_echo)
add_subdirectory (matrix_multiply)
add_subdirectory (load_fw)
if (WITH_PROXY_APPS)
add_subdirectory (rpc_demo)
endif (WITH_PROXY_APPS)
