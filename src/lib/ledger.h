#ifndef LIB_LEDGER_H
#define LIB_LEDGER_H

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
  LEDGER_OK = 0
} ledger_status;

ledger_status ledger_open_context(const char *directory);

#if defined(__cplusplus)
}
#endif
#endif
