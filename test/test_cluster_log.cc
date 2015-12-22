#include <ftw.h>

#include <gtest/gtest.h>

#include "cluster_log.h"

using namespace ledgerd;

namespace ledgerd_test {

static int setup(const char *directory) {
    return mkdir(directory, 0777);
}

static int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    int rv = remove(fpath);

    if (rv) perror(fpath);

    return rv;
}

int rmrf(const char *path) {
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

static int cleanup(const char *directory) {
    return rmrf(directory);
}

TEST(ClusterLog, WriteRead) {
    const char *dir = "/tmp/ledgerd_cluster_log";
    cleanup(dir);
    setup(dir);
    LedgerdServiceConfig config;
    config.set_grpc_address("0.0.0.0:64399");
    config.set_root_directory(dir);
    LedgerdService ledgerd_service(config);
    ClusterLog log(ledgerd_service);

    ClusterEvent event;
    Node* node = event.mutable_source_node();
    node->set_id(1);
    event.set_type(ClusterEventType::REGISTER_TOPIC);
    RegisterTopicEvent* register_topic = event.mutable_register_topic();
    register_topic->set_name("my_topic");

    ASSERT_EQ(paxos::LogStatus::LOG_OK, log.Write(1, &event));

    std::unique_ptr<ClusterEvent> read_event = log.Get(1);
    ASSERT_TRUE(read_event != nullptr);

    EXPECT_EQ(event.type(), read_event->type());
    EXPECT_EQ(event.source_node().id(), read_event->source_node().id());
    EXPECT_EQ(event.register_topic().name(), read_event->register_topic().name());

    cleanup(dir);
}

}
