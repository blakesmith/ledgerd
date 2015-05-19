#ifndef LIB_LEDGER_CRC32_H
#define LIB_LEDGER_CRC32_H

unsigned long crc32_compute(unsigned long inCrc32, const void *buf,
                            size_t bufLen);

#endif
