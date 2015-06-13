/*
 * From macke.asd.sgi.com!paul Tue May 30 02:54:00 1995
 * Date: Mon, 29 May 95 23:53:32 -0700
 * From: paul@macke.asd.sgi.com (Paul Haeberli)
 * To: ian@sq.com
 * Subject: Re:  fastimg.c?
 * 
 * Here you go:
 * 
 * good luck!
 *
 */

/*
 *   	fastimg -
 *		Faster reading and writing SGI format image files.
 *
 *      This code should work on machines with any byte order.
 *
 *				Paul Haeberli - 1991
 */
#include "unistd.h"
#include "stdio.h"

/*
 *	from image.h
 *
 */
typedef struct {
    unsigned short	imagic;		/* stuff saved on disk . . */
    unsigned short 	type;
    unsigned short 	dim;
    unsigned short 	xsize;
    unsigned short 	ysize;
    unsigned short 	zsize;
    unsigned long 	min;
    unsigned long 	max;
    unsigned long	wastebytes;	
    char 		name[80];
    unsigned long	colormap;

    long 		file;		/* stuff used in core only */
    unsigned short 	flags;
    short		dorev;
    short		x;
    short		y;
    short		z;
    short		cnt;
    unsigned short	*ptr;
    unsigned short	*base;
    unsigned short	*tmpbuf;
    unsigned long	offset;
    unsigned long	rleend;		/* for rle images */
    unsigned long	*rowstart;	/* for rle images */
    long		*rowsize;	/* for rle images */
} IMAGE;

#define IMAGIC 	0732

#define TYPEMASK		0xff00
#define BPPMASK			0x00ff
#define ITYPE_VERBATIM		0x0000
#define ITYPE_RLE		0x0100
#define ISRLE(type)		(((type) & 0xff00) == ITYPE_RLE)
#define ISVERBATIM(type)	(((type) & 0xff00) == ITYPE_VERBATIM)
#define BPP(type)		((type) & BPPMASK)
#define RLE(bpp)		(ITYPE_RLE | (bpp))
#define VERBATIM(bpp)		(ITYPE_VERBATIM | (bpp))
/*
 *	end of image.h stuff
 *
 */

#define RINTLUM (79)
#define GINTLUM (156)
#define BINTLUM (21)

#define ILUM(r,g,b)     ((int)(RINTLUM*(r)+GINTLUM*(g)+BINTLUM*(b))>>8)

#define OFFSET_R	3	/* this is byte order dependent */
#define OFFSET_G	2
#define OFFSET_B	1
#define OFFSET_A	0

#define CHANOFFSET(z)	(3-(z))	/* this is byte order dependent */

static expandrow();
static setalpha();
static copybw();
static interleaverow();
static int compressrow();
static lumrow();

#define TAGLEN	(5)

/*
 *	addlongimgtag - 
 *		this is used to extract image data from core dumps.
 *
 */
addlongimgtag(dptr,xsize,ysize)
unsigned long *dptr;
int xsize, ysize;
{
    dptr = dptr+(xsize*ysize);
    dptr[0] = 0x12345678;
    dptr[1] = 0x59493333;
    dptr[2] = 0x69434222;
    dptr[3] = xsize;
    dptr[4] = ysize;
}

/*
 *	byte order independent read/write of shorts and longs.
 *
 */
static unsigned short getshort(inf)
FILE *inf;
{
    unsigned char buf[2];

    fread(buf,2,1,inf);
    return (buf[0]<<8)+(buf[1]<<0);
}

static unsigned long getlong(inf)
FILE *inf;
{
    unsigned char buf[4];

    fread(buf,4,1,inf);
    return (buf[0]<<24)+(buf[1]<<16)+(buf[2]<<8)+(buf[3]<<0);
}

static putshort(outf,val)
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

static readheader(inf,image)
FILE *inf;
IMAGE *image;
{
    bzero(image,sizeof(IMAGE));
    image->imagic = getshort(inf);
    image->type = getshort(inf);
    image->dim = getshort(inf);
    image->xsize = getshort(inf);
    image->ysize = getshort(inf);
    image->zsize = getshort(inf);
}

static int writeheader(outf,image)
FILE *outf;
IMAGE *image;
{
    IMAGE t;

    bzero(&t,sizeof(IMAGE));
    fwrite(&t,sizeof(IMAGE),1,outf);
    fseek(outf,0,SEEK_SET);
    putshort(outf,image->imagic);
    putshort(outf,image->type);
    putshort(outf,image->dim);
    putshort(outf,image->xsize);
    putshort(outf,image->ysize);
    putshort(outf,image->zsize);
    putlong(outf,image->min);
    putlong(outf,image->max);
    putlong(outf,0);
    return fwrite("no name",8,1,outf);
}

static int writetab(outf,tab,len)
FILE *outf;
unsigned long *tab;
int len;
{
    int r;

    while(len) {
	r = putlong(outf,*tab++);
	len -= 4;
    }
    return r;
}

static readtab(inf,tab,len)
FILE *inf;
unsigned long *tab;
int len;
{
    while(len) {
	*tab++ = getlong(inf);
	len -= 4;
    }
}

/*
 *	sizeofimage - 
 *		return the xsize and ysize of an iris image file.
 *
 */
sizeofimage(name, xsize, ysize)
char *name;
int *xsize, *ysize;
{
    IMAGE image;
    FILE *inf;

    inf = fopen(name,"r");
    if(!inf) {
	fprintf(stderr,"sizeofimage: can't open image file %s\n",name);
	exit(1);
    }
    readheader(inf,&image);
    if(image.imagic != IMAGIC) {
	fprintf(stderr,"sizeofimage: bad magic number in image file\n");
	exit(1);
    }
    *xsize = image.xsize;
    *ysize = image.ysize;
    fclose(inf);
}

/*
 *	longimagedata - 
 *		read in a B/W RGB or RGBA iris image file and return a 
 *	pointer to an array of longs.
 *
 */
unsigned long *longimagedata(name)
char *name;
{
    unsigned long *base, *lptr;
    unsigned char *rledat, *verdat;
    long *starttab, *lengthtab;
    FILE *inf;
    IMAGE *image;
    int y, z, pos, len, tablen;
    int xsize, ysize, zsize;
    int bpp, rle, cur, badorder;
    int rlebuflen;

    inf = fopen(name,"r");
    if(!inf) {
	fprintf(stderr,"longimagedata: can't open image file %s\n",name);
	exit(1);
    }
    image = (IMAGE *)malloc(sizeof(IMAGE));
    readheader(inf,image);
    if(image->imagic != IMAGIC) {
	fprintf(stderr,"longimagedata: bad magic number in image file\n");
	exit(1);
    }
    rle = ISRLE(image->type);
    bpp = BPP(image->type);
    if(bpp != 1 ) {
	fprintf(stderr,"longimagedata: image must have 1 byte per pix chan\n");
	exit(1);
    }
    xsize = image->xsize;
    ysize = image->ysize;
    zsize = image->zsize;
    if(rle) {
	tablen = ysize*zsize*sizeof(long);
	starttab = (long *)malloc(tablen);
	lengthtab = (long *)malloc(tablen);
	rlebuflen = 1.05*xsize+10;
	rledat = (unsigned char *)malloc(rlebuflen);
	fseek(inf,512,SEEK_SET);
 	readtab(inf,starttab,tablen);
	readtab(inf,lengthtab,tablen);

/* check data order */
	cur = 0;
	badorder = 0;
	for(y=0; y<ysize; y++) {
	    for(z=0; z<zsize; z++) {
		if(starttab[y+z*ysize]<cur) {
		    badorder = 1;
		    break;
		}
		cur = starttab[y+z*ysize];
	    }
	    if(badorder) 
		break;
	}

	fseek(inf,512+2*tablen,SEEK_SET);
	cur = 512+2*tablen;
	base = (unsigned long *)
		malloc((xsize*ysize+TAGLEN)*sizeof(long));
	addlongimgtag(base,xsize,ysize);
  	if(badorder) {
	    for(z=0; z<zsize; z++) {
		lptr = base;
		for(y=0; y<ysize; y++) {
		    if(cur != starttab[y+z*ysize]) {
			fseek(inf,starttab[y+z*ysize],SEEK_SET);
			cur = starttab[y+z*ysize];
		    }
		    if(lengthtab[y+z*ysize]>rlebuflen) {
			fprintf(stderr,"longimagedata: rlebuf is too small - bad poop\n");
			exit(1);
		    }
		    fread(rledat,lengthtab[y+z*ysize],1,inf);
		    cur += lengthtab[y+z*ysize];
		    expandrow(lptr,rledat,3-z);
		    lptr += xsize;
		}
	    }
	} else {
	    lptr = base;
	    for(y=0; y<ysize; y++) {
		for(z=0; z<zsize; z++) {
		    if(cur != starttab[y+z*ysize]) {
			fseek(inf,starttab[y+z*ysize],SEEK_SET);
			cur = starttab[y+z*ysize];
		    }
		    fread(rledat,lengthtab[y+z*ysize],1,inf);
		    cur += lengthtab[y+z*ysize];
		    expandrow(lptr,rledat,3-z);
		}
		lptr += xsize;
	    }
    	}
	if(zsize == 3) 
	    setalpha(base,xsize*ysize);
	else if(zsize<3) 
	    copybw(base,xsize*ysize);
	fclose(inf);
	free(starttab);
	free(lengthtab);
	free(rledat);
	free(image);
	return base;
    } else {
	base = (unsigned long *)
		malloc((xsize*ysize+TAGLEN)*sizeof(long));
	addlongimgtag(base,xsize,ysize);
	verdat = (unsigned char *)malloc(xsize);
	fseek(inf,512,SEEK_SET);
	for(z=0; z<zsize; z++) {
	    lptr = base;
	    for(y=0; y<ysize; y++) {
		fread(verdat,xsize,1,inf);
		interleaverow(lptr,verdat,3-z,xsize);
		lptr += xsize;
	    }
	}
	if(zsize == 3) 
	    setalpha(base,xsize*ysize);
	else if(zsize<3) 
	    copybw(base,xsize*ysize);
	fclose(inf);
	free(verdat);
	free(image);
	return base;
    }
}

/* static utility functions for longimagedata */

static interleaverow(lptr,cptr,z,n)
unsigned char *lptr, *cptr;
int z, n;
{
    lptr += z;
    while(n--) {
	*lptr = *cptr++;
	lptr += 4;
    }
}

static copybw(lptr,n)
long *lptr;
int n;
{
    while(n>=8) {
	lptr[0] = 0xff000000+(0x010101*(lptr[0]&0xff));
	lptr[1] = 0xff000000+(0x010101*(lptr[1]&0xff));
	lptr[2] = 0xff000000+(0x010101*(lptr[2]&0xff));
	lptr[3] = 0xff000000+(0x010101*(lptr[3]&0xff));
	lptr[4] = 0xff000000+(0x010101*(lptr[4]&0xff));
	lptr[5] = 0xff000000+(0x010101*(lptr[5]&0xff));
	lptr[6] = 0xff000000+(0x010101*(lptr[6]&0xff));
	lptr[7] = 0xff000000+(0x010101*(lptr[7]&0xff));
	lptr += 8;
	n-=8;
    }
    while(n--) {
	*lptr = 0xff000000+(0x010101*(*lptr&0xff));
	lptr++;
    }
}

static setalpha(lptr,n)
unsigned char *lptr;
{
    while(n>=8) {
	lptr[0*4] = 0xff;
	lptr[1*4] = 0xff;
	lptr[2*4] = 0xff;
	lptr[3*4] = 0xff;
	lptr[4*4] = 0xff;
	lptr[5*4] = 0xff;
	lptr[6*4] = 0xff;
	lptr[7*4] = 0xff;
	lptr += 4*8;
	n -= 8;
    }
    while(n--) {
	*lptr = 0xff;
	lptr += 4;
    }
}

static expandrow(optr,iptr,z)
unsigned char *optr, *iptr;
int z;
{
    unsigned char pixel, count;

    optr += z;
    while(1) {
	pixel = *iptr++;
	if ( !(count = (pixel & 0x7f)) )
	    return;
	if(pixel & 0x80) {
	    while(count>=8) {
		optr[0*4] = iptr[0];
		optr[1*4] = iptr[1];
		optr[2*4] = iptr[2];
		optr[3*4] = iptr[3];
		optr[4*4] = iptr[4];
		optr[5*4] = iptr[5];
		optr[6*4] = iptr[6];
		optr[7*4] = iptr[7];
		optr += 8*4;
		iptr += 8;
		count -= 8;
	    }
	    while(count--) {
		*optr = *iptr++;
		optr+=4;
	    }
	} else {
	    pixel = *iptr++;
	    while(count>=8) {
		optr[0*4] = pixel;
		optr[1*4] = pixel;
		optr[2*4] = pixel;
		optr[3*4] = pixel;
		optr[4*4] = pixel;
		optr[5*4] = pixel;
		optr[6*4] = pixel;
		optr[7*4] = pixel;
		optr += 8*4;
		count -= 8;
	    }
	    while(count--) {
		*optr = pixel;
		optr+=4;
	    }
	}
    }
}

/*
 *	longstoimage -
 *		copy an array of longs to an iris image file.  Each long
 *	represents one pixel.  xsize and ysize specify the dimensions of
 *	the pixel array.  zsize specifies what kind of image file to
 *	write out.  if zsize is 1, the luminance of the pixels are
 *	calculated, and a sinlge channel black and white image is saved.
 *	If zsize is 3, an RGB image file is saved.  If zsize is 4, an
 *	RGBA image file is saved.
 *
 */
int longstoimage(lptr,xsize,ysize,zsize,name)
unsigned long *lptr;
char *name;
int xsize, ysize, zsize;
{
    FILE *outf;
    IMAGE *image;
    int tablen, y, z, pos, len;
    long *starttab, *lengthtab;
    unsigned char *rlebuf;
    unsigned long *lumbuf;
    int rlebuflen, goodwrite;

    goodwrite = 1;
    outf = fopen(name,"w");
    if(!outf) {
	fprintf(stderr,"longstoimage: can't open output file\n");
	return 0;
    }
    tablen = ysize*zsize*sizeof(long);

    image = (IMAGE *)malloc(sizeof(IMAGE));
    starttab = (long *)malloc(tablen);
    lengthtab = (long *)malloc(tablen);
    rlebuflen = 1.05*xsize+10;
    rlebuf = (unsigned char *)malloc(rlebuflen);
    lumbuf = (unsigned long *)malloc(xsize*sizeof(long));

    bzero(image,sizeof(IMAGE));
    image->imagic = IMAGIC; 
    image->type = RLE(1);
    if(zsize>1)
	image->dim = 3;
    else
	image->dim = 2;
    image->xsize = xsize;
    image->ysize = ysize;
    image->zsize = zsize;
    image->min = 0;
    image->max = 255;
    goodwrite *= writeheader(outf,image);
    fseek(outf,512+2*tablen,SEEK_SET);
    pos = 512+2*tablen;
    for(y=0; y<ysize; y++) {
	for(z=0; z<zsize; z++) {
	    if(zsize == 1) {
		lumrow(lptr,lumbuf,xsize);
		len = compressrow(lumbuf,rlebuf,CHANOFFSET(z),xsize);
	    } else {
		len = compressrow(lptr,rlebuf,CHANOFFSET(z),xsize);
	    }
	    if(len>rlebuflen) {
		fprintf(stderr,"longstoimage: rlebuf is too small - bad poop\n");
		exit(1);
	    }
	    goodwrite *= fwrite(rlebuf,len,1,outf);
	    starttab[y+z*ysize] = pos;
	    lengthtab[y+z*ysize] = len;
	    pos += len;
	}
	lptr += xsize;
    }

    fseek(outf,512,SEEK_SET);
    goodwrite *= writetab(outf,starttab,tablen);
    goodwrite *= writetab(outf,lengthtab,tablen);
    free(image);
    free(starttab);
    free(lengthtab);
    free(rlebuf);
    free(lumbuf);
    fclose(outf);
    if(goodwrite)
	return 1;
    else {
	fprintf(stderr,"longstoimage: not enough space for image!!\n");
	return 0;
    }
}

/* static utility functions for longstoimage */

static lumrow(rgbptr,lumptr,n) 
unsigned char *rgbptr, *lumptr;
int n;
{
    lumptr += CHANOFFSET(0);
    while(n--) {
	*lumptr = ILUM(rgbptr[OFFSET_R],rgbptr[OFFSET_G],rgbptr[OFFSET_B]);
	lumptr += 4;
	rgbptr += 4;
    }
}

static int compressrow(lbuf,rlebuf,z,cnt)
unsigned char *lbuf, *rlebuf;
int z, cnt;
{
    unsigned char *iptr, *ibufend, *sptr, *optr;
    short todo, cc;							
    long count;

    lbuf += z;
    iptr = lbuf;
    ibufend = iptr+cnt*4;
    optr = rlebuf;

    while(iptr<ibufend) {
	sptr = iptr;
	iptr += 8;
	while((iptr<ibufend)&& ((iptr[-8]!=iptr[-4])||(iptr[-4]!=iptr[0])))
	    iptr+=4;
	iptr -= 8;
	count = (iptr-sptr)/4;
	while(count) {
	    todo = count>126 ? 126:count;
	    count -= todo;
	    *optr++ = 0x80|todo;
	    while(todo>8) {
		optr[0] = sptr[0*4];
		optr[1] = sptr[1*4];
		optr[2] = sptr[2*4];
		optr[3] = sptr[3*4];
		optr[4] = sptr[4*4];
		optr[5] = sptr[5*4];
		optr[6] = sptr[6*4];
		optr[7] = sptr[7*4];
		optr += 8;
		sptr += 8*4;
		todo -= 8;
	    }
	    while(todo--) {
		*optr++ = *sptr;
		sptr += 4;
	    }
	}
	sptr = iptr;
	cc = *iptr;
	iptr += 4;
	while( (iptr<ibufend) && (*iptr == cc) )
	    iptr += 4;
	count = (iptr-sptr)/4;
	while(count) {
	    todo = count>126 ? 126:count;
	    count -= todo;
	    *optr++ = todo;
	    *optr++ = cc;
	}
    }
    *optr++ = 0;
    return optr - (unsigned char *)rlebuf;
}

#ifdef TESTMAIN
/*
 *	this reads in an image and writes it out, and then
 *	uses ipaste to display the converted image file.
 *
 */
main(argc,argv)
int argc;
char **argv;
{
    int xsize, ysize;
    unsigned long *lbuf;

    if(argc<2) {
	fprintf(stderr,"usage: fastimg: inimage.rgb\n");
	exit(1);
    }
    sizeofimage(argv[1],&xsize,&ysize);
    lbuf = longimagedata(argv[1]);
    longstoimage(lbuf,xsize,ysize,3,"/usr/tmp/t.rgb");
    system("ipaste /usr/tmp/t.rgb");
}
#endif






