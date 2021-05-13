---
layout: manual
Content-Style: 'text/css'
title: _MP(PMEM2_VM_RESERVATION_MAP_FIND, 3)
collection: libpmem2
header: PMDK
date: pmem2 API version 1.0
...

[comment]: <> (SPDX-License-Identifier: BSD-3-Clause)
[comment]: <> (Copyright 2021, Intel Corporation)

[comment]: <> (pmem2_vm_reservation_map_find.3 -- man page for libpmem2 pmem2_vm_reservation_map_find operation)

[NAME](#name)<br />
[SYNOPSIS](#synopsis)<br />
[DESCRIPTION](#description)<br />
[RETURN VALUE](#return-value)<br />
[ERRORS](#errors)<br />
[SEE ALSO](#see-also)<br />

# NAME #

**pmem2_vm_reservation_map_find**(), **pmem2_vm_reservation_map_find_previous**(),
**pmem2_vm_reservation_map_find_next**(), **pmem2_vm_reservation_map_find_first**() and
**pmem2_vm_reservation_map_find_last**() - search for the mapping located at the
desirable location

# SYNOPSIS #

```c
#include <libpmem2.h>

struct pmem2_map;
struct pmem2_vm_reservation;
int pmem2_vm_reservation_map_find(struct pmem2_vm_reservation *rsv,
		size_t reserv_offset, size_t len, struct pmem2_map **map_ptr);
int pmem2_vm_reservation_map_find_previous(struct pmem2_vm_reservation *rsv,
		struct pmem2_map *map, struct pmem2_map **map_ptr);
int pmem2_vm_reservation_map_find_next(struct pmem2_vm_reservation *rsv,
		struct pmem2_map *map, struct pmem2_map **map_ptr;
int pmem2_vm_reservation_map_find_first(struct pmem2_vm_reservation *rsv,
		struct pmem2_map **map_ptr);
int pmem2_vm_reservation_map_find_last(struct pmem2_vm_reservation *rsv,
		struct pmem2_map **map_ptr);
```

# DESCRIPTION #

The **pmem2_vm_reservation_map_find**() function searches for the earliest mapping,
stored in the virtual memory reservation, intersecting with the interval formed
by *reserv_offset* and *len* variables.

**pmem2_vm_reservation_map_find_previous**() function searches for the mapping in the
reservation, previous to the provided *map* mapping.

**pmem2_vm_reservation_map_find_next**() function searches for the mapping in the
reservation, next after the provided *map* mapping.

**pmem2_vm_reservation_map_find_first**() function searches for the mapping located
first in the reservation.

**pmem2_vm_reservation_map_find_last**() function searches for the mapping located
last in the reservation.

# RETURN VALUE #

The **pmem2_vm_reservation_map_find**(), **pmem2_vm_reservation_map_find_previous**(),
**pmem2_vm_reservation_map_find_next**(), **pmem2_vm_reservation_map_find_first**() and
**pmem2_vm_reservation_map_find_last**() return 0 on success or a negative error on failure.

It passes an address to the found mapping via user provided *map* pointer variable
on success, otherwise it passes *NULL* value when no mapping was found.

# ERRORS #

The **pmem2_vm_reservation_map_find**(), **pmem2_vm_reservation_map_find_previous**(),
**pmem2_vm_reservation_map_find_next**(), **pmem2_vm_reservation_map_find_first**() and
**pmem2_vm_reservation_map_find_last**() can fail with the following errors:

- **PMEM2_E_MAPPING_NOT_FOUND** - no mapping found at the desirable location of the reservation

# SEE ALSO #

**libpmem2**(7), and **<https://pmem.io>**
