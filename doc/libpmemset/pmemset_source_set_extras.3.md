---
layout: manual
Content-Style: 'text/css'
title: _MP(PMEMSET_SOURCE_SET_EXTRAS, 3)
collection: libpmemset
header: PMDK
date: pmemset API version 1.0
...

[comment]: <> (SPDX-License-Identifier: BSD-3-Clause)
[comment]: <> (Copyright 2021, Intel Corporation)

[comment]: <> (pmemset_source_set_extras.3 -- man page for pmemset_source_set_extras)

[NAME](#name)<br />
[SYNOPSIS](#synopsis)<br />
[DESCRIPTION](#description)<br />
[RETURN VALUE](#return-value)<br />
[SEE ALSO](#see-also)<br />

# NAME #

**pmemset_source_set_extras**() - store extra parameters in the source structure

# SYNOPSIS #

```c
#include <libpmemset.h>

struct pmemset_extras {
	struct pmemset_sds *sds;
	struct pmemset_badblock *bb;
};

struct pmemset_source;
void pmemset_source_set_extras(struct pmemset_source *src,
		struct pmemset_extras *ext);
```

# DESCRIPTION #

The **pmemset_source_set_extras**() stores extra parameters *ext* in the source
*src* structure.

Shutdown data state parameter *sds* can be initialized using **pmemset_sds_new**(3)
function.

Bad block parameter *bb* is not yet supported.

**pmemset_source_set_extras**() sets *sds* and *bb* variables to NULL.

# RETURN VALUE

The **pmemset_source_set_extras**() function does not return any value.

# SEE ALSO #

**pmemset_sds_new**(3),
**libpmemset**(7) and **<http://pmem.io>**
