SUBDIRS = lib

# TODO: Don't hardcode these
AM_CPPFLAGS = -I/usr/local/include -pthread
AM_CXXFLAGS = -std=c++11
AM_LDFLAGS = -L/usr/local/lib -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -lpthread -ldl

bin_PROGRAMS = ledgerd

ledgerd_SOURCES = \
	proto/ledgerd.pb.cc \
	proto/ledgerd.grpc.pb.cc \
	ledgerd.cc

ledgerd_LDADD = $(top_srcdir)/src/lib/libledger.la

PROTOC = protoc
PROTOS_PATH = proto
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=$(PROTOS_PATH) --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=$(PROTOS_PATH) $<
