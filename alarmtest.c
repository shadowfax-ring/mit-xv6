#include "types.h"
#include "stat.h"
#include "user.h"

void periodic(void);

int
main(int argc, char *argv[])
{
  int i;

  alarm(10, (void *)periodic);
  for(i = 0; i < 50*500000; i++){
    if((i++ % 500000) == 0)
      write(2, ".", 1);
  }
  exit();
  return 0;
}

void
periodic(void)
{
  printf(1, "alarm!\n");
}
