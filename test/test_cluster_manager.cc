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

    std::unique_ptr<ClusterEvent> event(new ClusterEvent());
    event->set_type(ClusterEventType::REGISTER_TOPIC);
    RegisterTopicEvent* rt = event->mutable_register_topic();
    rt->set_name("new_topic");
    rt->add_partition_ids(0);
    cm1.Send(std::move(event));

    cm1.Stop();
    cm2.Stop();
    cm3.Stop();

    cleanup("/tmp/ledgerd_cluster_c1");
    cleanup("/tmp/ledgerd_cluster_c2");
    cleanup("/tmp/ledgerd_cluster_c3");
}

}
