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
 * test_vm_reservation_region_occupied_by_map - create a vm_reservation in the
 * region belonging to existing mapping
 */
static int
test_vm_reservation_region_occupied_by_map(const struct test_case *tc,
		int argc, char *argv[])
{
	if (argc < 2)
		UT_FATAL(
				"usage: test_vm_reservation_region_occupied_by_map <file> <size>");

	char *file = argv[0];
	size_t size = ATOUL(argv[1]);
	struct pmem2_config cfg;
	struct pmem2_source *src;
	struct pmem2_vm_reservation *rsv;
	struct FHandle *fh;

	ut_pmem2_prepare_config(&cfg, &src, &fh, FH_FD, file, 0, 0, FH_RDWR);

	struct pmem2_map *map;

	/* map a region of virtual address space */
	int ret = pmem2_map(&cfg, src, &map);
	UT_PMEM2_EXPECT_RETURN(ret, 0);

	void *addr = pmem2_map_get_address(map);

	/* create a mapping in the region occupied by existing mapping */
	ret = pmem2_vm_reservation_new(&rsv, size, addr);
	UT_PMEM2_EXPECT_RETURN(ret, PMEM2_E_MAPPING_EXISTS);

	ret = pmem2_unmap(&map);
	UT_ASSERTeq(ret, 0);
	UT_ASSERTeq(map, NULL);
	PMEM2_SOURCE_DELETE(&src);
	UT_FH_CLOSE(fh);

	return 2;
}

/*
 * test_vm_reservation_region_occupied_by_reservation_by_reservation - create a
 * vm reservation in the region belonging to other existing vm reservation
 */
static int
test_vm_reservation_region_occupied_by_reservation(const struct test_case *tc,
		int argc, char *argv[])
{
	if (argc < 1)
		UT_FATAL(
				"usage: test_vm_reservation_region_occupied_by_reservation <reserv_size>");

	size_t reserv_size = ATOUL(argv[0]);
	struct pmem2_vm_reservation *rsv1;
	struct pmem2_vm_reservation *rsv2;

	/* reserve a region in the virtual address space */
	int ret = pmem2_vm_reservation_new(&rsv1, reserv_size, NULL);
	UT_PMEM2_EXPECT_RETURN(ret, 0);

	void *addr = pmem2_vm_reservation_get_address(rsv1);
	UT_ASSERTne(addr, NULL);

	/*
	 * make a vm reservation of the region occupied by other
	 * existing reservation
	 */
	ret = pmem2_vm_reservation_new(&rsv2, reserv_size, addr);
	UT_PMEM2_EXPECT_RETURN(ret, PMEM2_E_MAPPING_EXISTS);

	ret = pmem2_vm_reservation_delete(&rsv1);
	UT_ASSERTeq(ret, 0);

	return 1;
}

/*
 * test_cases -- available test cases
 */
static struct test_case test_cases[] = {
	TEST_CASE(test_vm_reservation_region_occupied_by_map),
	TEST_CASE(test_vm_reservation_region_occupied_by_reservation),
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
