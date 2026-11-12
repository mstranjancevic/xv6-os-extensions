#include "types.h"
#include "defs.h"
#include "proc.h"
#include "stat.h"

int
sys_fork(void)
{
	return fork();
}

int
sys_exit(void)
{
	exit();
	return 0;  // not reached
}

int
sys_wait(void)
{
	return wait();
}

int
sys_kill(void)
{
	int pid;

	if(argint(0, &pid) < 0)
		return -1;
	return kill(pid);
}

int
sys_getpid(void)
{
	return myproc()->pid;
}

int
sys_sbrk(void)
{
	int addr;
	int n;

	if(argint(0, &n) < 0)
		return -1;
	addr = myproc()->sz;
	if(growproc(n) < 0)
		return -1;
	return addr;
}

int
sys_sleep(void)
{
	int n;
	uint ticks0;

	if(argint(0, &n) < 0)
		return -1;
	acquire(&tickslock);
	ticks0 = ticks;
	while(ticks - ticks0 < n){
		if(myproc()->killed){
			release(&tickslock);
			return -1;
		}
		sleep(&ticks, &tickslock);
	}
	release(&tickslock);
	return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
	uint xticks;

	acquire(&tickslock);
	xticks = ticks;
	release(&tickslock);
	return xticks;
}

int
sys_snap_take(void)
{
	int slot;
	if(argint(0, &slot) < 0)
		return -4;
	return snap_take(slot);
}

int
sys_snap_restore(void)
{
	int slot;
	if(argint(0, &slot) < 0)
		return -3;
	return snap_restore(slot);
}

int
sys_snap_diff(void)
{
	int slot1, slot2;
	if(argint(0, &slot1) < 0)
		return -3;
	if(argint(1, &slot2) < 0)
		return -3;
	return snap_diff(slot1, slot2);
}

int
sys_snap_release(void)
{
	int slot;
	if(argint(0, &slot) < 0)
		return -3;
	return snap_release(slot);
}

int
sys_snap_status(void)
{
	int slot;
	struct snap_info *si;
	if(argint(0, &slot) < 0)
		return -2;
	if(argptr(1, (char**)&si, sizeof(struct snap_info)) < 0)
		return -2;
	return snap_status(slot, si);
}

int
sys_snap_empty(void)
{
	return snap_empty();
}

int
sys_snap_count_holes(void)
{
	int index;
	if(argint(0, &index) < 0)
		return -1;
	return snap_count_holes(index);
}
