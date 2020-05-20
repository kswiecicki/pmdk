// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

/*
 * map.c -- pmem2_map (common)
 */

#include "out.h"

#include "config.h"
#include "map.h"
#include "ravl_interval.h"
#include "os.h"
#include "os_thread.h"
#include "pmem2.h"
#include "pmem2_utils.h"
#include "ravl.h"
#include "sys_util.h"

#include <libpmem2.h>

/*
 * pmem2_map_get_address -- get mapping address
 */
void *
pmem2_map_get_address(struct pmem2_map *map)
{
	LOG(3, "map %p", map);

	return map->addr;
}

/*
 * pmem2_map_get_size -- get mapping size
 */
size_t
pmem2_map_get_size(struct pmem2_map *map)
{
	LOG(3, "map %p", map);

	return map->content_length;
}

/*
 * pmem2_map_get_store_granularity -- returns granularity of the mapped
 * file
 */
enum pmem2_granularity
pmem2_map_get_store_granularity(struct pmem2_map *map)
{
	LOG(3, "map %p", map);

	return map->effective_granularity;
}

/*
 * parse_force_granularity -- parse PMEM2_FORCE_GRANULARITY environment variable
 */
static enum pmem2_granularity
parse_force_granularity()
{
	char *ptr = os_getenv("PMEM2_FORCE_GRANULARITY");
	if (ptr) {
		char str[11]; /* strlen("CACHE_LINE") + 1 */

		if (util_safe_strcpy(str, ptr, sizeof(str))) {
			LOG(1, "Invalid value of PMEM2_FORCE_GRANULARITY");
			return PMEM2_GRANULARITY_INVALID;
		}

		char *s = str;
		while (*s) {
			*s = (char)toupper((char)*s);
			s++;
		}

		if (strcmp(str, "BYTE") == 0) {
			return PMEM2_GRANULARITY_BYTE;
		} else if (strcmp(str, "CACHE_LINE") == 0) {
			return PMEM2_GRANULARITY_CACHE_LINE;
		} else if (strcmp(str, "CACHELINE") == 0) {
			return PMEM2_GRANULARITY_CACHE_LINE;
		} else if (strcmp(str, "PAGE") == 0) {
			return PMEM2_GRANULARITY_PAGE;
		}

		LOG(1, "Invalid value of PMEM2_FORCE_GRANULARITY");
	}
	return PMEM2_GRANULARITY_INVALID;
}

/*
 * get_min_granularity -- checks min available granularity
 */
enum pmem2_granularity
get_min_granularity(bool eADR, bool is_pmem, enum pmem2_sharing_type sharing)
{
	enum pmem2_granularity force = parse_force_granularity();
	/* PMEM2_PRIVATE sharing does not require data flushing */
	if (sharing == PMEM2_PRIVATE)
		return PMEM2_GRANULARITY_BYTE;
	if (force != PMEM2_GRANULARITY_INVALID)
		return force;
	if (!is_pmem)
		return PMEM2_GRANULARITY_PAGE;
	if (!eADR)
		return PMEM2_GRANULARITY_CACHE_LINE;

	return PMEM2_GRANULARITY_BYTE;
}

/*
 * pmem2_validate_offset -- verify if the offset is a multiple of
 * the alignment required for the config
 */
int
pmem2_validate_offset(const struct pmem2_config *cfg, size_t *offset,
	size_t alignment)
{
	ASSERTne(alignment, 0);
	if (cfg->offset % alignment) {
		ERR("offset is not a multiple of %lu", alignment);
		return PMEM2_E_OFFSET_UNALIGNED;
	}

	*offset = cfg->offset;

	return 0;
}

static struct ravl *tree;
static os_rwlock_t lock;

/*
 * pmem2_map_init -- initialize the map module
 */
void
pmem2_map_init(void)
{
	if (ravl_interval_new(&tree, &lock))
		abort();
}

/*
 * pmem2_map_fini -- finalize the map module
 */
void
pmem2_map_fini(void)
{
	ravl_interval_delete(&tree, &lock);
}

/*
 * pmem2_register_mapping -- register mapping in the mappings tree
 */
int
pmem2_register_mapping(struct pmem2_map *map)
{
	struct ravl_interval ri;
	ri.min = (size_t)pmem2_map_get_address(map);
	ri.max = ri.min + pmem2_map_get_size(map);

	return ravl_interval_add(&tree, &lock, ri, map);
}

/*
 * pmem2_unregister_mapping -- unregister mapping from the mappings tree
 */
int
pmem2_unregister_mapping(struct pmem2_map *map)
{
	struct ravl_interval ri;
	ri.min = (size_t)pmem2_map_get_address(map);
	ri.max = ri.min + pmem2_map_get_size(map);

	return ravl_interval_remove(&tree, &lock, ri);
}

/*
 * pmem2_map_find -- find the earliest mapping overlapping with
 * (addr, addr+size) range
 */
struct pmem2_map *
pmem2_map_find(const void *addr, size_t len)
{
	struct ravl_interval ri;
	ri.min = (size_t)addr;
	ri.max = ri.min + len;

	return ravl_interval_find(&tree, &lock, ri, (void *)addr);
}
