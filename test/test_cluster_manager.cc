#include <gtest/gtest.h>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ftw.h>
#include <fcntl.h>

#include <map>

#include "cluster_manager.h"

using namespace ledgerd;

namespace ledgerd_cluster_test {

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

TEST(ClusterManager, BasicSend) {
    LedgerdServiceConfig c1;
    LedgerdServiceConfig c2;
    LedgerdServiceConfig c3;
    cleanup("/tmp/ledgerd_cluster_c1");
    cleanup("/tmp/ledgerd_cluster_c2");
    cleanup("/tmp/ledgerd_cluster_c3");
    setup("/tmp/ledgerd_cluster_c1");
    setup("/tmp/ledgerd_cluster_c2");
    setup("/tmp/ledgerd_cluster_c3");
    c1.set_root_directory("/tmp/ledgerd_cluster_c1");
    c2.set_root_directory("/tmp/ledgerd_cluster_c2");
    c3.set_root_directory("/tmp/ledgerd_cluster_c3");
    LedgerdService ls1(c1);
    LedgerdService ls2(c2);
    LedgerdService ls3(c3);

    const std::map<uint32_t, NodeInfo> nodes =
        {{0, NodeInfo("0.0.0.0:54321")},
         {1, NodeInfo("0.0.0.0:54322")},
         {2, NodeInfo("0.0.0.0:54323")}};
    ClusterManager cm1(0, ls1, "0.0.0.0:54321", nodes);
    ClusterManager cm2(1, ls2, "0.0.0.0:54322", nodes);
    ClusterManager cm3(2, ls3, "0.0.0.0:54323", nodes);

    cm1.Start();
    cm2.Start();
    cm3.Start();

    const std::vector<unsigned int> partition_ids = { 0 };
    uint64_t sequence = cm1.RegisterTopic("new_topic", partition_ids);
    cm1.WaitForSequence(sequence);
    cm2.WaitForSequence(sequence);
    cm3.WaitForSequence(sequence);

    auto cev1 = cm1.GetEvent(sequence);
    auto cev2 = cm2.GetEvent(sequence);
    auto cev3 = cm3.GetEvent(sequence);

    ASSERT_TRUE(cev1 != nullptr);
    ASSERT_TRUE(cev2 != nullptr);
    ASSERT_TRUE(cev3 != nullptr);
    ASSERT_EQ(ClusterEventType::REGISTER_TOPIC, cev1->type());
    ASSERT_EQ(ClusterEventType::REGISTER_TOPIC, cev2->type());
    ASSERT_EQ(ClusterEventType::REGISTER_TOPIC, cev3->type());
    EXPECT_EQ("new_topic", cev1->register_topic().name());
    EXPECT_EQ("new_topic", cev2->register_topic().name());
    EXPECT_EQ("new_topic", cev3->register_topic().name());

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    cm1.Stop();
    cm2.Stop();
    cm3.Stop();

    cleanup("/tmp/ledgerd_cluster_c1");
    cleanup("/tmp/ledgerd_cluster_c2");
    cleanup("/tmp/ledgerd_cluster_c3");
}

TEST(ClusterManager, GetTopics) {
    LedgerdServiceConfig c1;
    LedgerdServiceConfig c2;
    LedgerdServiceConfig c3;
    cleanup("/tmp/ledgerd_cluster_c1");
    cleanup("/tmp/ledgerd_cluster_c2");
    cleanup("/tmp/ledgerd_cluster_c3");
    setup("/tmp/ledgerd_cluster_c1");
    setup("/tmp/ledgerd_cluster_c2");
    setup("/tmp/ledgerd_cluster_c3");
    c1.set_root_directory("/tmp/ledgerd_cluster_c1");
    c2.set_root_directory("/tmp/ledgerd_cluster_c2");
    c3.set_root_directory("/tmp/ledgerd_cluster_c3");
    LedgerdService ls1(c1);
    LedgerdService ls2(c2);
    LedgerdService ls3(c3);

    const std::map<uint32_t, NodeInfo> nodes =
        {{0, NodeInfo("0.0.0.0:54321")},
         {1, NodeInfo("0.0.0.0:54322")},
         {2, NodeInfo("0.0.0.0:54323")}};
    ClusterManager cm1(0, ls1, "0.0.0.0:54321", nodes);
    ClusterManager cm2(1, ls2, "0.0.0.0:54322", nodes);
    ClusterManager cm3(2, ls3, "0.0.0.0:54323", nodes);

    cm1.Start();
    cm2.Start();
    cm3.Start();

    const std::vector<unsigned int> partition_ids = { 0, 1 };
    uint64_t sequence = cm2.RegisterTopic("new_topic", partition_ids);
    cm1.WaitForSequence(sequence);
    cm2.WaitForSequence(sequence);
    cm3.WaitForSequence(sequence);

    ClusterTopicList topic_list;
    cm1.GetTopics(&topic_list);

    EXPECT_EQ(1, topic_list.topics.size());
    EXPECT_EQ("new_topic", topic_list.topics[0].name);
    EXPECT_EQ(partition_ids, topic_list.topics[0].partition_ids);
    EXPECT_EQ(1, topic_list.topics[0].node_id);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    cm1.Stop();
    cm2.Stop();
    cm3.Stop();

    cleanup("/tmp/ledgerd_cluster_c1");
    cleanup("/tmp/ledgerd_cluster_c2");
    cleanup("/tmp/ledgerd_cluster_c3");
}

}
