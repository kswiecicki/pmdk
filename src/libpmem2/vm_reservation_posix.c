// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * vm_reservation_posix.c -- implementation of virtual memory
 *                           reservation API (POSIX)
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>

#include "alloc.h"
#include "libpmem2.h"
#include "map.h"
#include "out.h"
#include "pmem2_utils.h"

/*
 * pmem2_vm_reservation_new -- creates new virtual memory reservation
 */
int
pmem2_vm_reservation_new(struct pmem2_vm_reservation **rsv,
		size_t size, void *addr)
{
	int ret;
	(*rsv) = pmem2_malloc(sizeof(struct pmem2_vm_reservation), &ret);
	if (ret) {
		ERR("!pmem2_malloc");
		return PMEM2_E_ERRNO;
	}

	/* initialize the ravl interval tree */
	vm_reservation_init(*rsv);

	int map_flag = 0;
	if (addr) {
/*
 * glibc started exposing MAP_FIXED_NOREPLACE flag in version 4.17,
 * but even if the flag is not supported, we can imitate its behavior
 */
#ifdef MAP_FIXED_NOREPLACE
	map_flag = MAP_FIXED_NOREPLACE;
#else
	map_flag = 0;
#endif
	}

	/*
	 * Create a dummy mapping to find an unused region of given size.
	 * If the flag is supported and requested region is occupied,
	 * mmap will fail with EEXIST.
	 */
	char *daddr = mmap(addr, size, PROT_NONE,
			MAP_PRIVATE | MAP_ANONYMOUS | map_flag, -1, 0);
	if (daddr == MAP_FAILED) {
		if (errno == EEXIST) {
			ERR("!mmap MAP_FIXED_NOREPLACE");
			return PMEM2_E_MAPPING_EXISTS;
		}
		ERR("!mmap MAP_ANONYMOUS");
		return PMEM2_E_ERRNO;
	}

	/*
	 * When requested address is not specified, any returned address
	 * is acceptable. If kernel does not support flag and given addr
	 * is occupied, kernel chooses new addr randomly and returns it.
	 * We do not want that behavior, so we validate it and fail when
	 * addresses do not match.
	 */
	if (addr && daddr != addr) {
		munmap(daddr, size);
		ERR("mapping exists in the given address");
		return PMEM2_E_MAPPING_EXISTS;
	}

	(*rsv)->addr = daddr;
	(*rsv)->size = roundup(size, Pagesize);

	return 0;
}

/*
 * pmem2_vm_reservation_delete -- deletes reservation bound to
 *                                structure pmem2_vm_reservation
 */
int
pmem2_vm_reservation_delete(struct pmem2_vm_reservation **rsv)
{
	if (munmap((*rsv)->addr, (*rsv)->size)) {
		ERR("!munmap");
		return PMEM2_E_ERRNO;
	}

	vm_reservation_fini(*rsv);
	Free(*rsv);

	return 0;
}
