SUBDIRS = lib paxos

# TODO: Don't hardcode these
AM_CPPFLAGS = -I/usr/local/include -I$(top_srcdir)/src/lib -pthread
AM_CXXFLAGS = -std=c++11
AM_LDFLAGS = -L/usr/local/lib -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -lpthread -ldl

bin_PROGRAMS = ledgerd ledger_client

ledger_client_SOURCES = \
	proto/ledgerd.pb.cc \
	proto/ledgerd.grpc.pb.cc \
	command.h command.cc \
	command_executor.h command_executor.cc \
	command_parser.h command_parser.cc \
	grpc_command_executor.h grpc_command_executor.cc \
	ledger_client.h ledger_client.cc

ledgerd_SOURCES = \
	proto/ledgerd.pb.cc \
	proto/ledgerd.grpc.pb.cc \
	log.h \
	grpc_interface.h grpc_interface.cc \
	ledgerd.cc \
	service_config_parser.h service_config_parser.cc \
	ledgerd_consumer.h ledgerd_consumer.cc \
	ledgerd_service.h ledgerd_service.cc \
	ledgerd_service_config.h ledgerd_service_config.cc \
	cluster_log.h cluster_log.cc \
	cluster_listener.h cluster_listener.cc \
	cluster_manager.h cluster_manager.cc \
	node_info.h node_info.cc


ledgerd_LDADD = $(top_srcdir)/src/lib/libledger.la \
	$(top_srcdir)/src/paxos/libpaxos.la

PROTOC = protoc
PROTOS_PATH = proto
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=$(PROTOS_PATH) --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=$(PROTOS_PATH) $<
