AM_CFLAGS=$(PTHREAD_CFLAGS) -Wall -pedantic
lib_LTLIBRARIES = libledger.la

libledger_la_HEADERS = \
	ledger.h \
	topic.h

libledger_la_SOURCES = \
	common.h common.c \
	crc32.h crc32.c \
	dict.h dict.c \
	ledger.c \
	topic.c

libledger_ladir = $(includedir)/ledger
