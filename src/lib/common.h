#ifndef LIB_LEDGER_COMMON_H
#define LIB_LEDGER_COMMON_H

#include <unistd.h>

typedef enum {
    LEDGER_OK = 0,
    LEDGER_ERR_GENERAL = -1,
    LEDGER_ERR_MEMORY = -2,
    LEDGER_ERR_MKDIR = -3,
    LEDGER_ERR_ARGS = -4,
    LEDGER_ERR_BAD_TOPIC = -5,
    LEDGER_ERR_BAD_PARTITION = -6
} ledger_status;

#define ledger_check_rc(C, R, M) if(!(C)) { \
    rc = R; \
    goto error; \
}

ssize_t concat_path(const char *s1, const char *s2, char **out);
void *reallocarray(void *ptr, size_t nmemb, size_t size);
int ledger_pwrite(int fd, const void *buf, size_t count, off_t offset);

#endif
