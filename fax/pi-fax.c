/*
 * pi-fax - a G3 FAX Format PlugIn for the Adobe PhotoShop program.
 *
 * $Id: pi-fax.c,v 1.43 96/04/14 13:02:43 ian Exp $
 *
 * Copyright (c) 1995 Ian F. Darwin, but may be copied under the terms
 * of the accompanying LEGAL.NOTICE file (not the GNU Public License).
 * May be redistributed under the GPL if accompanied by notification
 * that a non-GPL'd version is available from the author.
 *
 * LIMITATIONS:
 *	1) This version has only been built on UNIX with X Windows,
 *	   not for the Macintosh window system nor for MS-Windows.
 *	   The major change would be the About box, but everything
 *	   would need to be vetted a little, esp. the memory allocation.
 *
 * If you make any enhancements, or port to any other platforms,
 * please send the changes back to me for incorporation in future versions. 
 *	Thanks!
 *	Ian Darwin <ian@darwinsys.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#include <Xm/Xm.h>		/* minimal Motif header for XtVaGetValues() */

#define	MAIN
 
#include "PIFormat.h"
#include "PIUtilities.h"
#include "g3.h"			/* format-specific stuff */
#include "pi-fax.h"		/* Program general interface */
#include "routines.h"		/* externs for DoXXX routines */

static char imsgbuf[1024];	/* for messages to GUI dialog. */
static int doneHashes = 0;

void init_hash()
{
	int i;
	extern tableentry* whash[];
	extern tableentry* bhash[];

	/* fprintf(stderr, "In init_hash() routine\n"); */

	for ( i = 0; i < HASHSIZE; ++i)
		whash[i] = bhash[i] = (tableentry*) 0;
	addtohash( whash, twtable, TABSIZE(twtable), WHASHA, WHASHB);
	addtohash( whash, mwtable, TABSIZE(mwtable), WHASHA, WHASHB);
	addtohash( whash, extable, TABSIZE(extable), WHASHA, WHASHB);
	addtohash( bhash, tbtable, TABSIZE(tbtable), BHASHA, BHASHB);
	addtohash( bhash, mbtable, TABSIZE(mbtable), BHASHA, BHASHB);
	addtohash( bhash, extable, TABSIZE(extable), BHASHA, BHASHB);

	doneHashes = 1;
}

/* Dispatch routine. Called from PhotoShop to ask us to do anything. */
static GHdl pvt_globals;	/* static so pm_error() can see it */
void
main_entry(short cmd, FormatRecordPtr stuff, long *data, short *result)
{
#define	globals	pvt_globals	/* only in this routine */

	if (!*data) {		/* first time through here */
		if ((*data = (long)NewHandle(sizeof(Globals))) == NULL) {
			*result = memFullErr;
			return;
		}
		memset((void*)*data, sizeof(Globals), 0);

	}
	/* on subsequent calls, PhotoShop will remember and pass this */
	globals = (GHdl) *data;

	if (!doneHashes)
		init_hash();

	/* Good time to initialize the X goo */
	if (!(gXshell = PSGetTopShell())) {
		fprintf(stderr, "imgpshop: can't get Top Shell\n");
		*result = argErr;
	}
	XtVaGetValues(gXshell, XmNvisual, &gXvisual, NULL);
	if (!&gXvisual) {
		fprintf(stderr, "img: can't get Top Shell's Visual\n");
		*result = argErr;
	}
	if (!(gXappcontext = PSGetAppContext())) {
		fprintf(stderr, "imgpshop: can't get App Context\n");
		*result = argErr;
	}

	/* Dispatch one request */
	gStuff = stuff;
	gResult = noErr;

	switch(cmd) {
	case formatSelectorAbout:
		DoAbout(globals);
		break;
	case formatSelectorReadPrepare:
		DoReadPrepare(globals);
		break;
	case formatSelectorReadStart:
		DoReadStart(globals);
		break;
	case formatSelectorReadContinue:
		DoReadContinue(globals);
		break;
	case formatSelectorReadFinish:
		DoReadFinish(globals);
		break;
	case formatSelectorOptionsPrepare:
	case formatSelectorOptionsStart:
	case formatSelectorOptionsFinish:
		gResult = noErr;
		break;
	case formatSelectorEstimatePrepare:
		DoEstimatePrepare(globals);
		break;
	case formatSelectorEstimateStart:
		DoEstimateStart(globals);
		break;
	case formatSelectorEstimateContinue:
		DoEstimateContinue(globals);
		break;
	case formatSelectorEstimateFinish:
		DoEstimateFinish(globals);
		break;
	case formatSelectorWritePrepare:
		DoWritePrepare(globals);
		break;
	case formatSelectorWriteStart:
		DoWriteStart(globals);
		break;
	case formatSelectorWriteContinue:
		DoWriteContinue(globals);
		break;
	case formatSelectorWriteFinish:
		DoWriteFinish(globals);
		break;
	default:
		gResult = formatBadParameters;
		break;
	}

	*result = gResult;
}

void
pm_message(char *s)
{
	fprintf(stderr, "%s\n", s);
}
void
pm_error(char *s)
{
	pm_message(s);
	gResult = argErr;
#undef	globals
}

/* 
 * Reading routines
 * DoReadPrepare just opens the file and reads its header.
 * DoReadStart analyses the header, sets things up, reads first row.
 * DoReadContinue is called repeatedly to read one row.
 * DoReadFinish is called at the end to clean up.
 */
static void
DoReadPrepare(GHdl globals)
{
	extern void g3r_init();

	gStuff->maxData = 0;	/* tell PShop we won't use their buffers */

	g3r_init(gRevBits);
}

static void
DoReadStart(GHdl globals)
{

	Mac2UnixPath(gStuff->fileSpec, gUnixname);
	fprintf(stderr, "File \"%s\"\n", gUnixname);
	if ((gFP = fopen(gUnixname, "r")) == NULL) {
		perror(gUnixname);
		gResult = argErr;
		return;
	}

	/* Certain variables must be set in gStuff:
	 *	loPlane, hiPlane - first and last planes in buffer
	 *		(0, 0 for grayscale; 0, 2 for interleaved RGB)
	 *	rowBytes - offset in bytes between rows of data
	 *	planeBytes - offset in bytes between planes/channels of data
	 *		(ignored if gstuff's loPlane == hiPlane)
	 *		(set to 1 if interleaved RGB; "img" data NOT!)
	 *	colBytes - offset in bytes between columns of data
	 *		(1 if non-interleaved, else hiPlane-loPlane+1)
	 *	planes - PShop's word for "channels" - 1 == grayscale, 3 == rgb
	 */
	gStuff->imageMode = plugInModeBitmap;
	gStuff->imageSize.h = MAXCOLS;
	gStuff->imageSize.v = MAXROWS;
	gStuff->depth = 1;		/* bitmap */
	gStuff->planes = 1;		/* bitmap */
	gStuff->imageHRes = 200;
	gStuff->imageVRes = 200;
	gStuff->colBytes = 1;
	gStuff->rowBytes = (MAXCOLS/8)+1;
	gStuff->planeBytes = 0;


	/* The PlugIn API wants us to use their organization of flow,
	 * in which we have to read one row on each call. Hence, we here
	 * set up to read the file as row-sized chunks of data,
	 * keeping track of where we are using fields in "gStuff".
	 */
	gStuff->loPlane = 0;
	gStuff->hiPlane = 0;
	/* Their use of "top" and "bottom" is topsy-turvy. */
#define	FIRST_TOP 0
#define FIRST_BOT 1
	gStuff->theRect.top = FIRST_TOP;
	gStuff->theRect.bottom = FIRST_BOT;
	gStuff->theRect.left = 0;
	gStuff->theRect.right = MAXCOLS;

	gStuff->data = gPD;

	/* Rest of this function is G3-specific */

	/* Options to control g3read/g3write */
	gKludge = 0;
	gRevBits = 0;

	if ( gKludge) { /* Skip extra lines to get in sync. */
		skiptoeol( gFP);
		skiptoeol( gFP);
		skiptoeol( gFP);
	}
	skiptoeol( gFP);

	gRow = gCols = 0;
	
}

#if	0
/* Use this if, while whacking, you need to see how bits
 * actually make it into the output array.
 */
int
MYgetfaxrow( FILE *F, int gR, char *gP)
{
	int i;
	extern int endoffile;
	for (i=0; i<200; i++)
		gP[i] = 0;
	for (i=10; i< 16; i++)
		gP[i] = i;
	if (gR >= 200)
		endoffile = 1;
}
#endif

/*
 * Need to keep rectangle "theRect" aimed at the part of the image
 * we're about to read in - one scanline at a time, in our case.
 */
static void
DoReadContinue(GHdl globals)
{
	unsigned int col;
	extern int endoffile;

	/* Read one row of one plane, and bomb out if it failed. */
	gResult = noErr;
	col = getfaxrow(gFP, gRow, gStuff->data);

	if (endoffile || ferror(gFP) || feof(gFP)) {
		fprintf(stderr, "Read EOF after %d rows.\n", gRow);
		gStuff->imageSize.v = gRow;	/* can we change it here? */
		gStuff->data = NULL;
		return;
	}

	if (col < MAXCOLS && col > gCols)
	    gCols = col;

	if (gResult != noErr) {
		gStuff->data = NULL;
		return;
	}

	UpdateProgress(++gRow, MAXROWS);

	/* advance the rectangle. */
	gStuff->theRect.top++;
	gStuff->theRect.bottom++;

	/* Did we hit the bottom of the rectangle? */
	if (gStuff->theRect.top >= MAXROWS) {
		gStuff->data = NULL;
		return;
	}
}


static void
DoReadFinish(GHdl globals)
{
	(void) fclose(gFP);
	gResult = noErr;
	return;
}

static void
DoEstimatePrepare(GHdl globals)
{
	gStuff->maxData = 0;
}

static void
DoEstimateStart(GHdl globals)
{
	int dataBytes;
    
	/* Bitmap files aren't very big */
	dataBytes = MAXROWS * MAXCOLS / 8;

	gStuff->minDataBytes = dataBytes;
	gStuff->maxDataBytes = dataBytes;

	gStuff->data = NULL;
}

static void
DoEstimateContinue(GHdl globals)
{
	/*NULLBODY*/
}

static void
DoEstimateFinish(GHdl globals)
{
	/*NULLBODY*/
}

static void
DoWritePrepare(GHdl globals)
{
	struct stat statbuf;
	extern void g3w_init();

	if (gStuff->imageMode != plugInModeBitmap) {
		gResult = argErr;
		return;
	}

	gStuff->data = gPD;

	gStuff->maxData = 0;
}

static void
DoWriteStart(GHdl globals)
{
	Mac2UnixPath(gStuff->fileSpec, gUnixname);
	gFP = fopen(gUnixname, "w");

	if (gFP == NULL) {
		char msgbuf[128];
		sprintf(msgbuf, "WritePrepare: %s:", gUnixname);
		perror(msgbuf);
		gResult = argErr;
	}

	gStuff->data = gPD;
	gStuff->theRect.top = FIRST_TOP;
	gStuff->theRect.bottom = FIRST_BOT;
	gStuff->theRect.left = 0;
	gStuff->theRect.right = MAXCOLS;
	gStuff->rowBytes = 1;
	gStuff->colBytes = (gStuff->imageSize.h/8) + 1;
}

static void
DoWriteContinue(GHdl globals)
{
	toFax(gStuff->data, gStuff->imageSize.h, gFP);

	++gStuff->theRect.top;
	++gStuff->theRect.bottom;

	UpdateProgress(gStuff->theRect.top, gStuff->imageSize.v);

	/* If all done, say so (clobber the buffer pointer) */
	if (gStuff->theRect.top >= gStuff->imageSize.v) 
		gStuff->data = NULL;
}

static void
DoWriteFinish(GHdl globals)
{
	int i;

	/* Finish it off. */
	for( i = 0; i < 3; ++i )
		puteol(gFP);
	flushbits(gFP);

	fclose(gFP);
}
