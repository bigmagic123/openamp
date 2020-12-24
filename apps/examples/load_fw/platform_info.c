/*
 * Copyright(c) 2019 Xilinx Ltd.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_info.h>
#include <common.h>

#ifdef WITH_ZYNQMP
extern struct remoteproc_ops zynqmp_apu_rproc_ops;
extern struct remoteproc_ops zynqmp_rpu_rproc_ops;
#endif /* #ifdef WITH_ZYNQMP */
#ifdef WITH_RV64
extern struct remoteproc_ops virt_rv64_rproc_ops;
#endif /* #ifdef WITH_RV64 */

static struct remoteproc rproc_inst;
static struct  remoteproc_ops ops;

static struct remoteproc * platform_create_proc(unsigned int cpu_id)
{
	struct remoteproc * rproc;

#ifdef WITH_RV64
	ops = virt_rv64_rproc_ops;
#elif defined(WITH_ZYNQMP)
	if (NODE_RPU_0 <= LOAD_FW_TARGET && LOAD_FW_TARGET <= NODE_RPU_1)
		ops = zynqmp_rpu_rproc_ops;
	else if (NODE_APU_0 <= LOAD_FW_TARGET && LOAD_FW_TARGET <= NODE_APU_1)
		ops = zynqmp_apu_rproc_ops;
#else
	return NULL;
#endif /* #ifdef WITH_ZYNQMP */

	rproc = remoteproc_init(&rproc_inst, &ops, &cpu_id);
	if (!rproc)
		return NULL;
	return &rproc_inst;
}

static void app_log_handler(enum metal_log_level level,
                   const char *format, ...)
{
	char msg[1024];
	va_list args;
	static const char *level_strs[] = {
		"metal: emergency: ",
		"metal: alert:     ",
		"metal: critical:  ",
		"metal: error:     ",
		"metal: warning:   ",
		"metal: notice:    ",
		"metal: info:      ",
		"metal: debug:     ",
	};

	va_start(args, format);
	vsnprintf(msg, sizeof(msg), format, args);
	va_end(args);

	if (level <= METAL_LOG_EMERGENCY || level > METAL_LOG_DEBUG)
		level = METAL_LOG_EMERGENCY;

	LPRINTF("%s%s", level_strs[level], msg);
}

#ifdef WITH_ZYNQMP
static XIpiPsu IpiInst;

static XStatus IpiConfigure(XIpiPsu *const IpiInstPtr)
{
    XStatus Status;
    XIpiPsu_Config *IpiCfgPtr;

    /* Look Up the config data */
    IpiCfgPtr = XIpiPsu_LookupConfig(XPAR_XIPIPSU_0_DEVICE_ID);
    if (NULL == IpiCfgPtr) {
        Status = XST_FAILURE;
        LPERROR("%s ERROR in getting CfgPtr\n", __func__);
        return Status;
    }

    /* Init with the Cfg Data */
    Status = XIpiPsu_CfgInitialize(IpiInstPtr, IpiCfgPtr,
                       IpiCfgPtr->BaseAddress);
    if (XST_SUCCESS != Status) {
        LPERROR("%s ERROR #%d in configuring IPI\n", __func__, Status);
        return Status;
    }
    return Status;
}
#endif /* #ifdef WITH_ZYNQMP */

struct remoteproc * app_init(unsigned int cpu_id){
	struct metal_init_params metal_param = {
		.log_handler = app_log_handler,
		.log_level = METAL_LOG_DEBUG,
	};
	metal_init(&metal_param);

#ifdef WITH_ZYNQMP
	if (XST_SUCCESS != IpiConfigure(&IpiInst)) {
		LPERROR("Failed to config IPI instance\r\n");
		return NULL;
	}

	if (XST_SUCCESS != XPm_InitXilpm(&IpiInst)) {
		LPERROR("Failed to initialize PM\r\n");
		return NULL;
	}
#endif /* #ifdef WITH_ZYNQMP */

	return platform_create_proc(cpu_id);
}
