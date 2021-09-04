.POSIX:

CONFIGFILE = config.mk
include $(CONFIGFILE)

OBJ =\
	editasroot.o\
	common.o\
	copier.o

HDR =\
	common.h

all: editasroot copier
$(OBJ): $(HDR)

.c.o:
	$(CC) -c -o $@ $< $(CFLAGS) $(CPPFLAGS)

editasroot: editasroot.o common.o
	$(CC) -o $@ $@.o common.o $(LDFLAGS)

copier: copier.o common.o
	$(CC) -o $@ $@.o common.o $(LDFLAGS)

install: editasroot copier
	mkdir -p -- "$(DESTDIR)$(PREFIX)/bin"
	mkdir -p -- "$(DESTDIR)$(LIBEXECDIR)"
	mkdir -p -- "$(DESTDIR)$(MANPREFIX)/man8/"
	cp -- editasroot "$(DESTDIR)$(PREFIX)/bin/"
	cp -- editasroot.8 "$(DESTDIR)$(MANPREFIX)/man8/"
	cp -- copier "$(DESTDIR)$(LIBEXECDIR)/"

uninstall:
	-rm -f -- "$(DESTDIR)$(PREFIX)/bin/editasroot"
	-rm -f -- "$(DESTDIR)$(LIBEXECDIR)/copier"
	-rmdir -- "$(DESTDIR)$(LIBEXECDIR)"
	-rm -f -- "$(DESTDIR)$(MANPREFIX)/man8/editasroot.8"

clean:
	-rm -f -- *.o *.a *.lo *.su *.so *.so.* *.gch *.gcov *.gcno *.gcda
	-rm -f -- editasroot copier

.SUFFIXES:
.SUFFIXES: .o .c

.PHONY: all install uninstall clean
