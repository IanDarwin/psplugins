/*
 * Read the header from an SGI "image" format file.
 */

#include <stdio.h>
#include <assert.h>
#include <malloc.h>

#include "img.h"

struct imghdr *
readhdr(FILE *fp)
{
	struct imghdr *h = (struct imghdr *)malloc(sizeof(struct imghdr));
	int i;

	if (!h)
		return h;
	h->magic = getshort(fp);
	h->storage = getc(fp);
	h->bpc = getc(fp);
	h->dimension = getshort(fp);
	h->xsize = getshort(fp);
	h->ysize = getshort(fp);
	h->zsize = getshort(fp);
	h->pixmin = getlong(fp);
	h->pixmax = getlong(fp);
	for (i=1; i<=4; i++)
		(void) getc(fp);
	for (i=0; i<80; i++)
		h->imgname[i] = getc(fp);
	h->cmap_type = getlong(fp);

	/* don't bother reading the 404 spaces :-) */
	if (fseek(fp, 512L, SEEK_SET) < 0) {
		perror("fseek past header");
		return NULL;
	}

	return h;
}

void
dumphdr(struct imghdr *h)
{
	switch (h->storage) {
		case VERBATIM:	
			printf("VERBATIM\n");
			break;
		case RLE:	
			printf("RLE Encoded\n");
			break;
		default:	
			printf("INVALID ENCODING %d\n", h->storage);
			break;
	}
	switch (h->bpc) {	
		case BPPsingle:
			printf("BPPsingle (1 byte per pixel channel)\n");
			break;
		case BPPdouble:
			printf("BPPdouble (2 bytes per pixel channel)\n");
			break;
		default:	printf("INVALID BPC\n");
			break;
	}
	switch (h->dimension) {
		case degenerate:
			printf("Dimension: DGENERATE\n"); break;
		case grayscale:
			printf("Dimension: GRAYSCALE\n"); break;
		case dcolor:
			printf("Dimension: DCOLOR\n"); break;
		default:
			printf("INVALID DIMENSION %d\n", h->dimension); break;
	}
	printf("Size %d x %d x %d", h->xsize, h->ysize, h->zsize);
	switch (h->zsize) {
		case NC_BW: printf(" (Bitmap)\n"); break;
		case NC_RGB: printf(" (RGB)\n"); break;
		case NC_RGBA: printf(" (RGBA)\n"); break;
		default: printf("INVALID ZSIZE %d", h->zsize); break;
	}
	printf("Pixels range from %d to %d\n", h->pixmin, h->pixmax);
	switch(h->cmap_type) {
	case NORMAL:
		printf("cmap NORMAL\n");
		break;
	case DITHERED_OBSOLETE:
		printf("cmap DITHERED_OBSOLETE\n");
		break;
	case SCREEN_OBSOLETE:
		printf("cmap SCREEN_OBSOLETE\n");
		break;
	case SGI_CMAP:
		printf("cmap JUST_A_COLORMAP\n");
		break;
	default: printf("INVALID CMAP_TYPE %d\n", h->cmap_type); break;
	}
}
