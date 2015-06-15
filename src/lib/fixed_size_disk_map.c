#define _XOPEN_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fixed_size_disk_map.h"
#include "murmur3.h"

#define FSD_MAP_SIZE(M) sizeof(fsd_map_hdr) +                           \
    (M)->nbuckets * sizeof(fsd_map_bucket_t) +                          \
    (M)->nbuckets * (M)->ncells_per_bucket * sizeof(fsd_map_cell_t);

#define MURMUR_SEED 42

int fsd_map_init(fsd_map_t *map, uint16_t nbuckets,
                 uint8_t ncells_per_bucket) {
    int rc;

    rc =  pthread_mutex_init(&map->lock, NULL);
    if(rc != 0) {
        return FSD_MAP_ERR_LOCK;
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
    fsd_map_hdr *hdr;
    size_t written;
    char *c = NULL;
    struct stat st;

    fd = open(path, O_RDWR|O_CREAT, 0644);
    if(fd < 0) {
        rc = FSD_MAP_ERR_OPEN;
        goto error;
    }
    rc = fstat(fd, &st);
    if(rc != 0) {
        rc = FSD_MAP_ERR_IO;
        goto error;
    }
    if(st.st_size == 0) {
        created = true;
    }

    map->mmap_len = FSD_MAP_SIZE(map);
    if(created) {
        c = malloc(map->mmap_len);
        if(c == NULL) {
            rc = FSD_MAP_ERR_MEMORY;
            goto error;
        }
        memset(c, 0, map->mmap_len);
        written = write(fd, c, map->mmap_len);
        if(written != map->mmap_len) {
            rc = FSD_MAP_ERR_IO;
            goto error;
        }
        free(c);
    }
    map->mmap = mmap(NULL, map->mmap_len, PROT_READ|PROT_WRITE,
                     MAP_SHARED, fd, 0);
    if(map->mmap == MAP_FAILED) {
        rc = FSD_MAP_ERR_MMAP;
        goto error;
    }

    hdr = (fsd_map_hdr *)map->mmap;
    if(created) {
        hdr->nbuckets = map->nbuckets;
        hdr->ncells_per_bucket = map->ncells_per_bucket;
    } else {
        if(hdr->nbuckets != map->nbuckets ||
           hdr->ncells_per_bucket != map->ncells_per_bucket) {
            rc = FSD_MAP_ERR_INVAL;
            goto error;
        }
    }

    close(fd);
    return FSD_MAP_OK;

error:
    if(fd > 0) {
        close(fd);
    }
    if(c) {
        free(c);
    }
    return rc;
}

int fsd_map_set(fsd_map_t *map, const char *key,
                size_t key_len, uint64_t value) {
    int rc;
    uint32_t hash;
    uint16_t bucket_idx;
    fsd_map_bucket_t *bucket, *buckets;
    fsd_map_cell_t *cell, *cells;

    MurmurHash3_x86_32(key, key_len, MURMUR_SEED, &hash);
    bucket_idx = hash % map->nbuckets;

    rc = pthread_mutex_lock(&map->lock);
    if(rc != 0) {
        rc = FSD_MAP_ERR_LOCK;
        goto error;
    }
    buckets = (fsd_map_bucket_t *)((uint8_t *)map->mmap + sizeof(fsd_map_hdr));
    bucket = &buckets[bucket_idx];

    // TODO: Check to make sure the cell is not full

    cells = (fsd_map_cell_t *)bucket++;
    cell = &cells[bucket->n_full_cells];
    bucket->n_full_cells++;

    cell->hash = hash;
    cell->value = value;

    pthread_mutex_unlock(&map->lock);
    return FSD_MAP_OK;

error:
    pthread_mutex_unlock(&map->lock);
    return rc;
}
int fsd_map_get(fsd_map_t *map, const char *key,
                size_t key_len, uint64_t *value) {
    int rc, i;
    uint32_t hash;
    uint16_t bucket_idx;
    fsd_map_bucket_t *bucket, *buckets;
    fsd_map_cell_t *cell, *cells;

    MurmurHash3_x86_32(key, key_len, MURMUR_SEED, &hash);
    bucket_idx = hash % map->nbuckets;

    rc = pthread_mutex_lock(&map->lock);
    if(rc != 0) {
        rc = FSD_MAP_ERR_LOCK;
        goto error;
    }
    buckets = (fsd_map_bucket_t *)(map->mmap + sizeof(fsd_map_hdr));
    bucket = &buckets[bucket_idx];

    cells = (fsd_map_cell_t *)bucket++;

    for(i = 0; i < bucket->n_full_cells; i++) {
        cell = &cells[i];
        if(cell->hash == hash) {
            *value = cell->value;
            pthread_mutex_unlock(&map->lock);
            return FSD_MAP_OK;
        }
    }

    pthread_mutex_unlock(&map->lock);
    return FSD_MAP_NOT_FOUND;

error:
    pthread_mutex_unlock(&map->lock);
    return rc;
}

void fsd_map_close(fsd_map_t *map) {
    if(map->mmap) {
        munmap(map->mmap, map->mmap_len);
    }
}
