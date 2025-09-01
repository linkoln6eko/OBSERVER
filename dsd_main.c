//DSD MAIN
#define _MAIN

#include "dsd.h"
#include "p25p1_const.h"
#include "x2tdma_const.h"
#include "dstar_const.h"
#include "nxdn_const.h"
#include "dmr_const.h"
#include "provoice_const.h"
#include "p25p1_heuristics.h"
#include "pa_devs.h"
#include <stdio.h>
#include "bp_runtime.h"
#include "bp_keygen.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h> // [ADDED] for sigfun prototype on some systems

int exitflag = 0;

int
comp (const void *a, const void *b)
{
  if (*((const int *) a) == *((const int *) b))
    return 0;
  else if (*((const int *) a) < *((const int *) b))
    return -1;
  else
    return 1;
}

void
noCarrier (dsd_opts * opts, dsd_state * state)
{
  state->dibit_buf_p = state->dibit_buf + 200;
  memset (state->dibit_buf, 0, sizeof (int) * 200);
  if (opts->mbe_out_f != NULL)
    {
      closeMbeOutFile (opts, state);
    }
  state->jitter = -1;
  state->lastsynctype = -1;
  state->carrier = 0;
  state->max = 15000;
  state->min = -15000;
  state->center = 0;
  state->err_str[0] = 0;
  sprintf (state->fsubtype, "              ");
  sprintf (state->ftype, "             ");
  state->errs = 0;
  state->errs2 = 0;
  state->lasttg = 0;
  state->lastsrc = 0;
  state->lastp25type = 0;
  state->repeat = 0;
  state->nac = 0;
  state->numtdulc = 0;
  sprintf (state->slot0light, " slot0 ");
  sprintf (state->slot1light, " slot1 ");
  state->firstframe = 0;
  if (opts->audio_gain == (float) 0)
    {
      state->aout_gain = 25;
    }
  memset (state->aout_max_buf, 0, sizeof (float) * 200);
  state->aout_max_buf_p = state->aout_max_buf;
  state->aout_max_buf_idx = 0;
  sprintf (state->algid, "________");
  sprintf (state->keyid, "________________");
  mbe_initMbeParms (state->cur_mp, state->prev_mp, state->prev_mp_enhanced);
}

void
initOpts (dsd_opts * opts)
{

  opts->onesymbol = 10;
  opts->mbe_in_file[0] = 0;
  opts->mbe_in_f = NULL;
  opts->errorbars = 1;
  opts->datascope = 0;
  opts->symboltiming = 0;
  opts->verbose = 2;
  opts->p25enc = 0;
  opts->p25lc = 0;
  opts->p25status = 0;
  opts->p25tg = 0;
  opts->scoperate = 15;
  sprintf (opts->audio_in_dev, "/dev/audio");
  opts->audio_in_fd = -1;
#ifdef USE_PORTAUDIO
  opts->audio_in_pa_stream = NULL;
#endif
  sprintf (opts->audio_out_dev, "/dev/audio");
  opts->audio_out_fd = -1;
#ifdef USE_PORTAUDIO
  opts->audio_out_pa_stream = NULL;
#endif
  opts->split = 0;
  opts->playoffset = 0;
  opts->mbe_out_dir[0] = 0;
  opts->mbe_out_file[0] = 0;
  opts->mbe_out_path[0] = 0;
  opts->mbe_out_f = NULL;
  opts->audio_gain = 0;
  opts->audio_out = 1;
  opts->wav_out_file[0] = 0;
  opts->wav_out_f = NULL;
  //opts->wav_out_fd = -1;
  opts->serial_baud = 115200;
  sprintf (opts->serial_dev, "/dev/ttyUSB0");
  opts->resume = 0;
  opts->frame_dstar = 0;
  opts->frame_x2tdma = 1;
  opts->frame_p25p1 = 1;
  opts->frame_nxdn48 = 0;
  opts->frame_nxdn96 = 1;
  opts->frame_dmr = 1;
  opts->frame_provoice = 0;
  opts->mod_c4fm = 1;
  opts->mod_qpsk = 1;
  opts->mod_gfsk = 1;
  opts->uvquality = 3;
  opts->inverted_x2tdma = 1;    // most transmitter + scanner + sound card combinations show inverted signals for this
  opts->inverted_dmr = 0;       // most transmitter + scanner + sound card combinations show non-inverted signals for this
  opts->mod_threshold = 26;
  opts->ssize = 36;
  opts->msize = 15;
  opts->playfiles = 0;
  opts->delay = 0;
  opts->use_cosine_filter = 1;
  opts->unmute_encrypted_p25 = 0;
}

void
initState (dsd_state * state)
{

  int i, j;

  state->dibit_buf = malloc (sizeof (int) * 1000000);
  state->dibit_buf_p = state->dibit_buf + 200;
  memset (state->dibit_buf, 0, sizeof (int) * 200);
  state->repeat = 0;
  state->audio_out_buf = malloc (sizeof (short) * 1000000);
  memset (state->audio_out_buf, 0, 100 * sizeof (short));
  state->audio_out_buf_p = state->audio_out_buf + 100;
  state->audio_out_float_buf = malloc (sizeof (float) * 1000000);
  memset (state->audio_out_float_buf, 0, 100 * sizeof (float));
  state->audio_out_float_buf_p = state->audio_out_float_buf + 100;
  state->audio_out_idx = 0;
  state->audio_out_idx2 = 0;
  state->audio_out_temp_buf_p = state->audio_out_temp_buf;
  //state->wav_out_bytes = 0;
  state->center = 0;
  state->jitter = -1;
  state->synctype = -1;
  state->min = -15000;
  state->max = 15000;
  state->lmid = 0;
  state->umid = 0;
  state->minref = -12000;
  state->maxref = 12000;
  state->lastsample = 0;
  for (i = 0; i < 128; i++)
    {
      state->sbuf[i] = 0;
    }
  state->sidx = 0;
  for (i = 0; i < 1024; i++)
    {
      state->maxbuf[i] = 15000;
    }
  for (i = 0; i < 1024; i++)
    {
      state->minbuf[i] = -15000;
    }
  state->midx = 0;
  state->err_str[0] = 0;
  sprintf (state->fsubtype, "              ");
  sprintf (state->ftype, "             ");
  state->symbolcnt = 0;
  state->rf_mod = 0;
  state->numflips = 0;
  state->lastsynctype = -1;
  state->lastp25type = 0;
  state->offset = 0;
  state->carrier = 0;
  for (i = 0; i < 25; i++)
    {
      for (j = 0; j < 16; j++)
        {
          state->tg[i][j] = 48;
        }
    }
  state->tgcount = 0;
  state->lasttg = 0;
  state->lastsrc = 0;
  state->nac = 0;
  state->errs = 0;
  state->errs2 = 0;
  state->mbe_file_type = -1;
  state->optind = 0;
  state->numtdulc = 0;
  state->firstframe = 0;
  sprintf (state->slot0light, " slot0 ");
  sprintf (state->slot1light, " slot1 ");
  state->aout_gain = 25;
  memset (state->aout_max_buf, 0, sizeof (float) * 200);
  state->aout_max_buf_p = state->aout_max_buf;
  state->aout_max_buf_idx = 0;
  state->samplesPerSymbol = 10;
  state->symbolCenter = 4;
  sprintf (state->algid, "________");
  sprintf (state->keyid, "________________");
  state->currentslot = 0;
  state->cur_mp = malloc (sizeof (mbe_parms));
  state->prev_mp = malloc (sizeof (mbe_parms));
  state->prev_mp_enhanced = malloc (sizeof (mbe_parms));
  mbe_initMbeParms (state->cur_mp, state->prev_mp, state->prev_mp_enhanced);
  state->p25kid = 0;

  state->debug_audio_errors = 0;
  state->debug_header_errors = 0;
  state->debug_header_critical_errors = 0;

  state->last_dibit = 0;

#ifdef TRACE_DSD
  state->debug_sample_index = 0;
  state->debug_label_file = NULL;
  state->debug_label_dibit_file = NULL;
  state->debug_label_imbe_file = NULL;
#endif

  initialize_p25_heuristics(&state->p25_heuristics);
}

// --- Hytera BP key CLI consumer -------------------------------------------
// Removes --hyt-bp-key / --bp-key / --bp-gen from argv and writes HYT_BP_KEY.
// [CHANGED] Добавлен разбор новых BP-опций автотюна/порогов/логов/гейта.
static void bp_consume_cli_key(int *argc, char **argv) {
  int in = 1, out = 1;
  while (in < *argc) {
    char *a = argv[in];
    if (a && strncmp(a, "--hyt-bp-key=", 13) == 0) {
      setenv("HYT_BP_KEY", a + 13, 1);
      in++;
    } else if (a && (strcmp(a, "--bp-gen-loop") == 0 || strncmp(a, "--bp-gen-loop=", 14) == 0)) {
      int hexlen = 64;
      if (strncmp(a, "--bp-gen-loop=", 14) == 0) {
        int maybe = atoi(a + 14);
        if (maybe == 10 || maybe == 32 || maybe == 64) hexlen = maybe;
      } else if ((in + 1) < *argc) {
        int maybe = atoi(argv[in + 1]);
        if (maybe == 10 || maybe == 32 || maybe == 64) { hexlen = maybe; in++; }
      }
      bp_enable_auto_rekey(1, hexlen);
      in++;
    } else if (a && strcmp(a, "--hyt-bp-key") == 0 && (in + 1) < *argc) {
      setenv("HYT_BP_KEY", argv[in + 1], 1);
      in += 2;
    } else if (a && strncmp(a, "--bp-key=", 9) == 0) {
      setenv("HYT_BP_KEY", a + 9, 1);
      in++;
    } else if (a && strcmp(a, "--bp-key") == 0 && (in + 1) < *argc) {
      setenv("HYT_BP_KEY", argv[in + 1], 1);
      in += 2;
    } else if (a && strncmp(a, "--bp-gen=", 9) == 0) {
      int hexlen = atoi(a + 9);
      if (hexlen != 10 && hexlen != 32 && hexlen != 64) hexlen = 64;
      char hex[65];
      if (bp_gen_hex(hexlen, hex, sizeof(hex)) == 0) {
        setenv("HYT_BP_KEY", hex, 1);
        fprintf(stderr, "[BP] generated key (%d hex): %s\n", hexlen, hex);
      }
      in++;
    } else if (a && strcmp(a, "--bp-gen") == 0) {
      int hexlen = 64;
      if ((in + 1) < *argc) {
        int maybe = atoi(argv[in + 1]);
        if (maybe == 10 || maybe == 32 || maybe == 64) { hexlen = maybe; in++; }
      }
      char hex[65];
      if (bp_gen_hex(hexlen, hex, sizeof(hex)) == 0) {
        setenv("HYT_BP_KEY", hex, 1);
        fprintf(stderr, "[BP] generated key (%d hex): %s\n", hexlen, hex);
      }
      in++;
    } else if (a && (strcmp(a, "--bp-rekey-now") == 0 || strncmp(a, "--bp-rekey-now=", 15) == 0)) {
      int hexlen = 64;
      if (strncmp(a, "--bp-rekey-now=", 15) == 0) {
        int maybe = atoi(a + 15);
        if (maybe == 10 || maybe == 32 || maybe == 64) hexlen = maybe;
      } else if ((in + 1) < *argc) {
        int maybe = atoi(argv[in + 1]);
        if (maybe == 10 || maybe == 32 || maybe == 64) { hexlen = maybe; in++; }
      }
      bp_rekey_now_random(hexlen);
      in++;
    } else if (a && (strcmp(a, "--bp-rekey-every") == 0 || strncmp(a, "--bp-rekey-every=", 18) == 0)) {
      int ms = 0;
      int hexlen = 64;
      if (strncmp(a, "--bp-rekey-every=", 18) == 0) {
        const char* p = a + 18;
        ms = atoi(p);
        const char* colon = strchr(p, ':');
        if (colon) {
          int maybe = atoi(colon + 1);
          if (maybe == 10 || maybe == 32 || maybe == 64) hexlen = maybe;
        }
      } else if ((in + 1) < *argc) {
        ms = atoi(argv[in + 1]);
        if ((in + 2) < *argc) {
          int maybe = atoi(argv[in + 2]);
          if (maybe == 10 || maybe == 32 || maybe == 64) { hexlen = maybe; in++; }
        }
        in++;
      }
      if (ms <= 0) ms = 30000; /* default 30s in ms */
      bp_enable_periodic_rekey(1, ms, hexlen);
      bp_rekey_now_random(hexlen); /* apply immediately */
      in++;
    }
    // ------------------------ [ADDED] AUTOTUNE/DEBUG/LIMITS ------------------------
    else if (a && strcmp(a, "--bp-autotune") == 0) {
      bp_set_autotune(1);
      in++;
    } else if (a && strcmp(a, "--no-bp-autotune") == 0) {
      bp_set_autotune(0);
      in++;
    } else if (a && (strcmp(a, "--bp-debug") == 0)) {
      bp_set_debug(1);
      in++;
    } else if (a && (strcmp(a, "--no-bp-debug") == 0)) {
      bp_set_debug(0);
      in++;
    } else if (a && (strcmp(a, "--bp-th-high") == 0 || strncmp(a, "--bp-th-high=", 13) == 0)) {
      int v = 0;
      if (strncmp(a, "--bp-th-high=", 13) == 0) { v = atoi(a + 13); }
      else if ((in + 1) < *argc) { v = atoi(argv[in + 1]); in++; }
      if (v > 0) bp_autotune_params(v, 0, 0, 0);
      in++;
    } else if (a && (strcmp(a, "--bp-th-midlow") == 0 || strncmp(a, "--bp-th-midlow=", 16) == 0)) {
      int v = 0;
      if (strncmp(a, "--bp-th-midlow=", 16) == 0) { v = atoi(a + 16); }
      else if ((in + 1) < *argc) { v = atoi(argv[in + 1]); in++; }
      if (v > 0) bp_autotune_params(0, v, 0, 0);
      in++;
    } else if (a && (strcmp(a, "--bp-cooldown-ms") == 0 || strncmp(a, "--bp-cooldown-ms=", 17) == 0)) {
      int v = 0;
      if (strncmp(a, "--bp-cooldown-ms=", 17) == 0) { v = atoi(a + 17); }
      else if ((in + 1) < *argc) { v = atoi(argv[in + 1]); in++; }
      if (v > 0) bp_autotune_params(0, 0, v, 0);
      in++;
    } else if (a && (strcmp(a, "--bp-improve-min") == 0 || strncmp(a, "--bp-improve-min=", 17) == 0)) {
      int v = 0;
      if (strncmp(a, "--bp-improve-min=", 17) == 0) { v = atoi(a + 17); }
      else if ((in + 1) < *argc) { v = atoi(argv[in + 1]); in++; }
      if (v > 0) bp_autotune_params(0, 0, 0, v);
      in++;
    }
    // ------------------------ [ADDED] REKEY GATE ------------------------
    else if (a && strcmp(a, "--bp-rekey-gate") == 0) {
      bp_set_rekey_gate(1);
      in++;
    } else if (a && strcmp(a, "--no-bp-rekey-gate") == 0) {
      bp_set_rekey_gate(0);
      in++;
    } else if (a && (strcmp(a, "--bp-stale-sec") == 0 || strncmp(a, "--bp-stale-sec=", 15) == 0)) {
      int v = -1;
      if (strncmp(a, "--bp-stale-sec=", 15) == 0) { v = atoi(a + 15); }
      else if ((in + 1) < *argc) { v = atoi(argv[in + 1]); in++; }
      bp_rekey_gate_params(v, -1, -1);
      in++;
    } else if (a && (strcmp(a, "--bp-rekey-grace-ms") == 0 || strncmp(a, "--bp-rekey-grace-ms=", 21) == 0)) {
      int v = -1;
      if (strncmp(a, "--bp-rekey-grace-ms=", 21) == 0) { v = atoi(a + 21); }
      else if ((in + 1) < *argc) { v = atoi(argv[in + 1]); in++; }
      bp_rekey_gate_params(-1, v, -1);
      in++;
    } else if (a && (strcmp(a, "--bp-rekey-kbad") == 0 || strncmp(a, "--bp-rekey-kbad=", 16) == 0)) {
      int v = -1;
      if (strncmp(a, "--bp-rekey-kbad=", 16) == 0) { v = atoi(a + 16); }
      else if ((in + 1) < *argc) { v = atoi(argv[in + 1]); in++; }
      bp_rekey_gate_params(-1, -1, v);
      in++;
    }
    // ------------------------------------------------------------------------------
    else {
      argv[out++] = a;
      in++;
    }
  }
  argv[out] = NULL;
  *argc = out;
}
// --------------------------------------------------------------------------

void
usage ()
{
  printf ("\n");
  printf ("Usage: dsd [options]            Live scanner mode\n");
  printf ("  or:  dsd [options] -r <files> Read/Play saved mbe data from file(s)\n");
  printf ("  or:  dsd -h                   Show help\n");
  printf ("\n");
  printf ("Display Options:\n");
  printf ("  -e            Show Frame Info and errorbars (default)\n");
  printf ("  -pe           Show P25 encryption sync bits\n");
  printf ("  -pl           Show P25 link control bits\n");
  printf ("  -ps           Show P25 status bits and low speed data\n");
  printf ("  -pt           Show P25 talkgroup info\n");
  printf ("  -q            Don't show Frame Info/errorbars\n");
  printf ("  -s            Datascope (disables other display options)\n");
  printf ("  -t            Show symbol timing during sync\n");
  printf ("  -v <num>      Frame information Verbosity\n");
  printf ("  -z <num>      Frame rate for datascope\n");
  printf ("\n");
  printf ("Input/Output options:\n");
  printf ("  -i <device>   Audio input device (default is /dev/audio, - for piped stdin)\n");
  printf ("  -o <device>   Audio output device (default is /dev/audio)\n");
  printf ("  -d <dir>      Create mbe data files, use this directory\n");
  printf ("  -r <files>    Read/Play saved mbe data from file(s)\n");
  printf ("  -g <num>      Audio output gain (default = 0 = auto, disable = -1)\n");
  printf ("  -n            Do not send synthesized speech to audio output device\n");
  printf ("  -w <file>     Output synthesized speech to a .wav file\n");
  printf ("  -a            Display port audio devices\n");
  printf ("\n");
  printf ("Scanner control options:\n");
  printf ("  -B <num>      Serial port baud rate (default=115200)\n");
  printf ("  -C <device>   Serial port for scanner control (default=/dev/ttyUSB0)\n");
  printf ("  -R <num>      Resume scan after <num> TDULC frames or any PDU or TSDU\n");
  printf ("\n");
  printf ("Decoder options:\n");
  printf ("  -fa           Auto-detect frame type (default)\n");
  printf ("  -f1           Decode only P25 Phase 1\n");
  printf ("  -fd           Decode only D-STAR\n");
  printf ("  -fi           Decode only NXDN48* (6.25 kHz) / IDAS*\n");
  printf ("  -fn           Decode only NXDN96 (12.5 kHz)\n");
  printf ("  -fp           Decode only ProVoice*\n");
  printf ("  -fr           Decode only DMR/MOTOTRBO\n");
  printf ("  -fx           Decode only X2-TDMA\n");
  printf ("  -l            Disable DMR/MOTOTRBO and NXDN input filtering\n");
  printf ("  -ma           Auto-select modulation optimizations (default)\n");
  printf ("  -mc           Use only C4FM modulation optimizations\n");
  printf ("  -mg           Use only GFSK modulation optimizations\n");
  printf ("  -mq           Use only QPSK modulation optimizations\n");
  printf ("  -pu           Unmute Encrypted P25\n");
  printf ("  -u <num>      Unvoiced speech quality (default=3)\n");
  printf ("  -xx           Expect non-inverted X2-TDMA signal\n");
  printf ("  -xr           Expect inverted DMR/MOTOTRBO signal\n");
  printf ("\n");
  printf ("  * denotes frame types that cannot be auto-detected.\n");
  printf ("\n");
  printf ("Advanced decoder options:\n");
  printf ("  -A <num>      QPSK modulation auto detection threshold (default=26)\n");
  printf ("  -S <num>      Symbol buffer size for QPSK decision point tracking\n");
  printf ("                 (default=36)\n");
  printf ("  -M <num>      Min/Max buffer size for QPSK decision point tracking\n");
  printf ("                 (default=15)\n");
  printf ("\n");
  printf ("BP / Генератор ключей / Автотюн:\n");
  printf ("  --hyt-bp-key=<HEX>         Задать ключ явно (10/32/64 hex → 5/16/32 байт)\n");
  printf ("  --bp-key <HEX>             То же, альтернативная форма\n");
  printf ("  --bp-gen [10|32|64]        Сгенерировать случайный ключ и применить (по умолчанию 64)\n");
  printf ("  --bp-gen-loop [10|32|64]   Авто-рекей на границе суперфрейма (длина — опционально)\n");
  printf ("  --bp-rekey-now [10|32|64]  Мгновенная случайная смена ключа\n");
  printf ("  --bp-rekey-every <ms>[:L]  Периодическая смена раз в <ms>, опционально длина L (10/32/64)\n");
  printf ("  --bp-autotune / --no-bp-autotune        Вкл/выкл автотюна по errorbar (по умолчанию ВКЛ)\n");
  printf ("  --bp-th-high <int>         Порог \"очень плохо\" (EWMA error) ⇒ REKEY (дефолт 30)\n");
  printf ("  --bp-th-midlow <int>       Граница средний/низкий ⇒ MUTATE/COMMIT (дефолт 10)\n");
  printf ("  --bp-cooldown-ms <int>     Минимальный интервал между действиями (дефолт 500 мс)\n");
  printf ("  --bp-improve-min <int>     Требуемое снижение EWMA после мутации для COMMIT (дефолт 3)\n");
  printf ("  --bp-debug / --no-bp-debug Вкл/выкл диагностического лога BP\n");
  printf ("  --bp-rekey-gate / --no-bp-rekey-gate    Гейт периодического rekey при включённом автотюне (дефолт ВКЛ)\n");
  printf ("  --bp-stale-sec <int>       Разрешить periodic-rekey, если нет действий автотюна T секунд (дефолт 30)\n");
  printf ("  --bp-rekey-grace-ms <int>  Минимальная пауза после любого rekey (дефолт 3000 мс)\n");
  printf ("  --bp-rekey-kbad <int>      Сколько подряд \"плохих\" EWMA нужно для разрешения rekey (дефолт 2)\n");
  printf ("\n");
  printf ("Примеры:\n");
  printf ("  Сгенерировать 64-hex ключ и применить: dsd -i pa:4 -o pa:3 --bp-gen\n");
  printf ("  Периодический рекей с гейтом:          dsd ... --bp-rekey-every 5000 --bp-rekey-gate --bp-th-high 20 --bp-rekey-kbad 3\n");
  printf ("  Обучение без периодического рекея:     dsd ... --no-bp-rekey-gate --bp-autotune --bp-th-midlow 8 --bp-th-high 20\n");
  printf ("\n");
  exit (0);
}

void
liveScanner (dsd_opts * opts, dsd_state * state)
{
#ifdef USE_PORTAUDIO
    if(opts->audio_in_type == 2)
    {
        PaError err = Pa_StartStream( opts->audio_in_pa_stream );
        if( err != paNoError )
        {
            fprintf( stderr, "An error occured while starting the portaudio input stream\n" );
            fprintf( stderr, "Error number: %d\n", err );
            fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
            return;
        }
    }
#endif
    while (1)
    {
      noCarrier (opts, state);
      state->synctype = getFrameSync (opts, state);
      // recalibrate center/umid/lmid
      state->center = ((state->max) + (state->min)) / 2;
      state->umid = (((state->max) - state->center) * 5 / 8) + state->center;
      state->lmid = (((state->min) - state->center) * 5 / 8) + state->center;
      while (state->synctype != -1)
        {
          processFrame (opts, state);

#ifdef TRACE_DSD
          state->debug_prefix = 'S';
#endif

          state->synctype = getFrameSync (opts, state);

#ifdef TRACE_DSD
          state->debug_prefix = '\0';
#endif

          // recalibrate center/umid/lmid
          state->center = ((state->max) + state->min) / 2;
          state->umid = (((state->max) - state->center) * 5 / 8) + state->center;
          state->lmid = (((state->min) - state->center) * 5 / 8) + state->center;
        }
    }
}

void
cleanupAndExit (dsd_opts * opts, dsd_state * state)
{
  noCarrier (opts, state);
  if (opts->wav_out_f != NULL)
    {
      closeWavOutFile (opts, state);
    }

#ifdef USE_PORTAUDIO
    if((opts->audio_in_type == 2) || (opts->audio_out_type == 2))
    {
        printf("Terminating portaudio.\n");
        PaError err = paNoError;
        if(opts->audio_in_pa_stream != NULL)
        {
            err = Pa_CloseStream( opts->audio_in_pa_stream );
            if( err != paNoError )
            {
                fprintf( stderr, "An error occured while closing the portaudio input stream\n" );
                fprintf( stderr, "Error number: %d\n", err );
                fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
            }
        }
        if(opts->audio_out_pa_stream != NULL)
        {
            err = Pa_IsStreamActive( opts->audio_out_pa_stream );
            if(err == 1)
                err = Pa_StopStream( opts->audio_out_pa_stream );
            if( err != paNoError )
            {
                fprintf( stderr, "An error occured while closing the portaudio output stream\n" );
                fprintf( stderr, "Error number: %d\n", err );
                fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
            }
            err = Pa_CloseStream( opts->audio_out_pa_stream );
            if( err != paNoError )
            {
                fprintf( stderr, "An error occured while closing the portaudio output stream\n" );
                fprintf( stderr, "Error number: %d\n", err );
                fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
            }
        }
        err = Pa_Terminate();
        if( err != paNoError )
        {
            fprintf( stderr, "An error occured while terminating portaudio\n" );
            fprintf( stderr, "Error number: %d\n", err );
            fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
        }
    }
#endif

  printf("\n");
  printf("Total audio errors: %i\n", state->debug_audio_errors);
  printf("Total header errors: %i\n", state->debug_header_errors);
  printf("Total irrecoverable header errors: %i\n", state->debug_header_critical_errors);

  printf("\n");
  printf("+P25 BER estimate: %.2f%%\n", get_P25_BER_estimate(&state->p25_heuristics));
  printf("-P25 BER estimate: %.2f%%\n", get_P25_BER_estimate(&state->inv_p25_heuristics));
  printf("\n");

#ifdef TRACE_DSD
  if (state->debug_label_file != NULL) {
      fclose(state->debug_label_file);
      state->debug_label_file = NULL;
  }
  if(state->debug_label_dibit_file != NULL) {
      fclose(state->debug_label_dibit_file);
      state->debug_label_dibit_file = NULL;
  }
  if(state->debug_label_imbe_file != NULL) {
      fclose(state->debug_label_imbe_file);
      state->debug_label_imbe_file = NULL;
  }
#endif

  printf ("Exiting.\n");
  exit (0);
}

void
sigfun (int sig)
{
    exitflag = 1;
    signal (SIGINT, SIG_DFL);
}

int
main (int argc, char **argv)
{
  int c;
  extern char *optarg;
  extern int optind, opterr, optopt;
  dsd_opts opts;
  dsd_state state;
  char versionstr[25];
  mbe_printVersion (versionstr);

  printf ("DSD-Listener\n");
  printf ("mbelib version %s\n", versionstr);

  initOpts (&opts);
  initState (&state);

  exitflag = 0;
  signal (SIGINT, sigfun);

  bp_consume_cli_key(&argc, argv); // [CHANGED] парсим и новые --bp-* флаги до getopt
  optind = 1;

  while ((c = getopt (argc, argv, "haep:qstv:z:i:o:d:g:nw:B:C:R:f:m:u:x:A:S:M:rl")) != -1)
    {
      opterr = 0;
      switch (c)
        {
        case 'h':
          usage ();
          exit (0);
        case 'a':
          printPortAudioDevices();
          exit(0);
        case 'e':
          opts.errorbars = 1;
          opts.datascope = 0;
          break;
        case 'p':
          if (optarg[0] == 'e')
            {
              opts.p25enc = 1;
            }
          else if (optarg[0] == 'l')
            {
              opts.p25lc = 1;
            }
          else if (optarg[0] == 's')
            {
              opts.p25status = 1;
            }
          else if (optarg[0] == 't')
            {
              opts.p25tg = 1;
            }
          else if (optarg[0] == 'u')
            {
             opts.unmute_encrypted_p25 = 1;
            }
          break;
        case 'q':
          opts.errorbars = 0;
          opts.verbose = 0;
          break;
        case 's':
          opts.errorbars = 0;
          opts.p25enc = 0;
          opts.p25lc = 0;
          opts.p25status = 0;
          opts.p25tg = 0;
          opts.datascope = 1;
          opts.symboltiming = 0;
          break;
        case 't':
          opts.symboltiming = 1;
          opts.errorbars = 1;
          opts.datascope = 0;
          break;
        case 'v':
          sscanf (optarg, "%d", &opts.verbose);
          break;
        case 'z':
          sscanf (optarg, "%d", &opts.scoperate);
          opts.errorbars = 0;
          opts.p25enc = 0;
          opts.p25lc = 0;
          opts.p25status = 0;
          opts.p25tg = 0;
          opts.datascope = 1;
          opts.symboltiming = 0;
          printf ("Setting datascope frame rate to %i frame per second.\n", opts.scoperate);
          break;
        case 'i':
          strncpy(opts.audio_in_dev, optarg, 1023);
          opts.audio_in_dev[1023] = '\0';
          break;
        case 'o':
          strncpy(opts.audio_out_dev, optarg, 1023);
          opts.audio_out_dev[1023] = '\0';
          break;
        case 'd':
          strncpy(opts.mbe_out_dir, optarg, 1023);
          opts.mbe_out_dir[1023] = '\0';
          printf ("Writing mbe data files to directory %s\n", opts.mbe_out_dir);
          break;
        case 'g':
          sscanf (optarg, "%f", &opts.audio_gain);
          if (opts.audio_gain < (float) 0 )
            {
              printf ("Disabling audio out gain setting\n");
            }
          else if (opts.audio_gain == (float) 0)
            {
              opts.audio_gain = (float) 0;
              printf ("Enabling audio out auto-gain\n");
            }
          else
            {
              printf ("Setting audio out gain to %f\n", opts.audio_gain);
              state.aout_gain = opts.audio_gain;
            }
          break;
        case 'n':
          opts.audio_out = 0;
          printf ("Disabling audio output to soundcard.\n");
          break;
        case 'w':
          strncpy(opts.wav_out_file, optarg, 1023);
          opts.wav_out_file[1023] = '\0';
          printf ("Writing audio to file %s\n", opts.wav_out_file);
          openWavOutFile (&opts, &state);
          break;
        case 'B':
          sscanf (optarg, "%d", &opts.serial_baud);
          break;
        case 'C':
          strncpy(opts.serial_dev, optarg, 1023);
          opts.serial_dev[1023] = '\0';
          break;
        case 'R':
          sscanf (optarg, "%d", &opts.resume);
          printf ("Enabling scan resume after %i TDULC frames\n", opts.resume);
          break;
        case 'f':
          if (optarg[0] == 'a')
            {
              opts.frame_dstar = 1;
              opts.frame_x2tdma = 1;
              opts.frame_p25p1 = 1;
              opts.frame_nxdn48 = 0;
              opts.frame_nxdn96 = 1;
              opts.frame_dmr = 1;
              opts.frame_provoice = 0;
            }
          else if (optarg[0] == 'd')
            {
              opts.frame_dstar = 1;
              opts.frame_x2tdma = 0;
              opts.frame_p25p1 = 0;
              opts.frame_nxdn48 = 0;
              opts.frame_nxdn96 = 0;
              opts.frame_dmr = 0;
              opts.frame_provoice = 0;
              printf ("Decoding only D-STAR frames.\n");
            }
          else if (optarg[0] == 'x')
            {
              opts.frame_dstar = 0;
              opts.frame_x2tdma = 1;
              opts.frame_p25p1 = 0;
              opts.frame_nxdn48 = 0;
              opts.frame_nxdn96 = 0;
              opts.frame_dmr = 0;
              opts.frame_provoice = 0;
              printf ("Decoding only X2-TDMA frames.\n");
            }
          else if (optarg[0] == 'p')
            {
              opts.frame_dstar = 0;
              opts.frame_x2tdma = 0;
              opts.frame_p25p1 = 0;
              opts.frame_nxdn48 = 0;
              opts.frame_nxdn96 = 0;
              opts.frame_dmr = 0;
              opts.frame_provoice = 1;
              state.samplesPerSymbol = 5;
              state.symbolCenter = 2;
              opts.mod_c4fm = 0;
              opts.mod_qpsk = 0;
              opts.mod_gfsk = 1;
              state.rf_mod = 2;
              printf ("Setting symbol rate to 9600 / second\n");
              printf ("Enabling only GFSK modulation optimizations.\n");
              printf ("Decoding only ProVoice frames.\n");
            }
          else if (optarg[0] == '1')
            {
              opts.frame_dstar = 0;
              opts.frame_x2tdma = 0;
              opts.frame_p25p1 = 1;
              opts.frame_nxdn48 = 0;
              opts.frame_nxdn96 = 0;
              opts.frame_dmr = 0;
              opts.frame_provoice = 0;
              printf ("Decoding only P25 Phase 1 frames.\n");
            }
          else if (optarg[0] == 'i')
            {
              opts.frame_dstar = 0;
              opts.frame_x2tdma = 0;
              opts.frame_p25p1 = 0;
              opts.frame_nxdn48 = 1;
              opts.frame_nxdn96 = 0;
              opts.frame_dmr = 0;
              opts.frame_provoice = 0;
              state.samplesPerSymbol = 20;
              state.symbolCenter = 10;
              opts.mod_c4fm = 0;
              opts.mod_qpsk = 0;
              opts.mod_gfsk = 1;
              state.rf_mod = 2;
              printf ("Setting symbol rate to 2400 / second\n");
              printf ("Enabling only GFSK modulation optimizations.\n");
              printf ("Decoding only NXDN 4800 baud frames.\n");
            }
          else if (optarg[0] == 'n')
            {
              opts.frame_dstar = 0;
              opts.frame_x2tdma = 0;
              opts.frame_p25p1 = 0;
              opts.frame_nxdn48 = 0;
              opts.frame_nxdn96 = 1;
              opts.frame_dmr = 0;
              opts.frame_provoice = 0;
              opts.mod_c4fm = 0;
              opts.mod_qpsk = 0;
              opts.mod_gfsk = 1;
              state.rf_mod = 2;
              printf ("Enabling only GFSK modulation optimizations.\n");
              printf ("Decoding only NXDN 9600 baud frames.\n");
            }
          else if (optarg[0] == 'r')
            {
              opts.frame_dstar = 0;
              opts.frame_x2tdma = 0;
              opts.frame_p25p1 = 0;
              opts.frame_nxdn48 = 0;
              opts.frame_nxdn96 = 0;
              opts.frame_dmr = 1;
              opts.frame_provoice = 0;
              printf ("Decoding only DMR/MOTOTRBO frames.\n");
            }
          break;
        case 'm':
          if (optarg[0] == 'a')
            {
              opts.mod_c4fm = 1;
              opts.mod_qpsk = 1;
              opts.mod_gfsk = 1;
              state.rf_mod = 0;
            }
          else if (optarg[0] == 'c')
            {
              opts.mod_c4fm = 1;
              opts.mod_qpsk = 0;
              opts.mod_gfsk = 0;
              state.rf_mod = 0;
              printf ("Enabling only C4FM modulation optimizations.\n");
            }
          else if (optarg[0] == 'g')
            {
              opts.mod_c4fm = 0;
              opts.mod_qpsk = 0;
              opts.mod_gfsk = 1;
              state.rf_mod = 2;
              printf ("Enabling only GFSK modulation optimizations.\n");
            }
          else if (optarg[0] == 'q')
            {
              opts.mod_c4fm = 0;
              opts.mod_qpsk = 1;
              opts.mod_gfsk = 0;
              state.rf_mod = 1;
              printf ("Enabling only QPSK modulation optimizations.\n");
            }
          break;
        case 'u':
          sscanf (optarg, "%i", &opts.uvquality);
          if (opts.uvquality < 1)
            {
              opts.uvquality = 1;
            }
          else if (opts.uvquality > 64)
            {
              opts.uvquality = 64;
            }
          printf ("Setting unvoice speech quality to %i waves per band.\n", opts.uvquality);
          break;
        case 'x':
          if (optarg[0] == 'x')
            {
              opts.inverted_x2tdma = 0;
              printf ("Expecting non-inverted X2-TDMA signals.\n");
            }
          else if (optarg[0] == 'r')
            {
              opts.inverted_dmr = 1;
              printf ("Expecting inverted DMR/MOTOTRBO signals.\n");
            }
          break;
        case 'A':
          sscanf (optarg, "%i", &opts.mod_threshold);
          printf ("Setting C4FM/QPSK auto detection threshold to %i\n", opts.mod_threshold);
          break;
        case 'S':
          sscanf (optarg, "%i", &opts.ssize);
          if (opts.ssize > 128)
            {
              opts.ssize = 128;
            }
          else if (opts.ssize < 1)
            {
              opts.ssize = 1;
            }
          printf ("Setting QPSK symbol buffer to %i\n", opts.ssize);
          break;
        case 'M':
          sscanf (optarg, "%i", &opts.msize);
          if (opts.msize > 1024)
            {
              opts.msize = 1024;
            }
          else if (opts.msize < 1)
            {
              opts.msize = 1;
            }
          printf ("Setting QPSK Min/Max buffer to %i\n", opts.msize);
          break;
        case 'r':
          opts.playfiles = 1;
          opts.errorbars = 0;
          opts.datascope = 0;
          state.optind = optind;
          break;
        case 'l':
          opts.use_cosine_filter = 0;
          break;
        default:
          usage ();
          exit (0);
        }
    }


  if (opts.resume > 0)
    {
      openSerial (&opts, &state);
    }


#ifdef USE_PORTAUDIO
  if((strncmp(opts.audio_in_dev, "pa:", 3) == 0)
  || (strncmp(opts.audio_out_dev, "pa:", 3) == 0))
  {
    printf("Initializing portaudio.\n");
    PaError err = Pa_Initialize();
    if( err != paNoError )
    {
        fprintf( stderr, "An error occured while initializing portaudio\n" );
        fprintf( stderr, "Error number: %d\n", err );
        fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
        exit(err);
    }
  }
#endif

  if (opts.playfiles == 1)
    {
      opts.split = 1;
      opts.playoffset = 0;
      opts.delay = 0;
      if (strlen(opts.wav_out_file) > 0)
        {
          openWavOutFile (&opts, &state);
        }
      else
        {
          openAudioOutDevice (&opts, SAMPLE_RATE_OUT);
        }
    }
  else if (strcmp (opts.audio_in_dev, opts.audio_out_dev) != 0)
    {
      opts.split = 1;
      opts.playoffset = 0;
      opts.delay = 0;
      if (strlen(opts.wav_out_file) > 0)
        {
          openWavOutFile (&opts, &state);
        }
      else
        {
          openAudioOutDevice (&opts, SAMPLE_RATE_OUT);
        }
      openAudioInDevice (&opts);
    }
  else
    {
      opts.split = 0;
      opts.playoffset = 25;     // 38
      opts.delay = 0;
      openAudioInDevice (&opts);
      opts.audio_out_fd = opts.audio_in_fd;
    }

  if (opts.playfiles == 1)
    {
      playMbeFiles (&opts, &state, argc, argv);
    }
  else
    {
        liveScanner (&opts, &state);
    }
  cleanupAndExit (&opts, &state);
  return (0);
}
