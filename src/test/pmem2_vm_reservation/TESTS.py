#!../env.py
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation
#

import os

import testframework as t


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


class PMEM2_VM_RESERVATION_NO_FILE(t.Test):
    test_type = t.Short
    reserv_size = 16 * t.MiB
    with_reserv_size = True

    def run(self, ctx):
        if self.with_reserv_size:
            ctx.exec('pmem2_vm_reservation', self.test_case, self.reserv_size)
        else:
            ctx.exec('pmem2_vm_reservation', self.test_case)


class TEST0(PMEM2_VM_RESERVATION):
    """create a vm reservation in the region belonging to existing mapping"""
    test_case = "test_vm_reservation_region_occupied_by_map"


class TEST1(PMEM2_VM_RESERVATION_NO_FILE):
    """
    create a vm reservation in the region belonging to other
    existing reservation
    """
    test_case = "test_vm_reservation_region_occupied_by_reservation"
