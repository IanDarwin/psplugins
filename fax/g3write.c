/* g3write.c - write a Group 3 FAX file
**
** Copyright (C) 1989 by Paul Haeberli <paul@manray.sgi.com>.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include <stdio.h>
#include "g3.h"
#include "bitset.h"

       void toFax ARGS((bit_set bitrow, int n, FILE *fp));
static void putwhitespan ARGS((FILE *fp, int c));
static void putblackspan ARGS((FILE *fp, int c));
static void putcode ARGS((FILE *fp, tableentry* te));
       void puteol ARGS((FILE *fp));
static void putinit ARGS((void));
static void putbit ARGS((FILE *fp, int d));
       void flushbits ARGS((FILE *fp));

extern int reversebits;

void
toFax(bit_set bitrow, int n, FILE *fp)
{
    int c, i = 0;

    while(n>0) {
	c = 0;
	while(!BIT_ISSET(i, &bitrow) && n>0) {	
	    ++i;
	    ++c;
	    --n;
	}
	putwhitespan(fp, c);
	c = 0;
	if(n==0)
	    break;
	while(BIT_ISSET(i, &bitrow) && n>0) {	
	    ++i;
	    ++c;
	    --n;
	}
	putblackspan(fp, c);
    }
    puteol(fp);
}

static void
putwhitespan(FILE *fp, int c)
{
    int tpos;
    tableentry* te;

    if(c>=64) {
	tpos = (c/64)-1;
	te = mwtable+tpos;
	c -= te->count;
	putcode(fp, te);
    }
    tpos = c;
    te = twtable+tpos;
    putcode(fp, te);
}

static void
putblackspan(FILE *fp, int c)
{
    int tpos;
    tableentry* te;

    if(c>=64) {
	tpos = (c/64)-1;
	te = mbtable+tpos;
	c -= te->count;
	putcode(fp, te);
    }
    tpos = c;
    te = tbtable+tpos;
    putcode(fp, te);
}

static void
putcode(FILE *fp, tableentry* te)
{
    unsigned int mask;
    int code;

    mask = 1<<(te->length-1);
    code = te->code;
    while(mask) {
 	if(code&mask)
	    putbit(fp, 1);
	else
	    putbit(fp, 0);
	mask >>= 1;
    }

}

void
puteol(FILE *fp)
{
    int i;

    for(i=0; i<11; ++i)
	putbit(fp, 0);
    putbit(fp, 1);
}

static int shdata;
static int shbit;

static void
putinit()
{
    shdata = 0;
    shbit = reversebits ? 0x01 : 0x80;
}

static void
putbit(FILE *fp, int d)
{
    if(d) 
	shdata = shdata|shbit;
    if ( reversebits )
	shbit = shbit<<1;
    else
	shbit = shbit>>1;
    if((shbit&0xff) == 0) {
	fputc(shdata, fp);
	shdata = 0;
	shbit = reversebits ? 0x01 : 0x80;
    }
}

void
flushbits(FILE *fp)
{
    if ( ( reversebits && shbit != 0x01 ) ||
	 ( ! reversebits && shbit != 0x80 ) ) {
	fputc(shdata, fp);
	shdata = 0;
	shbit = reversebits ? 0x01 : 0x80;
    }
}
