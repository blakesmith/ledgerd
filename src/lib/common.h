#ifndef LIB_LEDGER_COMMON_H
#define LIB_LEDGER_COMMON_H

#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

static __thread const char *ledger_error_message;

typedef enum {
    LEDGER_OK = 0,
    LEDGER_NEXT = -1,
    LEDGER_ERR_GENERAL = -2,
    LEDGER_ERR_MEMORY = -3,
    LEDGER_ERR_MKDIR = -4,
    LEDGER_ERR_ARGS = -5,
    LEDGER_ERR_BAD_TOPIC = -6,
    LEDGER_ERR_BAD_PARTITION = -7,
    LEDGER_ERR_BAD_META = -8,
    LEDGER_ERR_BAD_LOCKFILE = -9,
    LEDGER_ERR_IO = -10,
    LEDGER_ERR_POSITION_NOT_FOUND = -11,
    LEDGER_ERR_CONSUMER = -12
} ledger_status;

#define ledger_check_rc(C, R, M) if(!(C)) {     \
        rc = R;                                 \
        ledger_error_message = M;               \
        goto error;                             \
    }

ssize_t ledger_concat_path(const char *s1, const char *s2, char **out);
void *ledger_reallocarray(void *ptr, size_t nmemb, size_t size);
int ledger_pwrite(int fd, const void *buf, size_t count, off_t offset);
int ledger_pread(int fd, void *buf, size_t count, off_t offset);
const char *ledger_get_error();

#endif
