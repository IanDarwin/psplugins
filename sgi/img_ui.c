/*
 * X User Interface for the Image Format plug-in.
 *
 *	Ian Darwin <ian@darwinsys.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#include <Xm/Xm.h>
#include <Xm/MwmUtil.h>
#include <Xm/DialogS.h>
#include <Xm/MessageB.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/Text.h>
#include <Xm/PushBG.h>
#include <Xm/LabelG.h>
#include <Xm/PanedW.h>

#include "PIFormat.h"
#include "PIUtilities.h"
#include "img.h"		/* format-specific stuff */
#include "imgpshop.h"		/* Photoshop interface */


/* Tell the user all about this PlugIn. */

/* Basic dialog information... */
#define	DEF_WID	500
#define	DEF_HT	200

#include "DOSlogo.xbm"		/* image data */

/* After the dialog is created, center it. */
static void
centre_dialog(Widget dlg, XtPointer clnt_data, XtPointer call_data)
{
	Dimension scrn_wid, scrn_ht, dlg_wid, dlg_ht, pref_x, pref_y;

	scrn_wid = WidthOfScreen(XtScreen(dlg));
	scrn_ht = HeightOfScreen(XtScreen(dlg));
	XtVaGetValues(dlg,
		XtNwidth,	&dlg_wid,
		XtNheight,	&dlg_ht,
		NULL);
	if (dlg_wid == 0) dlg_wid = DEF_WID;
	if (dlg_ht  == 0) dlg_ht  = DEF_HT;
	pref_x = (Dimension)(scrn_wid-dlg_wid) / 2;
	pref_y = (Dimension)(scrn_ht -dlg_ht ) / 2; 
	if ((Dimension)(pref_x + dlg_wid) > scrn_wid)
		pref_x = 0;
	if ((Dimension)(pref_y + dlg_ht) > scrn_ht)
		pref_y = 0;
	XtVaSetValues(dlg,
		XmNx,		pref_x,
		XmNy,		pref_y,
		NULL);
}

void
DoAbout(GHdl globals)
{
	static char *msg = 
		"SGI `Image' Format Import Module $Revision: 1.5 $\n\n"
		"Reads (does not yet write) Silicon Graphics \"Image\"\n"
		"format raster files. These files are usually created by\n"
		"SGI utilities such as the video grabber.\n\n"
		"Copyright (C) 1995 Darwin Open Systems,\n"
		"     Email: info@darwinsys.com\n"
		"     WWW: http://www.uunet.ca/darwinsys\n\n"
		"If this program is useful to you, please feel free to send "
		"$10.00 to\n"
		"Darwin Open Systems, R R #1, Palgrave, ON Canada L0N 1P0."
		;
	Widget dlg;
	XmString message, title;
	Pixmap pixmap;
	Pixel fg, bg;
	Arg args[5];
	Cardinal n = 0;

	/* Setup for Dialog creation. First, say "no decorations please" */
	XtSetArg(args[n], XmNmwmDecorations, MWM_DECOR_BORDER); n++;

	message = XmStringCreateLtoR(msg, XmFONTLIST_DEFAULT_TAG);
	XtSetArg(args[n], XmNmessageString, message); n++;

	/* Seems silly to have a title with MWM_DECOR_BORDER, but other
	 * WM's (like olwm!) tend to ignore the MWM_DECOR hints...
	 */
	title = XmStringCreateLocalized("About SGI Image Plug-In");
	XtSetArg(args[n], XmNdialogTitle, title); n++;

	/* Tell Motif not to paint the dialog until we say to, so we can get
	 * its size, and move it, before it appears on-screen. */
	XtSetArg(args[n], XmNmappedWhenManaged, False); n++;

	/* Use parent's visual, not default, to avoid XErrors */
	XtSetArg(args[n],XmNvisual, gXvisual); n++;

	/* Finally, dialog creation; */
	dlg = XmCreateInformationDialog(gXshell, "info", args, n);

	XmStringFree(message);
	XmStringFree(title);

	/* Make our fancy logo into a pixmap */
	XtVaGetValues(dlg,
		XmNforeground, &fg,
		XmNbackground, &bg,
		NULL);
	pixmap = XCreatePixmapFromBitmapData(XtDisplay(dlg),
		RootWindowOfScreen(XtScreen(dlg)),
		(char*)DOSlogo_bits, DOSlogo_width, DOSlogo_height,
		fg, bg, DefaultDepthOfScreen(XtScreen(dlg)));

	XtVaSetValues(XmMessageBoxGetChild(dlg, XmDIALOG_SYMBOL_LABEL),
		XmNlabelPixmap,      pixmap,
		NULL);

	/* Kill off Cancel and Help buttons, leaving just OK button. */
	XtUnmanageChild(XmMessageBoxGetChild(dlg, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(dlg, XmDIALOG_HELP_BUTTON));

	/* Cause dlg to be realized, and our Map callback invoked */
	XtManageChild(dlg);

	centre_dialog(XtParent(dlg), 0, 0);	/* or just dlg?? */

	/* OK, we've tried to center it, now make it visible */
	XtVaSetValues(dlg, XmNmappedWhenManaged, True, NULL);

	/* User may run other apps, don't do server grab.  Just put up dialog */
	XtPopup(XtParent(dlg), XtGrabNone);

	/* Is all. Dialog will dismiss itself when user clicks OK. */
}

#ifdef	TEST_MAIN
/*
 * Test main driver, for X UI dialog hacking.
 */
main(argc, argv)
int argc;
char *argv[];
{
	XtAppContext app;
	Widget rc, main_w, menubar, w;
	extern void help_cb(), file_cb();
	XmString str1, str2, str3;
	Widget *cascade_btns;
	int num_btns;
	char *data;
	GHdl globals;

	data = (char*)malloc(sizeof(Globals));
	globals = (GHdl) &data;

	XtSetLanguageProc (NULL, NULL, NULL);

	gXshell = XtVaAppInitialize(&app, "Demos", NULL, 0,
		&argc, argv, NULL, NULL);
	XtVaSetValues(gXshell,
		XmNwidth,	100,
		XmNheight, 	100,
		NULL);

	XtRealizeWidget(gXshell);

	DoAbout(globals);

	XtAppMainLoop(app);
}
#endif
