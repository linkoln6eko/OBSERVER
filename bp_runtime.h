#ifndef BP_RUNTIME_H
#define BP_RUNTIME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize runtime once; may be called repeatedly. Can read HYT_BP_KEY env. */
void bp_runtime_init(void);

/* Set/replace key from HEX string (10/32/64 chars -> 5/16/32 bytes).
   Returns 0 on success, -1 on parse error. */
int  bp_set_key_hex(const char* hex);

/* Get current key as HEX string.
   Returns number of hex chars written (10/32/64), 0 if no key, or -1 if buffer too small. */
int  bp_get_key_hex(char* out, size_t outsz);

/* Monotonic sequence incremented on each successful key set/rekey/commit. */
uint64_t bp_get_key_seq(void);

/* Notify start of a new superframe for given slot (0/1). */
void bp_reset_superframe_for_slot(int slot);

/* Auto rekey at start of each superframe for active slot.
   hex_len must be 10|32|64 (defaults to 64 if invalid). */
void bp_enable_auto_rekey(int on, int hex_len);

/* Force immediate random rekey once (HEX length 10|32|64). */
void bp_rekey_now_random(int hex_len);

/* Enable/disable periodic rekey by wall time (milliseconds).
   When enabled, a new random key is generated and applied when at least
   interval_ms milliseconds have passed since the last rekey.
   hex_len must be 10|32|64 (defaults to 64 if invalid). */
void bp_enable_periodic_rekey(int on, int interval_ms, int hex_len);

/* Apply on-air XOR to 4x24 AMBE/IMBE frame matrix (values 0/1). */
void bp_on_air_xor(char fr[4][24], int slot);

/* ===================== [ADDED] AUTOTUNE ===================== */

/* [ADDED] Enable/disable automatic key autotune (mutate/rekey/commit on error feedback). */
void bp_autotune_enable(int on);

/* [ADDED] Feed current error level (per slot). The runtime applies:
   - REKEY if ewma >= th_high;
   - MUTATE if th_midlow <= ewma < th_high;
   - COMMIT if ewma dropped by >= improve_min since last MUTATE;
   - otherwise HOLD. */
void bp_quality_update(int slot, int error_level);

/* [ADDED] Parameter tuning (all values >=0; use negative to keep current):
   th_high     : error EWMA threshold for REKEY (very bad),
   th_midlow   : boundary between "medium" (mutate) and "low" (commit/lock),
   cooldown_ms : minimal interval between actions,
   improve_min : minimal EWMA improvement to treat as "drop" after a mutation. */
void bp_autotune_params(int th_high, int th_midlow, int cooldown_ms, int improve_min);

/* [ADDED] Gate periodic rekey by quality & activity:
   - when enabled, periodic rekey will be suppressed unless:
     (ewma >= th_high for kbad_required consecutive updates) OR
     (no autotune actions for >= stale_sec seconds) AND
     (grace_ms elapsed since the last rekey).
*/
void bp_rekey_gate_enable(int on);
/* [ADDED] */
void bp_rekey_gate_params(int stale_sec, int grace_ms, int kbad_required);

/* Optional enable/disable of XOR. */
void bp_set_enabled(int on);
int  bp_get_enabled(void);

/* Optional debug prints to stderr. */
void bp_set_debug(int on);
int  bp_get_debug(void);

/* ===================== [ADDED] Back-compat wrappers for CLI in dsd_main.c ===================== */
/* [ADDED] keep CLI flags in dsd_main.c working without touching that file */
void bp_set_autotune(int on);     /* alias to bp_autotune_enable(on) */
void bp_set_rekey_gate(int on);   /* alias to bp_rekey_gate_enable(on) */

#ifdef __cplusplus
}
#endif

#endif /* BP_RUNTIME_H */
