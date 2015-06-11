#ifndef FIXED_SIZE_DISK_MAP_H
#define FIXED_SIZE_DISK_MAP_H

#include <pthread.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

enum {
    FSD_MAP_OK,
    FSD_MAP_NOT_FOUND,
    FSD_MAP_ERR_GENERAL,
    FSD_MAP_ERR_MMAP,
    FSD_MAP_ERR_OPEN
};

typedef struct {
    uint32_t hash;
    uint64_t value;
} fsd_map_cell_t;

typedef struct {
    uint8_t n_full_cells;
} fsd_map_bucket_t;

typedef struct {
    uint16_t nbuckets;
    uint8_t ncells_per_bucket;
} fsd_map_hdr;

typedef struct {
    void *mmap;
    size_t mmap_len;
    uint16_t nbuckets;
    uint8_t ncells_per_bucket;
    pthread_mutex_t lock;
} fsd_map_t;

int fsd_map_init(fsd_map_t *map, uint16_t nbuckets,
                 uint8_t ncells_per_bucket);
int fsd_map_open(fsd_map_t *map, const char *path);
int fsd_map_set(fsd_map_t *map, const char *key,
                size_t key_len, uint64_t value);
int fsd_map_get(fsd_map_t *map, const char *key,
                size_t key_len, uint64_t *value);
void fsd_map_close(fsd_map_t *map);

#if defined(__cplusplus)
}
#endif
#endif
