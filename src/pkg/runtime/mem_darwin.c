// Copyright 2010 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "runtime.h"
#include "arch_GOARCH.h"
#include "defs_GOOS_GOARCH.h"
#include "os_GOOS.h"
#include "malloc.h"

void*
runtime·SysAlloc(uintptr n, uint64 *stat)
{
	void *v;

	v = runtime·mmap(nil, n, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
	if(v < (void*)4096)
		return nil;
	runtime·xadd64(stat, n);
	return v;
}

void
runtime·SysUnused(void *v, uintptr n)
{
	// Linux's MADV_DONTNEED is like BSD's MADV_FREE.
	runtime·madvise(v, n, MADV_FREE);
}

void
runtime·SysUsed(void *v, uintptr n)
{
	USED(v);
	USED(n);
}

void
runtime·SysFree(void *v, uintptr n, uint64 *stat)
{
	runtime·xadd64(stat, -(uint64)n);
	runtime·munmap(v, n);
}

void
runtime·SysFault(void *v, uintptr n)
{
	runtime·mmap(v, n, PROT_NONE, 0, -1, 0);
}

void*
runtime·SysReserve(void *v, uintptr n)
{
	void *p;

	p = runtime·mmap(v, n, PROT_NONE, MAP_ANON|MAP_PRIVATE, -1, 0);
	if(p < (void*)4096)
		return nil;
	return p;
}

enum
{
	ENOMEM = 12,
};

void
runtime·SysMap(void *v, uintptr n, uint64 *stat)
{
	void *p;
	
	runtime·xadd64(stat, n);
	p = runtime·mmap(v, n, PROT_READ|PROT_WRITE, MAP_ANON|MAP_FIXED|MAP_PRIVATE, -1, 0);
	if(p == (void*)ENOMEM)
		runtime·throw("runtime: out of memory");
	if(p != v)
		runtime·throw("runtime: cannot map pages in arena address space");
}
