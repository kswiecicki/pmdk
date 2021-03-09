---
layout: manual
Content-Style: 'text/css'
title: _MP(PMEMSET_REMOVE_RANGE, 3)
collection: libpmemset
header: PMDK
date: pmemset API version 1.0
...

[comment]: <> (SPDX-License-Identifier: BSD-3-Clause)
[comment]: <> (Copyright 2021, Intel Corporation)

[comment]: <> (pmemset_remove_range.3 -- man page for libpmemset pmemset_remove_range operation)

[NAME](#name)<br />
[SYNOPSIS](#synopsis)<br />
[DESCRIPTION](#description)<br />
[RETURN VALUE](#return-value)<br />
[ERRORS](#errors)<br />
[SEE ALSO](#see-also)<br />

# NAME #

**pmemset_remove_range**() - removes mapped parts overlapping with the provided range

# SYNOPSIS #

```c
#include <libpmemset.h>

struct pmemset;
int pmemset_remove_range(struct pmemset *set, void *addr, size_t len);
```

# DESCRIPTION #

**pmemset_remove_range**() - removes and unmaps existing mapped parts belonging to the *set* overlapping
with virtual memory range described by the *addr* and *len* variables.

This function discriminates coalesced part mappings and the individual parts composing it. It means
that only the mapped parts constituting the coalesced part mapping that overlap with the provided range
will be removed, not the whole coalesced part mapping. For more information about coalesced part
mappings see **pmemset_set_contiguous_part_coalescing(3)**.

# RETURN VALUE #

The **pmemset_remove_range**() function returns 0 on success or a negative error code on failure.

# ERRORS #

The **pmemset_remove_range**() function can fail with the following errors:

* **PMEMSET_E_CANNOT_FIND_PART_MAP** - the provided range doesn't overlap with any part mapping stored
in the pmemset

It can also return errors from the underlying **pmem2_vm_reservation_remove_range**(3) function
belonging to the libpmem2 library.

# SEE ALSO #

**pmem2_vm_reservation_remove_range**(3),
**pmemset_set_contiguous_part_coalescing(3)**,
**libpmemset**(7) and **<http://pmem.io>**
