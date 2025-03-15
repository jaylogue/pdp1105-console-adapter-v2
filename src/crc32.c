#include "crc32.h"

/* This is the basic CRC-32 calculation with some optimization but no
   table lookup. The the byte reversal is avoided by shifting the crc reg
   right instead of left and by using a reversed 32-bit word to represent
   the polynomial.
   When compiled to Cyclops with GCC, this function executes in 8 + 72n
   instructions, where n is the number of bytes in the input message. It
   should be doable in 4 + 61n instructions.
   If the inner loop is strung out (approx. 5*8 = 40 instructions),
   it would take about 6 + 46n instructions.
 */

uint32_t crc32(const uint8_t * buf, size_t len)
{
    int i, j;
    uint32_t byte, crc, mask;
 
    i = 0;
    crc = 0xFFFFFFFF;
    while (i < len) {
       byte = buf[i];            // Get next byte.
       crc = crc ^ byte;
       for (j = 7; j >= 0; j--) {    // Do eight times.
          mask = -(crc & 1);
          crc = (crc >> 1) ^ (0xEDB88320 & mask);
       }
       i = i + 1;
    }
    return ~crc;
}
