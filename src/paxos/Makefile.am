AM_CPPFLAGS = -I$(top_srcdir)/src/paxos $(PTHREAD_CFLAGS)
AM_CXXFLAGS = -std=c++11

lib_LTLIBRARIES = libpaxos.la

libpaxos_la_SOURCES = \
	group.h \
	node.h \
	instance.h \
	message.h message.cc \
	round.h round.cc

libpaxos_ladir = $(includedir)/ledger/paxos
