

#include "dsd.h"

void
playMbeFiles (dsd_opts * opts, dsd_state * state, int argc, char **argv)
{

  int i;
  char imbe_d[88];
  char ambe_d[49];

  for (i = state->optind; i < argc; i++)
    {
      sprintf (opts->mbe_in_file, "%s", argv[i]);
      openMbeInFile (opts, state);
      mbe_initMbeParms (state->cur_mp, state->prev_mp, state->prev_mp_enhanced);
      printf ("playing %s\n", opts->mbe_in_file);
      while (feof (opts->mbe_in_f) == 0)
        {
          if (state->mbe_file_type == 0)
            {
              readImbe4400Data (opts, state, imbe_d);
              mbe_processImbe4400Dataf (state->audio_out_temp_buf, &state->errs, &state->errs2, state->err_str, imbe_d, state->cur_mp, state->prev_mp, state->prev_mp_enhanced, opts->uvquality);
              processAudio (opts, state);
              if (opts->wav_out_f != NULL)
                {
                  writeSynthesizedVoice (opts, state);
                }

              if (opts->audio_out == 1)
                {
                  playSynthesizedVoice (opts, state);
                }
            }
          else if (state->mbe_file_type == 1)
            {
              readAmbe2450Data (opts, state, ambe_d);
              mbe_processAmbe2450Dataf (state->audio_out_temp_buf, &state->errs, &state->errs2, state->err_str, ambe_d, state->cur_mp, state->prev_mp, state->prev_mp_enhanced, opts->uvquality);
              processAudio (opts, state);
              if (opts->wav_out_f != NULL)
                {
                  writeSynthesizedVoice (opts, state);
                }
              if (opts->audio_out == 1)
                {
                  playSynthesizedVoice (opts, state);
                }
            }
          if (exitflag == 1)
            {
              cleanupAndExit (opts, state);
            }
        }
    }
}

void
processMbeFrame (dsd_opts * opts, dsd_state * state, char imbe_fr[8][23], char ambe_fr[4][24], char imbe7100_fr[7][24])
{

  int i;
  char imbe_d[88];
  char ambe_d[49];
#ifdef AMBE_PACKET_OUT
  char ambe_d_str[50];
#endif

  for (i = 0; i < 88; i++)
    {
      imbe_d[i] = 0;
    }

  if ((state->synctype == 0) || (state->synctype == 1))
    {
      //  0 +P25p1
      //  1 -P25p1

      mbe_processImbe7200x4400Framef (state->audio_out_temp_buf, &state->errs, &state->errs2, state->err_str, imbe_fr, imbe_d, state->cur_mp, state->prev_mp, state->prev_mp_enhanced, opts->uvquality);
      if (opts->mbe_out_f != NULL)
        {
          saveImbe4400Data (opts, state, imbe_d);
        }
    }
  else if ((state->synctype == 14) || (state->synctype == 15))
    {
      mbe_processImbe7100x4400Framef (state->audio_out_temp_buf, &state->errs, &state->errs2, state->err_str, imbe7100_fr, imbe_d, state->cur_mp, state->prev_mp, state->prev_mp_enhanced, opts->uvquality);
      if (opts->mbe_out_f != NULL)
        {
          saveImbe4400Data (opts, state, imbe_d);
        }
    }
  else if ((state->synctype == 6) || (state->synctype == 7))
    {
      mbe_processAmbe3600x2400Framef (state->audio_out_temp_buf, &state->errs, &state->errs2, state->err_str, ambe_fr, ambe_d, state->cur_mp, state->prev_mp, state->prev_mp_enhanced, opts->uvquality);
      if (opts->mbe_out_f != NULL)
        {
          saveAmbe2450Data (opts, state, ambe_d);
        }
    }
  else
    {
      mbe_processAmbe3600x2450Framef (state->audio_out_temp_buf, &state->errs, &state->errs2, state->err_str, ambe_fr, ambe_d, state->cur_mp, state->prev_mp, state->prev_mp_enhanced, opts->uvquality);
#ifdef AMBE_PACKET_OUT
      for(i=0; i<49; i++) {
          ambe_d_str[i] = ambe_d[i] + '0';
      }
      ambe_d_str[49] = '\0';
      // print binary string
      fprintf(stderr, "\n?\t?\t%s\t", ambe_d_str);
      // print error data
      fprintf(stderr, "E1: %d; E2: %d; S: %s", state->errs, state->errs2, state->err_str);
#endif
      if (opts->mbe_out_f != NULL)
        {
          saveAmbe2450Data (opts, state, ambe_d);
        }
    }

  if (opts->errorbars == 1)
    {
      printf ("%s", state->err_str);
    }

  state->debug_audio_errors += state->errs2;

  processAudio (opts, state);
  if (opts->wav_out_f != NULL)
    {
      writeSynthesizedVoice (opts, state);
    }

  if (opts->audio_out == 1)
    {
      playSynthesizedVoice (opts, state);
    }
}
