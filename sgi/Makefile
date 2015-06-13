# Makefile for "img", a reader for SGI image files.
# Builds both the PNM converter and the Photoshop plugin by default.
# $Id: Makefile,v 1.3 96/03/16 23:34:42 ian Exp $

MAC_NAME	= SGI Image
PLUGINID	= 68456

IMGTOPNM_SRC	= imgtopnm.c readhdr.c imgutil.c
IMGTOPNM_OBJ	= imgtopnm.o readhdr.o imgutil.o
IMGPSHOP_OBJ	= imgpshop.o readhdrp.o imgutilp.o img_ui.o

#CC		= gcc -ansi
CC		= cc -Xc

# Where is the Photoshop_SDK stuff installed?
SDK_DIR		= ../photoshop_sdk

PSHOP_INC	= -I$(SDK_DIR)/PlugInSDK/include 
#XM_INC		= -I/usr/openwin/include -I/usr/dt/include
XM_INC		= -I/usr/dt/include -I/usr/openwin/include

PIC		= -K pic
CFLAGS		= -g -DUNIX
CFLAGS_PS	= $(CFLAGS) $(PSHOP_INC) $(XM_INC) $(PIC)
LINT		= lint		# for dumb makes that don't know lint

#DEV		= /dev/rfd0
DEV		= /vol/dev/aliases/floppy0

SRC_DIST	= LEGAL_NOTICE BUGS sgi-img-readme.html \
		DOSlogo.xbm Makefile SGIIMAGESPEC \
		fastimg.c img.h img_ui.c imgpshop.c imgpshop.h \
		imgtopnm.c imgutil.c readhdr.c sgiw.c
BIN_DIST	= LEGAL_NOTICE %sgi-curves sgi-curves BUGS \
		sgi-img-readme.html

all:		imgpshop # imgtopnm 

test:		imgtopnm lee.rgb
		./imgtopnm lee.rgb>lee.pnm
		xv lee.pnm

imgpshop:	$(IMGPSHOP_OBJ)
		ld -Bsymbolic -o $@ -r $(IMGPSHOP_OBJ)
#		You must do "make resource" once.
resource:	imgpshop MakePlugIn
		rm -f $(MAC_NAME) %$(MAC_NAME)
		@echo "Use $(MAC_NAME) as the Plug-In Name *and* filename!"
		@echo "Use $(PLUGINID) as the ID, 1 as the Major Version"
		@echo "Use these Image Format Parameters:"
		@echo "Type Count 0"
		@echo "Ext Count 4"
		@echo "Flags Can read Can write"
		@echo "Type list: rgb rgba bw img"
		./MakePlugIn
		ln imgpshop $(MAC_NAME)
# The MakePlugIn program, from the SDK directory, is used to make up
# the "resource fork" for the Plug-in. Required! See the docco.
MakePlugIn:
		ln -s $(SDK_DIR)/MakePlugIn/Photoshop.MacFiles/MakePlugIn .

imgpshop.o:	imgpshop.c
		$(CC) $(CFLAGS_PS) -c imgpshop.c
img_ui.o:	img_ui.c
		$(CC) $(CFLAGS_PS) $(PSHOP_INC) $(XM_INC) -c img_ui.c
readhdrp.o:	readhdr.c
		$(CC) $(CFLAGS) -c -o readhdrp.o $(PIC) readhdr.c
imgutilp.o:	imgutil.c
		$(CC) $(CFLAGS) -c -o imgutilp.o $(PIC) imgutil.c
lint:		$(IMGTOPNM_SRC)
		$(LINT) $(lintflags) $(IMGTOPNM_SRC) >lint 2>&1
imgtopnm:	$(IMGTOPNM_OBJ)
		$(CC) -g $(LDFLAGS) $(IMGTOPNM_OBJ) -o $@
example.bw:	sgiw
		./sgiw
img_ui:		img_ui.c
		$(CC) $(CFLAGS) -DTEST_MAIN $(PSHOP_INC) $(XM_INC) $? -o $@ -L/usr/dt/lib -R/usr/dt/lib -L/usr/openwin/lib -lXm -lXt -lX11


clean:
		rm -f *.o core example.rgb sgiw imgtopnm

dist:			SGIIMAGE-bin.tar SGIIMAGE-src.tar

SGIIMAGE-bin.tar:	$(BIN_DIST) imgpshop
			tar cvfh $@ $(BIN_DIST) "%$(MAC_NAME)" "$(MAC_NAME)"
SGIIMAGE-src.tar:	$(SRC_DIST)
			tar cvfh $@ $(SRC_DIST)

bkup:			clean
			tar cvf $(DEV) .
