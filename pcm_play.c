
#include <stdio.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include "vgm_p.h"
#include <time.h>
#include <sys/time.h>
#define base 0x378
#define time 100000
#define LPT_PORT 0x378
    
u_int32_t a_delay, d_delay  = 0;

signed char dac_byte;
unsigned char dac_byte_u;

FILE *file;

FILE *dac_file;

unsigned long dac_file_size = 0;


void slx(int delay){
    
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    for(;;) {
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        if((((end.tv_sec*1000000000)+end.tv_nsec) - ((start.tv_sec*1000000000)+start.tv_nsec)) > delay)
        
        break;
        //  printf("Start = %06d, End = %06d\n", (int)start.tv_usec, (int)end.tv_usec );
        // if(start.tv_nsec > end.tv_nsec) 
        //    break;
    }
}

void ym2612_Reset(void)
{
    outb(0x0, LPT_PORT+2); //0000 0000

    outb(0x1, LPT_PORT+2); //0000 0001

    outb(0x0, LPT_PORT+2); //0000 0000

    usleep(1000);
}


void ym2612_Send(unsigned char addr, unsigned char data, bool setA1) //0x52 = A1 LOW, 0x53 = A1 HIGH
{
    // 1000000000/8000000 = 125
    
    a_delay = 0;
    d_delay = 0; //SSG, ADPCM wait cycle  = 0
        

  switch(setA1)
  {
      
          
//0 unused 
//0 unused 
//0 bidi off 
//0 irq off 
//x select printer 17 - A0 - hw inverted
//x initialise reset 16 - A1
//x auto linefeed 14 - WR - hw inverted
//x strobe 1 - reset - hw inverted
  
      
    case 0:
    //outb(0x0, LPT_PORT+2); //0000 0000 
    outb(0x9, LPT_PORT+2); //0000 1001
    outb(addr,LPT_PORT);
    outb(0xB, LPT_PORT+2); //0000 1011 
     slx(100);
    outb(0x9, LPT_PORT+2); //0000 1001 
     slx(a_delay);
    outb(0x1, LPT_PORT+2); //0000 0001 
   
    outb(data,LPT_PORT);
    outb(0x3, LPT_PORT+2); //0000 0011 
     slx(100);
    outb(0x1, LPT_PORT+2); //0000 0001 
    slx(d_delay);

    break;
    case 1:
        
    outb(0xD, LPT_PORT+2); //0000 1101 
    outb(addr,LPT_PORT);
    outb(0xF, LPT_PORT+2); //0000 1111
    slx(100);
    outb(0xD, LPT_PORT+2); //0000 1101 
    slx(a_delay);
    outb(0x5, LPT_PORT+2); //0000 0101 
    outb(data,LPT_PORT);
    outb(0x7, LPT_PORT+2); //0000 0111 
    slx(100);
    outb(0x5, LPT_PORT+2); //0000 0101 
     slx(d_delay);

   
    break;
    }
    

}


int8_t flip_char(u_int8_t digit)
{
    int8_t rdigit;

    if (digit > 127)
    {
        rdigit = digit && 0x7F;
        
    }
    if (digit <= 127)
    {
    
        rdigit = 128 + digit;
    }
return rdigit;
}


void ym_dac_write_b(u_int8_t dac_byte_u) {
    
    ym2612_Send(0x10, 0x1B, 1);
    ym2612_Send(0x1, 0xCC, 1);
    ym2612_Send(0x0E, (dac_byte_u), 1);
    ym2612_Send(0x10, 0x80, 1);
}

int ym_dac_write(void){
    
    printf("ym dac write\n");
    
    ym2612_Send(0x10, 0x1B, 1);
    //ym2612_Send(0x10, 0x80, 1);
    //ym2612_Send(0x0, 0x80, 1);
    //ym2612_Send(0x1, 0xC0, 1);
    
    //ym2612_Send(0x06, 0xF4, 1); //8000Hz
    //ym2612_Send(0x07, 0x01, 1);
    
    ym2612_Send(0x06, 0xB5, 1); //22050Hz
    ym2612_Send(0x07, 0x00, 1);
    
    //ym2612_Send(0x06, 0x5B, 1); //44100Hz
    //ym2612_Send(0x07, 0x00, 1);

    
    //ym2612_Send(0x09, 0xB5, 1);
    //ym2612_Send(0x0A, 0x65, 1);
    ym2612_Send(0x1, 0xCC, 1);
    //ym2612_Send(0x0B, 0xA0, 1);
 

    for (int l=0;l<dac_file_size;l++) {
     
    fread(&dac_byte_u, sizeof(unsigned char), 1, dac_file); 
    //printf("%lu\n",dac_byte_u);
    //sleep(1);
    // printf("%lu\n",dac_byte);
       
    ym2612_Send(0x0E, (dac_byte_u), 1);
   // ym2612_Send(0x10, 0x80, 1);
    //slx(44);
    //slx(5669);
    //slx(11338);
    //slx(80000);
    slx(32000);
//    slx(22676);
    //slx(45352);
    //slx(90703);
    // ym2612_Send(0x10, 0x13, 1,);
    
    }
    ym2612_Send(0x0, 0x0, 1);
    ym2612_Send(0x10, 0x80, 1);

    return 0;
        
}


int main (int argc, char *argv[])
{

        
    if (argc < 2 )
    {
        printf("No file!\n");
        printf("Error.\n");
        exit (1);
    }


    struct timespec start;
    struct timespec end;
   

    unsigned int  twait;

    bool parse = false;

    
    dac_file = fopen(argv[1], "rb");

    
    if(file < 0) {
       printf ("8bit mono signed raw file not found \r");
       exit(1);
    }
    
    fseek(dac_file,0,SEEK_END);
	dac_file_size = ftell(dac_file);
	rewind(dac_file);
    
    
    if (ioperm(base,3,1))
    printf("Couldn't get port at %x\n", base), exit(1);
    if (ioperm(base, 3, 1)) {perror("ioperm"); exit(1);}

  
        ym2612_Reset();
        
        ym_dac_write();

        //   ym_zero();
        
        
            ym2612_Reset();
            
            fclose(dac_file);
      
}
      
