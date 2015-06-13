/* Structure we use to keep track of our global data. */
typedef struct GlobalData {
	FormatRecordPtr	stuff;	/* huge record that PhotoShop hands us */
	short result;		/* like an application errno */

	char unixname[1024];	/* filename of file to read */
	FILE *fp;		/* stdio handle for reading file */

	Widget xshell;		/* glop for X11 */
	XtAppContext context;	/* glop for X11 */
	Visual      *visual;

	int kludge,		/* Option: skip a few top rows */
		reversebits;	/* useless: use Image->Invert */

	int row, cols;		/* Row we're on, columns in it. */

	char  pd[(MAXROWS/8)+1]; /* it's really allocated here,
				 * since it can never be ~250 bytes
				 */

} Globals, *GPtr, **GHdl;	/* last is Mac-style "handle", sigh */

#define	gStuff		((**globals).stuff)
#define	gResult		((**globals).result)
#define	gUnixname	((**globals).unixname)
#define gXshell		((**globals).xshell)
#define gXvisual	((**globals).visual)
#define gXappcontext	((**globals).context)
#define	gFP		((**globals).fp)
#define gKludge		((**globals).kludge)
#define gRevBits	((**globals).reversebits)
#define gRow		((**globals).row)
#define gCols		((**globals).cols)
#define	gPD		((**globals).pd)
