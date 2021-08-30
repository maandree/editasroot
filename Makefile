.POSIX:

CONFIGFILE = config.mk
include $(CONFIGFILE)

OBJ =\
	editasroot.o\
	copier.o

all: editasroot copier
$(OBJ): $(@:.o=.c)

.c.o:
	$(CC) -c -o $@ $< $(CFLAGS) $(CPPFLAGS)

editasroot: editasroot.o
	$(CC) -o $@ $@.o $(LDFLAGS)

copier: copier.o
	$(CC) -o $@ $@.o $(LDFLAGS)

install: editasroot copier
	mkdir -p -- "$(DESTDIR)$(PREFIX)/bin"
	mkdir -p -- "$(DESTDIR)$(LIBEXECDIR)"
	mkdir -p -- "$(DESTDIR)$(MANPREFIX)/man8/"
	cp -- editasroot "$(DESTDIR)$(PREFIX)/bin/"
	cp -- editasroot.1 "$(DESTDIR)$(MANPREFIX)/man8/"
	cp -- copier "$(DESTDIR)$(LIBEXECDIR)/"

uninstall:
	-rm -f -- "$(DESTDIR)$(PREFIX)/bin/editasroot"
	-rm -f -- "$(DESTDIR)$(LIBEXECDIR)/editasroot-copier"
	-rm -f -- "$(DESTDIR)$(MANPREFIX)/man8/editasroot.8"

clean:
	-rm -f -- *.o *.a *.lo *.su *.so *.so.* *.gch *.gcov *.gcno *.gcda
	-rm -f -- editasroot copier

.SUFFIXES:
.SUFFIXES: .o .c

.PHONY: all install uninstall clean
