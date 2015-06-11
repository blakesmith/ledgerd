AM_CPPFLAGS = -isystem $(top_srcdir)/lib/gtest/include -I$(top_srcdir)/src $(GTEST_CPPFLAGS) -I$(top_srcdir)/src/lib
AM_CXXFLAGS = $(GTEST_CXXFLAGS) -std=c++11
AM_LDFLAGS = $(GTEST_LDFLAGS) $(GTEST_LIBS) \
	     $(top_srcdir)/lib/gtest/lib/libgtest_main.la \
	     $(top_srcdir)/lib/gtest/lib/libgtest.la

# TESTS -- Programs run automatically by "make check"
# check_PROGRAMS -- Programs built by "make check" but not necessarily run
TESTS = libledger_tests 
check_PROGRAMS = libledger_tests

libledger_tests_SOURCES = \
	test_ledger.cc \
	test_consumer.cc \
	test_fixed_size_disk_map.cc \
	test_signal.cc \
	test_threading.cc

libledger_tests_LDADD = $(top_srcdir)/src/lib/libledger.la
