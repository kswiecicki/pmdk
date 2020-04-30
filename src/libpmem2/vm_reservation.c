// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "libpmem2.h"

/*
 * vm_reservation.c -- implementation of virtual memory allocation API
 */

int
pmem2_vm_reservation_new(struct pmem2_vm_reservation **rsv,
		size_t size, void *address)
{
	return PMEM2_E_NOSUPP;
}

int
pmem2_vm_reservation_delete(struct pmem2_vm_reservation **rsv)
{
	return PMEM2_E_NOSUPP;
}
