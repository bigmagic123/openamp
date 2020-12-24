/*
 * Memory based image store Operation
 *
 * Copyright(c) 2018 Xilinx Ltd.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <common.h>

struct mem_file {
	const void *base;
};

int mem_image_open(void *store, const char *path, const void **image_data)
{
	struct mem_file *image = store;
	int fd = -1;
	struct stat file_stat;
	char *buf = NULL;
	int ret = -1;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;

	memset(&file_stat, 0, sizeof(file_stat));
	if (fstat(fd, &file_stat))
		return -1;

	LPRINTF("file size %d\n", file_stat.st_size);
	buf = malloc(file_stat.st_size);
	if (!buf)
		return -1;

	ret = read(fd, buf, file_stat.st_size);
	if (ret > 0) {
		LPRINTF("%d read\n", ret);
	} else if (ret < 0) {
		LPRINTF("%s\n", strerror(errno));
		return ret;
	}
	*image_data = buf;
	image->base = buf;
	return file_stat.st_size;
}

void mem_image_close(void *store)
{
	//TODO
	/* The image is in memory, does nothing */
	(void)store;
}

int mem_image_load(void *store, size_t offset, size_t size,
		   const void **data, metal_phys_addr_t pa,
		   struct metal_io_region *io,
		   char is_blocking)
{
	struct mem_file *image = store;
	const void *fw_base = image->base;

	(void)is_blocking;

	LPRINTF("%s: offset=0x%x, size=0x%x\n\r",
		__func__, offset, size);
	if (pa == METAL_BAD_PHYS) {
		if (data == NULL) {
			LPERROR("%s: data is NULL while pa is ANY\r\n",
				__func__);
			return -EINVAL;
		}
		*data = (const void *)((const char *)fw_base + offset);
	} else {
		void *va;

		if (io == NULL) {
			LPERROR("%s, io is NULL while pa is not ANY\r\n",
				__func__);
			return -EINVAL;
		}
		va = metal_io_phys_to_virt(io, pa);
		if (va == NULL) {
			LPERROR("%s: no va is found\r\n", __func__);
			return -EINVAL;
		}
		LPRINTF("Copy to %p (pa: %lx) with size 0x%lx\n", va, pa, size);
		memcpy(va, (const void *)((const char *)fw_base + offset), size);
	}

	return (int)size;
}

struct image_store_ops mem_image_store_ops = {
	.open = mem_image_open,
	.close = mem_image_close,
	.load = mem_image_load,
	.features = SUPPORT_SEEK,
};

