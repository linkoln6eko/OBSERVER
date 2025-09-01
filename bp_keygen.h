
#ifndef BP_KEYGEN_H
#define BP_KEYGEN_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Generate n random bytes into out (returns 0 on success, -1 on failure). */
int bp_gen_bytes(size_t n, uint8_t *out);

/* Generate a HEX key of hex_len characters (10/32/64). Uppercase HEX.
   Writes a NUL-terminated string into out_hex of size out_sz.
   Returns 0 on success, -1 on bad length or RNG failure. */
int bp_gen_hex(int hex_len, char *out_hex, size_t out_sz);

#ifdef __cplusplus
}
#endif

#endif /* BP_KEYGEN_H */
