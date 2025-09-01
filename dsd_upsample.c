

#include "dsd.h"

void
upsample (dsd_state * state, float invalue)
{

  int i, j, sum;
  float *outbuf1, c, d;

  outbuf1 = state->audio_out_float_buf_p;
  outbuf1--;
  c = *outbuf1;
  d = invalue;
  // basic triangle interpolation
  outbuf1++;
  *outbuf1 = ((invalue * (float) 0.166) + (c * (float) 0.834));
  outbuf1++;
  *outbuf1 = ((invalue * (float) 0.332) + (c * (float) 0.668));
  outbuf1++;
  *outbuf1 = ((invalue * (float) 0.5) + (c * (float) 0.5));
  outbuf1++;
  *outbuf1 = ((invalue * (float) 0.668) + (c * (float) 0.332));
  outbuf1++;
  *outbuf1 = ((invalue * (float) 0.834) + (c * (float) 0.166));
  outbuf1++;
  *outbuf1 = d;
  outbuf1++;

  if (state->audio_out_idx2 > 24)
    {
      // smoothing
      outbuf1 -= 16;
      for (j = 0; j < 4; j++)
        {
          for (i = 0; i < 6; i++)
            {
              sum = 0;
              outbuf1 -= 2;
              sum += (int)*outbuf1;
              outbuf1 += 2;
              sum += (int)*outbuf1;
              outbuf1 += 2;
              sum += (int)*outbuf1;
              outbuf1 -= 2;
              *outbuf1 = (sum / (float) 3);
              outbuf1++;
            }
          outbuf1 -= 8;
        }
    }
}
