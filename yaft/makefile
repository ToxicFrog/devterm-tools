CC ?= gcc
CC_FOR_BUILD ?= gcc
#CC ?= clang

CFLAGS  ?= -std=c99 -pedantic -Wall -Wextra -O3 -s -pipe
LDFLAGS ?=

XCFLAGS  ?= -std=c99 -pedantic -Wall -Wextra -I/usr/include/X11/ -O3 -s -pipe
XLDFLAGS ?= -lX11

HDR = glyph.h yaft.h conf.h color.h parse.h terminal.h util.h \
	ctrlseq/esc.h ctrlseq/csi.h ctrlseq/osc.h ctrlseq/dcs.h \
	fb/common.h fb/linux.h fb/freebsd.h fb/netbsd.h fb/openbsd.h \
	x/x.h glyph_bold.h

PREFIX ?= $(DESTDIR)/usr/local
MANPREFIX ?= $(PREFIX)/share/man
TERMPREFIX ?= $(PREFIX)/share/terminfo		# Not used in Debian, instead
#TERMPREFIX ?= /etc/terminfo			        # store terminfo in /etc.

all: yaft

yaft: mkfont_bdf

yaftx: mkfont_bdf

mkfont_bdf: tools/mkfont_bdf.c tools/mkfont_bdf.h tools/bdf.h tools/util.h
	# Don't cross-compile this since it's part of the build chain.
	$(CC_FOR_BUILD) -o $@ $< $(CFLAGS) $(LDFLAGS)


# This makefile creates a default glyph.h, if you haven't created one using ./glyphbuilder.sh.
glyph.h: mkfont_bdf
	# If you want to use your favorite fonts, please run ./glyphbuilder.sh.
	# Or, you can change the following line and delete glyph.h.
	# USAGE: mkfont_bdf ALIAS BDF1 BDF2 BDF3... > glyph.h
	# ALIAS: glyph substitution rule file (see table/alias for more detail)
	# BDF1 BDF2 BDF3...: monospace bdf files (must be the same size)
	# If there is more than one glyph of the same codepoint, the glyph included in the first bdf file is choosen.
	./mkfont_bdf table/alias fonts/TamzenForPowerline10x20r.bdf > glyph.h

glyph_bold.h: mkfont_bdf
	BDF_VARIANT=bold ./mkfont_bdf table/alias fonts/TamzenForPowerline10x20b.bdf > glyph_bold.h

yaft: yaft.c $(HDR)
	# If you want to change configuration, please modify conf.h before make (see conf.h for more detail)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

yaftx: x/yaftx.c $(HDR)
	# If you want to change configuration, please modify conf.h before make (see conf.h for more detail)
	$(CC) -o $@ $< $(XCFLAGS) $(XLDFLAGS)

install: installshare
	mkdir -p $(PREFIX)/bin/
	install -m755 ./yaft $(PREFIX)/bin/yaft
	install -m755 ./yaft_wall $(PREFIX)/bin/yaft_wall

installx: installshare
	mkdir -p $(PREFIX)/bin/
	install -m755 ./yaftx $(PREFIX)/bin/yaftx

installshare:
	mkdir -p $(MANPREFIX)/man1/ $(TERMPREFIX)
	install -m644 ./man/yaft.1 $(MANPREFIX)/man1/yaft.1
	tic -o $(TERMPREFIX) info/yaft.src

uninstall:
	rm -f $(PREFIX)/bin/yaft
	rm -f $(PREFIX)/bin/yaft_wall

uninstallx:
	rm -f $(PREFIX)/bin/yaftx

clean:
	rm -f yaft yaftx mkfont_bdf glyph.h
