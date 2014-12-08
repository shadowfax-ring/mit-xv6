#include "types.h"
#include "stat.h"
#include "user.h"

void periodic(void);
void periodic_reentrant(void);

int
main(int argc, char *argv[])
{
  int i;

  alarm(10, (void *)periodic);
  //alarm(10, (void *)periodic_reentrant);
  for(i = 0; i < 50*500000; i++){
    if((i++ % 500000) == 0)
      write(2, ".", 1);
  }
  exit();
  return 0;
}

/* Initial callback */
void
periodic(void)
{
  printf(1, "alarm!\n");
}

/* == Test re-entrant call ==
 * Suppose the callback itself will consume longer time
 * than given alarm period. If the OS does not prevent
 * re-entrant calls, for each period, the function will
 * start from the beginning and dead loop in the callback
 * itself. */
void
periodic_reentrant(void)
{
	int i;
	printf(2, "\n\n==== Test Re-entrant ====\n\n");
	for(i = 0; i < 10*500000; i++){
		if (i % 5000 == 0)
			printf(2, "alarm%d! ", i/5000);
	}
	printf(2, "\n");
}

