/*
 * Some routines for reading parts of an SGI Image Format File.
 * Taken verbatim from:
 * Draft version  0.97
 * The SGI Image File Format
 * Paul Haeberli
 * paul@sgi.com
 * Silicon Graphics Computer Systems
 *
 * That standard was published on the Internet (ftp://ftp.sgi.com/graphics)
 * with no explicit copyright notice, so is presumably Copyright by SGI.
 * Permission to use is implicit in its publication as part of a standard
 * and in its publication by anonymous FTP with no other copyright notice.
 *
 * expandrow() was changed to read from a FILE* instead of walking through core.
 */

#include <stdio.h>

#include "img.h"

unsigned short	getshort(FILE *inf)
{
	unsigned char	buf[2];

	fread(buf, 2, 1, inf);
	return (buf[0] << 8) + (buf[1] << 0);
}


unsigned long getlong(FILE *inf)
{
	unsigned char	buf[4];

	fread(buf, 4, 1, inf);
	return (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + (buf[3] << 0);
}



void
expandrow(unsigned char *optr, FILE *ifp, int byteorshort)
{
	unsigned char	pixel, count;

#define NEXTPIXEL byteorshort == BPPsingle ? getc(ifp) : getshort(ifp);

	while (1) {
		pixel = NEXTPIXEL;
		if ( !(count = (pixel & 0x7f)) )
			return;
		if (pixel & 0x80) {		/* run of pixels */
			while (count--)
				*optr++ = NEXTPIXEL;
		} else {
			pixel = NEXTPIXEL;
			while (count--) {	/* replicated pixel */
				*optr++ = pixel;
			}
		}
	}
}
