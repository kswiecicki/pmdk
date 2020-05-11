// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * map_multiple_files.c -- implementation of virtual address allocation example
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#endif
#include <libpmem2.h>

int
main(int argc, char *argv[])
{
	char *addr;
	int *fd;
	int ret = 1;
	struct pmem2_config *cfg;
	struct pmem2_map **map;
	struct pmem2_source **src;
	struct pmem2_vm_reservation *rsv;
	pmem2_memset_fn memset_fn;
	size_t alignment;
	size_t n_files = argc - 1;
	size_t *file_size;
	size_t offset = 0;
	size_t reservation_size = 0;
	int align_iter;
	int map_iter;
	int memset_iter;
	int open_iter;
	int src_iter;
	int size_iter;

	if (argc < 2) {
		fprintf(stderr,
			"usage: ./map_multiple_files <file1> <file2> ...\n");
		return ret;
	}

	fd = malloc(sizeof(int) * n_files);
	if (fd == NULL) {
		perror("malloc");
		return ret;
	}

	map = malloc(sizeof(*map) * n_files);
	if (map == NULL) {
		perror("malloc");
		goto free_fd;
	}
	src = malloc(sizeof(*src) * n_files);
	if (src == NULL) {
		perror("malloc");
		goto free_map;
	}

	file_size = malloc(sizeof(size_t) * n_files);
	if (file_size == NULL) {
		perror("malloc");
		goto free_src;
	}

	for (open_iter = 0; open_iter < n_files; open_iter++) {
		if ((fd[open_iter] = open(argv[open_iter + 1], O_RDWR)) < 0) {
			perror("open");
			goto close_fds;
		}
	}

	for (src_iter = 0; src_iter < n_files; src_iter++) {
		if (pmem2_source_from_fd(&src[src_iter], fd[src_iter])) {
			pmem2_perror("pmem2_source_from_fd");
			goto free_src_elements;
		}
	}

	for (size_iter = 0; size_iter < n_files; size_iter++) {
		if (pmem2_source_size(src[size_iter], &file_size[size_iter])) {
			pmem2_perror("pmem2_source_size");
			goto free_src_elements;
		}

		reservation_size += file_size[size_iter];
	}

	for (align_iter = 0; align_iter < n_files; align_iter++) {
		pmem2_source_alignment(src[align_iter], &alignment);

		if (file_size[align_iter] % alignment != 0) {
			fprintf(stderr,
				"usage: files must be aligned\n");
			goto free_src_elements;
		}
	}

	if (pmem2_vm_reservation_new(&rsv, reservation_size, NULL)) {
		pmem2_perror("pmem2_vm_reservation_new");
		goto free_src_elements;
	}

	if (pmem2_config_new(&cfg)) {
		pmem2_perror("pmem2_config_new");
		goto delete_vm_reservation;
	}

	if (pmem2_config_set_required_store_granularity(
			cfg, PMEM2_GRANULARITY_PAGE)) {
		pmem2_perror("pmem2_config_set_required_store_granularity");
		goto delete_config;
	}

	for (map_iter = 0; map_iter < n_files; map_iter++) {
		if (pmem2_config_set_vm_reservation(
				cfg, rsv, offset) != PMEM2_E_NOSUPP) {
			pmem2_perror("pmem2_config_set_vm_reservation");
			goto unmap;
		}

		offset += file_size[map_iter];

		if (pmem2_map(cfg, src[map_iter], &map[map_iter])) {
			pmem2_perror("pmem2_map");
			goto unmap;
		}
	}

	addr = pmem2_map_get_address(map[0]);

	if (addr == NULL) {
		pmem2_perror("pmem2_map_get_address");
		goto unmap;
	}

	memset_fn = pmem2_get_memset_fn(map[0]);

	for (memset_iter = 1; memset_iter < n_files; memset_iter++) {
		if (memset_fn != pmem2_get_memset_fn(map[memset_iter])) {
			fprintf(stderr,
				"usage: files have different file systems\n");
			goto unmap;
		}
	}

	memset_fn(addr, '-', reservation_size, PMEM2_F_MEM_NONTEMPORAL);

	ret = 0;

unmap:
	for (map_iter--; map_iter >= 0; map_iter--) {
		pmem2_unmap(&map[map_iter]);
		free(map[map_iter]);
	}
delete_config:
	pmem2_config_delete(&cfg);
delete_vm_reservation:
	pmem2_vm_reservation_delete(&rsv);
free_src_elements:
	for (src_iter--; src_iter >= 0; src_iter--) {
		free(src[src_iter]);
	}
close_fds:
	for (open_iter--; open_iter >= 0; open_iter--) {
		close(fd[open_iter]);
	}
	free(file_size);
free_src:
	free(src);
free_map:
	free(map);
free_fd:
	free(fd);
	return ret;
}
