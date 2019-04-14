/*
 * beat.cxx
 *
 *  Created on: Feb. 5, 2019
 *  Author: Burl26
 *
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <math.h>
#include <string.h>
#include <jack/jack.h>
#include <jack/transport.h>
#include "beat.h"

//typedef jack_default_audio_sample_t sample_t;

#define POLYPHONIC 4		// beats with long decays will blend up to this number, otherwise new beats will be missed.

typedef struct BWaveStruct {
	long BWaveCtr;
	jack_default_audio_sample_t *BWavePtr;
}BWave, *BWAVE;

struct {
	// callback data
	jack_transport_state_t transport_state;
	jack_client_t *client;	// the JACK client
	jack_port_t *output_port; // the JACK output port
	char *client_name;		// JACK- holds the JACK client name
	long BeatDelta;			//$how much to increase/decrease beat on the next interval reload
	long BeatTarget;		//$The target beat interval
	long BeatInterval;		//$the interval between beats in samples (current interval)
	float Volume;			//$non-conflicting volume setting
	BWave Beat[POLYPHONIC];	// holds the polyphonic beats
	// pass-across data
	int	Running;			// buffers are being processed
	int Start;				// start stop command to proc
	jack_default_audio_sample_t *BeatWave;	// the wave data for the beat
	long BeatWaveLength;	// the length of the beat wave in samples
	long BeatIntervalCtr;	// counts up the beat interval
	float BeatVolume;		//$the volume of the beat
	float BPM;				//Beats per minute
	char jack_outclientA[64];  // jack connection for output channel 1
	char jack_outclientB[64];  // jack connection for output channel 2
} BeatStruct;

//**************************************************************************
// This is a realtime thread function, so no long blocking stuff
//void	BeatProc(void *pSoundBuffer, long bufferLen, long start)
int	BeatProc(jack_nframes_t bufferLen, void *start)
{
	long	i,j;
	jack_default_audio_sample_t sample;
	jack_position_t pos;

	int *st;
	st = (int *)start;

	// Get a pointer to the sample.
	jack_default_audio_sample_t *pSample = (jack_default_audio_sample_t *) jack_port_get_buffer (BeatStruct.output_port, bufferLen);
	// how many bytes?
	long	nbSample = bufferLen;// / sizeof(signed short);

	// if transport is not rolling, then just fill buffer with zeros and exit
	if (JackTransportRolling != jack_transport_query (BeatStruct.client, &pos)) {
		BeatStruct.Running = 0;
		// fill buffer with silence and exit
		memset (pSample, 0, sizeof (jack_default_audio_sample_t) * nbSample);
		return 0;
	}

	// when first turned on, reset everything
	if (0 == BeatStruct.Running){
		if (*st) {
			BeatStruct.Running = 1;
			// initialize beat buffers
			for (i=0; i<POLYPHONIC; i++) BeatStruct.Beat[i].BWavePtr = NULL;
			BeatStruct.BeatIntervalCtr = 0;
		}  else {
			BeatStruct.Running = 0;
			// fill buffer with silence and exit
			memset (pSample, 0, sizeof (jack_default_audio_sample_t) * nbSample);
			return 0;
		}
	} else {
		if (0 == *st) {
			BeatStruct.Running = 0;
			// fill buffer with silence and exit
			memset (pSample, 0, sizeof (jack_default_audio_sample_t) * nbSample);
			return 0;
		}
	}

	// to fill the buffer
	for (j=0; j<nbSample; j++) {
		// check for new beat when BeatInterval expires
		if (BeatStruct.BeatIntervalCtr == 0) {
			//****BEGINNING OF PROTECTED SECTION
			// update the volume
			//BeatStruct.Volume = (exp(0.04 * BeatStruct.BeatVolume)-1.f)/ 54.59815;
			BeatStruct.Volume = BeatStruct.BeatVolume;
			// load new BeatInterval as a function of the ramp;
			BeatStruct.BeatInterval += BeatStruct.BeatDelta;
			// clip
			if ((BeatStruct.BeatDelta > 0) && (BeatStruct.BeatInterval > BeatStruct.BeatTarget)) {
				BeatStruct.BeatInterval = BeatStruct.BeatTarget;
				BeatStruct.BeatDelta = 0;
			} else {
				if ((BeatStruct.BeatDelta < 0) && (BeatStruct.BeatInterval < BeatStruct.BeatTarget)) {
					BeatStruct.BeatInterval = BeatStruct.BeatTarget;
					BeatStruct.BeatDelta = 0;
				}
			}
			//****END OF PROTECTED SECTION
			// reset beat interval counter
			BeatStruct.BeatIntervalCtr = BeatStruct.BeatInterval;

			// on BeatInterval, find next available beat buffer.  If no buffers are available - too bad: that beat is missed.
			for (i=0; (i<POLYPHONIC); i++) {
				if (BeatStruct.Beat[i].BWavePtr == NULL) {
					// initialize beat
					BeatStruct.Beat[i].BWaveCtr = BeatStruct.BeatWaveLength;
					BeatStruct.Beat[i].BWavePtr = BeatStruct.BeatWave;
					break;
				}
			}
		}
		// default output wave value
		sample = 0.0f;
		// for each beat
		for (i=0; i<POLYPHONIC; i++) {
			if (BeatStruct.Beat[i].BWavePtr) {
				sample += ((float)(*BeatStruct.Beat[i].BWavePtr) * BeatStruct.Volume);
				BeatStruct.Beat[i].BWavePtr++;
				BeatStruct.Beat[i].BWaveCtr--;
				// when beat wave is done, free up the beat
				if (BeatStruct.Beat[i].BWaveCtr == 0) BeatStruct.Beat[i].BWavePtr = NULL;
			}
		}

		// write the sample to the buffer
		*pSample = (jack_default_audio_sample_t)sample;
		pSample++;

		// advance beat interval counter
		BeatStruct.BeatIntervalCtr--;
	}
	return 0;
}

int BeatInit()
{
	jack_status_t status;

	BeatStruct.Running = 0;
	BeatStruct.BeatWave = NULL;
	BeatStruct.BeatWaveLength = 0;
	BeatSetTempo(120.0f, 120.0f,0);
	BeatStruct.BeatIntervalCtr = 0;
	for (int i=0; i<POLYPHONIC; i++) {
		BeatStruct.Beat[i].BWavePtr = NULL;
		BeatStruct.Beat[i].BWaveCtr = 0;
	}

	/* Initial Jack setup, get sample rate */
	if (!BeatStruct.client_name) {
		BeatStruct.client_name = (char *) malloc (12 * sizeof (char));
		strcpy (BeatStruct.client_name, "UGLYMETRO");
	}
	if ((BeatStruct.client = jack_client_open (BeatStruct.client_name, JackNoStartServer, &status)) == 0) {
		//fprintf (stderr, "jack server not running?\n");
		return 1;
	}
	jack_set_process_callback (BeatStruct.client, BeatProc, &BeatStruct.Start);//BeatProc, &BeatStruct.Start);
	return 0;
}
void BeatConnect() {
	const char *bpm_string = "bpm";
	BeatStruct.output_port = jack_port_register (BeatStruct.client, bpm_string, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	jack_activate(BeatStruct.client);
	if (strlen(BeatStruct.jack_outclientA) > 0) jack_connect(BeatStruct.client,  jack_port_name (BeatStruct.output_port), BeatStruct.jack_outclientA);
	if (strlen(BeatStruct.jack_outclientB) > 0) jack_connect(BeatStruct.client,  jack_port_name (BeatStruct.output_port), BeatStruct.jack_outclientB);
	jack_transport_start(BeatStruct.client);
}
void BeatSetVolume(float v)
{
	BeatStruct.BeatVolume = v / 100;
}
void BeatClose()
{
	if (BeatStruct.BeatWave) free(BeatStruct.BeatWave);
	if (BeatStruct.client) jack_client_close(BeatStruct.client);
}

void BeatRun(int state)
{
	BeatStruct.Start = state;
}
void BeatSetTempo(float BPM_START, float BPM, long seconds)
{
	long bpm, bt, bd, bi, n;

	//****PROTECTED SECTION
	bi = BeatStruct.BeatInterval;
	//****END PROTECTED SECTION
	// override beat interval if there is a BPM_START
	if ((BPM_START >= MIN_TEMPO) && (BPM_START <= MAX_TEMPO)) {
		bi = (long)(48000.f * 60.f / BPM_START);
	}

	bpm = BPM;
	// hard-limit BPM to 230
	if (bpm > MAX_TEMPO) bpm = MAX_TEMPO;
	if (bpm < MIN_TEMPO) bpm = MIN_TEMPO;

	// hard limit seconds
	if (seconds < 0) seconds = 0;
	if (seconds > MAX_RAMP) seconds = MAX_RAMP;

	// calculate delta
	bt = (long)(48000.f * 60.f / bpm); // beats per minute converted to samples
	if (seconds < 1) {
		// step function
		bd = 0;
		bi = bt;
	} else {
		// ramp function
		n = (bt - bi);
		bd = ((n * n) / 2 + bi * n) / ((seconds * 48000) - (n / 2));
	}
	//****PROTECTED SECTION
	BeatStruct.BPM = bpm;
	BeatStruct.BeatTarget = bt;
	BeatStruct.BeatDelta = bd;
	BeatStruct.BeatInterval = bi;
	//****END PROTECTED SECTION
}
void BeatLoad(signed short *br, long bs)
{
	// sine wave
	float w,delta;
	long i;

	if (BeatStruct.BeatWave) free(BeatStruct.BeatWave);
	if ((br) && (bs > 0)) {
		BeatStruct.BeatWave = (jack_default_audio_sample_t *)malloc(bs / 2 * sizeof(jack_default_audio_sample_t));
		for (i=0; i<(bs/2); i++)
			BeatStruct.BeatWave[i] = (jack_default_audio_sample_t) br[i] / 32768.0f;
		BeatStruct.BeatWaveLength = bs/2;
		return;
	}

	// for now, instead of loading, we will generate the waveform
	// 5500 samples at 48kHz, 440Hz Sine Wave with linear attenuation
	BeatStruct.BeatWaveLength = 5500;
	BeatStruct.BeatWave = (jack_default_audio_sample_t *)malloc(BeatStruct.BeatWaveLength * sizeof(jack_default_audio_sample_t));
	w = 0.0;
	delta = 2.0f * (float)acos(-1.0f) * 440.0f / 48000.f;
	for (int i=0; i<BeatStruct.BeatWaveLength; i++) {
		BeatStruct.BeatWave[i] = (jack_default_audio_sample_t) ((1.f - ((float) i / (float)BeatStruct.BeatWaveLength)) * 0.8f * sin(w));
		w += delta;
	}
}
long BeatGetBPM()
{
	float f;
	if (BeatStruct.BeatInterval == 0) return 0;
	f = 48000.f * 60.f / (float)BeatStruct.BeatInterval;
	return((long) f);
}

void BeatSetJackClient(int ch, char *szch) {
	if (ch == 0) {
		BeatStruct.jack_outclientA[0] = 0;
		if (szch) strncpy(BeatStruct.jack_outclientA, szch, 64);
	}
	else {
		BeatStruct.jack_outclientB[0] = 0;
		if (szch) strncpy(BeatStruct.jack_outclientB, szch, 64);
	}
}
 void BeatGetJackClient(int ch, char *szch, int n) {
	const char **jc;
	if (szch == NULL) return;
	if (ch != 0) ch = 1;
	if (BeatStruct.output_port) {
		jc = jack_port_get_all_connections(BeatStruct.client, BeatStruct.output_port);
		strncpy(szch, jc[ch], n);
		jack_free(jc);
		return;
	}
}





