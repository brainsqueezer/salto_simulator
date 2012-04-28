# Set to 1 to enable debugging (slow, lots of output)
DEBUG	= 0
# Set to 1 to silence building
SILENT	= 1

# CROM/CRAM config
# 1 = 1K CROM, 1K CRAM, 1 set of S registers
# 2 = 2K CROM, 1K CRAM, 1 set of S registers
# 3 = 1K CROM, 3K CRAM, 8 sets of S registers
CRAM	= 1

# project name and version number
PROJECT		=	salto
VER_MAJOR	=	0
VER_MINOR	=	4
VER_MICRO	=	2
VERSION		=	$(VER_MAJOR).$(VER_MINOR).$(VER_MICRO)

# relative path names
SRC	=	src
OBJ	=	obj
BIN	=	bin
TEMP	=	temp
ZLIB	=	zlib
TOOLS	=	tools
DIRS	=	$(BIN) $(OBJ) $(TEMP)

# C-cache, C compiler and flags
GCC	= $(shell which gcc)
CFLAGS	+= -Wall -MD

# Linker and flags
LD	= $(GCC)
LDFLAGS =

# Archiver and flags
AR	= ar
ARFLAGS = r
# Archiver TOC updater
RANLIB	= ranlib

# Default libraries
LIBS	=
# Default include paths
INC	= -Iinclude -I$(TEMP)

# Lex and Yacc (required for the Alto assember aasm)
LEX	= flex
YACC	= yacc
YFLAGS	= -d -t -v

# Move command
MV	= mv

# Include path (use to find zlib.h header file)

# Specify the full path to a libz.so, if you know it, or
# otherwise just comment the following line to build our own
#
ifeq ($(wildcard /usr/local/include/zlib.h),/usr/local/include/zlib.h)
INCZ	=	-I/usr/local/include
LIBZ	=	-L/usr/local/lib -Wl,-rpath,/usr/local/lib -lz
else
ifeq ($(wildcard /usr/pkg/include/zlib.h),/usr/pkg/include/zlib.h)
INCZ	=	-I/usr/pkg/include
LIBZ	=	-L/usr/pkg/lib -Wl,-rpath,/usr/pkg/lib -lz
else
ifeq ($(wildcard /usr/include/zlib.h),/usr/include/zlib.h)
INCZ	=	-I/usr/include
ifeq ($(wildcard /usr/lib/libz.so),/usr/lib/libz.so)
LIBZ	=	-L/usr/lib -Wl,-rpath,/usr/lib -lz
else
ifeq ($(wildcard /lib/libz.so),/lib/libz.so)
LIBZ	=	-L/lib -Wl,-rpath,/lib -lz
else
LIBZ	=
endif
endif
endif
endif
endif

#
# If LIBZ is left empty, we will build our own in ./zlib
#LIBZ	=
#INCZ	=	-Izlib

# Additional libraries
LIBS	+=
# Additional include paths
INC	+=	$(INCZ)

# Define this, if your OS and compiler has stdint.h
ifeq ($(wildcard /usr/include/stdint.h),/usr/include/stdint.h)
CFLAGS	+= -DHAVE_STDINT_H=1
endif

# Define this to draw some nifty icons on the frontend
CFLAGS	+= -DFRONTEND_ICONS=1

# Heavy debugging
#CFLAGS += -DDEBUG_CPU_TIMESLICES=1
#CFLAGS += -DDEBUG_DISPLAY_TIMING=1

# CRAM configuration
CFLAGS	+= -DCRAM_CONFIG=$(CRAM)

# Add some flags depending on DEBUG=0 or 1
ifeq	($(strip $(DEBUG)),1)
CFLAGS	+= -g -O3 -DDEBUG=1
else
CFLAGS	+= -O3 -fno-strict-aliasing -DDEBUG=0
LDFLAGS	+=
endif

ifeq	($(strip $(SILENT)),1)
CC_MSG		=	@echo "==> compiling $< ..."
CC_RUN		=	@$(GCC)

LD_MSG		=	@echo "==> linking $@ ..."
LD_RUN		=	@$(LD)

AR_MSG		=	@echo "==> archiving $@ ..."
AR_RUN		=	@$(AR)

RANLIB_MSG	=	@echo "==> indexing $@ ..."
RANLIB_RUN	=	@$(RANLIB)

LEX_MSG		=	@echo "==> building lexer $< ..."
LEX_RUN 	=	@$(LEX)

YACC_RUN	=	@$(YACC)
YACC_MSG	=	@echo "==> building parser $< ..."

MV_RUN		=	@$(MV)

AASM_MSG	=	@echo "==> assembling Alto code $< ..."

else

CC_MSG		=	@echo
CC_RUN		=	$(GCC)

LD_MSG		=	@echo
LD_RUN		=	$(LD)

AR_MSG		=	@echo
AR_RUN		=	$(AR)

RANLIB_MSG	=	@echo
RANLIB_RUN	=	$(RANLIB)

LEX_MSG		=	@echo
LEX_RUN		=	$(LEX)

YACC_MSG	=	@echo
YACC_RUN	=	$(YACC)

MV_RUN		=	$(MV)

AASM_MSG	=	@echo
endif

# Pull in the SDL CFLAGS, -I includes and LIBS
INC	+= $(shell sdl-config --cflags)
LIBS	+= $(shell sdl-config --libs)

# The object files for the altogether binary
OBJS	=	$(OBJ)/salto.o $(OBJ)/zcat.o $(OBJ)/pics.o \
		$(OBJ)/md5.o $(OBJ)/png.o $(OBJ)/mng.o \
		$(OBJ)/alto.o $(OBJ)/timer.o $(OBJ)/debug.o \
		$(OBJ)/hardware.o $(OBJ)/printer.o \
		$(OBJ)/keyboard.o $(OBJ)/eia.o \
		$(OBJ)/display.o $(OBJ)/mouse.o \
		$(OBJ)/cpu.o \
		$(OBJ)/emu.o \
		$(OBJ)/memory.o $(OBJ)/mrt.o \
		$(OBJ)/dwt.o $(OBJ)/curt.o $(OBJ)/dht.o $(OBJ)/dvt.o \
		$(OBJ)/disk.o $(OBJ)/drive.o $(OBJ)/ksec.o $(OBJ)/kwd.o \
		$(OBJ)/ether.o \
		$(OBJ)/part.o \
		$(OBJ)/ram.o \
		$(OBJ)/unused.o

TARGETS :=

ifneq	($(strip $(LIBZ)),)

# Use a system provided zlib
LIBS	+= $(LIBZ)
LIBZOBJ =

else

# Use our own static zlib libz.a
DIRS	+= $(OBJ)/zlib
LIBS	+= $(OBJ)/$(ZLIB)/libz.a
TARGETS += $(OBJ)/$(ZLIB)/libz.a
LIBZOBJ	+= $(OBJ)/$(ZLIB)/adler32.o $(OBJ)/$(ZLIB)/compress.o \
	$(OBJ)/$(ZLIB)/crc32.o $(OBJ)/$(ZLIB)/deflate.o \
	$(OBJ)/$(ZLIB)/gzio.o $(OBJ)/$(ZLIB)/inffast.o \
	$(OBJ)/$(ZLIB)/inflate.o $(OBJ)/$(ZLIB)/infback.o \
	$(OBJ)/$(ZLIB)/inftrees.o $(OBJ)/$(ZLIB)/trees.o \
	$(OBJ)/$(ZLIB)/uncompr.o $(OBJ)/$(ZLIB)/zutil.o

LIBZCFLAGS = -O2 -Wall -fno-strict-aliasing \
	-Wwrite-strings -Wpointer-arith -Wconversion \
	-Wstrict-prototypes -Wmissing-prototypes
endif

TARGETS += $(BIN)/ppm2c $(BIN)/convbdf $(BIN)/salto \
	$(BIN)/aasm $(BIN)/adasm $(BIN)/edasm \
	$(BIN)/dumpdsk $(BIN)/aar $(BIN)/aldump $(BIN)/helloworld.bin

all:	dirs $(TARGETS)

$(BIN)/salto:	$(OBJS)
	$(LD_MSG)
	$(LD_RUN) $(LDFLAGS) -o $@ $^ $(LIBS)

dirs:
	@-mkdir -p $(DIRS) 2>/dev/null
	@echo "*************** AUTO CONFIGURATION ***************"
	@echo "CC_RUN ....... $(CC_RUN)"
	@echo "LD_RUN ....... $(LD_RUN)"
	@echo "AR_RUN ....... $(AR_RUN)"
	@echo "RANLIB_RUN ... $(RANLIB_RUN)"
	@echo "LEX_RUN ...... $(LEX_RUN)"
	@echo "YACC_RUN ..... $(YACC_RUN)"
	@echo "**************************************************"

$(OBJ)/%.o:	tools/%.c
	$(CC_MSG)
	$(CC_RUN) $(CFLAGS) -Iinclude -I$(TEMP) -Itools -o $@ -c $<


$(TEMP)/pics.c: $(BIN)/ppm2c
	@echo "==> embedding icons in C source $@ ..."
	@$(BIN)/ppm2c $(wildcard pics/*.ppm) >$@

$(BIN)/ppm2c:	$(OBJ)/ppm2c.o
	$(LD_MSG)
	$(LD_RUN) $(LDFLAGS) -o $@ $^

$(OBJ)/%.o:	$(TEMP)/%.c
	$(CC_MSG)
	$(CC_RUN) $(CFLAGS) $(INC) -Itools -o $@ -c $<

$(OBJ)/$(ZLIB)/%.o:	 $(SRC)/$(ZLIB)/%.c
	$(CC_MSG)
	$(CC_RUN) $(LIBZCFLAGS) $(INC) -o $@ -c $<

$(OBJ)/%.o:	 $(SRC)/%.c
	$(CC_MSG)
	$(CC_RUN) $(CFLAGS) $(INC) -o $@ -c $<

$(OBJ)/$(ZLIB)/libz.a:	$(LIBZOBJ)
	$(AR_MSG)
	$(AR_RUN) $(ARFLAGS) $@ $^
	$(RANLIB_MSG)
	$(RANLIB_RUN) $@

$(BIN)/convbdf:	$(OBJ)/convbdf.o
	$(LD_MSG)
	$(LD_RUN) $(LDFLAGS) -o $@ $^

$(BIN)/aasm:	$(OBJ)/aasm.tab.o $(OBJ)/aasmyy.o
	$(LD_MSG)
	$(LD_RUN) $(LDFLAGS) -o $@ $^

$(TEMP)/aasmyy.c:	tools/aasm.l
	$(LEX_MSG)
	$(LEX_RUN) -o$@ -i $<

$(TEMP)/aasm.tab.c:	tools/aasm.y
	$(YACC_MSG)
	$(YACC_RUN) $(YFLAGS) -baasm $<
	$(MV_RUN) aasm.output $(TEMP)
	$(MV_RUN) aasm.tab.? $(TEMP)

$(BIN)/adasm:	$(OBJ)/adasm.o
	$(LD_MSG)
	$(LD_RUN) $(LDFLAGS) -o $@ $^

$(BIN)/edasm:	$(OBJ)/edasm.o
	$(LD_MSG)
	$(LD_RUN) $(LDFLAGS) -o $@ $^

$(BIN)/dumpdsk:	$(OBJ)/dumpdsk.o
	$(LD_MSG)
	$(LD_RUN) $(LDFLAGS) -o $@ $^

$(BIN)/aar:	$(OBJ)/aar.o
	$(LD_MSG)
	$(LD_RUN) $(LDFLAGS) -o $@ $^

$(BIN)/aldump:	$(OBJ)/aldump.o $(OBJ)/png.o $(OBJ)/md5.o
	$(LD_MSG)
	$(LD_RUN) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BIN)/helloworld.bin: asm/helloworld.asm $(BIN)/aasm
	$(AASM_MSG)
	$(BIN)/aasm -l -o $@ $<

helloworld:	all
ifeq	($(strip $(DEBUG)),1)
	./salto -all $(BIN)/helloworld.bin
else
	./salto $(BIN)/helloworld.bin
endif

clean:
	rm -rf salto *.core $(TEMP) $(OBJ) $(BIN)

distclean:	clean
	rm -rf helloworld.bin alto.mng alto*.png alto.dump log *.bck */*.bck

dist:	distclean
	cd .. && tar -czf $(PROJECT)-$(VERSION).tar.gz \
	`find salto \
		-regex ".*\.[chly]$$" \
		-o -regex ".*\.asm$$" \
		-o -regex ".*\.txt$$" \
		-o -regex ".*\.mu$$" \
		-o -regex "salto/README$$" \
		-o -regex "salto/COPYING$$" \
		-o -regex "salto/Makefile$$" \
		-o -regex "salto/Doxyfile$$" \
		-o -regex "salto/roms/.*$$" \
		-o -regex "salto/pics/.*$$" | \
		grep -v Alto_ROMs | \
		grep -v CVS`


include $(wildcard $(OBJ)/*.d)
