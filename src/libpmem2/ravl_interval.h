// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * ravl_interval.h -- internal definitions for ravl_interval
 */

#ifndef RAVL_INTERVAL_H
#define RAVL_INTERVAL_H

#include "libpmem2.h"
#include "os_thread.h"
#include "ravl.h"

/*
 * ravl_interval - structure representing two points
 *                 on the number line
 */
struct ravl_interval {
	size_t min;
	size_t max;
};

int ravl_interval_new(struct ravl **tree, os_rwlock_t *lock);
void  ravl_interval_delete(struct ravl **tree, os_rwlock_t *lock);
void *ravl_interval_find(struct ravl **tree, os_rwlock_t *lock,
		struct ravl_interval ri, void *map);
int ravl_interval_add(struct ravl **tree, os_rwlock_t *lock,
		struct ravl_interval ri, void *addr);
int ravl_interval_remove(struct ravl **tree, os_rwlock_t *lock,
		struct ravl_interval ri);

#endif
