/**
 *
 * cpulimit - a cpu limiter for Linux
 *
 * Copyright (C) 2005-2012, by:  Angelo Marletta <marlonx80@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __PROCESS_ITERATOR_H

#define __PROCESS_ITERATOR_H

#include <unistd.h>
#include <limits.h>
#include <dirent.h>

//USER_HZ detection, from openssl code
#ifndef HZ
# if defined(_SC_CLK_TCK) \
     && (!defined(OPENSSL_SYS_VMS) || __CTRL_VER >= 70000000)
#  define HZ ((double)sysconf(_SC_CLK_TCK))
# else
#  ifndef CLK_TCK
#   ifndef _BSD_CLK_TCK_ /* FreeBSD hack */
#    define HZ  100.0
#   else /* _BSD_CLK_TCK_ */
#    define HZ ((double)_BSD_CLK_TCK_)
#   endif
#  else /* CLK_TCK */
#   define HZ ((double)CLK_TCK)
#  endif
# endif
#endif

#ifdef __FreeBSD__
#include <kvm.h>
#endif

// process descriptor
struct process {
	//pid of the process
	pid_t pid;
	//pid of the process
	pid_t ppid;
	//start time
	int starttime;
	//cputime used by the process expressed in milliseconds
	int cputime;
	//actual cpu usage estimation (value in range 0-1)
	double cpu_usage;
	//1 if the process is zombie
	int is_zombie;
	//absolute path of the executable file
	char command[PATH_MAX+1];
};

struct process_filter {
	int pid;
	int include_children;
	char program_name[PATH_MAX+1];
};

struct process_iterator {
#ifdef __linux__
	DIR *dip;
	int boot_time;
#elif defined __FreeBSD__
	struct kinfo_proc *procs;
	int count;
	int i;
#elif defined __APPLE__
	struct kinfo_proc *proclist;
#endif
	struct process_filter *filter;
};

int init_process_iterator(struct process_iterator *i, struct process_filter *filter);

int get_next_process(struct process_iterator *i, struct process *p);

int close_process_iterator(struct process_iterator *i);

#endif