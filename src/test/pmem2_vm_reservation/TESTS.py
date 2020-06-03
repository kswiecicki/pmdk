#!../env.py
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation
#

import os

import testframework as t
from testframework import granularity as g


class PMEM2_VM_RESERVATION(t.Test):
    test_type = t.Short
    filesize = 16 * t.MiB
    with_size = True

    def run(self, ctx):
        filepath = ctx.create_holey_file(self.filesize, 'testfile',)
        if self.with_size:
            filesize = os.stat(filepath).st_size
            ctx.exec('pmem2_vm_reservation', self.test_case, filepath,
                     filesize)
        else:
            ctx.exec('pmem2_vm_reservation', self.test_case, filepath)


class PMEM2_VM_RESERVATION_ASYNC(t.Test):
    test_type = t.Short
    filesize = 16 * t.MiB

    def run(self, ctx):
        filepath = ctx.create_holey_file(self.filesize, 'testfile',)
        filesize = os.stat(filepath).st_size
        ctx.exec('pmem2_vm_reservation', self.test_case, filepath,
                 filesize, self.threads, self.ops_per_thread)


@t.windows_exclude
@t.require_devdax(t.DevDax('devdax1'))
class PMEM2_VM_RESERVATION_DEVDAX(t.Test):
    test_type = t.Short
    with_size = True

    def run(self, ctx):
        dd = ctx.devdaxes.devdax1
        if self.with_size:
            ctx.exec('pmem2_vm_reservation', self.test_case, dd.path, dd.size)
        else:
            ctx.exec('pmem2_vm_reservation', self.test_case, dd.path)


@t.windows_exclude
@t.require_devdax(t.DevDax('devdax1'))
class PMEM2_VM_RESERVATION_ASYNC_DEVDAX(t.Test):
    test_type = t.Short
    with_size = True

    def run(self, ctx):
        dd = ctx.devdaxes.devdax1
        if self.with_size:
            ctx.exec('pmem2_vm_reservation', self.test_case, dd.path, dd.size,
                     self.threads, self.ops_per_thread)
        else:
            ctx.exec('pmem2_vm_reservation', self.test_case, dd.path,
                     self.threads, self.ops_per_thread)


class TEST0(PMEM2_VM_RESERVATION):
    """create a vm reservation in the region belonging to existing mapping"""
    test_case = "test_vm_reserv_new_region_occupied_map"


class TEST1(PMEM2_VM_RESERVATION_DEVDAX):
    """
    DevDax create a vm reservation in the region belonging to existing mapping
    """
    test_case = "test_vm_reserv_new_region_occupied_map"


class TEST2(PMEM2_VM_RESERVATION):
    """
    create a vm reservation in the region belonging to other
    existing vm reservation
    """
    test_case = "test_vm_reserv_new_region_occupied_reserv"


class TEST3(PMEM2_VM_RESERVATION_DEVDAX):
    """
    DevDax create a vm reservation in the region belonging to other
    existing vm reservation
    """
    test_case = "test_vm_reserv_new_region_occupied_reserv"


class TEST4(PMEM2_VM_RESERVATION):
    """map a file to a vm reservation"""
    test_case = "test_vm_reserv_map_file"


class TEST5(PMEM2_VM_RESERVATION_DEVDAX):
    """DevDax map a file to a vm reservation"""
    test_case = "test_vm_reserv_map_file"


class TEST6(PMEM2_VM_RESERVATION):
    """
    map a part of the file to a vm reservation smaller than the whole file
    """
    test_case = "test_vm_reserv_map_part_file"


class TEST7(PMEM2_VM_RESERVATION_DEVDAX):
    """
    DevDax map a part of the file to a vm reservation
    smaller than the whole file
    """
    test_case = "test_vm_reserv_map_part_file"


class TEST8(PMEM2_VM_RESERVATION):
    """delete a vm reservation that contains a mapping"""
    test_case = "test_vm_reserv_delete_contains_mapping"


class TEST9(PMEM2_VM_RESERVATION_DEVDAX):
    """DevDax delete a vm reservation that contains a mapping"""
    test_case = "test_vm_reserv_delete_contains_mapping"


class TEST10(PMEM2_VM_RESERVATION):
    """
    map multiple files to a vm reservation, then
    unmap every 2nd mapping and map the mappings again
    """
    test_case = "test_vm_reserv_map_unmap_multiple_files"


class TEST11(PMEM2_VM_RESERVATION_DEVDAX):
    """
    DevDax map multiple files to a vm reservation, then
    unmap every 2nd mapping and map the mappings again
    """
    test_case = "test_vm_reserv_map_unmap_multiple_files"


class TEST12(PMEM2_VM_RESERVATION):
    """map a file to a vm reservation with insufficient space"""
    test_case = "test_vm_reserv_map_insufficient_space"


class TEST13(PMEM2_VM_RESERVATION):
    """
    map a file to a vm reservation and overlap whole other existing mapping
    belonging to the same reservation
    """
    test_case = "test_vm_reserv_map_full_overlap"


class TEST14(PMEM2_VM_RESERVATION_DEVDAX):
    """
    DevDax map a file to a vm reservation and overlap whole other
    existing mapping belonging to the same reservation
    """
    test_case = "test_vm_reserv_map_full_overlap"


class TEST15(PMEM2_VM_RESERVATION):
    """
    map a file to a vm reservation overlapping with the ealier half
    of the other existing mapping
    """
    test_case = "test_vm_reserv_map_partial_overlap_below"


class TEST16(PMEM2_VM_RESERVATION_DEVDAX):
    """
    DevDax map a file to a vm reservation overlapping with the ealier half
    of the other existing mapping
    """
    test_case = "test_vm_reserv_map_partial_overlap_below"


class TEST17(PMEM2_VM_RESERVATION):
    """
    map a file to a vm reservation overlapping with the latter half
    of the other existing mapping
    """
    test_case = "test_vm_reserv_map_partial_overlap_above"


class TEST18(PMEM2_VM_RESERVATION_DEVDAX):
    """
    DevDax map a file to a vm reservation overlapping with the latter half
    of the other existing mapping
    """
    test_case = "test_vm_reserv_map_partial_overlap_above"


@g.require_granularity(g.PAGE, g.CACHELINE)
class TEST19(PMEM2_VM_RESERVATION):
    """
    map a file with invalid granularity to a vm reservation in the middle of
    the vm reservation bigger than the file, then map a file that covers whole
    vm reservation
    """
    test_case = "test_vm_reserv_map_invalid_granularity"


class TEST20(PMEM2_VM_RESERVATION_ASYNC):
    """
    map and unmap asynchronously multiple times to the whole vm reservation
    region
    """
    test_case = "test_vm_reserv_async_map_unmap_multiple_files"
    threads = 32
    ops_per_thread = 10000


class TEST21(PMEM2_VM_RESERVATION_ASYNC_DEVDAX):
    """
    DevDax map and unmap asynchronously multiple times to the whole
    whole vm reservation region
    """
    test_case = "test_vm_reserv_async_map_unmap_multiple_files"
    threads = 32
    ops_per_thread = 1000
