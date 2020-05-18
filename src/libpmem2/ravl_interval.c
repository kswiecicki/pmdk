// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * ravl_interval.c -- ravl_interval implementation
 */

#include "alloc.h"
#include "map.h"
#include "ravl_interval.h"
#include "pmem2_utils.h"
#include "sys_util.h"
#include "os_thread.h"
#include "ravl.h"

/*
 * ravl_interval_entry - structure holding an interval and address
 */
struct ravl_interval_entry {
	struct ravl_interval interval;
	void *addr;
};

/*
 * ravl_interval_compare -- compare intervals by left boundary
 */
static int
ravl_interval_compare(const void *lhs, const void *rhs)
{
	const struct ravl_interval *l = (struct ravl_interval *)lhs;
	const struct ravl_interval *r = (struct ravl_interval *)rhs;

	if (l->min < r->min)
		return -1;
	if (l->min > r->min)
		return 1;
	return 0;
}

/*
 * ravl_interval_delete - finalize the ravl interval module
 */
void
ravl_interval_delete(struct ravl **tree, os_rwlock_t *lock)
{
	ravl_delete(*tree);
	*tree = NULL;
	os_rwlock_destroy(lock);
}

/*
 * ravl_interval_new -- initialize the ravl interval module
 */
int
ravl_interval_new(struct ravl **tree, os_rwlock_t *lock)
{
	os_rwlock_init(lock);
	*tree = ravl_new_sized(ravl_interval_compare,
		sizeof(struct ravl_interval_entry));

	if (!(*tree))
		return PMEM2_E_ERRNO;

	return 0;
}

/*
 * ravl_interval_add -- add interval entry to the tree
 */
int
ravl_interval_add(struct ravl **tree, os_rwlock_t *lock,
		struct ravl_interval ri, void *addr)
{
	int ret;

	struct ravl_interval_entry rie;
	rie.interval = ri;
	rie.addr = addr;

	util_rwlock_wrlock(lock);
	ret = ravl_emplace_copy(*tree, &rie);
	util_rwlock_unlock(lock);

	if (ret)
		return PMEM2_E_ERRNO;

	return 0;
}

/*
 * ravl_interval_remove -- remove interval entry from the tree
 */
int
ravl_interval_remove(struct ravl **tree, os_rwlock_t *lock,
		struct ravl_interval ri)
{
	int ret = 0;

	util_rwlock_wrlock(lock);
	struct ravl_node *n = ravl_find(*tree, &ri, RAVL_PREDICATE_EQUAL);

	if (n)
		ravl_remove(*tree, n);
	else
		ret = PMEM2_E_MAPPING_NOT_FOUND;

	util_rwlock_unlock(lock);

	return ret;
}

/*
 * ravl_interval_find_prior_or_eq -- find overlapping interval starting prior to
 *                                   the current one or at the same place
 */
static void *
ravl_interval_find_prior_or_eq(struct ravl **tree, struct ravl_interval *ri)
{
	struct ravl_node *n;
	struct ravl_interval_entry *cur;

	n = ravl_find(*tree, ri, RAVL_PREDICATE_LESS_EQUAL);
	if (!n)
		return NULL;

	cur = ravl_data(n);
	/*
	 * If the end of the found mapping is below the searched address, then
	 * this is not our mapping.
	 */
	if (cur->interval.max <= ri->min)
		return NULL;

	return cur->addr;
}

/*
 * ravl_interval_find_later -- find overlapping interval starting later than
 *                             the current one
 */
static void *
ravl_interval_find_later(struct ravl **tree, struct ravl_interval *ri)
{
	struct ravl_node *n;
	struct ravl_interval_entry *cur;

	n = ravl_find(*tree, ri, RAVL_PREDICATE_LESS_EQUAL);
	if (!n)
		return NULL;

	cur = ravl_data(n);

	/*
	 * If the beginning of the found interval is above the end of
	 * the searched range, then this is not our interval.
	 */
	if (cur->interval.min >= ri->max)
		return NULL;

	return cur->addr;
}

/*
 * ravl_interval_find -- find the earliest interval with (min, max) range
 */
void *
ravl_interval_find(struct ravl **tree, os_rwlock_t *lock,
		struct ravl_interval ri, void *map)
{
	void *addr;

	util_rwlock_rdlock(lock);

	addr = ravl_interval_find_prior_or_eq(tree, &ri);
	if (!addr)
		addr = ravl_interval_find_later(tree, &ri);

	util_rwlock_unlock(lock);

	return addr;
}
