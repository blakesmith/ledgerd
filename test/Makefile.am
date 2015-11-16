SUBDIRS = lib paxos

AM_CPPFLAGS = -isystem $(top_srcdir)/lib/gtest/include -I$(top_srcdir)/src $(GTEST_CPPFLAGS) -I$(top_srcdir)/src/lib
AM_CXXFLAGS = $(GTEST_CXXFLAGS) -std=c++11
AM_LDFLAGS = $(GTEST_LDFLAGS) $(GTEST_LIBS) \
	     $(top_srcdir)/lib/gtest/lib/libgtest_main.la \
	     $(top_srcdir)/lib/gtest/lib/libgtest.la \
             -L/usr/local/lib -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -lpthread -ldl


TESTS = ledgerd_tests 
check_PROGRAMS = ledgerd_tests

ledgerd_tests_SOURCES = \
	test_grpc_interface.cc \
	test_ledger_service.cc \
	test_command_parser.cc

ledgerd_tests_LDADD = $(top_srcdir)/src/lib/libledger.la \
	$(top_srcdir)/src/ledgerd_consumer.o \
	$(top_srcdir)/src/ledgerd_service.o \
	$(top_srcdir)/src/ledgerd_service_config.o \
	$(top_srcdir)/src/grpc_interface.o \
	$(top_srcdir)/src/command.o \
	$(top_srcdir)/src/command_parser.o \
	$(top_srcdir)/src/proto/ledgerd.grpc.pb.o \
	$(top_srcdir)/src/proto/ledgerd.pb.o


