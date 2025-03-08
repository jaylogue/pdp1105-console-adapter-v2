#ifndef CRC32_H
#define CRC32_H

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t crc32(const uint8_t * buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif // CRC32_H
