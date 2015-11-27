AM_CPPFLAGS = -isystem $(top_srcdir)/lib/gtest/include -I$(top_srcdir)/src $(GTEST_CPPFLAGS) -I$(top_srcdir)/src/lib
AM_CXXFLAGS = $(GTEST_CXXFLAGS) -std=c++11
AM_LDFLAGS = $(GTEST_LDFLAGS) $(GTEST_LIBS) \
	     $(top_srcdir)/lib/gtest/lib/libgtest_main.la \
	     $(top_srcdir)/lib/gtest/lib/libgtest.la

# TESTS -- Programs run automatically by "make check"
# check_PROGRAMS -- Programs built by "make check" but not necessarily run
TESTS = libpaxos_tests
check_PROGRAMS = libpaxos_tests

libpaxos_tests_SOURCES = \
	test_group.cc \
	test_instance.cc

libpaxos_tests_LDADD = $(top_srcdir)/src/paxos/libpaxos.la
