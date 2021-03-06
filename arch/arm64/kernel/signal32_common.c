/*
 * Based on arch/arm/kernel/signal.c
 *
 * Copyright (C) 1995-2009 Russell King
 * Copyright (C) 2012 ARM Ltd.
 * Modified by Will Deacon <will.deacon@arm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/compat.h>
#include <linux/signal.h>
#include <linux/uaccess.h>

#include <asm/signal32_common.h>
#include <asm/unistd.h>

int put_sigset_t(compat_sigset_t __user *uset, sigset_t *set)
{
	compat_sigset_t	cset;

	cset.sig[0] = set->sig[0] & 0xffffffffull;
	cset.sig[1] = set->sig[0] >> 32;

	return copy_to_user(uset, &cset, sizeof(*uset));
}

int get_sigset_t(sigset_t *set, const compat_sigset_t __user *uset)
{
	compat_sigset_t s32;

	if (copy_from_user(&s32, uset, sizeof(*uset)))
		return -EFAULT;

	set->sig[0] = s32.sig[0] | (((long)s32.sig[1]) << 32);
	return 0;
}

int copy_siginfo_to_user32(compat_siginfo_t __user *to, const siginfo_t *from)
{
	int err;

	if (!access_ok(VERIFY_WRITE, to, sizeof(*to)))
		return -EFAULT;

	/* If you change siginfo_t structure, please be sure
	 * this code is fixed accordingly.
	 * It should never copy any pad contained in the structure
	 * to avoid security leaks, but must copy the generic
	 * 3 ints plus the relevant union member.
	 * This routine must convert siginfo from 64bit to 32bit as well
	 * at the same time.
	 */
	err = __put_user(from->si_signo, &to->si_signo);
	err |= __put_user(from->si_errno, &to->si_errno);
	err |= __put_user(from->si_code, &to->si_code);
	if (from->si_code < 0)
		err |= __copy_to_user(&to->_sifields._pad, &from->_sifields._pad,
				      SI_PAD_SIZE);
	else switch (siginfo_layout(from->si_signo, from->si_code)) {
	case SIL_KILL:
		err |= __put_user(from->si_pid, &to->si_pid);
		err |= __put_user(from->si_uid, &to->si_uid);
		break;
	case SIL_TIMER:
		 err |= __put_user(from->si_tid, &to->si_tid);
		 err |= __put_user(from->si_overrun, &to->si_overrun);
		 err |= __put_user(from->si_int, &to->si_int);
		break;
	case SIL_POLL:
		err |= __put_user(from->si_band, &to->si_band);
		err |= __put_user(from->si_fd, &to->si_fd);
		break;
	case SIL_FAULT:
		err |= __put_user((compat_uptr_t)(unsigned long)from->si_addr,
				  &to->si_addr);
#ifdef BUS_MCEERR_AO
		/*
		 * Other callers might not initialize the si_lsb field,
		 * so check explicitly for the right codes here.
		 */
		if (from->si_signo == SIGBUS &&
		    (from->si_code == BUS_MCEERR_AR || from->si_code == BUS_MCEERR_AO))
			err |= __put_user(from->si_addr_lsb, &to->si_addr_lsb);
#endif
		break;
	case SIL_CHLD:
		err |= __put_user(from->si_pid, &to->si_pid);
		err |= __put_user(from->si_uid, &to->si_uid);
		err |= __put_user(from->si_status, &to->si_status);
		err |= __put_user(from->si_utime, &to->si_utime);
		err |= __put_user(from->si_stime, &to->si_stime);
		break;
	case SIL_RT:
		err |= __put_user(from->si_pid, &to->si_pid);
		err |= __put_user(from->si_uid, &to->si_uid);
		err |= __put_user(from->si_int, &to->si_int);
		break;
	case SIL_SYS:
		err |= __put_user((compat_uptr_t)(unsigned long)
				from->si_call_addr, &to->si_call_addr);
		err |= __put_user(from->si_syscall, &to->si_syscall);
		err |= __put_user(from->si_arch, &to->si_arch);
		break;
	}
	return err;
}

int copy_siginfo_from_user32(siginfo_t *to, compat_siginfo_t __user *from)
{
	if (copy_from_user(to, from, __ARCH_SI_PREAMBLE_SIZE) ||
	    copy_from_user(to->_sifields._pad,
			   from->_sifields._pad, SI_PAD_SIZE))
		return -EFAULT;

	return 0;
}

