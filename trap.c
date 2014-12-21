#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

int mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm);
#ifdef LAZY_PAGE_ALLOC
int lazyalloc(uint addr);
#endif

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
  
  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(proc->killed)
      exit();
    proc->tf = tf;
    syscall();
    if(proc->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpu->id == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
	// when timer interrupt comes from userspace
	if(proc && (tf->cs & 3) == 3 && !proc->alarmhandlerfired) {
		proc->accumticks++;
		if (proc->accumticks == proc->alarmticks) {
			proc->accumticks = 0;
			// following three lines can be interpreted as follows
			// 1) push tf->eip (instruction after finishing the callback)
			// 2) move proc->alarmhandler, eip (reposition next instruction)
			*((uint *)(tf->esp - 4)) = tf->eip;
			*((uint *)(tf->esp - 8)) = tf->eax;
			*((uint *)(tf->esp - 12)) = tf->ecx;
			*((uint *)(tf->esp - 16)) = tf->edx;

			// Add instructions in the stack after handler returns
			// movl $SYS_restore_caller_saved_reg, %eax
			// int $T_SYSCALL
			// ret (can be omitted)
			*((uint *)(tf->esp - 20)) = 0xc340cd00;
			*((uint *)(tf->esp - 24)) = 0x000018b8;

			// set return address of the alarm handler
			*((uint *)(tf->esp - 28)) = tf->esp - 24;
			tf->esp -= 28;
#ifdef DEBUG_TRAPFRAME
			cprintf("old ESP = %x\n", tf->esp);
#endif
			tf->eip = (uint) proc->alarmhandler;

			/* avoid re-entrant calls to alarm handler */
			proc->alarmhandlerfired = 1;
		}
	}
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpu->id, tf->cs, tf->eip);
    lapiceoi();
    break;
#ifdef LAZY_PAGE_ALLOC
  case T_PGFLT:
	if (lazyalloc(rcr2()) < 0) {
		proc->killed = 1;
	}
	break;
#endif 
  //PAGEBREAK: 13
  default:
    if(proc == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpu->id, tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            proc->pid, proc->name, tf->trapno, tf->err, cpu->id, tf->eip, 
            rcr2());
    proc->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running 
  // until it gets to the regular system call return.)
  if(proc && proc->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(proc && proc->state == RUNNING && tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(proc && proc->killed && (tf->cs&3) == DPL_USER)
    exit();
}

#ifdef LAZY_PAGE_ALLOC
int lazyalloc(uint addr)
{
    cprintf("Enter Page Fault!\n");
    uint a = PGROUNDDOWN(addr);
    char *mem = kalloc();
	if (mem == 0) {
      cprintf("lazyalloc: kalloc out of memory\n");
	  return -1;
	}
	memset(mem, 0, PGSIZE);
	if (mappages(proc->pgdir, (char*)a, PGSIZE, v2p(mem), PTE_W|PTE_U) != 0) {
		cprintf("lazyalloc: manpages error\n");
		return -1;
	}
	return 0;
}
#endif

