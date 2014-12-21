#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

#define DEFAULT_PAGE_ALLOC

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
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
#ifdef DEFAULT_PAGE_ALLOC
  if(growproc(n) < 0)
    return -1;
#endif
  proc->sz += n;
  switchuvm(proc);
  //cprintf("sbrk: extend size: %d\n", n);
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
    if(proc->killed){
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
sys_halt(void)
{
  char *p = "Shutdown";

  // This is a special power-off sequence supported by Bochs and QEMU,
  // but not by physical hardware. */
  for( ; *p; p++) {
    outb(0x8900, *p);
  }
  return 0;
}

int
sys_alarm(void)
{
   int ticks;
   void (*handler)();

   //cprintf("Enter alarm!\n");
   if(argint(0, &ticks) < 0)
     return -1;
   if(argptr(1, (char**)&handler, 1) < 0)
     return -1;
   proc->alarmticks = ticks;
   proc->alarmhandler = handler;

   return 0;
}

int sys_restore_caller_saved_regs(void)
{
	struct trapframe *tf = proc->tf;
#ifdef DEBUG_TRAPFRAME
	cprintf("new ESP = %x\n", tf->esp);
#endif

	// Locate the position of $esp to the saved registers on kernel stack
	// Since this is another trapframe, it's different from the initial
	// trapframe for the timer. The esp has already poped after the timer
	// callback, so the offest is not 12.
	tf->esp += 8;

	tf->edx = *(uint *)(tf->esp);	
	tf->esp += 4;

	tf->ecx = *(uint *)(tf->esp);	
	tf->esp += 4;

	tf->eax = *(uint *)(tf->esp);	
	tf->esp += 4;

	tf->eip = *(uint *)(tf->esp);
	tf->esp += 4;

	proc->alarmhandlerfired = 0;

	return 0;
}

