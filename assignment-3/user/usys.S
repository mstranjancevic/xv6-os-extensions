#include "kernel/syscall.h"
#include "kernel/traps.h"

#define SYSCALL(name) \
	.globl name; \
	name: \
		movl $SYS_ ## name, %eax; \
		int $T_SYSCALL; \
		ret

SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)
SYSCALL(snap_take)
SYSCALL(snap_restore)
SYSCALL(snap_diff)
SYSCALL(snap_release)
SYSCALL(snap_status)
SYSCALL(snap_empty)
SYSCALL(snap_count_holes)
