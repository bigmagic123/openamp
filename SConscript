# RT-Thread building script for bridge

import os
from building import *

cwd = GetCurrentDir()
objs = []
list = os.listdir(cwd)
include_path = [cwd + '/apps/system/generic/machine/rv64_virt']
include_path += [cwd + '/lib/rpmsg']
include_path += [cwd + '/lib/include/openamp']
include_path += [cwd + '/lib/include']


src = [cwd +  '/apps/examples/rpmsg_sample_echo/rpmsg-sample-echo.c']
src += [cwd + '/apps/tests/msg/rpmsg-update.c']
src += [cwd + '/apps/examples/matrix_multiply/matrix_multiplyd.c']
src += [cwd + '/apps/system/generic/machine/rv64_virt/virt_rv64_rproc.c']
src += [cwd + '/apps/system/generic/machine/rv64_virt/helper.c']
src += [cwd + '/apps/system/generic/machine/rv64_virt/platform_info.c']
src += [cwd + '/apps/system/generic/machine/rv64_virt/rsc_table.c']
src += [cwd + '/lib/rpmsg/rpmsg.c']
src += [cwd + '/lib/rpmsg/rpmsg_virtio.c']
src += [cwd + '/lib/proxy/rpmsg_retarget.c']
src += [cwd + '/lib/remoteproc/rsc_table_parser.c']
src += [cwd + '/lib/remoteproc/remoteproc_virtio.c']
src += [cwd + '/lib/remoteproc/elf_loader.c']
src += [cwd + '/lib/remoteproc/remoteproc.c']
src += [cwd + '/lib/virtio/virtqueue.c']
src += [cwd + '/lib/virtio/virtio.c']

CPPDEFINES = []
CPPDEFINES += ['DEFAULT_LOGGER_ON']
CPPDEFINES += ['RPMSG_NO_IPI']

objs = DefineGroup('openamp', src, depend = ['PKG_USING_OPENAMP'], CPPPATH = include_path, CPPDEFINES = CPPDEFINES)

Return('objs')
