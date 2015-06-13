/*
 * This program will write out a valid B/W SGI image file:
 * Taken directly from Paul Haeberli's Image Spec document.
 */
 
#include "stdio.h"
 
#define IXSIZE      (23)
#define IYSIZE      (15)
 
putbyte(outf,val)
FILE *outf;
unsigned char val;
{
    unsigned char buf[1];
 
    buf[0] = val;
    fwrite(buf,1,1,outf);
}
 
putshort(outf,val)
FILE *outf;
unsigned short val;
{
    unsigned char buf[2];
 
    buf[0] = (val>>8);
    buf[1] = (val>>0);
    fwrite(buf,2,1,outf);
}
 
static int putlong(outf,val)
FILE *outf;
unsigned long val;
{
    unsigned char buf[4];
 
    buf[0] = (val>>24);
    buf[1] = (val>>16);
    buf[2] = (val>>8);
    buf[3] = (val>>0);
    return fwrite(buf,4,1,outf);
}
 
main()
{
    FILE *of;
    char iname[80];
    unsigned char outbuf[IXSIZE];
    int i, x, y;
 
    of = fopen("example.bw","w");
    if(!of) {
        fprintf(stderr,"sgiimage: can't open output file\n");
        exit(1);
    }
    putshort(of,474);       /* MAGIC                       */
    putbyte(of,0);          /* STORAGE is VERBATIM         */
    putbyte(of,1);          /* BPC is 1                    */
    putshort(of,2);         /* DIMENSION is 2              */
    putshort(of,IXSIZE);    /* XSIZE                       */
    putshort(of,IYSIZE);    /* YSIZE                       */
    putshort(of,1);         /* ZSIZE                       */
    putlong(of,0);          /* PIXMIN is 0                 */
    putlong(of,255);        /* PIXMAX is 255               */
    for (i=0; i<4; i++)      /* DUMMY 4 bytes       */
        putbyte(of,0);
    for (i=0; i<80; i++)
	iname[i] = 0;
    strcpy(iname,"No Name");
    fwrite(iname,80,1,of);  /* IMAGENAME           */
    putlong(of,0);          /* COLORMAP is 0       */
    for (i=0; i<404; i++)    /* DUMMY 404 bytes     */
        putbyte(of,0);
 
    for (y=0; y<IYSIZE; y++) {
        for (x=0; x<IXSIZE; x++) 
            outbuf[x] = (255*x)/(IXSIZE-1);
        fwrite(outbuf,IXSIZE,1,of);
    }
    fclose(of);
}
