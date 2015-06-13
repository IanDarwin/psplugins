/*
 * Read an SGI "image" format file, write as a PNM ASCII-format file.
 */

#include <stdio.h>
#include <assert.h>
#include <malloc.h>

#include "img.h"

static char *progname;

static void
UpdateProgress(int sofar, int total)
{
#if	0
	fprintf(stderr, "On row %d of %d\n", sofar, total);
#endif
}

static void readtab(FILE *inf, unsigned long *tab, int len);

static int
readimg(FILE *fp, char *fname)
{
	struct imghdr *h = readhdr(fp);
	short encoding, channels;
	short pixsize;
	unsigned long width, height;
	unsigned long pix, darkest, lightest;
	int c, row, col;
	unsigned short **rowtab;
	int tablen;
	unsigned long *starttab, *lengthtab, rleoffset, rlelength;

	if (!h) {
		printf("Out of memory\n");
		exit(1);
	}
	if (h->magic != IMGMAGIC)
		return -1;

	fprintf(stderr, "Image name \"%s(%s)\", size %dx%dx%d\n",
		fname, h->imgname, h->xsize, h->ysize, h->zsize);

#define	mismatch(tag, val, exp) {\
	fprintf(stderr, "imgtopnm: invalid %s (%d), expecting %s\n", tag, val, exp);\
	return -2;\
	}

	if (h->storage == VERBATIM) encoding = VERBATIM;
	else if (h->storage == RLE) encoding = RLE;
	else mismatch("Storage encoding", h->storage, "VERBATIM or RLE");

	if (h->bpc == BPPsingle) pixsize = BPPsingle;
	else if (h->bpc == BPPdouble) pixsize = BPPdouble;
	else mismatch("Pixel Size", h->bpc, "1 or 2");

	switch(h->dimension) {
	case degenerate:
		width = h->xsize;
		height = 1;
		channels = 1;
		break;
	case grayscale:
		width = h->xsize;
		height = h->ysize;
		assert(h->zsize == 1);
		channels = 1;
		break;
	case dcolor:
		width = h->xsize;
		height = h->ysize;
		assert(h->zsize == 3 || h->zsize == 4);
		channels = h->zsize;
		break;
	default:
		mismatch("Dimensionality", h->dimension, "1, 2 or 3");
		/*NOTREACHED*/
	}

	darkest = h->pixmin;
	lightest = h->pixmax;

	switch(h->cmap_type) {
	case NORMAL:
		break;
	case DITHERED_OBSOLETE:
	case SCREEN_OBSOLETE:
		fprintf(stderr, "Obsolete image format not supported\n");
		return -2;
		break;
	case SGI_CMAP:
		fprintf(stderr, "Image is an SGI colormap, not an image\n");
		return -2;
		break;
	default:
		mismatch("Colormap Type", h->cmap_type, "Normal (0)");
		/*NOTREACHED*/
	}

	/* Now read the RLE encoding tables, if present */
	switch(encoding) {
	case VERBATIM:
		break;
	case RLE:
		tablen = h->ysize*h->zsize*4 /* not sizeof(long) */;
		starttab = (unsigned long *) malloc(tablen);
		lengthtab = (unsigned long *) malloc(tablen);
		assert(4==sizeof(long)); /* if not, rewrite using getlong */
		fseek(fp, 512, SEEK_SET);
		readtab(fp, starttab,  tablen);
		readtab(fp, lengthtab,  tablen);
		rowtab = (unsigned short **) malloc(h->ysize * channels);
		break;
	default:
		printf("Logic error: encoding set to %d\n", encoding);
		break;
	}

	printf("P%u\n"
		"# created by '%s %s'\n"
		"%lu %lu\n"
		"%u\n", 
		h->zsize>1?6:1,
		progname, fname, width, height, h->pixmax);
	(void)fflush(stdout);

	/* Finally, read the image data. */
	for (c=0; c<channels; c++) {	/* not the little language */
		fprintf(stderr, "Reading Channel %d of %d\n", c+1, channels);
		for (row=0; row<height; row++) {
			UpdateProgress(c*height+row, channels*height);
			if (encoding == VERBATIM) {
				for (col=0; col<width; col++) {
					if (pixsize == BPPsingle)
						pix = getc(fp);
					else
						pix = getshort(fp);
					/*printf("\t%d %d %d", pix, pix, pix);*/
					printf("%lu ", pix);
				}
				putchar('\n');
			} else {/* case RLE */
				rleoffset = starttab[row+c*height];
				rlelength = lengthtab[row+c*height];
				rowtab[c*height + row] = (unsigned short *)malloc(width * sizeof(short));
				assert(rowtab[c*height + row]);
				expandrow(rowtab[c*height + row], fp, pixsize);
			}
		}
	}
	if (encoding == RLE) {
		fprintf(stderr, "Writing...\n");
		/* Invert top-to-bottom for pnm format */
		/* TODO off by one?? get rid of junk at bottom */
		for (row=3*(height-1); row>=0; row-=3)
			for (col=0; col<width; col++)
			printf("%c%c%c",
				(char)rowtab[row+0][col],
				(char)rowtab[row+1][col],
				(char)rowtab[row+2][col]);
	}
	printf("\n");
	return 0;
}

static void readtab(FILE *inf, unsigned long *tab, int len)
{
    while(len) {
	*tab++ = getlong(inf);
	len -= 4;
    }
}

int main(int argc, char *argv[])
{
	FILE *f;

	progname = argv[0];
	if (argc-1 != 1) {
		fprintf(stderr, "Usage: %s imagefile [> pnmfile]\n", progname);
		return -1;
	}

	if (!(f = fopen(argv[1], "r"))) {
		perror(argv[1]);
		return 1;
	}

	if (readimg(f, argv[1]) < 0)
		printf("%s: not an SGI image file\n", argv[1]);

	return 0;
}
