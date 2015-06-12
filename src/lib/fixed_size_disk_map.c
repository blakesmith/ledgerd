#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "fixed_size_disk_map.h"

#define FSD_MAP_SIZE(M) sizeof(fsd_map_hdr) +                           \
    (M)->nbuckets * sizeof(fsd_map_bucket_t) +                          \
    (M)->nbuckets * M->ncells_per_bucket * sizeof(fsd_map_cell_t);

int fsd_map_init(fsd_map_t *map, uint16_t nbuckets,
                 uint8_t ncells_per_bucket) {
    int rc;

    rc =  pthread_mutex_init(&map->lock, NULL);
    if(rc != 0) {
        return FSD_MAP_ERR_GENERAL;
    }
    map->nbuckets = nbuckets;
    map->ncells_per_bucket = ncells_per_bucket;
    map->mmap = NULL;
    map->mmap_len = 0;

    return FSD_MAP_OK;
}

int fsd_map_open(fsd_map_t *map, const char *path) {
    int rc;
    int fd = -1;
    bool created = false;

    fd = open(path, O_RDWR|O_CREAT, 0644);
    if(fd < 0) {
        rc = FSD_MAP_ERR_OPEN;
        goto error;
    }
    if(errno == EEXIST) {
        created = true;
    }

    map->mmap_len = FSD_MAP_SIZE(map);
    map->mmap = mmap(NULL, map->mmap_len, PROT_READ|PROT_WRITE,
                     MAP_SHARED, fd, 0);
    if(map->mmap == MAP_FAILED) {
        rc = FSD_MAP_ERR_MMAP;
        goto error;
    }
    close(fd);
    if(created) {
        memset(map->mmap, 0, map->mmap_len);
    }
    return FSD_MAP_OK;

error:
    if(fd > 0) {
        close(fd);
    }
    return rc;
}

int fsd_map_set(fsd_map_t *map, const char *key,
                size_t key_len, uint64_t value) {
    return 0;
}
int fsd_map_get(fsd_map_t *map, const char *key,
                size_t key_len, uint64_t *value) {
    return FSD_MAP_NOT_FOUND;
}

void fsd_map_close(fsd_map_t *map) {
    if(map->mmap) {
        munmap(map->mmap, map->mmap_len);
    }
}
