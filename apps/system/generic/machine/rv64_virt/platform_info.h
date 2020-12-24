#ifndef PLATFORM_INFO_H_
#define PLATFORM_INFO_H_

#include <openamp/remoteproc.h>
#include <openamp/virtio.h>
#include <openamp/rpmsg.h>

#if defined __cplusplus
extern "C" {
#endif

#define PRINT printf

/* memory attributes */

/* Interrupt vectors */

/* POLL device */
#ifdef RPMSG_NO_IPI
#undef POLL_BASE_ADDR
//qemu virt_memmap hw/riscv/virt.c
//[VIRT_SHM_IPI] =     { 0x90000000,    0x00080000 },
#define POLL_BASE_ADDR 0x90000000UL
#define POLL_STOP 0x1U
#endif /* RPMSG_NO_IPI */

//TODO: move to common
#define SHM_BASE            0x90100000UL

#define RSC_MEM_PA          SHM_BASE
#define RSC_MEM_SIZE        0x00020000UL
//shared buffer: 0x90120000--0x90160000
#define SHARED_BUF_PA       (SHM_BASE + RSC_MEM_SIZE)
#define SHARED_BUF_SIZE     0x00040000UL
//TODO: move to common end

struct remoteproc_priv {
	const char *poll_dev_name;
	const char *poll_dev_bus_name;
	struct metal_device *poll_dev;
	struct metal_io_region *poll_io;
#ifndef RPMSG_NO_IPI
	unsigned int ipi_chn_mask; /**< IPI channel mask */
	atomic_int ipi_nokick;
#endif /* !RPMSG_NO_IPI */
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
