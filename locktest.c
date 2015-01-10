#include "types.h"
#include "defs.h"
#include "param.h"
//#include "stat.h"
//#include "user.h"
#include "spinlock.h"

int
main(int argc, char *argv[])
{
	struct spinlock lk;
	initlock(&lk, "test lock");
	acquire(&lk);
	acquire(&lk);
	release(&lk);

	exit();
	return 0;
}

