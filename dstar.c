

#include "dsd.h"
#include "dstar_const.h"
#include "dstar_header.h"


void processDSTAR(dsd_opts * opts, dsd_state * state) {
	// extracts AMBE frames from D-STAR voice frame
	int i, j, dibit;
	char ambe_fr[4][24];
	unsigned char data[9];
	unsigned int bits[4];
	int framecount;
	int sync_missed = 0;
	unsigned char slowdata[4];
	unsigned int bitbuffer = 0;
	const int *w, *x;

	if (opts->errorbars == 1) {
		printf("e:");
	}

#ifdef DSTAR_DUMP
	printf ("\n");
#endif

	if (state->synctype == 18) {
		framecount = 0;
		state->synctype = 6;
	} else if (state->synctype == 19) {
		framecount = 0;
		state->synctype = 7;
	} else {
		framecount = 1; //just saw a sync frame; there should be 20 not 21 till the next
	}

	while (sync_missed < 3) {

		memset(ambe_fr, 0, 96);
		// voice frame
	    w = dW;
	    x = dX;

		for (i = 0; i < 72; i++) {

			dibit = getDibit(opts, state);

			bitbuffer <<= 1;
			if (dibit == 1) {
				bitbuffer |= 0x01;
			}
			if ((bitbuffer & 0x00FFFFFF) == 0x00AAB468) {
				// we're slipping bits
				printf("sync in voice after i=%d, restarting\n", i);
				//ugh just start over
				i = 0;
			    w = dW;
			    x = dX;
				framecount = 1;
				continue;
			}

			ambe_fr[*w][*x] = (1 & dibit);
			w++;
			x++;
		}


		processMbeFrame(opts, state, NULL, ambe_fr, NULL);

		//  data frame - 24 bits
		for (i = 73; i < 97; i++) {
			dibit = getDibit(opts, state);
			bitbuffer <<= 1;
			if (dibit == 1) {
				bitbuffer |= 0x01;
			}
			if ((bitbuffer & 0x00FFFFFF) == 0x00AAB468) {
				// looking if we're slipping bits
				if (i != 96) {
					printf("sync after i=%d\n", i);
					i = 96;
				}
			}
		}

		slowdata[0] = (bitbuffer >> 16) & 0x000000FF;
		slowdata[1] = (bitbuffer >> 8) & 0x000000FF;
		slowdata[2] = (bitbuffer) & 0x000000FF;
		slowdata[3] = 0;

		if ((bitbuffer & 0x00FFFFFF) == 0x00AAB468) {
			//We got sync!
			//printf("Sync on framecount = %d\n", framecount);
			sync_missed = 0;
		} else if ((bitbuffer & 0x00FFFFFF) == 0xAAAAAA) {
			//End of transmission
			printf("End of transmission\n");
			goto end;
		} else if (framecount % 21 == 0) {
			printf("Missed sync on framecount = %d, value = %x/%x/%x\n",
					framecount, slowdata[0], slowdata[1], slowdata[2]);
			sync_missed++;
		} else if (framecount != 0 && (bitbuffer & 0x00FFFFFF) != 0x000000) {
			slowdata[0] ^= 0x70;
			slowdata[1] ^= 0x4f;
			slowdata[2] ^= 0x93;
			//printf("unscrambled- %s",slowdata);

		} else if (framecount == 0) {
			//printf("never scrambled-%s\n",slowdata);
		}

		framecount++;
	}

	end: if (opts->errorbars == 1) {
		printf("\n");
	}
}

void processDSTAR_HD(dsd_opts * opts, dsd_state * state) {

	int i, j;
	int radioheaderbuffer[660];

	for (j = 0; j < 660; j++) {
					radioheaderbuffer[j] = getDibit(opts, state);
			}

	// Note: These routines contain GPLed code. Remove if you object to that.
	// Due to this, they are in a separate source file.
	dstar_header_decode(radioheaderbuffer);

	//We officially have sync now, so just pass on to the above routine:

	processDSTAR(opts, state);

}

