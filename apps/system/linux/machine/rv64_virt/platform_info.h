/*
 * Copyright (c) 2016 Xilinx, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* This file populates resource table for BM remote
 * for use by the Linux Master */

#ifndef PLATFORM_INFO_H_
#define PLATFORM_INFO_H_

#include <openamp/remoteproc.h>
#include <openamp/virtio.h>
#include <openamp/rpmsg.h>

#if defined __cplusplus
extern "C" {
#endif

#define RPMSG_NO_IPI
#define RPU_CPU_ID          0 /* RPU remote CPU Index. We only talk to
                               * one CPU in the exmaple. We set the CPU
                               * index to 0. */
//#define IPI_CHN_BITMASK     0x08 /* IPI channel bit mask for IPI
//					* from/to RPU0 */
//#define IPI_DEV_NAME        "ff360000.ipi" /* IPI device name */
#define DEV_BUS_NAME        "platform" /* device bus name. "platform" bus
                                        * is used in Linux kernel for generic
					* devices */
/* libmetal devices names used in the examples.
 * They are platform devices, you find them in Linux sysfs
 * sys/bus/platform/devices */
#define SHM_DEV_NAME        "90100000.shm" /* shared device name */
//TODO: move to common
#define SHM_BASE            0x90100000UL

#define RSC_MEM_PA          SHM_BASE
#define RSC_MEM_SIZE        0x00020000UL
//shared buffer: 0x90120000--0x90160000
#define SHARED_BUF_PA       (SHM_BASE + RSC_MEM_SIZE)
#define SHARED_BUF_SIZE     0x00040000UL
//TODO: move to common end
#define RING_SIZE	    0x00004000UL
//start: 0x90160000
#define RING_TX             (SHARED_BUF_PA + SHARED_BUF_SIZE)
//start: 0x90164000
#define RING_RX             (RING_TX + RING_SIZE)

#ifdef RPMSG_NO_IPI
#define POLL_DEV_NAME        "90000000.shm" /* shared device name */
#define POLL_STOP 0x1U
#endif /* RPMSG_NO_IPI */

struct remoteproc_priv {
	const char *ipi_name; /**< IPI device name */
	const char *ipi_bus_name; /**< IPI bus name */
	const char *rsc_name; /**< rsc device name */
	const char *rsc_bus_name; /**< rsc bus name */
	const char *shm_name; /**< shared memory device name */
	const char *shm_bus_name; /**< shared memory bus name */
	struct metal_device *ipi_dev; /**< pointer to IPI device */
	struct metal_io_region *ipi_io; /**< pointer to IPI i/o region */
	struct metal_device *shm_dev; /**< pointer to shared memory device */
	struct metal_io_region *shm_io; /**< pointer to sh mem i/o region */

	struct remoteproc_mem shm_mem; /**< shared memory */
	unsigned int ipi_chn_mask; /**< IPI channel mask */
	atomic_int ipi_nokick;
#ifdef RPMSG_NO_IPI
	const char *shm_poll_name; /**< shared memory device name */
	const char *shm_poll_bus_name; /**< shared memory bus name */
	struct metal_device *shm_poll_dev; /**< pointer to poll mem device */
	struct metal_io_region *shm_poll_io; /**< pointer to poll mem i/o */
#endif /* RPMSG_NO_IPI */

};


/**
 * platform_init - initialize the platform
 *
 * It will initialize the platform.
 *
 * @argc: number of arguments
 * @argv: array of the input arguements
 * @platform: pointer to store the platform data pointer
 *
 * return 0 for success or negative value for failure
 */
int platform_init(int argc, char *argv[], void **platform);

/**
 * platform_create_rpmsg_vdev - create rpmsg vdev
 *
 * It will create rpmsg virtio device, and returns the rpmsg virtio
 * device pointer.
 *
 * @platform: pointer to the private data
 * @vdev_index: index of the virtio device, there can more than one vdev
 *              on the platform.
 * @role: virtio master or virtio slave of the vdev
 * @rst_cb: virtio device reset callback
 * @ns_bind_cb: rpmsg name service bind callback
 *
 * return pointer to the rpmsg virtio device
 */
struct rpmsg_device *
platform_create_rpmsg_vdev(void *platform, unsigned int vdev_index,
			   unsigned int role,
			   void (*rst_cb)(struct virtio_device *vdev),
			   rpmsg_ns_bind_cb ns_bind_cb);

/**
 * platform_poll - platform poll function
 *
 * @platform: pointer to the platform
 *
 * return negative value for errors, otherwise 0.
 */
int platform_poll(void *platform);

/**
 * platform_release_rpmsg_vdev - release rpmsg virtio device
 *
 * @rpdev: pointer to the rpmsg device
 */
void platform_release_rpmsg_vdev(struct rpmsg_device *rpdev, void *platform);

/**
 * platform_cleanup - clean up the platform resource
 *
 * @platform: pointer to the platform
 */
void platform_cleanup(void *platform);

#if defined __cplusplus
}
#endif

#endif /* PLATFORM_INFO_H_ */

