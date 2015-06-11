#include "fixed_size_disk_map.h"

int fsd_map_init(fsd_map_t *map, uint16_t nbuckets,
                 const char *path) {
    return 0;
}

int fsd_map_open(fsd_map_t *map) {
    return 0;
}

int fsd_map_set(fsd_map_t *map, const char *key,
                size_t key_len, uint64_t value) {
    return 0;
}
int fsd_map_get(fsd_map_t *map, const char *key,
                size_t key_len, uint64_t *value) {
    *value = 0;
    return 0;
}

void fsd_map_close(fsd_map_t *map) {
}
