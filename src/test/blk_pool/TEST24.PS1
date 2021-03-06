# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2015-2019, Intel Corporation
#
# src/test/blk_pool/TEST24 -- unit test for pmemblk_create
#

. ..\unittest\unittest.ps1

require_test_type medium

setup

#
# TEST24 existing file, file size >= min required size, bsize == 1024
#        (bsize doesn't match the value from pool header)
#
expect_normal_exit $Env:EXE_DIR\blk_pool$Env:EXESUFFIX `
    c $DIR\testfile 512 20 0640

expect_normal_exit $Env:EXE_DIR\blk_pool$Env:EXESUFFIX `
    o $DIR\testfile 1024

check

pass
