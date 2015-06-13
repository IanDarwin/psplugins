/*
 * Structure of an SGI "image" as it exists on disk.
 * Of course we can't just read(2) into this, as every C
 * compiler does a slightly different job of structure alignment...
 */

struct imghdr {
#define	IMGMAGIC	474			/* decimal, not octal */
	short magic;
	enum { VERBATIM = 0, RLE = 1 } storage;	/* encoding */
	enum { BPPsingle = 1, BPPdouble = 2 } bpc;/* bytes per pixel channel */
	enum { degenerate = 1, grayscale = 2, dcolor = 3 } dimension;
	short xsize, ysize;
	enum { NC_BW = 1, NC_RGB=3, NC_RGBA = 4 } zsize;
	long pixmin, pixmax;			/* darkest...lightest */
	char dummy[4];				/* wasted */
	char imgname[80];			/* optional image name */
	enum { NORMAL = 0, DITHERED_OBSOLETE = 1,
		SCREEN_OBSOLETE = 2, SGI_CMAP = 3} cmap_type;
	char dummy2[404];			/* pads to 512 on disk */
};

unsigned short getshort(FILE *fp);
unsigned long getlong(FILE *fp);
struct imghdr *readhdr(FILE *fp);
void expandrow(unsigned char *optr, FILE *ifp, int byteorshort);
