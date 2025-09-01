
#include "bp_keygen.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(__APPLE__) || defined(__OpenBSD__) || defined(__FreeBSD__)
  #define BP_HAVE_ARC4RANDOM 1
  #include <stdlib.h>
#else
  #include <stdlib.h>
  #include <fcntl.h>
  #include <unistd.h>
#endif

static const char HEX_UP[16] = {
  '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
};

int bp_gen_bytes(size_t n, uint8_t *out){
  if (!out || n == 0) return -1;
#if BP_HAVE_ARC4RANDOM
  arc4random_buf(out, n);
  return 0;
#else
  int fd = open("/dev/urandom", O_RDONLY);
  if (fd >= 0){
    size_t got = 0;
    while (got < n){
      ssize_t r = read(fd, out + got, n - got);
      if (r <= 0) break;
      got += (size_t)r;
    }
    close(fd);
    if (got == n) return 0;
  }
  /* last resort: weak PRNG (not recommended, but prevents total failure) */
  unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)getpid();
  srand(seed);
  for (size_t i=0;i<n;i++) out[i] = (uint8_t)(rand() & 0xFF);
  return 0;
#endif
}

int bp_gen_hex(int hex_len, char *out_hex, size_t out_sz){
  size_t need_bytes = 0;
  if (hex_len == 10) need_bytes = 5;
  else if (hex_len == 32) need_bytes = 16;
  else if (hex_len == 64) need_bytes = 32;
  else return -1;

  size_t need_hex = (size_t)hex_len;
  if (!out_hex || out_sz < need_hex + 1) return -1;

  uint8_t tmp[32];
  if (bp_gen_bytes(need_bytes, tmp) != 0) return -1;

  for (size_t i=0, j=0; i<need_bytes; ++i){
    out_hex[j++] = HEX_UP[(tmp[i] >> 4) & 0xF];
    out_hex[j++] = HEX_UP[(tmp[i]     ) & 0xF];
  }
  out_hex[need_hex] = '\0';
  return 0;
}
