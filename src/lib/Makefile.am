AM_CFLAGS=$(PTHREAD_CFLAGS) -Wall -pedantic -std=c11
lib_LTLIBRARIES = libledger.la

libledger_la_HEADERS = \
	consumer.h \
	journal.h \
	ledger.h \
	message.h \
	partition.h \
	signal.h \
	topic.h

libledger_la_SOURCES = \
	common.h common.c \
	consumer.c \
	crc32.h crc32.c \
	dict.h dict.c \
	journal.c \
	ledger.c \
	message.c \
	partition.c \
	signal.c \
	topic.c

libledger_ladir = $(includedir)/ledger
