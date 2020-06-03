// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * pmem2_vm_reservation.c -- pmem2_vm_reservation unittests
 */

#include <stdbool.h>

#include "config.h"
#include "pmem2_utils.h"
#include "source.h"
#include "map.h"
#include "out.h"
#include "pmem2.h"
#include "unittest.h"
#include "ut_pmem2.h"
#include "ut_pmem2_setup.h"

/*
 * get_align_by_name -- fetch map alignment for an unopened file
 */
static size_t
get_align_by_name(const char *filename)
{
	struct pmem2_source *src;
	size_t align;
	int fd = OPEN(filename, O_RDONLY);
	PMEM2_SOURCE_FROM_FD(&src, fd);
	PMEM2_SOURCE_ALIGNMENT(src, &align);
	PMEM2_SOURCE_DELETE(&src);
	CLOSE(fd);

	return align;
}

/*
 * offset_align_to_devdax - align offset of the virtual memory reservation
 *                          to device DAX granularity
 */
static size_t
offset_align_to_devdax(void *rsv_addr, size_t alignment)
{
	/*
	 * Address of the vm_reservation, is always aligned to the OS allocation
	 * granularity. DevDax demands its own granularity, we need to calculate
	 * the offset, so that (reservation address + offset) is aligned to the
	 * closest address, contained in the vm reservation, compatible with
	 * DevDax granularity.
	 */
	size_t mod_align = (size_t)rsv_addr % alignment;
	if (mod_align)
		return (alignment - mod_align);

	return 0;
}

/*
 * test_vm_reserv_new_region_occupied_map - create a reservation
 * in the region belonging to existing mapping
 */
static int
test_vm_reserv_new_region_occupied_map(const struct test_case *tc,
		int argc, char *argv[])
{
	if (argc < 2)
		UT_FATAL("usage: test_vm_reserv_new_region_occupied_map "
				"<file> <size>");

	char *file = argv[0];
	size_t size = ATOUL(argv[1]);
	void *addr;
	struct FHandle *fh;
	struct pmem2_config cfg;
	struct pmem2_map *map;
	struct pmem2_source *src;
	struct pmem2_vm_reservation *rsv;

	ut_pmem2_prepare_config(&cfg, &src, &fh, FH_FD, file, 0, 0, FH_RDWR);

	/* map a region of virtual address space */
	int ret = pmem2_map(&cfg, src, &map);
	UT_ASSERTeq(ret, 0);

	addr = pmem2_map_get_address(map);
	UT_ASSERTne(addr, NULL);

	/* create a reservation in the region occupied by existing mapping */
	ret = pmem2_vm_reservation_new(&rsv, addr, size);
	UT_PMEM2_EXPECT_RETURN(ret, PMEM2_E_MAPPING_EXISTS);

	ret = pmem2_unmap(&map);
	UT_ASSERTeq(ret, 0);
	UT_ASSERTeq(map, NULL);
	PMEM2_SOURCE_DELETE(&src);
	UT_FH_CLOSE(fh);

	return 2;
}

/*
 * test_vm_reserv_new_region_occupied_reserv - create a vm reservation
 * in the region belonging to other existing vm reservation
 */
static int
test_vm_reserv_new_region_occupied_reserv(const struct test_case *tc,
		int argc, char *argv[])
{
	if (argc < 2)
		UT_FATAL("usage: test_vm_reserv_new_region_occupied_reserv "
				"<file> <size>");

	size_t size = ATOUL(argv[1]);
	void *rsv_addr;
	struct pmem2_vm_reservation *rsv1;
	struct pmem2_vm_reservation *rsv2;

	/* reserve a region in the virtual address space */
	int ret = pmem2_vm_reservation_new(&rsv1, NULL, size);
	UT_ASSERTeq(ret, 0);

	rsv_addr = pmem2_vm_reservation_get_address(rsv1);
	UT_ASSERTne(rsv_addr, NULL);
	UT_ASSERTeq(pmem2_vm_reservation_get_size(rsv1), size);

	/*
	 * Make a vm reservation of the region occupied by other
	 * existing reservation.
	 */
	ret = pmem2_vm_reservation_new(&rsv2, rsv_addr, size);
	UT_PMEM2_EXPECT_RETURN(ret, PMEM2_E_MAPPING_EXISTS);

	ret = pmem2_vm_reservation_delete(&rsv1);
	UT_ASSERTeq(ret, 0);

	return 2;
}

/*
 * test_vm_reserv_map_file - map a file to a vm reservation
 */
static int
test_vm_reserv_map_file(const struct test_case *tc,
		int argc, char *argv[])
{
	if (argc < 2)
		UT_FATAL("usage: test_vm_reserv_map_file <file> <size>");

	char *file = argv[0];
	size_t size = ATOUL(argv[1]);
	size_t alignment = get_align_by_name(file);
	void *rsv_addr;
	size_t rsv_offset;
	size_t rsv_size;
	struct FHandle *fh;
	struct pmem2_config cfg;
	struct pmem2_map *map;
	struct pmem2_vm_reservation *rsv;
	struct pmem2_source *src;

	rsv_size = size + alignment;

	int ret = pmem2_vm_reservation_new(&rsv, NULL, rsv_size);
	UT_ASSERTeq(ret, 0);

	rsv_addr = pmem2_vm_reservation_get_address(rsv);
	UT_ASSERTne(rsv_addr, 0);
	UT_ASSERTeq(pmem2_vm_reservation_get_size(rsv), rsv_size);

	/* in case of DevDax */
	rsv_offset = offset_align_to_devdax(rsv_addr, alignment);

	ut_pmem2_prepare_config(&cfg, &src, &fh, FH_FD, file, 0, 0, FH_RDWR);
	pmem2_config_set_vm_reservation(&cfg, rsv, rsv_offset);

	ret = pmem2_map(&cfg, src, &map);
	UT_PMEM2_EXPECT_RETURN(ret, 0);

	UT_ASSERTeq(pmem2_map_get_address(map), (char *)rsv_addr + rsv_offset);

	ret = pmem2_unmap(&map);
	UT_ASSERTeq(ret, 0);
	UT_ASSERTeq(map, NULL);
	ret = pmem2_vm_reservation_delete(&rsv);
	UT_ASSERTeq(ret, 0);
	PMEM2_SOURCE_DELETE(&src);
	UT_FH_CLOSE(fh);

	return 2;
}

/*
 * test_vm_reserv_map_part_file - map a part of the file to a vm reservation
 */
static int
test_vm_reserv_map_part_file(const struct test_case *tc,
		int argc, char *argv[])
{
	if (argc < 2)
		UT_FATAL("usage: test_vm_reserv_map_part_file <file> <size>");

	char *file = argv[0];
	size_t size = ATOUL(argv[1]);
	size_t alignment = get_align_by_name(file);
	size_t offset = 0;
	void *rsv_addr;
	size_t rsv_offset;
	size_t rsv_size;
	struct FHandle *fh;
	struct pmem2_config cfg;
	struct pmem2_map *map;
	struct pmem2_vm_reservation *rsv;
	struct pmem2_source *src;

	/* map only part of the file */
	offset = ALIGN_DOWN(size / 2, alignment);

	/* reservation size is not big enough for the whole file */
	rsv_size = size - offset + alignment;

	int ret = pmem2_vm_reservation_new(&rsv, NULL, rsv_size);
	UT_ASSERTeq(ret, 0);

	rsv_addr = pmem2_vm_reservation_get_address(rsv);
	UT_ASSERTne(rsv_addr, 0);
	UT_ASSERTeq(pmem2_vm_reservation_get_size(rsv), rsv_size);

	/* in case of DevDax */
	rsv_offset = offset_align_to_devdax(rsv_addr, alignment);

	ut_pmem2_prepare_config(&cfg, &src, &fh, FH_FD, file, 0, offset,
			FH_RDWR);
	pmem2_config_set_vm_reservation(&cfg, rsv, rsv_offset);

	ret = pmem2_map(&cfg, src, &map);
	UT_PMEM2_EXPECT_RETURN(ret, 0);

	UT_ASSERTeq(pmem2_map_get_address(map), (char *)rsv_addr + rsv_offset);

	ret = pmem2_unmap(&map);
	UT_ASSERTeq(ret, 0);
	UT_ASSERTeq(map, NULL);
	ret = pmem2_vm_reservation_delete(&rsv);
	UT_ASSERTeq(ret, 0);
	PMEM2_SOURCE_DELETE(&src);
	UT_FH_CLOSE(fh);

	return 2;
}

/*
 * test_vm_reserv_delete_contains_mapping - delete a vm reservation that
 *                                          contains mapping
 */
static int
test_vm_reserv_delete_contains_mapping(const struct test_case *tc,
		int argc, char *argv[])
{
	if (argc < 2)
		UT_FATAL("usage: test_vm_reserv_delete_contains_mapping "
				"<file> <size>");

	char *file = argv[0];
	size_t size = ATOUL(argv[1]);
	size_t alignment = get_align_by_name(file);
	void *rsv_addr;
	size_t rsv_size;
	size_t rsv_offset;
	struct FHandle *fh;
	struct pmem2_config cfg;
	struct pmem2_map *map;
	struct pmem2_source *src;
	struct pmem2_vm_reservation *rsv;

	rsv_size = size + alignment;

	/* create a reservation in the virtual memory */
	int ret = pmem2_vm_reservation_new(&rsv, NULL, rsv_size);
	UT_ASSERTeq(ret, 0);

	rsv_addr = pmem2_vm_reservation_get_address(rsv);
	UT_ASSERTne(rsv_addr, 0);
	UT_ASSERTeq(pmem2_vm_reservation_get_size(rsv), rsv_size);

	/* in case of DevDax */
	rsv_offset = offset_align_to_devdax(rsv_addr, alignment);

	ut_pmem2_prepare_config(&cfg, &src, &fh, FH_FD, file, 0, 0, FH_RDWR);
	pmem2_config_set_vm_reservation(&cfg, rsv, rsv_offset);

	/* create a mapping in the reserved region */
	ret = pmem2_map(&cfg, src, &map);
	UT_ASSERTeq(ret, 0);

	/* delete the reservation while it contains a mapping */
	ret = pmem2_vm_reservation_delete(&rsv);
	UT_PMEM2_EXPECT_RETURN(ret, PMEM2_E_VM_RESERVATION_NOT_EMPTY);

	ret = pmem2_unmap(&map);
	UT_PMEM2_EXPECT_RETURN(ret, 0);
	UT_ASSERTeq(map, NULL);
	ret = pmem2_vm_reservation_delete(&rsv);
	UT_ASSERTeq(ret, 0);
	PMEM2_SOURCE_DELETE(&src);
	UT_FH_CLOSE(fh);

	return 2;
}

/*
 * test_vm_reserv_map_unmap_multiple_files - map multiple files to a
 * vm reservation, then unmap every 2nd mapping and map the mapping again
 */
static int
test_vm_reserv_map_unmap_multiple_files(const struct test_case *tc,
		int argc, char *argv[])
{
	if (argc < 2)
		UT_FATAL("usage: test_vm_reserv_map_unmap_multiple_files "
				"<file> <size>");

	char *file = argv[0];
	size_t size = ATOUL(argv[1]);
	size_t alignment = get_align_by_name(file);
	void *rsv_addr;
	size_t rsv_offset;
	size_t rsv_size;
	struct FHandle *fh;
	struct pmem2_config cfg;
	struct pmem2_map **map;
	struct pmem2_vm_reservation *rsv;
	struct pmem2_source *src;
	size_t NMAPPINGS = 10;

	map = MALLOC(sizeof(struct pmem2_map *) * NMAPPINGS);

	rsv_size = NMAPPINGS * size + alignment;

	int ret = pmem2_vm_reservation_new(&rsv, NULL, rsv_size);
	UT_ASSERTeq(ret, 0);

	rsv_addr = pmem2_vm_reservation_get_address(rsv);
	UT_ASSERTne(rsv_addr, NULL);
	UT_ASSERTeq(pmem2_vm_reservation_get_size(rsv), rsv_size);

	/* in case of DevDax */
	size_t align_offset = offset_align_to_devdax(rsv_addr, alignment);
	rsv_offset = align_offset;

	ut_pmem2_prepare_config(&cfg, &src, &fh, FH_FD, file, 0, 0, FH_RDWR);

	for (size_t i = 0; i < NMAPPINGS; i++, rsv_offset += size) {
		pmem2_config_set_vm_reservation(&cfg, rsv, rsv_offset);

		ret = pmem2_map(&cfg, src, &map[i]);
		UT_PMEM2_EXPECT_RETURN(ret, 0);

		UT_ASSERTeq((char *)rsv_addr + rsv_offset,
				pmem2_map_get_address(map[i]));
	}

	for (size_t i = 0; i < NMAPPINGS; i += 2) {
		ret = pmem2_unmap(&map[i]);
		UT_ASSERTeq(ret, 0);
		UT_ASSERTeq(map[i], NULL);
	}

	rsv_offset = align_offset;
	for (size_t i = 0; i < NMAPPINGS; i += 2, rsv_offset += 2 * size) {
		pmem2_config_set_vm_reservation(&cfg, rsv, rsv_offset);

		ret = pmem2_map(&cfg, src, &map[i]);
		UT_PMEM2_EXPECT_RETURN(ret, 0);

		UT_ASSERTeq((char *)rsv_addr + rsv_offset,
				pmem2_map_get_address(map[i]));
	}

	for (size_t i = 0; i < NMAPPINGS; i++) {
		ret = pmem2_unmap(&map[i]);
		UT_ASSERTeq(ret, 0);
		UT_ASSERTeq(map[i], NULL);
	}

	ret = pmem2_vm_reservation_delete(&rsv);
	UT_ASSERTeq(ret, 0);
	PMEM2_SOURCE_DELETE(&src);
	UT_FH_CLOSE(fh);
	FREE(map);

	return 2;
}

/*
 * test_vm_reserv_map_insufficient_space - map a file to a vm reservation
 *                                         with insufficient space
 */
static int
test_vm_reserv_map_insufficient_space(const struct test_case *tc,
		int argc, char *argv[])
{
	if (argc < 2)
		UT_FATAL("usage: test_vm_reserv_map_insufficient_space "
				"<file> <size>");

	char *file = argv[0];
	size_t size = ATOUL(argv[1]);
	void *rsv_addr;
	size_t rsv_size;
	struct FHandle *fh;
	struct pmem2_config cfg;
	struct pmem2_map *map;
	struct pmem2_vm_reservation *rsv;
	struct pmem2_source *src;

	rsv_size = size / 2;

	int ret = pmem2_vm_reservation_new(&rsv, NULL, rsv_size);
	UT_ASSERTeq(ret, 0);
	UT_ASSERTeq(pmem2_vm_reservation_get_size(rsv), rsv_size);

	rsv_addr = pmem2_vm_reservation_get_address(rsv);
	UT_ASSERTne(rsv_addr, NULL);

	ut_pmem2_prepare_config(&cfg, &src, &fh, FH_FD, file, 0, 0, FH_RDWR);
	pmem2_config_set_vm_reservation(&cfg, rsv, 0);

	ret = pmem2_map(&cfg, src, &map);
	UT_PMEM2_EXPECT_RETURN(ret, PMEM2_E_LENGTH_OUT_OF_RANGE);

	ret = pmem2_vm_reservation_delete(&rsv);
	UT_ASSERTeq(ret, 0);
	PMEM2_SOURCE_DELETE(&src);
	UT_FH_CLOSE(fh);

	return 2;
}

/*
 * test_vm_reserv_map_full_overlap - map a file to a vm reservation
 *                                   and overlap existing mapping
 */
static int
test_vm_reserv_map_full_overlap(const struct test_case *tc,
		int argc, char *argv[])
{
	if (argc < 2)
		UT_FATAL("usage: test_vm_reserv_map_full_overlap "
				"<file> <size>");

	char *file = argv[0];
	size_t size = ATOUL(argv[1]);
	size_t alignment = get_align_by_name(file);
	void *rsv_addr;
	size_t rsv_offset;
	size_t rsv_size;
	struct FHandle *fh;
	struct pmem2_config cfg;
	struct pmem2_map *map;
	struct pmem2_map *overlap_map;
	struct pmem2_vm_reservation *rsv;
	struct pmem2_source *src;

	rsv_size = size + alignment;

	int ret = pmem2_vm_reservation_new(&rsv, NULL, rsv_size);
	UT_ASSERTeq(ret, 0);

	rsv_addr = pmem2_vm_reservation_get_address(rsv);
	UT_ASSERTne(rsv_addr, NULL);
	UT_ASSERTeq(pmem2_vm_reservation_get_size(rsv), rsv_size);

	/* in case of DevDax */
	rsv_offset = offset_align_to_devdax(rsv_addr, alignment);

	ut_pmem2_prepare_config(&cfg, &src, &fh, FH_FD, file, 0, 0, FH_RDWR);
	pmem2_config_set_vm_reservation(&cfg, rsv, rsv_offset);

	ret = pmem2_map(&cfg, src, &map);
	UT_PMEM2_EXPECT_RETURN(ret, 0);

	ret = pmem2_map(&cfg, src, &overlap_map);
	UT_PMEM2_EXPECT_RETURN(ret, PMEM2_E_MAPPING_EXISTS);

	ret = pmem2_unmap(&map);
	UT_ASSERTeq(ret, 0);
	UT_ASSERTeq(map, NULL);
	ret = pmem2_vm_reservation_delete(&rsv);
	UT_ASSERTeq(ret, 0);
	PMEM2_SOURCE_DELETE(&src);
	UT_FH_CLOSE(fh);

	return 2;
}

/*
 * test_vm_reserv_map_partial_overlap_below - map a file to a vm reservation
 * overlapping with the ealier half of the other existing mapping
 */
static int
test_vm_reserv_map_partial_overlap_below(const struct test_case *tc,
		int argc, char *argv[])
{
	if (argc < 2)
		UT_FATAL("usage: test_vm_reserv_map_partial_overlap_below "
				"<file> <size>");

	char *file = argv[0];
	size_t size = ATOUL(argv[1]);
	size_t alignment = get_align_by_name(file);
	void *rsv_addr;
	size_t rsv_size;
	size_t rsv_offset;
	struct FHandle *fh;
	struct pmem2_config cfg;
	struct pmem2_map *map;
	struct pmem2_map *overlap_map;
	struct pmem2_vm_reservation *rsv;
	struct pmem2_source *src;

	rsv_size = size + size / 2 + alignment;

	int ret = pmem2_vm_reservation_new(&rsv, NULL, rsv_size);
	UT_ASSERTeq(ret, 0);

	rsv_addr = pmem2_vm_reservation_get_address(rsv);
	UT_ASSERTne(rsv_addr, NULL);
	UT_ASSERTeq(pmem2_vm_reservation_get_size(rsv), rsv_size);

	/* in case of DevDax */
	size_t offset_align = offset_align_to_devdax(rsv_addr, alignment);

	ut_pmem2_prepare_config(&cfg, &src, &fh, FH_FD, file, 0, 0, FH_RDWR);

	rsv_offset = ALIGN_DOWN(size / 2, alignment) + offset_align;
	pmem2_config_set_vm_reservation(&cfg, rsv, rsv_offset);

	ret = pmem2_map(&cfg, src, &map);
	UT_ASSERTeq(ret, 0);

	rsv_offset = offset_align;
	pmem2_config_set_vm_reservation(&cfg, rsv, rsv_offset);

	ret = pmem2_map(&cfg, src, &overlap_map);
	UT_PMEM2_EXPECT_RETURN(ret, PMEM2_E_MAPPING_EXISTS);

	ret = pmem2_unmap(&map);
	UT_ASSERTeq(ret, 0);
	UT_ASSERTeq(map, NULL);
	ret = pmem2_vm_reservation_delete(&rsv);
	UT_ASSERTeq(ret, 0);
	PMEM2_SOURCE_DELETE(&src);
	UT_FH_CLOSE(fh);

	return 2;
}

/*
 * test_vm_reserv_map_partial_overlap_above - map a file to a vm reservation
 * overlapping with the latter half of the other existing mapping
 */
static int
test_vm_reserv_map_partial_overlap_above(const struct test_case *tc,
		int argc, char *argv[])
{
	if (argc < 2)
		UT_FATAL("usage: test_vm_reserv_map_partial_overlap_above "
				"<file> <size>");

	char *file = argv[0];
	size_t size = ATOUL(argv[1]);
	size_t alignment = get_align_by_name(file);
	void *rsv_addr;
	size_t rsv_size;
	size_t rsv_offset;
	struct FHandle *fh;
	struct pmem2_config cfg;
	struct pmem2_map *map;
	struct pmem2_map *overlap_map;
	struct pmem2_vm_reservation *rsv;
	struct pmem2_source *src;

	rsv_size = size + size / 2 + alignment;

	int ret = pmem2_vm_reservation_new(&rsv, NULL, rsv_size);
	UT_ASSERTeq(ret, 0);

	rsv_addr = pmem2_vm_reservation_get_address(rsv);
	UT_ASSERTne(rsv_addr, NULL);
	UT_ASSERTeq(pmem2_vm_reservation_get_size(rsv), rsv_size);

	/* in case of DevDax */
	size_t offset_align = offset_align_to_devdax(rsv_addr, alignment);

	ut_pmem2_prepare_config(&cfg, &src, &fh, FH_FD, file, 0, 0, FH_RDWR);

	rsv_offset = offset_align;
	pmem2_config_set_vm_reservation(&cfg, rsv, rsv_offset);

	ret = pmem2_map(&cfg, src, &map);
	UT_ASSERTeq(ret, 0);

	rsv_offset = ALIGN_DOWN(size / 2, alignment) + offset_align;
	pmem2_config_set_vm_reservation(&cfg, rsv, rsv_offset);

	ret = pmem2_map(&cfg, src, &overlap_map);
	UT_PMEM2_EXPECT_RETURN(ret, PMEM2_E_MAPPING_EXISTS);

	ret = pmem2_unmap(&map);
	UT_ASSERTeq(ret, 0);
	UT_ASSERTeq(map, NULL);
	ret = pmem2_vm_reservation_delete(&rsv);
	UT_ASSERTeq(ret, 0);
	PMEM2_SOURCE_DELETE(&src);
	UT_FH_CLOSE(fh);

	return 2;
}

/*
 * test_vm_reserv_map_invalid_granularity - map a file with invalid granularity
 * to a vm reservation in the middle of the vm reservation bigger than
 * the file, then map a file that covers whole vm reservation
 */
static int
test_vm_reserv_map_invalid_granularity(const struct test_case *tc,
	int argc, char *argv[])
{
	if (argc < 2)
		UT_FATAL("usage: test_vm_reserv_map_invalid_granularity "
				"<file> <size>");

	char *file = argv[0];
	size_t size = ATOUL(argv[1]);
	size_t offset = 0;
	size_t rsv_offset;
	size_t rsv_size;
	struct pmem2_config cfg;
	struct pmem2_map *map;
	struct pmem2_vm_reservation *rsv;
	struct pmem2_source *src;
	struct FHandle *fh;

	/* map only half of the file */
	offset = size / 2;

	rsv_size = size;
	/* map it to the middle of the vm reservation */
	rsv_offset = size / 4;

	int ret = pmem2_vm_reservation_new(&rsv, NULL, rsv_size);
	UT_ASSERTeq(ret, 0);
	UT_ASSERTeq(pmem2_vm_reservation_get_size(rsv), rsv_size);

	ut_pmem2_prepare_config(&cfg, &src, &fh, FH_FD, file, 0, offset,
			FH_RDWR);
	pmem2_config_set_vm_reservation(&cfg, rsv, rsv_offset);

	/* spoil requested granularity */
	enum pmem2_granularity gran = cfg.requested_max_granularity;
	cfg.requested_max_granularity = PMEM2_GRANULARITY_BYTE;

	ret = pmem2_map(&cfg, src, &map);
	UT_PMEM2_EXPECT_RETURN(ret, PMEM2_E_GRANULARITY_NOT_SUPPORTED);

	/* map whole file */
	offset = 0;
	rsv_offset = 0;

	/* restore correct granularity */
	cfg.requested_max_granularity = gran;
	cfg.offset = offset;

	pmem2_config_set_vm_reservation(&cfg, rsv, rsv_offset);

	ret = pmem2_map(&cfg, src, &map);
	UT_ASSERTeq(ret, 0);

	UT_ASSERTeq((char *)pmem2_vm_reservation_get_address(rsv) +
		rsv_offset, pmem2_map_get_address(map));

	ret = pmem2_unmap(&map);
	UT_ASSERTeq(ret, 0);
	UT_ASSERTeq(map, NULL);
	ret = pmem2_vm_reservation_delete(&rsv);
	UT_ASSERTeq(ret, 0);
	PMEM2_SOURCE_DELETE(&src);
	UT_FH_CLOSE(fh);

	return 2;
}

#define MAX_THREADS 32

struct worker_args {
	struct pmem2_config cfg;
	struct pmem2_source *src;
	struct pmem2_map **map;
	size_t n_ops;
	os_mutex_t lock;
};

static void *
map_worker(void *arg)
{
	struct worker_args *warg = arg;

	for (size_t n = 0; n < warg->n_ops; n++) {
		if (!(*warg->map)) {
			int ret = pmem2_map(&warg->cfg, warg->src, warg->map);
			if (ret != PMEM2_E_MAPPING_EXISTS)
				UT_ASSERTeq(ret, 0);
		}
	}

	return NULL;
}

static void *
unmap_worker(void *arg)
{
	struct worker_args *warg = arg;

	for (size_t n = 0; n < warg->n_ops; n++) {
		if (*(warg->map)) {
			int ret = pmem2_unmap(warg->map);
			if (ret != PMEM2_E_MAPPING_NOT_FOUND)
				UT_ASSERTeq(ret, 0);
		}
	}

	return NULL;
}

static void
run_worker(void *(worker_func)(void *arg), struct worker_args args[],
		os_thread_t *threads, size_t n_threads)
{
	for (size_t n = 0; n < n_threads; n++)
		THREAD_CREATE(&threads[n], NULL, worker_func, &args[n]);
}

/*
 * test_vm_reserv_async_map_unmap_multiple_files - map and unmap
 * asynchronously multiple files to the vm reservation. Mappings
 * will occur to 3 different overlapping regions of the vm reservation.
 */
static int
test_vm_reserv_async_map_unmap_multiple_files(const struct test_case *tc,
	int argc, char *argv[])
{
	if (argc < 4)
		UT_FATAL("usage: test_vm_reserv_async_map_unmap_multiple_files"
				"<file> <size> <threads> <ops/thread>");

	char *file = argv[0];
	size_t size = ATOUL(argv[1]);
	size_t n_threads = ATOU(argv[2]);
	size_t ops_per_thread = ATOU(argv[3]);
	size_t alignment = get_align_by_name(file);
	void *rsv_addr;
	size_t rsv_size;
	size_t rsv_offset;
	struct pmem2_config cfg;
	struct pmem2_map *map[MAX_THREADS];
	struct pmem2_vm_reservation *rsv;
	struct pmem2_source *src;
	struct FHandle *fh;
	struct worker_args args[MAX_THREADS];

	for (size_t n = 0; n < n_threads; n++)
		map[n] = NULL;

	/* amount of files side by side that virtual memory reservation fit */
	size_t n_files = 2;

	/* reservation region is the size of n_files files */
	rsv_size = n_files * size + alignment;

	int ret = pmem2_vm_reservation_new(&rsv, NULL, rsv_size);
	UT_ASSERTeq(ret, 0);

	rsv_addr = pmem2_vm_reservation_get_address(rsv);
	UT_ASSERTne(rsv_addr, NULL);
	UT_ASSERTeq(pmem2_vm_reservation_get_size(rsv), rsv_size);

	/* in case of DevDax */
	size_t offset_align = offset_align_to_devdax(rsv_addr, alignment);

	ut_pmem2_prepare_config(&cfg, &src, &fh, FH_FD, file, 0, 0, FH_RDWR);

	/*
	 * The offset increases by half of the file size. There are
	 * (2 * n_files - 1) offsets that will fit in the reservation region.
	 */
	size_t offset_inc = ALIGN_DOWN(size / 2, alignment);
	size_t n_mapping_regions = n_files * 2 - 1;
	for (size_t n = 0; n < n_threads; n++) {
		/* calculate offset for each thread */
		rsv_offset = (n % n_mapping_regions) * offset_inc
				+ offset_align;
		pmem2_config_set_vm_reservation(&cfg, rsv, rsv_offset);

		args[n].cfg = cfg;
		args[n].src = src;
		args[n].map = &(map[n]);
		args[n].n_ops = ops_per_thread;
	}

	os_thread_t *map_threads = malloc(sizeof(os_thread_t) * MAX_THREADS);
	os_thread_t *unmap_threads = malloc(sizeof(os_thread_t) * MAX_THREADS);

	run_worker(map_worker, args, map_threads, n_threads);
	run_worker(unmap_worker, args, unmap_threads, n_threads);

	for (size_t n = 0; n < n_threads; n++)
		THREAD_JOIN(&map_threads[n], NULL);

	for (size_t n = 0; n < n_threads; n++)
		THREAD_JOIN(&unmap_threads[n], NULL);

	FREE(unmap_threads);
	FREE(map_threads);

	for (size_t n = 0; n < n_threads; n++)
		if (map[n])
			pmem2_unmap(&map[n]);

	ret = pmem2_vm_reservation_delete(&rsv);
	UT_ASSERTeq(ret, 0);
	PMEM2_SOURCE_DELETE(&src);
	UT_FH_CLOSE(fh);

	return 4;
}

/*
 * test_cases -- available test cases
 */
static struct test_case test_cases[] = {
	TEST_CASE(test_vm_reserv_new_region_occupied_map),
	TEST_CASE(test_vm_reserv_new_region_occupied_reserv),
	TEST_CASE(test_vm_reserv_map_file),
	TEST_CASE(test_vm_reserv_map_part_file),
	TEST_CASE(test_vm_reserv_delete_contains_mapping),
	TEST_CASE(test_vm_reserv_map_unmap_multiple_files),
	TEST_CASE(test_vm_reserv_map_insufficient_space),
	TEST_CASE(test_vm_reserv_map_full_overlap),
	TEST_CASE(test_vm_reserv_map_partial_overlap_above),
	TEST_CASE(test_vm_reserv_map_partial_overlap_below),
	TEST_CASE(test_vm_reserv_map_invalid_granularity),
	TEST_CASE(test_vm_reserv_async_map_unmap_multiple_files),
};

#define NTESTS (sizeof(test_cases) / sizeof(test_cases[0]))

int
main(int argc, char *argv[])
{
	START(argc, argv, "pmem2_vm_reservation");
	util_init();
	out_init("pmem2_vm_reservation", "TEST_LOG_LEVEL", "TEST_LOG_FILE",
			0, 0);
	TEST_CASE_PROCESS(argc, argv, test_cases, NTESTS);
	out_fini();
	DONE(NULL);
}

#ifdef _MSC_VER
MSVC_CONSTR(libpmem2_init)
MSVC_DESTR(libpmem2_fini)
#endif
