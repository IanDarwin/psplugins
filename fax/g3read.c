/* g3read.c - read a Group 3 FAX file 
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

#define	BIT_SETSIZE	MAXCOLS
#include "bitset.h"

int endoffile;
static int eols = 0;
static int rawzeros;
static int shdata;
static char msgbuf[256];

int reversebits = 0;	/* shared w/ g3write */

tableentry* whash[HASHSIZE];
tableentry* bhash[HASHSIZE];

void addtohash ARGS(( tableentry* hash[], tableentry* te, int n, int a, int b ));
static tableentry* hashfind ARGS(( tableentry* hash[], int length, int code, int a, int b ));
int getfaxrow ARGS(( FILE* inf, int row, bit_set bitrow ));
void skiptoeol ARGS(( FILE* file ));
static int rawgetbit ARGS(( FILE* file ));

void g3r_init()
{
	eols = endoffile = 0;
}

void
addtohash(hash, te, n, a, b)
	tableentry* hash[];
	tableentry* te;
	int n, a, b;
{
	unsigned int pos;

	while (n--) {
		pos = ((te->length+a)*(te->code+b))%HASHSIZE;
		if (hash[pos] != 0)
			pm_error( "internal error: addtohash fatal hash collision" );
		hash[pos] = te;
		te++;
	}
}

static tableentry*
hashfind(hash, length, code, a, b)
    tableentry* hash[];
    int length, code;
    int a, b;
    {
    unsigned int pos;
    tableentry* te;

    pos = ((length+a)*(code+b))%HASHSIZE;
    if (pos < 0 || pos >= HASHSIZE)
	pm_error(
	    "internal error: bad hash position, length %d code %d pos %d",
	    length, code, pos );
    te = hash[pos];
    return ((te && te->length == length && te->code == code) ? te : 0);
    }

int
getfaxrow(FILE *inf, int row, bit_set bitrow )
{
	int col;
	bit* bP;
	int curlen, curcode, nextbit;
	int count, color;
	tableentry* te;

	BIT_ZERO(&bitrow);
	col = 0;
	rawzeros = 0;
	curlen = 0;
	curcode = 0;
	color = 1;
	count = 0;
	while (!endoffile) {
		if (col >= MAXCOLS) {
			skiptoeol(inf);
			return (col); 
		}
		do {
			if (rawzeros >= 11) {
				nextbit = rawgetbit(inf);
				if (nextbit) {
					if (col == 0)
						/* XXX should be 6 */
						endoffile = (++eols == 3);
					else
						eols = 0;
#ifdef notdef
					if (col && col < MAXCOLS)
						pm_message(
					       "warning, row %d short (len %d)",
						    row, col );
#endif /*notdef*/
					return (col); 
				}
			} else
				nextbit = rawgetbit(inf);
			curcode = (curcode<<1) + nextbit;
			curlen++;
		} while (curcode <= 0);
		if (curlen > 13) {
			sprintf(msgbuf,
	  "bad code word at row %d, col %d (len %d code 0x%x), skipping to EOL",
			    row, col, curlen, curcode, 0 );
			pm_message(msgbuf);
			skiptoeol(inf);
			return (col);
		}
		if (color) {
			if (curlen < 4)
				continue;
			te = hashfind(whash, curlen, curcode, WHASHA, WHASHB);
		} else {
			if (curlen < 2)
				continue;
			te = hashfind(bhash, curlen, curcode, BHASHA, BHASHB);
		}
		if (!te)
			continue;
		switch (te->tabid) {
		case TWTABLE:
		case TBTABLE:
			count += te->count;
			if (col+count > MAXCOLS) 
				count = MAXCOLS-col;
			if (count > 0) {
				if (color) {
					col += count;
					count = 0;
				} else {
					for ( ; count > 0; --count, ++col )
						BIT_SET(col, &bitrow);
				}
			}
			curcode = 0;
			curlen = 0;
			color = !color;
			break;
		case MWTABLE:
		case MBTABLE:
			count += te->count;
			curcode = 0;
			curlen = 0;
			break;
		case EXTABLE:
			count += te->count;
			curcode = 0;
			curlen = 0;
			break;
		default:
			pm_error("internal error - invalid table type" );
		}
	}
	return (0);
}

void
skiptoeol( file )
    FILE* file;
    {
    while ( rawzeros < 11 )
	(void) rawgetbit( file );
    for ( ; ; )
	{
	if ( rawgetbit( file ) )
	    break;
	}
    }

static int shbit = 0;

static int
rawgetbit( file )
    FILE* file;
    {
    int b;

    if ( ( shbit & 0xff ) == 0 )
	{
	shdata = getc( file );
	if ( shdata == EOF ) {
	    sprintf( msgbuf, "EOF / read error at line %d", eols );
	    pm_error(msgbuf);
	}
	shbit = reversebits ? 0x01 : 0x80;
	}
    if ( shdata & shbit )
	{
	rawzeros = 0;
	b = 1;
	}
    else
	{
	rawzeros++;
	b = 0;
	}
    if ( reversebits )
	shbit <<= 1;
    else
	shbit >>= 1;
    return b;
    }
