// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * pmemset_part.c -- pmemset_part unittests
 */

#include <string.h>

#include "config.h"
#include "fault_injection.h"
#include "libpmemset.h"
#include "libpmem2.h"
#include "out.h"
#include "part.h"
#include "source.h"
#include "unittest.h"
#include "ut_pmemset_utils.h"

static void create_config(struct pmemset_config **cfg) {
	int ret = pmemset_config_new(cfg);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);
	UT_ASSERTne(cfg, NULL);

	ret = pmemset_config_set_required_store_granularity(*cfg,
		PMEM2_GRANULARITY_PAGE);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);
	UT_ASSERTne(cfg, NULL);
}

/*
 * test_part_new_enomem - test pmemset_part allocation with error injection
 */
static int
test_part_new_enomem(const struct test_case *tc, int argc,
		char *argv[])
{
	if (argc < 1)
		UT_FATAL("usage: test_part_new_enomem <path>");

	const char *file = argv[0];
	struct pmemset *set;
	struct pmemset_part *part;
	struct pmemset_source *src;
	struct pmemset_config *cfg;

	if (!core_fault_injection_enabled())
		return 1;

	create_config(&cfg);

	int ret = pmemset_new(&set, cfg);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_source_from_file(&src, file);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);
	UT_ASSERTne(src, NULL);

	core_inject_fault_at(PMEM_MALLOC, 1, "pmemset_malloc");

	ret = pmemset_part_new(&part, set, src, 0, 0);
	UT_PMEMSET_EXPECT_RETURN(ret, -ENOMEM);
	UT_ASSERTeq(part, NULL);

	return 1;
}

/*
 * test_part_new_invalid_source_file - create a new part from a source
 *                                     with invalid path assigned
 */
static int
test_part_new_invalid_source_file(const struct test_case *tc, int argc,
		char *argv[])
{
	if (argc < 1)
		UT_FATAL("usage: test_part_new_invalid_source_file <path>");

	const char *file = argv[0];
	struct pmemset *set;
	struct pmemset_part *part;
	struct pmemset_source *src;
	struct pmemset_config *cfg;

	create_config(&cfg);

	int ret = pmemset_new(&set, cfg);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_source_from_file(&src, file);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_part_new(&part, set, src, 0, 0);
	UT_PMEMSET_EXPECT_RETURN(ret, PMEMSET_E_INVALID_FILE_PATH);
	UT_ASSERTeq(part, NULL);

	return 1;
}

/*
 * test_part_new_valid_source_file - create a new part from a source
 *                                   with valid path assigned
 */
static int
test_part_new_valid_source_file(const struct test_case *tc, int argc,
		char *argv[])
{
	if (argc < 1)
		UT_FATAL("usage: test_part_new_valid_source_file <path>");

	const char *file = argv[0];
	struct pmemset *set;
	struct pmemset_part *part;
	struct pmemset_source *src;
	struct pmemset_config *cfg;

	create_config(&cfg);

	int ret = pmemset_new(&set, cfg);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_source_from_file(&src, file);
	UT_ASSERTeq(ret, 0);

	ret = pmemset_part_new(&part, set, src, 0, 0);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);
	UT_ASSERTne(part, NULL);

	ret = pmemset_source_delete(&src);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);
	UT_ASSERTeq(src, NULL);

	ret = pmemset_part_delete(&part);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_delete(&set);
	UT_ASSERTeq(ret, 0);

	return 1;
}

/*
 * test_part_new_valid_source_pmem2 - create a new part from a source
 *                                    with valid pmem2_source assigned
 */
static int
test_part_new_valid_source_pmem2(const struct test_case *tc, int argc,
		char *argv[])
{
	if (argc < 1)
		UT_FATAL("usage: test_part_new_valid_source_pmem2 <path>");

	const char *file = argv[0];
	struct pmem2_source *pmem2_src;
	struct pmemset *set;
	struct pmemset_part *part;
	struct pmemset_source *src;
	struct pmemset_config *cfg;

	create_config(&cfg);

	int ret = pmemset_new(&set, cfg);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	int fd = OPEN(file, O_RDWR);

	ret = pmem2_source_from_fd(&pmem2_src, fd);
	UT_ASSERTeq(ret, 0);

	ret = pmemset_source_from_pmem2(&src, pmem2_src);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);
	UT_ASSERTne(src, NULL);

	ret = pmemset_part_new(&part, set, src, 0, 0);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);
	UT_ASSERTne(part, NULL);

	ret = pmemset_source_delete(&src);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);
	UT_ASSERTeq(src, NULL);

	ret = pmemset_part_delete(&part);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);
	UT_ASSERTeq(part, NULL);

	ret = pmem2_source_delete(&pmem2_src);
	UT_ASSERTeq(ret, 0);

	CLOSE(fd);

	return 1;
}

/*
 * test_part_map_valid_source_pmem2 - create a new part from a source
 *                                    with valid pmem2_source and map part
 */
static int
test_part_map_valid_source_pmem2(const struct test_case *tc, int argc,
		char *argv[])
{
	if (argc < 1)
		UT_FATAL("usage: test_part_map_valid_source_pmem2 <path>");

	const char *file = argv[0];
	struct pmemset_part *part;
	struct pmemset_source *src;
	struct pmem2_source *pmem2_src;
	struct pmemset *set;
	struct pmemset_config *cfg;

	int fd = OPEN(file, O_RDWR);

	int ret = pmem2_source_from_fd(&pmem2_src, fd);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_source_from_pmem2(&src, pmem2_src);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	create_config(&cfg);

	ret = pmemset_new(&set, cfg);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_part_new(&part, set, src, 0, 64 * 1024);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_part_map(&part, NULL, NULL);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	pmemset_config_delete(&cfg);
	pmem2_source_delete(&pmem2_src);
	CLOSE(fd);

	return 1;
}

/*
 * test_part_map_valid_source_file - create a new part from a source
 *                                    with valid file path and map part
 */
static int
test_part_map_valid_source_file(const struct test_case *tc, int argc,
		char *argv[])
{
	if (argc < 1)
		UT_FATAL("usage: test_part_map_valid_source_file <path>");

	const char *file = argv[0];
	struct pmemset_part *part;
	struct pmemset_source *src;
	struct pmemset *set;
	struct pmemset_config *cfg;

	int ret = pmemset_source_from_file(&src, file);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	create_config(&cfg);

	ret = pmemset_new(&set, cfg);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_part_new(&part, set, src, 0, 64 * 1024);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_part_map(&part, NULL, NULL);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	pmemset_source_delete(&src);
	pmemset_config_delete(&cfg);
	pmemset_delete(&set);

	return 1;
}

/*
 * test_part_map_invalid_offset - create a new part from a source
 *                                    with invalid offset value
 */
static int
test_part_map_invalid_offset(const struct test_case *tc, int argc,
		char *argv[])
{
	if (argc < 1)
		UT_FATAL("usage: test_part_map_invalid_offset <path>");

	const char *file = argv[0];
	struct pmemset_part *part;
	struct pmemset_source *src;
	struct pmemset *set;
	struct pmemset_config *cfg;

	int ret = pmemset_source_from_file(&src, file);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	create_config(&cfg);

	ret = pmemset_new(&set, cfg);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_part_new(&part, set, src,
			(size_t)(INT64_MAX) + 1, 64 * 1024);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_part_map(&part, NULL, NULL);
	UT_PMEMSET_EXPECT_RETURN(ret, PMEMSET_E_INVALID_OFFSET_VALUE);

	ret = pmemset_part_delete(&part);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_config_delete(&cfg);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_delete(&set);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	return 1;
}

/*
 * test_part_map_gran_read - try to read effective granularity before
 * part mapping and after part mapping.
 */
static int
test_part_map_gran_read(const struct test_case *tc, int argc,
		char *argv[])
{
	if (argc < 1)
		UT_FATAL("usage: test_part_map_gran_read <path>");

	const char *file = argv[0];
	struct pmemset_part *part;
	struct pmemset_source *src;
	struct pmemset *set;
	struct pmemset_config *cfg;

	int ret = pmemset_source_from_file(&src, file);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	create_config(&cfg);

	ret = pmemset_new(&set, cfg);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_part_new(&part, set, src, 0, 64 * 1024);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	enum pmem2_granularity effective_gran =
		pmemset_get_store_granularity(set);
	ASSERTeq(effective_gran, -1);

	ret = pmemset_part_map(&part, NULL, NULL);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	effective_gran =
		pmemset_get_store_granularity(set);
	ASSERTne(effective_gran, -1);

	ret = pmemset_source_delete(&src);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_config_delete(&cfg);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_delete(&set);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	return 1;
}

static ut_jmp_buf_t Jmp;

/*
 * signal_handler -- called on SIGSEGV
 */
static void
signal_handler(int sig)
{
	ut_siglongjmp(Jmp);
}

/*
 * test_unmap_part - test if data is unavailable after pmemset_delete
 */
static int
test_unmap_part(const struct test_case *tc, int argc,
		char *argv[])
{
	if (argc < 1)
		UT_FATAL("usage: test_part_map_invalid_offset <path>");

	const char *file = argv[0];
	struct pmemset_part *part;
	struct pmemset_source *src;
	struct pmemset *set;
	struct pmemset_config *cfg;

	int ret = pmemset_source_from_file(&src, file);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	create_config(&cfg);

	ret = pmemset_new(&set, cfg);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_part_new(&part, set, src, 0, 0);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	struct pmemset_part_descriptor desc;
	ret = pmemset_part_map(&part, NULL, &desc);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	memset(desc.addr, 1, desc.size);
	pmemset_persist(set, desc.addr, desc.size);

	ret = pmemset_config_delete(&cfg);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_delete(&set);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	struct sigaction v;
	sigemptyset(&v.sa_mask);
	v.sa_flags = 0;
	v.sa_handler = signal_handler;
	SIGACTION(SIGSEGV, &v, NULL);
	if (!ut_sigsetjmp(Jmp)) {
		/* memcpy should now fail */
		memset(desc.addr, 1, desc.size);
		UT_FATAL("memcpy successful");
	}
	signal(SIGSEGV, SIG_DFL);

	return 1;
}

/*
 * test_part_map_first - get the first (earliest in the memory)
 *                       mapping from the set
 */
static int
test_part_map_first(const struct test_case *tc, int argc,
		char *argv[])
{
	if (argc < 1)
		UT_FATAL("usage: test_part_map_firt <path>");

	const char *file = argv[0];
	struct pmem2_source *pmem2_src;
	struct pmemset *set;
	struct pmemset_config *cfg;
	struct pmemset_part *part;
	struct pmemset_part_map *first_pmap = NULL;
	struct pmemset_source *src;
	size_t part_size = 64 * 1024;

	int fd = OPEN(file, O_RDWR);

	int ret = pmem2_source_from_fd(&pmem2_src, fd);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_source_from_pmem2(&src, pmem2_src);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	create_config(&cfg);

	ret = pmemset_new(&set, cfg);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_part_new(&part, set, src, 0, part_size);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_part_map(&part, NULL, NULL);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	pmemset_first_part_map(set, &first_pmap);
	UT_ASSERTne(first_pmap, NULL);

	pmemset_config_delete(&cfg);
	pmem2_source_delete(&pmem2_src);
	CLOSE(fd);

	return 1;
}

/*
 * test_part_map_descriptor - test retrieving first (earliest in the memory)
 *                            mapping from the set
 */
static int
test_part_map_descriptor(const struct test_case *tc, int argc,
		char *argv[])
{
	if (argc < 1)
		UT_FATAL("usage: test_part_map_descriptor <path>");

	const char *file = argv[0];
	struct pmem2_source *pmem2_src;
	struct pmemset *set;
	struct pmemset_config *cfg;
	struct pmemset_part *part;
	struct pmemset_part_descriptor desc;
	struct pmemset_part_map *first_pmap = NULL;
	struct pmemset_source *src;
	size_t part_size = 64 * 1024;

	int fd = OPEN(file, O_RDWR);

	int ret = pmem2_source_from_fd(&pmem2_src, fd);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_source_from_pmem2(&src, pmem2_src);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	create_config(&cfg);

	ret = pmemset_new(&set, cfg);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_part_new(&part, set, src, 0, part_size);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_part_map(&part, NULL, NULL);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	pmemset_first_part_map(set, &first_pmap);
	UT_ASSERTne(first_pmap, NULL);

	desc = pmemset_descriptor_part_map(first_pmap);
	UT_ASSERTne(desc.addr, NULL);
	UT_ASSERTeq(desc.size, part_size);

	pmemset_config_delete(&cfg);
	pmem2_source_delete(&pmem2_src);
	CLOSE(fd);

	return 1;
}

/*
 * test_part_map_next - test retrieving next mapping from the set
 */
static int
test_part_map_next(const struct test_case *tc, int argc,
		char *argv[])
{
	if (argc < 1)
		UT_FATAL("usage: test_part_map_firt <path>");

	const char *file = argv[0];
	struct pmem2_source *pmem2_src;
	struct pmemset *set;
	struct pmemset_config *cfg;
	struct pmemset_part_descriptor first_desc;
	struct pmemset_part_descriptor second_desc;
	struct pmemset_part *part;
	struct pmemset_part_map *first_pmap = NULL;
	struct pmemset_part_map *second_pmap = NULL;
	struct pmemset_source *src;
	size_t part_size = 64 * 1024;

	int fd = OPEN(file, O_RDWR);

	int ret = pmem2_source_from_fd(&pmem2_src, fd);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_source_from_pmem2(&src, pmem2_src);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	create_config(&cfg);

	ret = pmemset_new(&set, cfg);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_part_new(&part, set, src, 0, part_size);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_part_map(&part, NULL, NULL);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_part_new(&part, set, src, 0, part_size);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	ret = pmemset_part_map(&part, NULL, NULL);
	UT_PMEMSET_EXPECT_RETURN(ret, 0);

	pmemset_first_part_map(set, &first_pmap);
	UT_ASSERTne(first_pmap, NULL);

	pmemset_next_part_map(set, first_pmap, &second_pmap);
	UT_ASSERTne(second_pmap, NULL);

	first_desc = pmemset_descriptor_part_map(first_pmap);
	second_desc = pmemset_descriptor_part_map(second_pmap);
	UT_ASSERT(first_desc.addr < second_desc.addr);
	UT_ASSERTeq(first_desc.size, second_desc.size);

	pmemset_config_delete(&cfg);
	pmem2_source_delete(&pmem2_src);
	CLOSE(fd);

	return 1;
}

/*
 * test_cases -- available test cases
 */
static struct test_case test_cases[] = {
	TEST_CASE(test_part_new_enomem),
	TEST_CASE(test_part_new_invalid_source_file),
	TEST_CASE(test_part_new_valid_source_file),
	TEST_CASE(test_part_new_valid_source_pmem2),
	TEST_CASE(test_part_map_valid_source_pmem2),
	TEST_CASE(test_part_map_valid_source_file),
	TEST_CASE(test_part_map_invalid_offset),
	TEST_CASE(test_part_map_gran_read),
	TEST_CASE(test_unmap_part),
	TEST_CASE(test_part_map_first),
	TEST_CASE(test_part_map_descriptor),
	TEST_CASE(test_part_map_next),
};

#define NTESTS (sizeof(test_cases) / sizeof(test_cases[0]))

int
main(int argc, char **argv)
{
	START(argc, argv, "pmemset_part");

	util_init();
	out_init("pmemset_part", "TEST_LOG_LEVEL", "TEST_LOG_FILE", 0, 0);
	TEST_CASE_PROCESS(argc, argv, test_cases, NTESTS);
	out_fini();

	DONE(NULL);
}
