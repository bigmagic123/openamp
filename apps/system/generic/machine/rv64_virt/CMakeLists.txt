collect (APP_COMMON_SOURCES platform_info.c)
collect (APP_COMMON_SOURCES rv64_virt_linux_proc.c)
collect (APP_COMMON_SOURCES helper.c)
collect (APP_COMMON_SOURCES rsc_table.c)
collect (APP_INC_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
if (RPMSG_NO_IPI)
  add_definitions(-DRPMSG_NO_IPI)
endif (RPMSG_NO_IPI)

