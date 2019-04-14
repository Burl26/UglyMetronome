/*
 * beat.h
 *
 *  Created on: Feb. 5, 2019
 *      Author: larry
 */

#ifndef BEAT_H
#define BEAT_H
#define MAX_TEMPO	250
#define MIN_TEMPO	20
#define MAX_RAMP	600

#include <sys/time.h>

int BeatInit();
void BeatSetVolume(float v);
void BeatClose();
void BeatRun(int state);
void BeatSetTempo(float startt, float stopt, long ramptime);
void BeatLoad(signed short *bw, long l);
long BeatGetBPM();
void BeatSetJackClient(int ch, char *szch);
void BeatGetJackClient(int ch, char *szch, int n);
void BeatConnect();
#endif /* BEAT_H_ */
