SUBDIRS = lib

AM_CPPFLAGS = -isystem $(top_srcdir)/lib/gtest/include -I$(top_srcdir)/src $(GTEST_CPPFLAGS) -I$(top_srcdir)/src
AM_CXXFLAGS = $(GTEST_CXXFLAGS) -std=c++11
AM_LDFLAGS = $(GTEST_LDFLAGS) $(GTEST_LIBS) \
	     $(top_srcdir)/lib/gtest/lib/libgtest_main.la \
	     $(top_srcdir)/lib/gtest/lib/libgtest.la


TESTS = ledgerd_tests 
check_PROGRAMS = ledgerd_tests

ledgerd_tests_SOURCES = \
	test_ledger_service.cc

ledgerd_tests_LDADD = $(top_srcdir)/src/ledgerd_service.o \
	$(top_srcdir)/src/ledgerd_service_config.o


