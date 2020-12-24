/*
 * RV64 Virt APU life cycle management remoteproc example implementation
 *
 * Copyright(c) 2020 Bamvor Jian ZHANG
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common.h>
#include <metal/device.h>

#define DEV_BUS_NAME	"platform"
#define RPROC_DEV_NAME  "90200000.rproc"
static struct metal_io_region *rproc_io;	/**< pointer to rproc i/o region */
static struct metal_device *rproc_dev;		/**< pointer to rproc device */
struct remoteproc_mem rproc_mem;		/**< rproc memory */

static struct remoteproc *apu_rproc_init(struct remoteproc *rproc,
				 struct remoteproc_ops *ops, void *arg)
{
	struct rproc_priv *priv;
	unsigned int cpu_id = *((unsigned int *)arg);
	struct metal_device *dev;
	metal_phys_addr_t rproc_pa;
	int ret = -1;

	if(cpu_id < RV_NODE_APU_0 || cpu_id > RV_NODE_APU_N) {
		LPERROR("%s: invalid node id: %d \n\r",__func__, cpu_id);
		return NULL;
	}

	LPRINTF("%s: node id: %d\n\r", __func__, cpu_id);
	priv = metal_allocate_memory(sizeof(*priv));
	if (!priv)
		return NULL;

	memset(priv, 0, sizeof(*priv));
	priv->rproc = rproc;
	priv->cpu_id = cpu_id;
	priv->rproc->ops = ops;
	metal_list_init(&priv->rproc->mems);
	priv->rproc->priv = priv;
	rproc->state = RPROC_READY;
	/* Get remoteproc device */
	ret = metal_device_open(DEV_BUS_NAME, RPROC_DEV_NAME,
				&dev);
	if (ret) {
		LPERROR("ERROR: failed to open shm device: %d.\r\n",
			ret);
		goto err_open;
	}
	LPRINTF("Successfully open rproc device.\r\n");
	rproc_dev = dev;
	rproc_io = metal_device_io_region(dev, 0);
	if (!rproc_io)
		goto err_get_io;

	rproc_pa = metal_io_phys(rproc_io, 0);
	remoteproc_init_mem(&rproc_mem, "rproc", rproc_pa, rproc_pa,
			    metal_io_region_size(rproc_io),
			    rproc_io);
	remoteproc_add_mem(rproc, &rproc_mem);
	LPRINTF("Successfully added rproc shared memory\r\n");
	return priv->rproc;

err_get_io:
	metal_device_close(rproc_dev);
err_open:
	return NULL;
}

static void apu_rproc_remove(struct remoteproc *rproc)
{
	struct rproc_priv *priv;

	priv = (struct rproc_priv *)rproc->priv;
	metal_free_memory(priv);
}

static void *apu_rproc_mmap(struct remoteproc *rproc,
		    metal_phys_addr_t *pa, metal_phys_addr_t *da,
		    size_t size, unsigned int attribute,
		    struct metal_io_region **io)
{
	metal_phys_addr_t lpa, lda;
	struct metal_io_region *tmpio;

	(void)attribute;
	(void)size;
	(void)rproc;

	lpa = *pa;
	lda = *da;

	if (lpa == METAL_BAD_PHYS && lda == METAL_BAD_PHYS)
		return NULL;
	if (lpa == METAL_BAD_PHYS)
		lpa = lda;
	if (lda == METAL_BAD_PHYS)
		lda = lpa;
	tmpio = rproc_io;
	if (!tmpio)
		return NULL;

	*pa = lpa;
	*da = lda;
	if (io)
		*io = tmpio;
	return metal_io_phys_to_virt(tmpio, lpa);
}

#if 0
static int apu_rproc_start(struct remoteproc *rproc)
{
	struct rproc_priv *priv;
	int ret;

	priv = rproc->priv;
	ret = XPm_RequestWakeUp(priv->cpu_id, true,rproc->bootaddr,
		REQUEST_ACK_NO);
	if (ret != XST_SUCCESS) {
		LPRINTF("%s: Failed to start APU 0x%x, ret=0x%x\n\r",
			__func__, priv->cpu_id, ret);
		return -1;
	} 
	return 0;
}

static int apu_rproc_stop(struct remoteproc *rproc)
{
	/* It is lacking a stop operation in the libPM */
	(void)rproc;
	return 0;
}

static int apu_rproc_shutdown(struct remoteproc *rproc)
{
	struct rproc_priv *priv;
	int ret;
	struct remoteproc_mem *mem;
	struct metal_list *node;

	priv = rproc->priv;
	/* Delete all the registered remoteproc memories */
	metal_list_for_each(&rproc->mems, node) {
		struct metal_list *tmpnode;
		metal_phys_addr_t pa, pa_end;

		mem = metal_container_of(node, struct remoteproc_mem, node);
		tmpnode = node;
		/* Release TCM resource */
		pa = mem->pa;
		pa_end = metal_io_phys(mem->io, metal_io_region_size(mem->io));
		if (pa_end <= 0x7FFFFFFF)
			XPm_ReleaseNode(NODE_DDR);
		if (pa >= 0xFFFC0000 && pa < 0xFFFD0000)
			XPm_ReleaseNode(NODE_OCM_BANK_0);
		if (pa <= 0xFFFDFFFF && pa_end >= 0xFFFD0000)
			XPm_ReleaseNode(NODE_OCM_BANK_1);
		if (pa <= 0xFFFEFFFF && pa_end >= 0xFFFE0000)
			XPm_ReleaseNode(NODE_OCM_BANK_2);
		if (pa_end >= 0xFFFF0000)
			XPm_ReleaseNode(NODE_OCM_BANK_3);

		node = tmpnode->prev;
		metal_list_del(tmpnode);
		metal_free_memory(mem->io);
		metal_free_memory(mem);
	}
	ret = XPm_ForcePowerDown(priv->cpu_id, REQUEST_ACK_NO);
	if (ret != XST_SUCCESS)
		return -1;
	return 0;
}
#endif

struct remoteproc_ops virt_rv64_rproc_ops = {
    .init = apu_rproc_init,
    .remove = apu_rproc_remove,
//    .start = apu_rproc_start,
//    .stop = apu_rproc_stop,
//    .shutdown = apu_rproc_shutdown,
    .mmap = apu_rproc_mmap,
};
