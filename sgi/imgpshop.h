/* Structure we use to keep track of our global data. */
typedef struct GlobalData {
	FormatRecordPtr	stuff;	/* huge record that PhotoShop hands us */
	short result;		/* like an application errno */

	char unixname[1024];	/* filename of file to read */
	FILE *fp;		/* stdio handle for reading file */

	Widget xshell;		/* glop for X11 */
	XtAppContext context;	/* glop for X11 */
	Visual      *visual;

	struct imghdr	*h;	/* SGI-specific stuff in img.h */

	void *pixelData;	/* a one-scanline buffer */

	int32	done, total;	/* rows, for progress bar */

	/* Stuff from the image header */
	unsigned long darkest, lightest;
	short encoding;
	short channels;
	short pixsize;
	int tablen;
	unsigned long *starttab, *lengthtab, rleoffset, rlelength;

} Globals, *GPtr, **GHdl;	/* last is Mac-style "handle", sigh */

#define	gStuff		((**globals).stuff)
#define	gResult		((**globals).result)
#define	gUnixname	((**globals).unixname)
#define gXshell		((**globals).xshell)
#define gXvisual	((**globals).visual)
#define gXappcontext	((**globals).context)
#define	gFP		((**globals).fp)
#define	gH		((**globals).h)
#define XSIZE		gH->xsize
#define YSIZE		gH->ysize
#define ZSIZE		gH->zsize
#define gPixelData	((**globals).pixelData)
#define	gDone		((**globals).done)
#define	gTotal		((**globals).total)
#define	gDarkest	((**globals).darkest)
#define	gLightest	((**globals).lightest)
#define gEncoding	((**globals).encoding)
#define gPixsize	((**globals).pixsize)
#define gTablen		((**globals).tablen)
#define gStarttab	((**globals).starttab)
#define gLengthtab	((**globals).lengthtab)
#define gRleoffset	((**globals).rleoffset)
#define gRlelength;	((**globals).rlelength)
