#include <gtest/gtest.h>

#include "service_config_parser.h"

namespace ledgerd_test {

using namespace ledgerd;

TEST(ServiceConfigParser, AllOptions) {
    ServiceConfigParser parser;
    const char *argv[] { "ledgerd",
            "--root", "/tmp/ledgerd"};
    int argc = sizeof(argv) / sizeof(char*);
    
    auto config = parser.MakeServiceConfig(argc, const_cast<char**>(argv));
    ASSERT_TRUE(config != nullptr);
    EXPECT_EQ("/tmp/ledgerd", config->get_root_directory());
}

}
