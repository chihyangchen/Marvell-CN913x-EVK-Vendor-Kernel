/*
 * Copyright 2010 Tilera Corporation. All Rights Reserved.
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 *   NON INFRINGEMENT.  See the GNU General Public License for
 *   more details.
 */

/* Adjust unistd.h to provide 32-bit numbers and functions. */
#define __SYSCALL_COMPAT

#include <linux/compat.h>
#include <linux/syscalls.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/signal.h>
#include <asm/syscalls.h>
#include <asm/byteorder.h>

/*
 * Syscalls that take 64-bit numbers traditionally take them in 32-bit
 * "high" and "low" value parts on 32-bit architectures.
 * In principle, one could imagine passing some register arguments as
 * fully 64-bit on TILE-Gx in 32-bit mode, but it seems easier to
 * adopt the usual convention.
 */

#ifdef __BIG_ENDIAN
#define SYSCALL_PAIR(name) u32, name ## _hi, u32, name ## _lo
#else
#define SYSCALL_PAIR(name) u32, name ## _lo, u32, name ## _hi
#endif

COMPAT_SYSCALL_DEFINE4(truncate64, char __user *, filename, u32, dummy,
		       SYSCALL_PAIR(length))
{
	return sys_truncate(filename, ((loff_t)length_hi << 32) | length_lo);
}

COMPAT_SYSCALL_DEFINE4(ftruncate64, unsigned int, fd, u32, dummy,
		       SYSCALL_PAIR(length))
{
	return sys_ftruncate(fd, ((loff_t)length_hi << 32) | length_lo);
}

COMPAT_SYSCALL_DEFINE6(pread64, unsigned int, fd, char __user *, ubuf,
		       size_t, count, u32, dummy, SYSCALL_PAIR(offset))
{
	return sys_pread64(fd, ubuf, count,
			   ((loff_t)offset_hi << 32) | offset_lo);
}

COMPAT_SYSCALL_DEFINE6(pwrite64, unsigned int, fd, char __user *, ubuf,
		       size_t, count, u32, dummy, SYSCALL_PAIR(offset))
{
	return sys_pwrite64(fd, ubuf, count,
			    ((loff_t)offset_hi << 32) | offset_lo);
}

COMPAT_SYSCALL_DEFINE6(sync_file_range2, int, fd, unsigned int, flags,
		       SYSCALL_PAIR(offset), SYSCALL_PAIR(nbytes))
{
	return sys_sync_file_range(fd, ((loff_t)offset_hi << 32) | offset_lo,
				   ((loff_t)nbytes_hi << 32) | nbytes_lo,
				   flags);
}

COMPAT_SYSCALL_DEFINE6(fallocate, int, fd, int, mode,
		       SYSCALL_PAIR(offset), SYSCALL_PAIR(len))
{
	return sys_fallocate(fd, mode, ((loff_t)offset_hi << 32) | offset_lo,
			     ((loff_t)len_hi << 32) | len_lo);
}

/*
 * Avoid bug in generic sys_llseek() that specifies offset_high and
 * offset_low as "unsigned long", thus making it possible to pass
 * a sign-extended high 32 bits in offset_low.
 * Note that we do not use SYSCALL_PAIR here since glibc passes the
 * high and low parts explicitly in that order.
 */
COMPAT_SYSCALL_DEFINE5(llseek, unsigned int, fd, unsigned int, offset_high,
		       unsigned int, offset_low, loff_t __user *, result,
		       unsigned int, origin)
{
	return sys_llseek(fd, offset_high, offset_low, result, origin);
}

/* Provide the compat syscall number to call mapping. */
#undef __SYSCALL
#define __SYSCALL(nr, call) [nr] = (call),

/* See comments in sys.c */
#define compat_sys_fadvise64_64 sys32_fadvise64_64
#define compat_sys_readahead sys32_readahead
#define sys_llseek compat_sys_llseek

#define sys_openat		compat_sys_openat
#define sys_open_by_handle_at	compat_sys_open_by_handle_at

/* Call the assembly trampolines where necessary. */
#define compat_sys_rt_sigreturn _compat_sys_rt_sigreturn
#define sys_clone _sys_clone

/*
 * Note that we can't include <linux/unistd.h> here since the header
 * guard will defeat us; <asm/unistd.h> checks for __SYSCALL as well.
 */
void *compat_sys_call_table[__NR_syscalls] = {
	[0 ... __NR_syscalls-1] = sys_ni_syscall,
#include <asm/unistd.h>
};
