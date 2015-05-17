SUBDIRS = lib src test
ACLOCAL_AMFLAGS = -I m4

clean-all:
	rm -rf aclocal.m4 autom4te.cache compile configure config config.h config.h.in config.log config.status depcomp install-sh missing stamp-h1 Makefile Makefile.in src/Makefile.in test/Makefile.in

pkg: all
	sudo checkinstall --pkgname=$(PACKAGE) \
			  --pkgversion=$(VERSION) \
			  --default \
			  --strip=no

