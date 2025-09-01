#include "bp_runtime.h"
#include "bp_keygen.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

/* ======================= Internal state ======================= */
static uint8_t  g_key[32];
static size_t   g_key_len = 0;           /* 0, 5, 16, 32 */
static uint8_t  g_sf_pos[2] = {0,0};     /* 0..17 */
static uint64_t g_sf_idx[2] = {0,0};
static int      g_enabled = 1;
static int      g_debug   = 0;
static int      g_inited  = 0;

/* Superframe-based auto-rekey */
static int      g_auto_rekey  = 0;
static int      g_auto_hexlen = 64;

/* Periodic (wall-clock) rekey */
static int      g_periodic_on          = 0;
static int      g_periodic_hexlen      = 64;
static int      g_periodic_interval_ms = 0;
static uint64_t g_last_rekey_ms        = 0;   /* time of ACTUAL rekey */

/* [ADDED] Разделили «время попытки» и «время реального rekey» */
static uint64_t g_last_rekey_attempt_ms = 0;  /* [ADDED] time of last periodic check */

/* ======================= Autotune state ======================= */
static uint8_t  g_base_key[32];
static int      g_have_base = 0;
static uint8_t  g_mut_mask[32];
static int      g_autotune_on = 1;

static double   g_err_ewma[2] = {0.0, 0.0};
static int      g_err_init[2] = {0,0};
static uint64_t g_next_action_at_ms[2] = {0,0};
static int      g_mut_count_since_commit[2] = {0,0};

static int      g_th_high     = 30;
static int      g_th_midlow   = 10;
static int      g_cooldown_ms = 500;
static int      g_improve_min = 3;

/* Rekey gate params */
static int      g_gate_periodic = 1;
static int      g_gate_stale_sec = 30;
static int      g_gate_grace_ms  = 3000;
static int      g_gate_kbad      = 2;
static uint64_t g_last_action_ms = 0;
static int      g_consec_bad[2]  = {0,0};

static uint8_t  g_stage_key[32];
static int      g_has_stage = 0;
static double   g_last_ewma_after_mut[2] = {0.0,0.0};
static int      g_has_last_mut[2] = {0,0};

static uint64_t g_key_seq = 0;

/* ======================= helpers ======================= */
static uint64_t now_ms(void){
  struct timeval tv; gettimeofday(&tv, NULL);
  return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)(tv.tv_usec/1000);
}

static int hex2nibble(char c){
  if (c>='0' && c<='9') return c-'0';
  if (c>='a' && c<='f') return 10 + (c-'a');
  if (c>='A' && c<='F') return 10 + (c-'A');
  return -1;
}

static int parse_hex(const char* s, uint8_t* out, size_t* out_len){
  if (!s || !out || !out_len) return -1;
  size_t n = strlen(s);
  size_t blen = 0;
  if (n==10) blen = 5;
  else if (n==32) blen = 16;
  else if (n==64) blen = 32;
  else return -1;
  for (size_t i=0;i<blen;i++){
    int hi = hex2nibble(s[2*i]);
    int lo = hex2nibble(s[2*i+1]);
    if (hi<0 || lo<0) return -1;
    out[i] = (uint8_t)((hi<<4)|lo);
  }
  *out_len = blen;
  return 0;
}

static char nibble2hex(int v){ return (v<10)?('0'+v):('A'+(v-10)); }

static int bytes_to_hex(const uint8_t* in, size_t len, char* out, size_t outsz){
  size_t need = len*2 + 1;
  if (!out || outsz < need) return -1;
  for (size_t i=0;i<len;i++){
    out[2*i]   = nibble2hex((in[i]>>4)&0xF);
    out[2*i+1] = nibble2hex(in[i]&0xF);
  }
  out[len*2] = 0;
  return (int)(len*2);
}

static void set_key_from_bytes(const uint8_t* k, size_t len, int also_set_base){
  if (!k || (len!=5 && len!=16 && len!=32)) return;
  memcpy(g_key, k, len);
  g_key_len = len;
  if (also_set_base){
    memcpy(g_base_key, k, len);
    g_have_base = 1;
  }
  memset(g_mut_mask, 0, sizeof(g_mut_mask));
  g_has_stage = 0;
  g_key_seq++;
  g_last_action_ms = now_ms();
}

/* commit mutated bytes into base key */
static void commit_mutations(void){
  if (!g_have_base || !g_has_stage) return;
  memcpy(g_base_key, g_stage_key, g_key_len);
  g_have_base = 1;
  memset(g_mut_mask, 0, sizeof(g_mut_mask));
  g_has_stage = 0;
  g_key_seq++;
  g_last_action_ms = now_ms();
}

static void gen_keystream_bits(uint64_t pos, uint64_t* out_r, int* out_bits){
  uint64_t s = 0x9E3779B97F4A7C15ULL ^ pos;
  for (size_t i=0;i<g_key_len;i++){
    s ^= (uint64_t)g_base_key[i] << ((i%8)*8);
  }
  if (g_has_stage){
    for (size_t i=0;i<g_key_len;i++){
      if (g_mut_mask[i]){
        s ^= ((uint64_t)g_stage_key[i]) << ((i%8)*8);
      }
    }
  }
  s ^= (s >> 30); s *= 0xbf58476d1ce4e5b9ULL;
  s ^= (s >> 27); s *= 0x94d049bb133111ebULL;
  s ^= (s >> 31);
  *out_r = s;
  *out_bits = 64;
}

/* ======================= public ======================= */

void bp_runtime_init(void){
  if (g_inited) return;
  g_inited = 1;

  const char* env = getenv("HYT_BP_KEY");
  if (env){
    uint8_t b[32]; size_t bl=0;
    if (parse_hex(env, b, &bl)==0){
      set_key_from_bytes(b, bl, 1);
      if (g_debug){
        char hex[65]; bytes_to_hex(g_key, g_key_len, hex, sizeof(hex));
        fprintf(stderr, "[BP] env key loaded (%zu bytes) -> %s\n", bl, hex);
      }
    }
  }
}

int bp_set_key_hex(const char* hex){
  uint8_t b[32]; size_t bl=0;
  if (parse_hex(hex, b, &bl) != 0) return -1;
  set_key_from_bytes(b, bl, 1);
  if (g_debug){
    fprintf(stderr, "[BP] set-key (len=%zu)\n", bl);
  }
  return 0;
}

int bp_get_key_hex(char* out, size_t outsz){
  if (g_key_len==0) return 0;
  return bytes_to_hex(g_key, g_key_len, out, outsz);
}

uint64_t bp_get_key_seq(void){ return g_key_seq; }

void bp_reset_superframe_for_slot(int slot){
  if (slot!=0 && slot!=1) return;
  g_sf_pos[slot] = 0;
}

void bp_enable_auto_rekey(int on, int hex_len){
  g_auto_rekey = on?1:0;
  g_auto_hexlen = (hex_len==10||hex_len==32||hex_len==64)?hex_len:64;
  if (g_debug) fprintf(stderr, "[BP] auto-rekey: %s (%d)\n", g_auto_rekey?"ON":"OFF", g_auto_hexlen);
}

/* rekey using project RNG */
void bp_rekey_now_random(int hex_len){
  size_t bl = (hex_len==10?5:(hex_len==32?16:32));
  uint8_t tmp[32];
  if (bp_gen_bytes(bl, tmp) != 0) return;
  set_key_from_bytes(tmp, bl, 1);
  g_last_rekey_ms = now_ms(); /* [CHANGED] фиксируем только при реальном rekey */
  if (g_debug){
    char out[65]; bytes_to_hex(g_key, bl, out, sizeof(out));
    fprintf(stderr, "[BP] rekey-now: len=%zu key=%s\n", bl, out);
  }
}

void bp_enable_periodic_rekey(int on, int interval_ms, int hex_len){
  g_periodic_on = on?1:0;
  g_periodic_interval_ms = interval_ms>0?interval_ms:0;
  g_periodic_hexlen = (hex_len==10||hex_len==32||hex_len==64)?hex_len:64;
  /* [ADDED] сбрасываем только таймер попыток; g_last_rekey_ms не трогаем */
  g_last_rekey_attempt_ms = 0; /* [ADDED] */
  if (g_debug) fprintf(stderr, "[BP] periodic-rekey: %s every=%dms len=%d\n",
                       g_periodic_on?"ON":"OFF", g_periodic_interval_ms, g_periodic_hexlen);
}

/* ======================= Autotune API ======================= */
void bp_autotune_enable(int on){ g_autotune_on = on?1:0; }
void bp_autotune_params(int th_high, int th_midlow, int cooldown_ms, int improve_min){
  if (th_high>=0)     g_th_high = th_high;
  if (th_midlow>=0)   g_th_midlow = th_midlow;
  if (cooldown_ms>=0) g_cooldown_ms = cooldown_ms;
  if (improve_min>=0) g_improve_min = improve_min;
}
void bp_rekey_gate_enable(int on){ g_gate_periodic = on?1:0; }
void bp_rekey_gate_params(int stale_sec, int grace_ms, int kbad_required){
  if (stale_sec >= 0)   g_gate_stale_sec = stale_sec;
  if (grace_ms >= 0)    g_gate_grace_ms  = grace_ms;
  if (kbad_required>=0) g_gate_kbad      = kbad_required;
}

/* CLI-алиасы */
void bp_set_autotune(int on){ bp_autotune_enable(on); }
void bp_set_rekey_gate(int on){ bp_rekey_gate_enable(on); }

/* простая мутация */
static void mutate_once(int slot){
  if (g_key_len==0) return;
  if (!g_have_base){ memcpy(g_base_key, g_key, g_key_len); g_have_base = 1; }
  memcpy(g_stage_key, g_base_key, g_key_len);
  uint64_t seed = g_sf_idx[slot] ^ ((uint64_t)slot<<32) ^ (g_key_seq<<1);
  size_t idx = (size_t)(seed % g_key_len);
  uint8_t from = g_stage_key[idx];
  uint8_t bit = (uint8_t)(1u << (seed % 8));
  uint8_t to = (uint8_t)(from ^ bit);
  g_stage_key[idx] = to;
  g_mut_mask[idx] = 1;
  g_has_stage = 1;
  memcpy(g_key, g_stage_key, g_key_len);  /* применять staged сразу */
  g_last_action_ms = now_ms();
  if (g_debug) fprintf(stderr, "[BP] MUTATE idx=%zu from=0x%02X to=0x%02X\n", idx, from, to);
}

void bp_quality_update(int slot, int error_level){
  if (!g_autotune_on || (slot!=0 && slot!=1)) return;
  uint64_t t = now_ms();

  double alpha = 0.30;
  if (!g_err_init[slot]){
    g_err_ewma[slot] = (double)error_level;
    g_err_init[slot] = 1;
  } else {
    g_err_ewma[slot] = alpha*(double)error_level + (1.0-alpha)*g_err_ewma[slot];
  }

  if ((int)g_err_ewma[slot] >= g_th_high) g_consec_bad[slot]++; else g_consec_bad[slot]=0;

  if (t < g_next_action_at_ms[slot]) return;

  if ((int)g_err_ewma[slot] >= g_th_high){
    if ((int)(t - g_last_rekey_ms) >= g_gate_grace_ms){
      bp_rekey_now_random((int)(g_key_len?g_key_len*2:64));
      g_next_action_at_ms[slot] = t + g_cooldown_ms;
      if (g_debug) fprintf(stderr, "[BP] autotune: REKEY (slot=%d, ewma=%.1f >= %d)\n", slot, g_err_ewma[slot], g_th_high);
    }
    return;
  }

  if (g_has_last_mut[slot]){
    double delta = g_last_ewma_after_mut[slot] - g_err_ewma[slot];
    if (delta >= (double)g_improve_min){
      commit_mutations();
      g_mut_count_since_commit[slot] = 0;
      g_has_last_mut[slot] = 0;
      g_next_action_at_ms[slot] = t + g_cooldown_ms;
      if (g_debug) fprintf(stderr, "[BP] autotune: COMMIT (slot=%d, Δewma=%.1f)\n", slot, delta);
      return;
    } else {
      if (g_debug) fprintf(stderr, "[BP] autotune: rollback (slot=%d, Δewma=%.1f)\n", slot, delta);
      memcpy(g_key, g_base_key, g_key_len);
      memset(g_mut_mask, 0, sizeof(g_mut_mask));
      g_has_last_mut[slot] = 0;
      g_next_action_at_ms[slot] = t + g_cooldown_ms;
      g_last_action_ms = t;
      return;
    }
  }

  if ((int)g_err_ewma[slot] >= g_th_midlow){
    mutate_once(slot);
    g_last_ewma_after_mut[slot] = g_err_ewma[slot];
    g_has_last_mut[slot] = 1;
    g_mut_count_since_commit[slot]++;
    g_next_action_at_ms[slot] = t + g_cooldown_ms;
    if (g_debug) fprintf(stderr, "[BP] autotune: MUTATE (slot=%d, ewma=%.1f)\n", slot, g_err_ewma[slot]);
  }
}

/* ======================= XOR path ======================= */
void bp_on_air_xor(char fr[4][24], int slot){
  if (!g_enabled || (g_key_len==0)) return;
  if (slot!=0 && slot!=1) slot = 0;

  if (g_auto_rekey && g_sf_pos[slot] == 0){
    bp_rekey_now_random(g_auto_hexlen);
    if (g_debug) fprintf(stderr, "[BP] auto-rekey (slot=%d, len=%d)\n", slot, g_auto_hexlen);
  }

  /* [CHANGED] Периодический rekey: используем таймер ПОПЫТОК, а не last_rekey_ms */
  if (g_periodic_on && g_periodic_interval_ms > 0){
    uint64_t t = now_ms();
    if (t - g_last_rekey_attempt_ms >= (uint64_t)g_periodic_interval_ms){
      g_last_rekey_attempt_ms = t; /* [ADDED] сбрасываем окно только попыток */

      int allow = 1;
      if (g_gate_periodic){
        allow = 0;
        int kbad_ok = (g_consec_bad[slot] >= g_gate_kbad);
        int stale_ok = ((int)((t - g_last_action_ms)/1000) >= g_gate_stale_sec);
        int grace_ok = ((int)(t - g_last_rekey_ms) >= g_gate_grace_ms);
        if (grace_ok && (kbad_ok || stale_ok)) allow = 1;
      }

      if (allow){
        if (g_debug) fprintf(stderr, "[BP] periodic-rekey (len=%d, every=%d ms)\n", g_periodic_hexlen, g_periodic_interval_ms);
        bp_rekey_now_random(g_periodic_hexlen); /* [CHANGED] last_rekey_ms обновится ТОЛЬКО здесь */
      } else {
        if (g_debug) fprintf(stderr, "[BP] periodic-rekey: gated (not-bad-enough)\n");
        /* [CHANGED] НЕ трогаем g_last_rekey_ms — чтобы не блокировать будущий rekey */
      }
    }
  }

  int bits = 0;
  uint64_t r=0; int rbits=0;
  for (int row=0; row<3; ++row){
    for (int col=0; col<24; ++col){
      if (rbits == 0){
        uint64_t pos = (g_sf_idx[slot]<<8) | g_sf_pos[slot];
        gen_keystream_bits(pos, &r, &rbits);
      }
      int kb = (int)((r >> 63) & 1u); r <<= 1; rbits--;
      fr[row][col] = (char)((fr[row][col] ^ kb) & 1);
      ++bits;
    }
  }
  g_sf_pos[slot] = (uint8_t)((g_sf_pos[slot] + 1) % 18);
  if (g_sf_pos[slot] == 0) g_sf_idx[slot]++;
  if (g_debug && g_sf_pos[slot] == 1){
    fprintf(stderr, "[BP] slot=%d idx=%llu keylen=%lu\n", slot, (unsigned long long)g_sf_idx[slot], (unsigned long)g_key_len);
  }
}

void bp_set_enabled(int on){ g_enabled = on ? 1 : 0; }
int  bp_get_enabled(void){ return g_enabled; }
void bp_set_debug(int on){ g_debug = on ? 1 : 0; }
int  bp_get_debug(void){ return g_debug; }
