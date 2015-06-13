# Makefile for "pi-fax", a File Format plug-in for G3 FAX files
# $Id: Makefile,v 1.7 96/04/15 16:24:55 ian Exp $

MAC_NAME	= FAX Group 3
PLUGINID	= 68457
PI_FAX_SRC	= pi-fax.c pi-ui.c g3read.c g3write.c
PI_FAX_OBJ	= pi-fax.o pi-ui.o g3read.o g3write.o

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

SRC_DIST	= LEGAL_NOTICE pifax-readme.html BUGS DOSlogo.xbm Makefile \
		pi-*.c g3*.c *.h
BIN_DIST	= LEGAL_NOTICE BUGS  pifax-readme.html

all:		pi-fax # test-ui

pi-fax:		$(PI_FAX_OBJ)
		ld -Bsymbolic -o $@ -r $(PI_FAX_OBJ)
		@rm -f "$(MAC_NAME)"
		ln $@ "$(MAC_NAME)"

#		You must do "make resource" once.
resource:	pi-fax MakePlugIn
		rm -f $(MAC_NAME) %$(MAC_NAME)
		@echo "Use $(MAC_NAME) as the Plug-In Name *and* filename!"
		@echo "Use $(PLUGINID) as the ID, 1 as the Major Version"
		@echo "Use these Image Format Parameters:"
		@echo "Type Count 0"
		@echo "Ext Count 3"
		@echo "Flags Can read Can write"
		@echo "Creator Type flags X Y (or whatever, non-blank)."
		@echo "Type list: g3__ fax 001"
		./MakePlugIn
		ln pi-fax $(MAC_NAME)
# The MakePlugIn program, from the SDK directory, is used to make up
# the "resource fork" for the Plug-in. Required! See the docco.
MakePlugIn:
		ln -s $(SDK_DIR)/MakePlugIn/Photoshop.MacFiles/MakePlugIn .
pi-fax.o:	pi-fax.c pi-fax.h g3.h routines.h
		$(CC) $(CFLAGS_PS) -c pi-fax.c
pi-ui.o:	pi-ui.c pi-fax.h g3.h
		$(CC) $(CFLAGS_PS) -c pi-ui.c
g3read.o g3write.o: pi-fax.h g3.h
test-ui:	pi-ui.c pi-fax.h g3.h
		$(CC) $(CFLAGS) -DTEST_MAIN $(PSHOP_INC) $(XM_INC) $? -o $@ -L/usr/dt/lib -R/usr/dt/lib -L/usr/openwin/lib -lXm -lXt -lX11
lint:		$(PI_FAX_SRC)
		$(LINT) $(lintflags) $(PI_FAX_SRC) >lint 2>&1

clean:
		rm -f *.o core pi-fax

dist:			FAXFORMAT-bin.tar FAXFORMAT-src.tar

FAXFORMAT-bin.tar:	$(BIN_DIST)
			tar cvfh $@ $(BIN_DIST) "%$(MAC_NAME)" "$(MAC_NAME)"
FAXFORMAT-src.tar:	$(SRC_DIST)
			tar cvfh $@ $(SRC_DIST)

bkup:			clean
			tar cvf $(DEV) .

