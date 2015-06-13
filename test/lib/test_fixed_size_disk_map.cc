#include <gtest/gtest.h>

#include <ftw.h>

#include "fixed_size_disk_map.h"

namespace fsd_map_tests {

static const char *MAP_PATH = "/tmp/fsd_map";

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

TEST(FixedSizeDiskMap, TestGetAndSetEmptyMap) {
    fsd_map_t map;
    uint64_t val;

    cleanup(MAP_PATH);
    
    ASSERT_EQ(0, fsd_map_init(&map, 16, 5));
    ASSERT_EQ(0, fsd_map_open(&map, MAP_PATH));
    EXPECT_EQ(FSD_MAP_NOT_FOUND, fsd_map_get(&map, "hello", 5, &val));

    fsd_map_close(&map);
    cleanup(MAP_PATH);
}

TEST(FixedSizeDiskMap, TestGetAndSet) {
    fsd_map_t map;
    uint64_t val;

    cleanup(MAP_PATH);

    ASSERT_EQ(0, fsd_map_init(&map, 16, 5));
    ASSERT_EQ(0, fsd_map_open(&map, MAP_PATH));
    EXPECT_EQ(FSD_MAP_OK, fsd_map_set(&map, "hello", 5, 10));
    ASSERT_EQ(FSD_MAP_OK, fsd_map_get(&map, "hello", 5, &val));
    EXPECT_EQ(10, val);

    fsd_map_close(&map);
    cleanup(MAP_PATH);
}
}
