#include <stdio.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <time.h>
#include <sys/time.h>
#include  <stdlib.h>
#define base 0x378
#define time 100000
#define LPT_PORT 0x378


void ym_reset(void)
{
       outb(0x0, LPT_PORT+2); //0000 0000
        outb(0x1, LPT_PORT+2); //0000 0000
       outb(0x0, LPT_PORT+2); //0000 0000
    
}

int main()

{
   if (ioperm(base,3,1))
  printf("Couldn't get port at %x\n", base), exit(1);
  if (ioperm(base, 3, 1)) {perror("ioperm"); exit(1);}
  
  ym_reset();

 return 0;
    
}
