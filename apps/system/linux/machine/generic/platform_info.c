/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2016 Xilinx, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**************************************************************************
 * FILE NAME
 *
 *       platform_info.c
 *
 * DESCRIPTION
 *
 *       This file implements APIs to get platform specific
 *       information for OpenAMP.
 *
 **************************************************************************/

#include <metal/alloc.h>
#include <metal/atomic.h>
#include <metal/io.h>
#include <metal/irq.h>
#include <metal/shmem.h>
#include <metal/utilities.h>
#include <metal/device.h>
#include <openamp/remoteproc.h>
#include <openamp/rpmsg_virtio.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include "platform_info.h"

#define DEV_BUS_NAME        "platform" /* device bus name. "platform" bus
                                        * is used in Linux kernel for generic
					* devices */
/* libmetal devices names used in the examples.
 * They are platform devices, you find them in Linux sysfs
 * sys/bus/platform/devices */
#define SHM_DEV_NAME        "90000000.shm" /* shared device name */

#define RSC_MEM_PA          0x90000000UL
#define RSC_MEM_SIZE        0x2000UL
#define SHARED_BUF_PA       0x90100000UL
#define SHARED_BUF_SIZE     0x00100000UL

#define _rproc_wait() metal_cpu_yield()

struct remoteproc_priv rproc_priv = {
	.shm_name = SHM_DEV_NAME,
	.shm_bus_name = DEV_BUS_NAME,
};

static struct remoteproc rproc_inst;

/* External functions */
extern int init_system(void);
extern void cleanup_system(void);

static struct remoteproc *
linux_proc_init(struct remoteproc *rproc,
		struct remoteproc_ops *ops, void *arg)
{
	struct remoteproc_priv *prproc = arg;
	struct metal_device *dev;
	metal_phys_addr_t mem_pa;
	int ret;

	if (!rproc || !prproc || !ops)
		return NULL;
	rproc->priv = prproc;
	rproc->ops = ops;
	prproc->ipi_dev = NULL;
	prproc->shm_dev = NULL;
	/* Get shared memory device */
	ret = metal_device_open(prproc->shm_bus_name, prproc->shm_name,
				&dev);
	if (ret) {
		fprintf(stderr, "ERROR: failed to open shm device: %d.\r\n",
			ret);
		goto err1;
	}
	printf("Successfully open shm device.\r\n");
	prproc->shm_dev = dev;
	prproc->shm_io = metal_device_io_region(dev, 0);
	if (!prproc->shm_io)
		goto err2;

#ifdef RPMSG_IPI
#ifdef RPMSG_NO_IPI
	/* Get poll shared memory device */
	ret = metal_device_open(prproc->shm_poll_bus_name,
				prproc->shm_poll_name,
				&dev);
	if (ret) {
		fprintf(stderr,
			"ERROR: failed to open shm poll device: %d.\r\n",
			ret);
		goto err1;
	}
	printf("Successfully open shm poll device.\r\n");
	prproc->shm_poll_dev = dev;
	prproc->shm_poll_io = metal_device_io_region(dev, 0);
	if (!prproc->shm_poll_io)
		goto err2;
	metal_io_write32(prproc->shm_poll_io, 0, !POLL_STOP);
#endif /* RPMSG_NO_IPI */
#endif

	mem_pa = metal_io_phys(prproc->shm_io, 0);
	remoteproc_init_mem(&prproc->shm_mem, "shm", mem_pa, mem_pa,
			    metal_io_region_size(prproc->shm_io),
			    prproc->shm_io);
	remoteproc_add_mem(rproc, &prproc->shm_mem);
	printf("Successfully added shared memory\r\n");
#ifdef RPMSG_IPI
	/* Get IPI device */
	ret = metal_device_open(prproc->ipi_bus_name, prproc->ipi_name,
				&dev);
	if (ret) {
		printf("failed to open ipi device: %d.\r\n", ret);
		goto err2;
	}
	prproc->ipi_dev = dev;
	prproc->ipi_io = metal_device_io_region(dev, 0);
	if (!prproc->ipi_io)
		goto err3;
	printf("Successfully probed IPI device\r\n");
	atomic_store(&prproc->ipi_nokick, 1);
	metal_io_write32(prproc->ipi_io, IPI_IER_OFFSET,
			 prproc->ipi_chn_mask);
#endif /* #ifdef RPMSG_IPI */

	printf("Successfully initialized Linux remoteproc.\r\n");
	return rproc;
#ifdef RPMSG_IPI
err3:
	metal_device_close(prproc->ipi_dev);
#endif /* #ifdef RPMSG_IPI */
err2:
	metal_device_close(prproc->shm_dev);
err1:
	return NULL;
}


static void linux_proc_remove(struct remoteproc *rproc)
{
	struct remoteproc_priv *prproc;

	if (!rproc)
		return;
	prproc = rproc->priv;
#ifdef RPMSG_IPI
	metal_io_write32(prproc->ipi_io, IPI_IDR_OFFSET, prproc->ipi_chn_mask);
	dev = prproc->ipi_dev;
	if (dev) {
		metal_irq_disable((uintptr_t)dev->irq_info);
		metal_irq_unregister((uintptr_t)dev->irq_info);
		metal_device_close(dev);
	}
#endif /* #ifdef RPMSG_IPI */
	if (prproc->shm_dev)
		metal_device_close(prproc->shm_dev);
}

static void *
linux_proc_mmap(struct remoteproc *rproc, metal_phys_addr_t *pa,
		metal_phys_addr_t *da, size_t size,
		unsigned int attribute, struct metal_io_region **io)
{
	struct remoteproc_priv *prproc;
	metal_phys_addr_t lpa, lda;
	struct metal_io_region *tmpio;

	(void)attribute;
	(void)size;
	if (!rproc)
		return NULL;
	prproc = rproc->priv;
	lpa = *pa;
	lda = *da;

	if (lpa == METAL_BAD_PHYS && lda == METAL_BAD_PHYS)
		return NULL;
	if (lpa == METAL_BAD_PHYS)
		lpa = lda;
	if (lda == METAL_BAD_PHYS)
		lda = lpa;
	tmpio = prproc->shm_io;
	if (!tmpio)
		return NULL;

	*pa = lpa;
	*da = lda;
	if (io)
		*io = tmpio;
	return metal_io_phys_to_virt(tmpio, lpa);
}

static int linux_proc_notify(struct remoteproc *rproc, uint32_t id)
{
#ifdef RPMSG_IPI
	struct remoteproc_priv *prproc;

	(void)id;
	if (!rproc)
		return -1;
	prproc = rproc->priv;

#ifdef RPMSG_NO_IPI
	metal_io_write32(prproc->shm_poll_io, 0, POLL_STOP);
#else /* RPMSG_NO_IPI */
	metal_io_write32(prproc->ipi_io, IPI_TRIG_OFFSET,
			 prproc->ipi_chn_mask);
#endif /* !RPMSG_NO_IPI */
#else
	(void)rproc;
	(void)id;
#endif /* #ifdef RPMSG_IPI */
	return 0;
}

/* processor operations from r5 to a53. It defines
 * notification operation and remote processor managementi operations. */
static struct remoteproc_ops linux_proc_ops = {
	.init = linux_proc_init,
	.remove = linux_proc_remove,
	.mmap = linux_proc_mmap,
	.notify = linux_proc_notify,
	.start = NULL,
	.stop = NULL,
	.shutdown = NULL,
};

/* RPMsg virtio shared buffer pool */
static struct rpmsg_virtio_shm_pool shpool;

static struct remoteproc *
platform_create_proc(int proc_index, int rsc_index)
{
	void *rsc_table;
	int rsc_size;
	int ret;
	metal_phys_addr_t pa;

	(void)proc_index;
	(void)rsc_index;
	rsc_size = RSC_MEM_SIZE;

	/* Initialize remoteproc instance */
	if (!remoteproc_init(&rproc_inst, &linux_proc_ops,
			     &rproc_priv))
		return NULL;
	printf("Successfully initialized remoteproc\r\n");

	/* Mmap resource table */
	pa = RSC_MEM_PA;
	printf("Calling mmap resource table.\r\n");
	rsc_table = remoteproc_mmap(&rproc_inst, &pa, NULL, rsc_size,
				    0, NULL);
	if (!rsc_table) {
		fprintf(stderr, "ERROR: Failed to mmap resource table.\r\n");
		return NULL;
	}
	printf("Successfully mmap resource table.\r\n");
#define OPENAMP_HACK_RSC
#ifndef OPENAMP_HACK_RSC
	/* parse resource table to remoteproc */
	ret = remoteproc_set_rsc_table(&rproc_inst, rsc_table, rsc_size);
	if (ret) {
		printf("Failed to intialize remoteproc\r\n");
		remoteproc_remove(&rproc_inst);
		return NULL;
	}
	printf("Successfully set resource table to remoteproc.\r\n");
#else
	printf("Skip setting resource table to remoteproc.\r\n");
#endif /* #ifndef OPENAMP_HACK_RSC */

	return &rproc_inst;
}

int platform_init(int argc, char *argv[], void **platform)
{
	unsigned long proc_id = 0;
	unsigned long rsc_id = 0;
	struct remoteproc *rproc;

	if (!platform) {
		fprintf(stderr, "Failed to initialize platform, NULL pointer"
			"to store platform data.\r\n");
		return -EINVAL;
	}
	/* Initialize HW system components */
	init_system();

	if (argc >= 2) {
		proc_id = strtoul(argv[1], NULL, 0);
	}

	if (argc >= 3) {
		rsc_id = strtoul(argv[2], NULL, 0);
	}

	rproc = platform_create_proc(proc_id, rsc_id);
	if (!rproc) {
		fprintf(stderr, "Failed to create remoteproc device.\r\n");
		return -EINVAL;
	}
	*platform = rproc;
	return 0;
}

struct  rpmsg_device *
platform_create_rpmsg_vdev(void *platform, unsigned int vdev_index,
			   unsigned int role,
			   void (*rst_cb)(struct virtio_device *vdev),
			   rpmsg_ns_bind_cb ns_bind_cb)
{
	struct remoteproc *rproc = platform;
	struct rpmsg_virtio_device *rpmsg_vdev;
	struct virtio_device *vdev;
	void *shbuf;
	struct metal_io_region *shbuf_io;
	int ret;

	/* Setup resource table */
	rpmsg_vdev = metal_allocate_memory(sizeof(*rpmsg_vdev));
	if (!rpmsg_vdev)
		return NULL;
	shbuf_io = remoteproc_get_io_with_pa(rproc, SHARED_BUF_PA);
	if (!shbuf_io)
		return NULL;
	shbuf = metal_io_phys_to_virt(shbuf_io, SHARED_BUF_PA);

	printf("creating remoteproc virtio\r\n");
	/* TODO: can we have a wrapper for the following two functions? */
	vdev = remoteproc_create_virtio(rproc, vdev_index, role, rst_cb);
	if (!vdev) {
		printf("failed remoteproc_create_virtio\r\n");
		goto err1;
	}

	printf("initializing rpmsg shared buffer pool\r\n");
	/* Only RPMsg virtio master needs to initialize the shared buffers pool */
	rpmsg_virtio_init_shm_pool(&shpool, shbuf, SHARED_BUF_SIZE);

	printf("initializing rpmsg vdev\r\n");
	/* RPMsg virtio slave can set shared buffers pool argument to NULL */
	ret =  rpmsg_init_vdev(rpmsg_vdev, vdev, ns_bind_cb,
			       shbuf_io,
			       &shpool);
	if (ret) {
		printf("failed rpmsg_init_vdev\r\n");
		goto err2;
	}
	return rpmsg_virtio_get_rpmsg_device(rpmsg_vdev);
err2:
	remoteproc_remove_virtio(rproc, vdev);
err1:
	metal_free_memory(rpmsg_vdev);
	return NULL;
}

int platform_poll(void *priv)
{
	struct remoteproc *rproc = priv;
	struct remoteproc_priv *prproc;
	unsigned int flags;
	int ret;

	prproc = rproc->priv;
	while(1) {
#ifdef RPMSG_NO_IPI
		if (metal_io_read32(prproc->shm_poll_io, 0)) {
			ret = remoteproc_get_notification(rproc,
							  RSC_NOTIFY_ID_ANY);
			if (ret)
				return ret;
			break;
		}
#else
		flags = metal_irq_save_disable();
		if (!(atomic_flag_test_and_set(&prproc->ipi_nokick))) {
			metal_irq_restore_enable(flags);
			ret = remoteproc_get_notification(rproc,
							  RSC_NOTIFY_ID_ANY);
			if (ret)
				return ret;
			break;
		}
		_rproc_wait();
		metal_irq_restore_enable(flags);
#endif /* RPMSG_NO_IPI */
	}
	return 0;
}

void platform_release_rpmsg_vdev(struct rpmsg_device *rpdev)
{
	(void)rpdev;
}

void platform_cleanup(void *platform)
{
	struct remoteproc *rproc = platform;

	if (rproc)
		remoteproc_remove(rproc);
	cleanup_system();
}
