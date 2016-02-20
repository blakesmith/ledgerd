#include <gtest/gtest.h>

#include "service_config_parser.h"

namespace ledgerd_test {

using namespace ledgerd;

TEST(ServiceConfigParser, AllOptions) {
    ServiceConfigParser parser;
    const char *argv[] { "ledgerd",
            "--root", "/tmp/ledgerd",
            "--grpc-address", "0.0.0.0:643",
            "--cluster-address", "0.0.0.0:645",
            "--peer-nodes", "{0=>0.0.0.0:645,1=>0.0.0.0:645}",
            "--cluster-node-id", "2",
            "--default-partition-count", "5"};
    int argc = sizeof(argv) / sizeof(char*);
    
    auto config = parser.MakeServiceConfig(argc, const_cast<char**>(argv));
    ASSERT_TRUE(config != nullptr);
    EXPECT_EQ("/tmp/ledgerd", config->get_root_directory());
    EXPECT_EQ("0.0.0.0:643", config->get_grpc_address());
    EXPECT_EQ("0.0.0.0:645", config->grpc_cluster_address());
    EXPECT_EQ(2, config->cluster_node_id());
    EXPECT_EQ(5, config->default_partition_count());
    EXPECT_EQ(2, config->node_info().size());
}

}
