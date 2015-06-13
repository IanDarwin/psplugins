/*
 * imgpshop - an SGI "Image" Format PlugIn for the Adobe PhotoShop program.
 *
 * $Id: imgpshop.c,v 1.33 96/02/02 09:12:32 ian Exp $
 *
 * Copyright (c) 1995 Ian F. Darwin, but may be copied under the terms
 * of the accompanying LEGAL.NOTICE file (not the GNU Public Virus).
 * May be redistributed under the GPL if accompanied by notification
 * that a non-GPL'd version is available from the author.
 *
 * LIMITATIONS:
 *	1) This version reads Image files, but does not write them.
 *	2) This version has only been built on UNIX with X Windows,
 *	   not for the Macintosh window system nor for MS-Windows.
 *	   Not that you're likely to find SGI Image files, there, but...
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
 
#include "PIFormat.h"
#include "PIUtilities.h"
#include "img.h"		/* format-specific stuff */
#include "imgpshop.h"		/* Photoshop interface */

/* Since this is a read-only module, we don't handle, and don't provide,
 * Options, Estimate*, or Write* routines.
 */
extern void DoAbout(GHdl globals);
static void DoReadPrepare(GHdl globals);
static void DoReadStart(GHdl globals);
static void DoReadContinue(GHdl globals);
static void DoReadFinish(GHdl globals);

static char imsgbuf[1024];	/* for messages to GUI dialog. */

/* Dispatch routine. Called from PhotoShop to ask us to do anything. */
void
main_entry(short cmd, FormatRecordPtr stuff, long *data, short *result)
{
	GHdl globals;

	if (!*data) {		/* first time through here */
		if ((*data = (long)NewHandle(sizeof(Globals))) == NULL) {
			*result = memFullErr;
			return;
		}
		memset((void*)*data, sizeof(Globals), 0);
	}
	globals = (GHdl) *data;	/* on subsequent calls, PhotoShop will have
				 * cached this value and will provide it */

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
	case formatSelectorEstimateStart:
	case formatSelectorEstimateContinue:
	case formatSelectorEstimateFinish:
	case formatSelectorWritePrepare:
	case formatSelectorWriteStart:
	case formatSelectorWriteContinue:
	case formatSelectorWriteFinish:
	default:
		gResult = formatBadParameters;
		break;
	}

	*result = gResult;
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

	Mac2UnixPath(gStuff->fileSpec, gUnixname);
	if ((gFP = fopen(gUnixname, "r")) == NULL) {
		perror(gUnixname);
		gResult = argErr;
	}

	if ((gH = readhdr(gFP)) == 0) {
		fprintf(stderr, "%s: bad IMAGE header\n", gUnixname);
		gResult = argErr;
	}

	if (gH->magic != IMGMAGIC) {
		gResult = openErr;
		return;
	}

#if	0
	dumphdr(gH);
#endif

	gStuff->imageSize.h = XSIZE;
	gStuff->imageSize.v = YSIZE;
	gStuff->maxData = 0;	/* tell PShop we won't use their buffers */

}

/* Read an RLE table */
static void
readtab(FILE *inf, unsigned long *tab, int len)
{
    while(len) {
	*tab++ = getlong(inf);
	len -= 4;
    }
}


#define	mismatch(tag, val, exp) {\
	fprintf(stderr, "%s: invalid %s (%d), expecting %s\n", tag, val, exp);\
	gResult = openErr; \
	return;\
	}

static void
DoReadStart(GHdl globals)
{
	long pix;

	fprintf(stderr, "File \"%s\", Image name \"%s\", size %dx%dx%d\n",
		gUnixname, gH->imgname, XSIZE, YSIZE, ZSIZE);

	if (gH->storage == VERBATIM) gEncoding = VERBATIM;
	else if (gH->storage == RLE) gEncoding = RLE;
	else mismatch("Storage encoding", gH->storage, "VERBATIM or RLE");

	if (gH->bpc == BPPsingle) gPixsize = BPPsingle;
	else if (gH->bpc == BPPdouble) gPixsize = BPPdouble;
	else mismatch("Pixel Size", gH->bpc, "1 or 2");

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

	switch(gH->dimension) {
	case degenerate:
		/* UNSUPPORTED - does anybody really use this format?? */
		fprintf(stderr, "\"Line bitmap\" format not supported\n");
		gResult = argErr;
		return;
	case grayscale:
		gStuff->imageMode = plugInModeGrayScale;
		if (ZSIZE != 1)
			mismatch("image is grayscale but ZSIZE", ZSIZE, "2");
		break;
	case dcolor:
		gStuff->imageMode = plugInModeRGBColor;
		if (!(ZSIZE == 3 || ZSIZE == 4))
			mismatch("image is color but zsize", ZSIZE, "3 or 4");
		break;
	default:
		mismatch("Dimensionality", gH->dimension, "2 or 3");
		break;
	}

	gStuff->planes = ZSIZE;			/* 1, 3, or 4 */
	gStuff->colBytes = 1;
	gStuff->rowBytes = XSIZE;		/* TODO * gPixsize */
	gStuff->planeBytes = 0;
	gStuff->depth = gPixsize * 8;		/* either 8 or 16 */

	gDarkest = gH->pixmin;
	gLightest = gH->pixmax;

	switch(gH->cmap_type) {
	case NORMAL:
		break;
	case DITHERED_OBSOLETE:
	case SCREEN_OBSOLETE:
		fprintf(stderr, "Obsolete image format not supported\n");
		gResult = argErr;
		return;
	case SGI_CMAP:
		fprintf(stderr, "Image is an SGI colormap, not an image\n");
		gResult = argErr;
		return;
		break;
	default:
		mismatch("Colormap Type", gH->cmap_type, "Normal (0)");
		break;
	}

	/* Set up the progress variables. */
	gDone = 0;
	gTotal = YSIZE;

	/* Read the RLE encoding tables, if present */
	/* fprintf(stderr, "About to read RLE tables\n"); */
	if (fseek(gFP, 512, SEEK_SET) < 0) {
		printf("seek past header failed, get help!\n");
		gResult = argErr;
		return;
	}
	switch(gEncoding) {
	case VERBATIM:
		/* fprintf(stderr, "No RLE tables to read\n"); */
		break;
	case RLE:
		gTablen = YSIZE*ZSIZE*4 /* not sizeof(long) - always 4 */;
		gStarttab = (unsigned long *) malloc(gTablen);
		gLengthtab = (unsigned long *) malloc(gTablen);
		readtab(gFP, gStarttab,  gTablen);
		readtab(gFP, gLengthtab, gTablen);
		/* fprintf(stderr, "Successfully read the RLE tables!\n"); */
		break;
	default:
		printf("Logic error: encoding set to %d\n", gEncoding);
		break;
	}

	/* The PlugIn API wants us to use their organization of flow,
	 * in which we have to read one row on each call. Hence, we here
	 * set up to read the file as row-sized chunks of data,
	 * keeping track of where we are using fields in "gStuff".
	 */
	gStuff->loPlane = 0;
	gStuff->hiPlane = 0;
	/* Their use of "top" and "bottom" is topsy-turvy. */
#define	FIRST_TOP (YSIZE - 1)
#define FIRST_BOT (YSIZE    )
	gStuff->theRect.top = FIRST_TOP;
	gStuff->theRect.bottom = FIRST_BOT;
	gStuff->theRect.left = 0;
	gStuff->theRect.right = XSIZE;

	/* Allocate a pixel buffer that will hold the bytes
	 * for one plane of data, for one scanline.
	 */
	if (!(gPixelData = malloc(XSIZE))) {
		gResult = memFullErr;
		return;
	}

	gStuff->data = gPixelData;

}

/*
 * Need to keep rectangle "theRect" aimed at the part of the image
 * we're about to read in - one scanline at a time, in our case.
 */
static void
DoReadContinue(GHdl globals)
{
	unsigned int col;
	unsigned char *obuf = gPixelData;	/* TODO for 16bit images */

	/* Read one row of one plane, and bomb out if it failed. */
	gResult = noErr;
	switch (gEncoding) {
	case VERBATIM:
		/* Get the line one pixel at a time directly */
		for (col=0; col<XSIZE; col++) {
			*obuf++ = getc(gFP);
		}
		break;
	case RLE:
		/* Get the RLE info for this scanline, and uncompress */
#define		rowno		gStuff->theRect.bottom
#define		channo 		gStuff->loPlane
		gRleoffset = gStarttab[(YSIZE-rowno)+channo*YSIZE];
		gRlelength = gLengthtab[(YSIZE-rowno)+channo*YSIZE];

		/* printf("Fseek %ld\n", gRleoffset); */
		if (fseek(gFP, gRleoffset, SEEK_SET) < 0) {
			printf("fseek(gFP, %d, 0) failed, get help!\n",
				gRleoffset);
			gResult = argErr;
			return;
		}
		expandrow(obuf, gFP, gPixsize);
		obuf += XSIZE;
		break;
	default:
		fprintf(stderr, "ReadContinue: Unknown gEncoding %d\n",
			gEncoding);
		gResult = argErr;
		break;
	}
	if (gResult != noErr) {
		gPixelData = NULL;
		return;
	}
	if (ferror(gFP) || feof(gFP)) {
		fprintf(stderr, "Read error in middle of file!\n");
		gStuff->data = NULL;
		gResult = argErr;
		return;
	}

	UpdateProgress(++gDone, gTotal);

	/* slide the rectangle down. */
	gStuff->theRect.top--;
	gStuff->theRect.bottom--;

	/* Did we hit the bottom of the rectangle? */
	if (gStuff->theRect.top+1 < 0) {
		/* try for another plane, else we are all done.  */
		++gStuff->loPlane;
		++gStuff->hiPlane;
		if (gStuff->loPlane >= gStuff->planes) {
			gStuff->data = NULL;
			return;
		}
		gStuff->theRect.top = FIRST_TOP;
		gStuff->theRect.bottom = FIRST_BOT;
	}
}




static void
DoReadFinish(GHdl globals)
{
	if (gPixelData) {
		free(gPixelData);
		gPixelData = 0;
	}

	if (gEncoding == RLE) {
		if (gStarttab) 
			free(gStarttab);
		gStarttab = 0;
		if (gLengthtab)
			free(gLengthtab);
		gLengthtab = 0;
	}
	gResult = noErr;
	return;
}

