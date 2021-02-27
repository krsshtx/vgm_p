#include <stdio.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include "vgm_p.h"
#include <time.h>
#include <sys/time.h>

// YM vgm 
// initial idea Romanych NedoPC #4 2006
// initial code Aidan Lawrence for STM32 MegaBlaster 2018
// this version krssh 2021
//
// YM2608 revision.
//
#define base 0x378
#define time 100000
#define LPT_PORT 0x378
bool ready = true;
int dcounter;
bool samplePlaying = false;

#define MAX_PCM_BUFFER_SIZE 524288

u_int16_t loopCount = 0;
u_int8_t maxLoops = 2;

FILE *file;
FILE *file2;
FILE *file3;
FILE *file4;
FILE *file10;
          
FILE *dac_file;
    
u_int32_t bufferPos = 0;
u_int32_t cmdPos = 0;
u_int16_t waitSamples = 0;
u_int32_t pcmBufferPosition = 0;

u_int32_t PCMSize = 0;
u_int32_t PCMoffset = 0;

u_int32_t ROMsize = 0;
u_int32_t ROMoffset = 0;

u_int32_t ramcounter = 0;
u_int32_t ramcounterold = 0;

unsigned char segaram[262144], *p; // описываем статический массив и указатель
unsigned char segarambyte;
    
unsigned char segaramin[262144];
unsigned char DATAtype;

u_int32_t RAMwrite;
    
u_int32_t startaddrh, startaddrl, stopaddrh, stopaddrl;
u_int32_t startaddrfull, startaddrhm, startaddrlm;
u_int32_t stopaddrfull, stopaddrhm, stopaddrlm;
     
u_int32_t a_delay, d_delay  = 0;
     
static VGMHeader header;

unsigned char segabyte,dac_byte; 

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
 
u_int8_t readBuffer()

    {
    fread(&segabyte, sizeof(unsigned char), 1, file); 
    bufferPos++;
    cmdPos++;
    return segabyte;
}

u_int16_t readBuffer16()

{
  u_int16_t d;
  unsigned char v0 = readBuffer();
  unsigned char v1 = readBuffer();
  d = u_int16_t(v0 + (v1 << 8));
  bufferPos+=2;
  cmdPos+=2;
  return d;
}

u_int32_t readBuffer32()
{
  u_int32_t d;
  unsigned char v0 = readBuffer();
  unsigned char v1 = readBuffer();
  unsigned char v2 = readBuffer();
  unsigned char v3 = readBuffer();
  d = u_int32_t(v0 + (v1 << 8) + (v2 << 16) + (v3 << 24));
  bufferPos+=4;
  cmdPos+=4;
  return d;
}

void ym2612_Reset(void)
{
    outb(0x1, LPT_PORT+2); //0000 0000
       slx(3000);
    outb(0x0, LPT_PORT+2); //0000 0001
       slx(3000);
    outb(0x1, LPT_PORT+2); //0000 0000
       slx(3000);
    outb(0x1, LPT_PORT+2); //0000 0001
       slx(3000);
//    outb(0x4, LPT_PORT+2); //0000 0100
  //     slx(3000);
 //   outb(0x5, LPT_PORT+2); //0000 0001
  //     slx(3000);
 //   outb(0x0, LPT_PORT+2); //0000 0000
//       slx(3000);
//    outb(0x5, LPT_PORT+2); //0000 0001
 //      slx(3000);
}




void ym2612_Send(unsigned char addr, unsigned char data, bool setA1) //0x52 = A1 LOW, 0x53 = A1 HIGH
{
    // 1000000000/8000000 = 125
    
    a_delay = 5;
    d_delay = 5; //SSG, ADPCM wait cycle  = 0
        
    if ((addr > 0x21 ) && (addr < 0xB6))   { //FM wait cycle > 17
    //ad_delay =  2125;    }
    a_delay =  2200;    }
         
    if (setA1 == 0 ) {
         
        if ((addr > 0x10 ) && (addr < 0x1D)) {
                //ad_delay =  2125;    }
                a_delay =  2200;    
        }
    }
         
    if ((addr > 0x21 ) && (addr < 0x9E))   {
    d_delay =  10375;    }
         
    if ((addr > 0xA0 ) && (addr < 0xB6))   {
    d_delay =  5875;    }
        
         if (setA1 == 0 ) {
             
            if (addr == 0x10 ) {
            //d_delay =  72000;    }
            d_delay =  75000;    }
                
            if ((addr > 0x11 ) && (addr < 0x1D))   {
            d_delay =  10375;    }
         }
    
    
  switch(setA1)
  {
    case 0:
    //outb(0x0, LPT_PORT+2); //0000 0000 
    outb(0x9, LPT_PORT+2); //0000 1001
    outb(addr,LPT_PORT);
    outb(0xB, LPT_PORT+2); //0000 1011 
     slx(a_delay);
    outb(0x9, LPT_PORT+2); //0000 1001 
    outb(0x1, LPT_PORT+2); //0000 0001 
    outb(data,LPT_PORT);
    outb(0x3, LPT_PORT+2); //0000 0011 
     slx(d_delay);
    outb(0x1, LPT_PORT+2); //0000 0001 

    break;
    case 1:
        
    outb(0xD, LPT_PORT+2); //0000 1101 
    outb(addr,LPT_PORT);
    outb(0xF, LPT_PORT+2); //0000 1111
     slx(a_delay);
    outb(0xD, LPT_PORT+2); //0000 1101 
    outb(0x5, LPT_PORT+2); //0000 0101 
    outb(data,LPT_PORT);
    outb(0x7, LPT_PORT+2); //0000 0111 
     slx(d_delay);
    outb(0x5, LPT_PORT+2); //0000 0101 

   
    break;
    }
    
    
//0 unused 
//0 unused 
//0 bidi off 
//0 irq off 
//x select printer 17 - A0 - hw inverted
//x initialise reset 16 - A1
//x auto linefeed 14 - WR - hw inverted
//x strobe 1 - reset - hw inverted

    
}


int ym_ram_write(void)
{
    
    u_int8_t datal;
    u_int8_t offset;
    
    offset = 5;
    
printf("\nRAM write.\n");
    
 ym2612_Send(0x10, 0x13, 1);
 ym2612_Send(0x10, 0x80, 1);
 ym2612_Send(0x0, 0x60, 1);
 ym2612_Send(0x1, 0x02, 1);
 
 // bits >> 2 (PC-8801)
 // bytes >> 5 (PC-9801)
 //
 // 256kbytes = 64 seconds of adpcm 4 bit audio at 8khz
 
 // with 8bit RAM ROMoffset >> 5 etc.
 // full offset = 0x0FFF
 
 // >>2
 // 0xFFFF
 
 
   
 ym2612_Send(0x02, (ROMoffset >> offset ) & 0xFF, 1);
 ym2612_Send(0x03, (ROMoffset >> offset ) >> 8 , 1);
 
 ym2612_Send(0x04, ((PCMSize -1 + ROMoffset) >> offset)  & 0xFF, 1);
 ym2612_Send(0x05, ((PCMSize -1 + ROMoffset) >> offset) >> 8 , 1);
 
 ym2612_Send(0x0c, ((ROMsize-1) >> offset) & 0xFF, 1);
 ym2612_Send(0x0d, ((ROMsize-1) >> offset) >> 8 , 1);
   
 
   printf("ROMoffset: ");
   printf("%lu",ROMoffset >> offset);
   
   printf(" End: ");
   printf("%lu",(PCMSize  >> offset)+ (ROMoffset >> offset));

   printf(" PCMSize: ");
   printf("%lu\n\n",PCMSize  >> offset);
   
  
    for (int l=ramcounterold;l<(ramcounterold+PCMSize);l++) {
     
        datal = segaram[l];
     
        ym2612_Send(0x08, datal, 1);

        ym2612_Send(0x10, 0x1b, 1);
        ym2612_Send(0x10, 0x13, 1);

        }
  
    ym2612_Send(0x0, 0x0, 1);
    ym2612_Send(0x10, 0x80, 1);

    PCMoffset=PCMoffset+PCMSize;

    return 0;
}

int ym_zero(void)

{
    printf("ymzero\n");
    
    int RAMsize = 262143;
    
    ym2612_Send(0x10, 0x13, 1);
    ym2612_Send(0x10, 0x80, 1);
    ym2612_Send(0x0, 0x60, 1);
    ym2612_Send(0x1, 0x0, 1);
    
    ym2612_Send(0x02, 0, 1);
    ym2612_Send(0x03, 0 , 1);
    ym2612_Send(0x04, ((RAMsize-1) >> 2)  & 0xFF, 1);
    ym2612_Send(0x05, ((RAMsize-1) >> 2) >> 8 , 1);
    ym2612_Send(0x0c, ((RAMsize-1) >> 2) & 0xFF, 1);
    ym2612_Send(0x0d, ((RAMsize-1) >> 2) >> 8 , 1);
   
    for (int l=0;l<RAMsize-1;l++) {
     
        ym2612_Send(0x08, 0x00, 1);
        ym2612_Send(0x10, 0x1b, 1);
        ym2612_Send(0x10, 0x13, 1);
        }
        
    ym2612_Send(0x0, 0x0, 1);
    ym2612_Send(0x10, 0x80, 1);

    return 0;
}


int ym_dac_write(void){
    
    int dts=0;
    
    printf("ym dac write\n");
    
    ym2612_Send(0x10, 0x17, 1);
    ym2612_Send(0x10, 0x80, 1);
    ym2612_Send(0x0, 0x80, 1);
    ym2612_Send(0x1, 0xC0, 1);
    
    ym2612_Send(0x06, 0xF4, 1);
    ym2612_Send(0x07, 0x01, 1);
    
    ym2612_Send(0x09, 0xE7, 1);
    ym2612_Send(0x0A, 0x24, 1);
 
    ym2612_Send(0x0B, 0xA0, 1);
 

    for (int l=0;l<3340578;l++) {
     
    fread(&dac_byte, sizeof(unsigned char), 1, dac_file); 
    
    // printf("%lu\n",dac_byte);
       
    ym2612_Send(0x08, dac_byte, 1);
    ym2612_Send(0x10, 0x1b, 1);
    slx(35000);
    // ym2612_Send(0x10, 0x13, 1,0,0);
    }
    ym2612_Send(0x0, 0x0, 1);
    ym2612_Send(0x10, 0x80, 1);

    return 0;
        
}


char ym0=0x52;
char ym1=0x53;
bool addr2b=false;
bool wait7f=false;
u_int8_t wait7fsum;
u_int8_t wait7fsuma;
int lbuf;
unsigned int failedCmd;

int firstreg = 0;

u_int16_t parseVGM() 
{
    
  u_int8_t cmd = readBuffer();
  
  //  printf("%d",cmd);printf("<cmd ");
        switch(cmd)
  {
      
    case 0x52:
    {
    u_int8_t addr = readBuffer();
    u_int8_t data = readBuffer();

    //fwrite(&ym0, sizeof(unsigned char), 1, file2);
    //fwrite(&addr, sizeof(unsigned char), 1, file2);
    //fwrite(&data, sizeof(unsigned char), 1, file2);

    if (addr !=0x21 ){
    ym2612_Send(addr, data, 0);}

    }
    return 0;

    case 0x53:
    {
    u_int8_t addr = readBuffer();
    u_int8_t data = readBuffer();

    
     //fwrite(&ym1, sizeof(unsigned char), 1, file2);
     //fwrite(&addr, sizeof(unsigned char), 1, file2);
     //fwrite(&data, sizeof(unsigned char), 1, file2);
    
     if (addr !=0x21 ){
     ym2612_Send(addr, data, 1);}

    }
    return 0;

    case 0x54:
    {
    u_int8_t addr = readBuffer();
    u_int8_t data = readBuffer();
    
    if (addr !=0x21 ){
        ym2612_Send(addr, data, 1);}

        }
    return 0;

    case 0x55:
    {
    u_int8_t addr = readBuffer();
    u_int8_t data = readBuffer();

    if (addr !=0x21 ){
        ym2612_Send(addr, data, 0);}
      
        }
    return 0;

    case 0x56:
    {
    u_int8_t addr = readBuffer();
    u_int8_t data = readBuffer();

    if (addr !=0x21 ) {

    ym2612_Send(addr, data, 0);}

    }
    return 0;

    case 0x57:
    {
    u_int8_t addr = readBuffer();
    u_int8_t data = readBuffer();
    
    if (addr == 0x00 )
    {
        if ( data == 0x21 ) {
            
        //  data =  0x17;
        //  printf("%lu",data);printf(" x00 data  \n");
        }
    }
        if (addr == 0x01 )
    {
        if ( data == 0xc2 || data == 0x82 || data == 0x42) {
        //     data = data - 2;
        //data = 0xc0;
        }

        if ( data == 0x2) {
            
         //   data = 0x0;
         // printf("%lu",data);printf(" x02 data +2 \n");
            
        }
    }
    if (addr == 0x02 )
    {
        startaddrl = data;
        
        //  data = data >> 3 ;
        //    printf("%lu",startaddrl);printf(" x02 startl \n");
        //    break;
    }
        if (addr == 0x03 )
    {
        startaddrh = data ;
        
        //  data = data >> 3 ;
        //  printf("%lu",startaddrl);printf(" x02 startl \n");
        //  printf("%lu",startaddrh );printf(" x03 starth \n");
        //  startaddrfull = ((startaddrh << 8)+startaddrl)  ;
        //  startaddrfull = 0;
        //  startaddrfull = ((startaddrh << 8)+startaddrl)   ;
        //  printf("%lu",startaddrfull);printf("  startfull ");
        //  startaddrlm = startaddrfull & 0xFF ;
        //  startaddrhm = startaddrfull >> 8;
         
        //  ym2612_Send(0x02, startaddrlm, 1);
        //  ym2612_Send(0x03, startaddrhm, 1);
        //  break;

    }
    
        if (addr == 0x04 )
    {
        stopaddrl = data;
        //  data = data >> 3 ;
        printf("%lu",stopaddrl);printf(" x04 stopl ");
        //  break;
       
    }
        if (addr == 0x05 )
    {
        stopaddrh = data;
        //  data = data >> 3 ;
        printf("%lu",stopaddrh);printf(" x05 stoph \n");

        //  stopaddrfull = ((stopaddrh << 8)+stopaddrl)  ;
        //  stopaddrfull=0;
        //  stopaddrfull = ((stopaddrh << 8)+stopaddrl)   ;
        //  printf("%lu",stopaddrfull);printf(" stopfull \n");
        //  stopaddrlm = stopaddrfull & 0xFF ;
        //  stopaddrhm = stopaddrfull >> 8;
        //  ym2612_Send(0x04, stopaddrlm, 1);
        //  ym2612_Send(0x05, stopaddrhm, 1);
        //  break;
    }

        //      if (addr == 0x09 )
        //  {
        //       printf("%lu",data);printf("x09 delta \n");
        //  }
        //      if (addr == 0x0a )
        //  {
        //       printf("%lu",data);printf("x0a delta \n");
        //  }
    
        //if (addr !=0x21 )
        // {
        ym2612_Send(addr, data, 1);
        //  }

    }
    return 0;
 
    case 0x58:
    {
    u_int8_t addr = readBuffer();
    u_int8_t data = readBuffer();
  
        if ((addr >= 0x10 ) && (addr <= 0x1f)) {
            
            printf("%lu",addr);printf(" addr side \n");
            addr=addr-16;
        
            if (addr == 0x12 )
            {
             startaddrl = data;
             printf("%lu",startaddrl);printf(" x02 startl \n");
            }
            if (addr == 0x13 )
            {
             startaddrh = data;
             printf("%lu",startaddrh);printf(" x03 starth \n");
            }

            ym2612_Send(addr, data, 1);
            
            break;
            }
    
    // if (addr !=0x21 ){
    ym2612_Send(addr, data, 0);
        
    }

    //}
    return 0;

    case 0x59:
    {
    u_int8_t addr = readBuffer();
    u_int8_t data = readBuffer();

    //  if (addr !=0x21 ){
    ym2612_Send(addr, data, 1);
    }
    // }
    return 0;
    
    case 0xA0:
    {
    u_int8_t addr = readBuffer();
    u_int8_t data = readBuffer();

    if (addr !=0x21 ){
    ym2612_Send(addr, data, 0);}

    }
    return 0;
    
    case 0xA5:
    {
    u_int8_t addr = readBuffer();
    u_int8_t data = readBuffer();
    
    if (addr !=0x21 ){
    ym2612_Send(addr, data, 1);}
      
    }
    return 0;

    case 0xD2: //Konami SCC1
    { 
    readBuffer();
    readBuffer();
    readBuffer();
    }
    return 0;

    case 0xB4: //Konami SCC1
    { 
    readBuffer();
    readBuffer();
    }
    return 0;
        
    case 0x70:
    case 0x71:
    case 0x72:
    case 0x73:
    case 0x74:
    case 0x75:
    case 0x76:
    case 0x77:
    case 0x78:
    case 0x79:
    case 0x7A:
    case 0x7B:
    case 0x7C:
    case 0x7D:
    case 0x7E:
    case 0x7F:
    {
      
      //fwrite(&cmd, sizeof(unsigned char), 1, file2);
      return ((cmd & 0x0F)+1);

    }
  
    case 0x80:
    case 0x81:
    case 0x82:
    case 0x83:
    case 0x84:
    case 0x85:
    case 0x86:
    case 0x87:
    case 0x88:
    case 0x89:
    case 0x8A:
    case 0x8B:
    case 0x8C:
    case 0x8D:
    case 0x8E:
    case 0x8F:
    {
      
      //fwrite(&cmd, sizeof(unsigned char), 1, file2);
      u_int32_t wait = cmd & 0x0F;
      u_int8_t addr = 0x2A;
      u_int8_t data = segaram[pcmBufferPosition];
      
      //fwrite(&data, sizeof(unsigned char), 1, file4);
      
      pcmBufferPosition++;
      //printf("r");
      ym2612_Send(addr, data, 0);
      
      return (int)wait;
     
    }

    case 0x4F:
        
    // sn76489.Send(0x06);
    readBuffer();
    
    //sn76489.Send(readBuffer());
    return 0;
    
    case 0x50:
    //  sn76489.Send(readBuffer());
    readBuffer();
        
    return 0;
    
    case 0x61:
       
    {
    
    lbuf=readBuffer16();
     // fwrite(&lbuf, sizeof(int), 1, file2);
    }
    return (int)lbuf;
        
    case 0x62:
    return 735;
    //usleep(16667);
    
    case 0x63:
    return 882;
        
    //usleep(20000);
    return 0;
    
    case 0x67:
    {
    printf("Data block read.\n");
        
    readBuffer(); //Discard 0x66 
    //  pcmBufferPosition = cmdPos;
   
    DATAtype=readBuffer(); 
        
    if ((DATAtype == 0x81) || (DATAtype == 0x82)) {
                
        pcmBufferPosition = 0;
        PCMSize = readBuffer32() - 8 ;
      
        printf("PCMSize: ");    
        printf("%lu",PCMSize);
        
        ROMsize = readBuffer32(); 
      
        printf(" ROMsize: ");
        printf("%lu",ROMsize);
        
        ROMoffset = readBuffer32(); 
      
        printf(" ROMoffset: ");
        printf("%lu",ROMoffset);
        
        printf(" End: ");
        printf("%lu\n",PCMSize+ROMoffset);
     
        if(PCMSize > MAX_PCM_BUFFER_SIZE)
             {
                printf("PCM Size too big!\n");
             }
        ramcounterold=ramcounter;
      
        for (u_int32_t i = 0; i < PCMSize; i++)
             {
        
                segaram[ramcounter]=readBuffer();
                fwrite(&segaram[ramcounter], sizeof(unsigned char), 1, file3);
                ramcounter++;
             }
        ym_ram_write();

    }
     
     if (DATAtype == 0xC0) { //FM towns ram write
          
        printf(" datatype: ");    
        printf("%lu\n",DATAtype);
        printf("%lu\n",RAMwrite);
        
        RAMwrite = readBuffer16();
      
        fseek ( file , RAMwrite , SEEK_CUR );
    }
      
    if (DATAtype == 0xC2) {
          
        printf(" datatype: ");    
        printf("%lu\n",DATAtype);
        printf("%lu\n",RAMwrite);
        
        RAMwrite = readBuffer32();
      
        fseek ( file , RAMwrite , SEEK_CUR );
    }
     
    if (DATAtype == 0x00) {
     
            pcmBufferPosition = 0;
            PCMSize = readBuffer32() ;
      
            if(PCMSize > MAX_PCM_BUFFER_SIZE)
            {
                printf("PCM Size too big!\n");
            }
       
            for (u_int32_t i = 0; i < PCMSize; i++)
                {
                    segaram[i]=readBuffer();
                    fwrite(&segaram[i], sizeof(unsigned char), 1, file3);
                }
    }
      
    if (DATAtype == 0x91) {
          
        printf(" datatype: ");    
        printf("%lu\n",DATAtype);
              
        RAMwrite = readBuffer32();
      
        printf("%lu\n",RAMwrite);
        
        fseek ( file , RAMwrite  , SEEK_CUR );
    }
    
    /*
    if (DATAtype == 0x82) { //ym2610 adpcm romdata
          
        printf(" datatype: ");    
        printf("%lu\n",DATAtype);
          
        RAMwrite = readBuffer32();
      
        printf("%lu\n",RAMwrite);
        
        fseek ( file , RAMwrite  , SEEK_CUR );
    }
    */
    
    if (DATAtype == 0x83) { //ym2610 delta-t romdata
          
        printf(" datatype: ");    
        printf("%lu\n",DATAtype);
          
        RAMwrite = readBuffer32();
      
        printf("%lu\n",RAMwrite);
        
        fseek ( file , RAMwrite  , SEEK_CUR );
    }
      
    }
    
    return 0;

    case 0xC8:
    {
      readBuffer16();
    }
    return 0;

    case 0xE0:
    {
      pcmBufferPosition = readBuffer32();
    }
    //PCMseek
    return 0;
    
    case 0x66:
    {
    printf (" Loop. \n");
    ready = false;

    cmdPos = 0;

    loopCount++;
     
    if (loopCount < maxLoops) {
    fseek(file,header.loopOffset, 0);   
    
    ready = true;
    
    }
    }
    return 0;
    default:
    {
    //  commandFailed = true;
    //  printf ("Failed cmd \r");
    failedCmd = cmd;
    //  printf("%lu",failedCmd);printf("<failedCmd \n");
        
    }
    return 0;
  }

  return 0;
}


void udelay(long us){
    
    struct timespec current;
    struct timespec start;

    clock_gettime(CLOCK_MONOTONIC, &start);
    
     while( ( current.tv_nsec -  start.tv_nsec  ) < us );
        {
        clock_gettime(CLOCK_MONOTONIC, &current);
          printf (" sleep \n");
        }
   
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

    //file = fopen("chase.vgm", "rb");
    file = fopen(argv[1], "rb");

    //  dac_file = fopen("segaram_l.bin", "rb");
    //  dac_file = fopen("encode.raw", "rb");
    
    if(file < 0) {
       printf ("File not found \r");
       exit(1);
    }
    /*
    if(dac_file < 0) {
       printf ("File not found \r");
       exit(1);
    }
    */
    
    //VGM Header
    header.indent = readBuffer32(); //VGM
    header.EoF = readBuffer32(); //EoF
    header.version = readBuffer32(); //Version
    header.sn76489Clock = readBuffer32(); //SN Clock
    header.ym2413Clock = readBuffer32(); //YM2413 Clock
    header.gd3Offset = readBuffer32(); //GD3 Offset
    header.totalSamples = readBuffer32(); //Total Samples
    header.loopOffset = readBuffer32(); //Loop Offset
    header.loopNumSamples = readBuffer32(); //Loop # Samples
    header.rate = readBuffer32(); //Rate
    header.snX = readBuffer32(); //SN etc.
    header.ym2612Clock = readBuffer32(); //YM2612 Clock
    header.ym2151Clock = readBuffer32(); //YM2151 Clock
    header.vgmDataOffset = readBuffer32(); //VGM data Offset
    header.segaPCMClock = readBuffer32(); //Sega PCM Clock
    header.spcmInterface = readBuffer32(); //SPCM Interface

   
    //Jump to VGM data start and compute loop location
    if(header.vgmDataOffset == 0x0C){
        header.vgmDataOffset = 0x40;
        printf("%lu",header.vgmDataOffset);printf(" vgmDataOffset \n");}
    else
    {
        header.vgmDataOffset += 0x34;
        printf("%lu",header.vgmDataOffset);printf(" vgmDataOffset \n");
    }
  
    if(header.vgmDataOffset != 0x40)
        {
        for(u_int32_t i = 0x40; i<header.vgmDataOffset; i++)
        readBuffer();
        }
    if(header.loopOffset == 0x00)
        {
            header.loopOffset = header.vgmDataOffset;
            printf("%lu",header.loopOffset);printf(" loopOffset\n");
        }
        
        else{
            header.loopOffset += 0x1C;
            printf("%lu",header.loopOffset);printf(" loopOffset\n");
        }
        
        //   printf ("clock \r",header.ym2612Clock);
    
    remove("segafm.bin");
 
    file2 = fopen("segafm.bin", "wb");
    file3 = fopen("segaram.bin", "wb");
    file4 = fopen("segapcm.bin", "wb");
    
              
    file10 = fopen("ymram.bin", "wb");
  
    FILE *fchar;
  
    if (ioperm(base,3,1))
    printf("Couldn't get port at %x\n", base), exit(1);
    if (ioperm(base, 3, 1)) {perror("ioperm"); exit(1);}
  

    ym2612_Reset();
      
   //   ym_dac_write();
   //   ym_zero();
      
  
    while(loopCount < maxLoops)
        {

            waitSamples = parseVGM();
    
            if(waitSamples > 0) {
        
            twait=(waitSamples*22676) ; //-periodnsec;
        
            slx(twait);
                   
            }
   
        }   
   
    usleep(1000);
    ym2612_Reset();
           
    fclose(file);
    fclose(file2);	
    fclose(file3);	
    fclose(file4);
    //  fclose(file10);
    
    //fclose(dac_file);
  

    return 0;
}
	

