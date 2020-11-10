// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * source.c -- implementation of common config API
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "libpmemset.h"

#include "alloc.h"
#include "os.h"
#include "pmemset_utils.h"
#include "source.h"

struct pmemset_source {
	enum pmemset_source_type type;
	union {
		struct {
			char *path;
		} file;
		struct {
			struct pmem2_source *src;
		} pmem2;
	};
};

/*
 * pmemset_source_from_pmem2 -- create pmemset source using source from pmem2
 */
int
pmemset_source_from_pmem2(struct pmemset_source **src,
		struct pmem2_source *pmem2_src)
{
	PMEMSET_ERR_CLR();

	*src = NULL;

	if (!pmem2_src) {
		ERR("pmem2_source cannot be NULL");
		return PMEMSET_E_INVALID_PMEM2_SOURCE;
	}

	int ret;
	struct pmemset_source *srcp = pmemset_malloc(sizeof(**src), &ret);
	if (ret)
		return ret;

	ASSERTne(srcp, NULL);

	srcp->type = PMEMSET_SOURCE_PMEM2;
	srcp->pmem2.src = pmem2_src;

	*src = srcp;

	return 0;
}

#ifndef _WIN32
/*
 * pmemset_source_from_file -- initializes source structure and stores a path
 *                             to the file
 */
int
pmemset_source_from_file(struct pmemset_source **src, const char *file)
{
	LOG(3, "src %p file %s", src, file);
	PMEMSET_ERR_CLR();

	*src = NULL;

	if (!file) {
		ERR("file path cannot be empty");
		return PMEMSET_E_INVALID_FILE_PATH;
	}

	int ret;
	struct pmemset_source *srcp = pmemset_malloc(sizeof(**src), &ret);
	if (ret)
		return ret;

	srcp->type = PMEMSET_SOURCE_FILE;
	srcp->file.path = strdup(file);

	if (srcp->file.path == NULL) {
		ERR("!strdup");
		Free(srcp);
		return PMEMSET_E_ERRNO;
	}

	*src = srcp;

	return 0;
}

/*
 * pmemset_source_from_temporary -- not supported
 */
int
pmemset_source_from_temporary(struct pmemset_source **src, const char *dir)
{
	return PMEMSET_E_NOSUPP;
}
#else
/*
 * pmemset_source_from_fileU -- initializes source structure and stores a path
 *                              to the file
 */
int
pmemset_source_from_fileU(struct pmemset_source **src, const char *file)
{
	LOG(3, "src %p file %s", src, file);
	PMEMSET_ERR_CLR();

	*src = NULL;

	if (!file) {
		ERR("file path cannot be empty");
		return PMEMSET_E_INVALID_FILE_PATH;
	}

	int ret;
	struct pmemset_source *srcp = pmemset_malloc(sizeof(**src), &ret);
	if (ret)
		return ret;

	srcp->type = PMEMSET_SOURCE_FILE;
	srcp->file.path = Strdup(file);

	if (srcp->file.path == NULL) {
		ERR("!strdup");
		Free(srcp);
		return PMEMSET_E_ERRNO;
	}
	*src = srcp;

	return 0;
}

/*
 * pmemset_source_from_fileW -- initializes source structure and stores a path
 *                              to the file
 */
int
pmemset_source_from_fileW(struct pmemset_source **src, const wchar_t *file)
{
	const char *ufile = util_toUTF8(file);

	return pmemset_source_from_fileU(src, ufile);
}

/*
 * pmemset_source_from_temporaryU -- not supported
 */
int
pmemset_source_from_temporaryU(struct pmemset_source **src, const char *dir)
{
	return PMEMSET_E_NOSUPP;
}

/*
 * pmemset_source_from_temporaryW -- not supported
 */
int
pmemset_source_from_temporaryW(struct pmemset_source **src, const wchar_t *dir)
{
	return PMEMSET_E_NOSUPP;
}
#endif

/*
 * pmemset_source_delete -- delete pmemset_source structure
 */
int
pmemset_source_delete(struct pmemset_source **src)
{
	if ((*src)->type == PMEMSET_SOURCE_FILE)
		Free((*src)->file.path);

	Free(*src);
	*src = NULL;

	return 0;
}

/*
 * pmemset_source_validate__file - check the validity of source created
 *                                 from file
 */
static int
pmemset_source_validate_file(const struct pmemset_source *src)
{
	os_stat_t stat;
	if (os_stat(src->file.path, &stat) < 0) {
		if (errno == ENOENT) {
			ERR("invalid path specified in the source");
			return PMEMSET_E_INVALID_FILE_PATH;
		}
		ERR("!stat");
		return PMEMSET_E_ERRNO;
	}

	return 0;
}

/*
 * pmemset_source_validate_pmem2 - check the validity of source created
 *                                 from pmem2 source
 */
static int
pmemset_source_validate_pmem2(const struct pmemset_source *src)
{
	if (!src->pmem2.src) {
		ERR("invalid pmem2_source specified in the data source");
		return PMEMSET_E_INVALID_PMEM2_SOURCE;
	}

	return 0;
}

static int (*pmemset_source_validate_func[MAX_PMEMSET_SOURCE_TYPE])
	(const struct pmemset_source *src) =
		{ NULL, pmemset_source_validate_pmem2,
		pmemset_source_validate_file };

/*
 * pmemset_source_validate -- check the validity of created source
 */
int
pmemset_source_validate(const struct pmemset_source *src)
{
	enum pmemset_source_type type = src->type;
	if (type == PMEMSET_SOURCE_UNSPECIFIED ||
			type >= MAX_PMEMSET_SOURCE_TYPE) {
		ERR("invalid source type");
		return PMEMSET_E_INVALID_SOURCE_TYPE;
	}

	return pmemset_source_validate_func[type](src);
}
