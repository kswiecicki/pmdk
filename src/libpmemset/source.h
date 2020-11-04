/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2020, Intel Corporation */

/*
 * source.h -- internal definitions for pmemset_source
 */
#ifndef PMEMSET_SOURCE_H
#define PMEMSET_SOURCE_H

enum pmemset_source_type {
	PMEMSET_SOURCE_UNSPECIFIED,
	PMEMSET_SOURCE_PMEM2,
	PMEMSET_SOURCE_FILE,

	MAX_PMEMSET_SOURCE_TYPE
};

int pmemset_source_validate(const struct pmemset_source *src);

int pmemset_source_get_pmem2_map(const struct pmemset_source *src,
		struct pmem2_config *cfg, struct pmem2_map **map);

#endif /* PMEMSET_SOURCE_H */
