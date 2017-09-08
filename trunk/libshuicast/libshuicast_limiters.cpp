
#include "libshuicast_limiters.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

MYFLOAT __PI2 = -2.0 * 3.1415926535897932384626433832795 * 1000000.0;
MYFLOAT Stepup = 2.0;

Limiters::Limiters() : 
	lastLeftSample(0.0),
	lastRightSample(0.0),
	lastMonoSample(0.0),
	lastLeftFixedSample(0.0),
	lastRightFixedSample(0.0),
	lastMonoFixedSample(0.0),

	previousSampleRate(0),
	previousPreEmph(0.0),
	stereoAttenuation(0.0),
	monoAttenuation(0.0),
	preEmphMult(0.0),

	SampleRate(0),
	PreEmphasis(0),
	LimitdB(20.0),
	dst(NULL),
	dst_size(0),
	outputStereo(NULL),
	outputMono(NULL),
	deemphasis(TRUE),
	gainMult(1.0),
	GaindB(0.0)
{
}

Limiters::~Limiters()
{
	SafeFree(dst);
}
void Limiters::SafeFree(void * b)
{
	if(b)
	{
		free(b);
	}
}
Limiters::Limiters(MYFLOAT inPreEmphasis, int inSampleRate, MYFLOAT inLimitdB, MYFLOAT inGaindB) :
	PreEmphasis(inPreEmphasis),
	SampleRate(inSampleRate),
	LimitdB(inLimitdB),
	GaindB(inGaindB)
{
}
Limiters::Limiters(int inSampleRate, MYFLOAT inPreEmphasis) : 
	PreEmphasis(inPreEmphasis),
	SampleRate(inSampleRate),
	LimitdB(20.0),
	GaindB(0.0)
{
}

void Limiters::multiLimit(float * samples, int len, int inSampleRate, MYFLOAT * inPreEmphasis, MYFLOAT * inLimitdB, MYFLOAT * inGaindB)
{
}

//#define CALCRMS(sum, len) ((sum > 0.0) ? (MYFLOAT)20 * log10(sqrt(sum/(len < 1 ? 1 : len))) : -120.0)
#define CALCRMS(sum, len) ((sum > 0.0) ? (MYFLOAT)10 * log10(sum/(len < 1 ? 1 : len)) : -120.0)
#define CALCPEAK(sum) ((sum > 0.0) ? (MYFLOAT)20 * log10(sum) : -120.0)

void limitStereoToStereoMono::limit(float * samples, int len, int inSampleRate, MYFLOAT inPreEmphasis, MYFLOAT inLimitdB, MYFLOAT inGaindB)
{
	if(dst == NULL || len > dst_size)
	{
		if(dst == NULL)
		{
			dst = (float *) malloc(len * sizeof(float) * 4);
		}
		else
		{
			dst = (float *) realloc((void *) dst, len * sizeof(float) * 4);
		}
		dst_size = len;
	}
	outputSize = len;
	if(inSampleRate > 0) 
	{
		SampleRate = inSampleRate;
	}
	if(inPreEmphasis >= 0)
	{
		PreEmphasis = inPreEmphasis;
	}
	if(inLimitdB <= 0.0)
	{
		LimitdB = inLimitdB;
	}
	outputStereo = dst;
	outputMono = dst + (len * 2);

	MYFLOAT sumLeft = 0.0;
	MYFLOAT sumRight = 0.0;
	MYFLOAT sumMono = 0.0;
	PeakL = 0.0;
	PeakR = 0.0;
	PeakM = 0.0;
	if(SampleRate <= 0 || inLimitdB > 19.9)
	{
		CopyMemory(outputStereo, samples, sizeof(float) * len * 2);
		float * srcStereo = outputStereo;
		float * dstMono = outputMono;
		for(int i = 0; i < len; i++)
		{
			MYFLOAT lft = *(srcStereo++);
			MYFLOAT rgt = *(srcStereo++);
			MYFLOAT mon = 0.0;
#if 1
			mon = (lft + rgt) / 2.0;
#else
			mon = ((lft * fabs(lft) + rgt * fabs(rgt)) / 2.0);
			if(mon < 0.0) mon = -sqrt(-mon);
			else mon = sqrt(mon);
#endif
			*(dstMono++) = *(dstMono++) = (float) (mon);
			sumLeft += lft * lft;
			sumRight += rgt * rgt;
			sumMono += mon * mon;
			MYFLOAT val = fabs(lft);
			if(val > PeakL) PeakL = val;
			val = fabs(rgt);
			if(val > PeakR) PeakR = val;
			val = fabs(mon);
			if(val > PeakM) PeakM = val;
		}
		PeakL = CALCPEAK(PeakL);
		PeakR = CALCPEAK(PeakR);
		PeakM = CALCPEAK(PeakM);
	}
	else
	{
		MYFLOAT stereoMul = 1.0;
		MYFLOAT monoMul = 1.0;
		if(inGaindB != GaindB)
		{
			GaindB = inGaindB;
			gainMult = (MYFLOAT) pow(10.0, GaindB / 20.0);
		}
		if(SampleRate != previousSampleRate || PreEmphasis != previousPreEmph)
		{
			if(PreEmphasis <= 0.0)
			{
				preEmphMult = 0.0;
			}
			else
			{
				preEmphMult = (MYFLOAT) exp(__PI2 / (PreEmphasis * SampleRate));
			}
			previousSampleRate = SampleRate;
			previousPreEmph = PreEmphasis;
		}
		float * ptr = samples;
		float * dstStereo = outputStereo;
		float * dstMono = outputMono;

		if(stereoAttenuation < -Stepup)
		{
			stereoAttenuation += Stepup;
			stereoMul = (MYFLOAT) pow(10.0, stereoAttenuation / 20.0);
		}
		else
		{
			stereoAttenuation = 0.0;
			stereoMul = 1.0;
		}
		stereoMul *= gainMult;
		if(monoAttenuation < -Stepup)
		{
			monoAttenuation += Stepup;
			monoMul = (MYFLOAT) pow(10.0, monoAttenuation / 20.0);
		}
		else
		{
			monoAttenuation = 0.0;
			monoMul = 1.0;
		}
		monoMul *= gainMult;
		for(int i = 0; i < len; i++)
		{
			float lft = *(ptr++);
			float rgt = *(ptr++);
			float mon = lft + rgt;
			lastLeftFixedSample = (lft + preEmphMult * lastLeftSample) * stereoMul;
			lastRightFixedSample = (rgt + preEmphMult * lastRightSample) * stereoMul;
			lastMonoFixedSample = (mon + preEmphMult * lastMonoSample) * monoMul;
			*(dstStereo++) = (float) lastLeftFixedSample;
			*(dstStereo++) = (float) lastRightFixedSample;
			*(dstMono++) = (float) lastMonoFixedSample;
			dstMono++; // skip it - filled in later
			MYFLOAT val = fabs(lastLeftFixedSample);
			if(val > PeakL) PeakL = val;
			val = fabs(lastRightFixedSample);
			if(val > PeakR) PeakR = val;
			val = fabs(lastMonoFixedSample);
			if(val > PeakM) PeakM = val;
			lastLeftSample = lft;
			lastRightSample = rgt;
			lastMonoSample = mon;
		}

		PeakL = CALCPEAK(PeakL);
		PeakR = CALCPEAK(PeakR);
		PeakM = CALCPEAK(PeakM);
		PeakS = (PeakL > PeakR) ? PeakL : PeakR;
		// USE PEAK INSTEAD
		if(PeakS > LimitdB)
		{
			MYFLOAT atten = LimitdB - PeakS;
			PeakL += atten;
			PeakR += atten;
			stereoAttenuation += atten;
			stereoMul = (float) pow(10.0, atten / 20.0);
		}
		else
		{
			stereoMul = 1.0;
		}

		if(PeakM > LimitdB)
		{
			MYFLOAT atten = LimitdB - PeakM;
			PeakM += atten;
			monoAttenuation += atten;
			monoMul = (float) pow(10.0, atten / 20.0);
		}
		else
		{
			monoMul = 1.0;
		}

		dstStereo = outputStereo;
		dstMono = outputMono;
		monoWillClip = stereoWillClip = leftWillClip = rightWillClip = false;
		for(int i=0; i < len; i++)
		{
			MYFLOAT lft, rgt, mon;
			if(deemphasis)
			{
			// CALCULATE RMS IN HERE
				float val = *dstStereo;
				lft = (val - lastLeftFixedSample * preEmphMult) * stereoMul;
				MYFLOAT clipper = fabs(lft);
				if(clipper > 1.0)
				{
					lft /= clipper;
					leftWillClip = true;
				}
				*(dstStereo++) = (float) lft;
				lastLeftFixedSample = val;

				val = *dstStereo;
				rgt = (val - lastRightFixedSample * preEmphMult) * stereoMul;
				clipper = fabs(rgt);
				if(clipper > 1.0)
				{
					rgt /= clipper;
					rightWillClip = true;
				}
				*(dstStereo++) = (float) rgt;
				lastRightFixedSample = val;

				val = *dstMono;
				mon = (val - lastMonoFixedSample * preEmphMult) * monoMul;
				clipper = fabs(mon);
				if(clipper > 1.0)
				{
					mon /= clipper;
					monoWillClip = true;
				}
				*(dstMono++) = *(dstMono++) = (float) mon;
				lastMonoFixedSample = val;
				sumLeft += lft * lft;
				sumRight += rgt * rgt;
				sumMono += mon * mon;
			}
			else
			{
			// CALCULATE RMS IN HERE
				lft = lastLeftFixedSample = (*(dstStereo) * (float) stereoMul);
				rgt = lastRightFixedSample = (*(dstStereo+1) * (float) stereoMul);
			
				mon = lastMonoFixedSample = (*(dstMono) * (float) monoMul);
				MYFLOAT clipper = fabs(lft);
				if(clipper > 1.0)
				{
					lft /= clipper;
					leftWillClip = true;
				}
				clipper = fabs(rgt);
				if(clipper > 1.0)
				{
					rgt /= clipper;
					rightWillClip = true;
				}
				clipper = fabs(mon);
				if(clipper > 1.0)
				{
					mon /= clipper;
					monoWillClip = true;
				}
				*(dstStereo++) = (float) lft;
				*(dstStereo++) = (float) rgt;
				*(dstMono++) = *(dstMono++) = (float) mon;
				sumLeft += lft * lft;
				sumRight += rgt * rgt;
				sumMono += mon * mon;
			}
		}
		stereoWillClip = (leftWillClip || rightWillClip);
	}
	RmsL = CALCRMS(sumLeft, len);
	RmsR = CALCRMS(sumRight, len);
	RmsM = CALCRMS(sumMono, len);
}

void limitMonoToStereoMono::limit(float * samples, int len, int inSampleRate, MYFLOAT inPreEmphasis, MYFLOAT inLimitdB, MYFLOAT inGaindB)
{
	if(dst == NULL || len > dst_size)
	{
		if(dst == NULL)
		{
			dst = (float *) malloc(len * sizeof(float) * 2);
		}
		else
		{
			dst = (float *) realloc((void *) dst, len * sizeof(float) * 2);
		}
		dst_size = len;
	}
	outputSize = len;
	if(inSampleRate > 0) 
	{
		SampleRate = inSampleRate;
	}
	if(inPreEmphasis >= 0)
	{
		PreEmphasis = inPreEmphasis;
	}
	if(inLimitdB <= 0.0)
	{
		LimitdB = inLimitdB;
	}
	outputMono = dst;
	outputStereo = dst;
	MYFLOAT sumMono = 0.0;
	PeakM = 0.0;
	if(SampleRate <= 0 || inLimitdB > 19.9)
	{
		float * dstMono = dst;
		float * srcMono = samples;
		for(int i = 0; i < len; i++)
		{
			MYFLOAT val = *(srcMono++);
			*(dstMono++) = *(dstMono++) = (float) val;
			sumMono += val * val;
			val = fabs(val);
			if (val > PeakM) PeakM = val;
		}
		PeakM = CALCPEAK(PeakM);
	}
	else
	{
		MYFLOAT monoMul = 1.0;
		if(inGaindB != GaindB)
		{
			GaindB = inGaindB;
			gainMult = (MYFLOAT) pow(10.0, GaindB / 20.0);
		}

		if(SampleRate != previousSampleRate || PreEmphasis != previousPreEmph)
		{
			if(PreEmphasis <= 0.0)
			{
				preEmphMult = 0.0;
			}
			else
			{
				preEmphMult = (MYFLOAT) exp(__PI2 / (PreEmphasis * SampleRate));
			}
			previousSampleRate = SampleRate;
			previousPreEmph = PreEmphasis;
		}
		float * ptr = samples;
		float * dstMono = outputMono;

		if(monoAttenuation < -Stepup)
		{
			monoAttenuation += Stepup;
			monoMul = (MYFLOAT) pow(10.0, monoAttenuation / 20.0);
		}
		else
		{
			monoAttenuation = 0.0;
			monoMul = 1.0;
		}
		monoMul *= gainMult;
		for(int i = 0; i < len; i++)
		{
			float mon = *(ptr++);
			lastMonoFixedSample = (mon + preEmphMult * lastMonoSample) * monoMul;
			*(dstMono++) = (float) lastMonoFixedSample;
			dstMono++;
			MYFLOAT val = fabs(lastMonoFixedSample);
			if(val > PeakM) PeakM = val;
			lastMonoSample = mon;
		}

		PeakM = CALCPEAK(PeakM);
		// USE PEAK INSTEAD
		if(PeakM > LimitdB)
		{
			MYFLOAT atten = LimitdB - PeakM;
			PeakM += atten;
			monoAttenuation += atten;
			monoMul = (float) pow(10.0, atten / 20.0);
		}
		else
		{
			monoMul = 1.0;
		}

		dstMono = outputMono;
		monoWillClip = false;
		for(int i=0; i < len; i++)
		{
			MYFLOAT mono;
			if(deemphasis)
			{
			// CALCULATE RMS IN HERE
				float val = *dstMono;
				mono = (val - lastMonoFixedSample * preEmphMult) * monoMul;
				MYFLOAT clipper = fabs(mono);
				if(clipper > 1.0)
				{
					monoWillClip = true;
					mono /= clipper;
				}
				*(dstMono++) = *(dstMono++) = (float) mono;
				lastMonoFixedSample = val;
				sumMono += mono * mono;
			}
			else
			{
			// CALCULATE RMS IN HERE
				mono = lastMonoFixedSample = (*(dstMono) * (float) monoMul);
				MYFLOAT clipper = fabs(mono);
				if(clipper > 1.0)
				{
					monoWillClip = true;
					mono /= clipper;
				}
				*(dstMono++) = *(dstMono++) = (float) mono;
				sumMono += mono * mono;
			}
		}
	}
	RmsL = RmsR = RmsM = CALCRMS(sumMono, len);
	PeakL = PeakR = PeakM;
}

limitMultiMono::limitMultiMono(int numChannels)
{
	channels = numChannels; 
	sourceIsStereo = false; 
	PeakChannel = (MYFLOAT *) malloc(sizeof(MYFLOAT) * channels);
	RmsChannel = (MYFLOAT *) malloc(sizeof(MYFLOAT) * channels);
	channelWillClip  = (BOOL *) malloc(sizeof(BOOL) * channels);
	lastChannelSample = (MYFLOAT *) malloc(sizeof(MYFLOAT) * channels);
	lastChannelFixedSample = (MYFLOAT *) malloc(sizeof(MYFLOAT) * channels);
	PreEmphasisChannel = (MYFLOAT *) malloc(sizeof(MYFLOAT) * channels);
	LimitdBChannel = (MYFLOAT *) malloc(sizeof(MYFLOAT) * channels);
	attenuationChannel = (MYFLOAT *) malloc(sizeof(MYFLOAT) * channels);
	previousPreEmphChannel = (MYFLOAT *) malloc(sizeof(MYFLOAT) * channels);
	preEmphMultChannel = (MYFLOAT *) malloc(sizeof(MYFLOAT) * channels);
	outputMonoChannel = (float **) malloc(sizeof(float *) * channels);
	gainMultChannel = (MYFLOAT *) malloc(sizeof(MYFLOAT) * channels);
	GaindBChannel = (MYFLOAT *) malloc(sizeof(MYFLOAT) * channels);
	for(int i = 0; i < channels; i++)
	{
		PeakChannel[i] = 0.0;
		RmsChannel[i] = 0.0;
		channelWillClip[i]  = false;
		lastChannelSample[i] = 0.0;
		lastChannelFixedSample[i] = 0.0;
		PreEmphasisChannel[i] = 0.0;
		LimitdBChannel[i] = 20.0;
		attenuationChannel[i] = 0.0;
		previousPreEmphChannel[i] = 0.0;
		gainMultChannel[i] = 1.0;
		GaindBChannel[i] = 0.0;
	}
}
limitMultiMono::~limitMultiMono()
{
	SafeFree(PeakChannel);
	SafeFree(RmsChannel);
	SafeFree(channelWillClip);
	SafeFree(lastChannelSample);
	SafeFree(lastChannelFixedSample);
	SafeFree(PreEmphasisChannel);
	SafeFree(LimitdBChannel);
	SafeFree(attenuationChannel);
	SafeFree(previousPreEmphChannel);
	SafeFree(preEmphMultChannel);
	SafeFree(outputMonoChannel);
	SafeFree(gainMultChannel);
	SafeFree(GaindBChannel);
}

void limitMultiMono::multiLimit(float * samples, int len, int inSampleRate, MYFLOAT * inPreEmphasis, MYFLOAT * inLimitdB, MYFLOAT * inGaindB)
{
	outputSize = len;
	int blockOffset = len * 2;
	if(dst == NULL || len > dst_size)
	{
		if(dst == NULL)
		{
			dst = (float *) malloc(len * sizeof(float) * channels * 2);
		}
		else
		{
			dst = (float *) realloc((void *) dst, len * sizeof(float) * channels * 2);
		}
		dst_size = len;
	}
	
	outputMono = dst;

	PeakM = -120.0;
	RmsM = -120.0;

	if(inSampleRate > 0) 
	{
		SampleRate = inSampleRate;
	}

	for(int c = 0; c < channels; c++)
	{
		outputMonoChannel[c] = outputMono + (blockOffset * c);
		PeakChannel[c] = 0.0;
		if(inGaindB)
		{
			if(inGaindB[c] != GaindBChannel[c])
			{
				GaindBChannel[c] = inGaindB[c];
				gainMultChannel[c] = (MYFLOAT) pow(10.0, GaindBChannel[c] / 20.0);
			}
		}
		if(inPreEmphasis)
		{
			if(inPreEmphasis[c] >= 0)
			{
				PreEmphasisChannel[c] = inPreEmphasis[c];
			}
		}
		else
		{
			PreEmphasisChannel[c] = 0.0;
		}
		if(inLimitdB)
		{
			if(inLimitdB[c] < 20.0)
			{
				LimitdBChannel[c] = inLimitdB[c];
			}
		}
		else
		{
			LimitdBChannel[c] = 20.0;
		}
		MYFLOAT sumMono = 0.0;
		MYFLOAT monoMul = 1.0;
		
		if(SampleRate != previousSampleRate || PreEmphasisChannel[c] != previousPreEmphChannel[c])
		{
			if(PreEmphasisChannel[c] <= 0.0)
			{
				preEmphMultChannel[c] = 0.0;
			}
			else
			{
				preEmphMultChannel[c] = (MYFLOAT) exp(__PI2 / (PreEmphasisChannel[c] * SampleRate));
			}
			previousPreEmphChannel[c] = PreEmphasisChannel[c];
		}
		float * ptr = &samples[c];
		float * dstMono = outputMonoChannel[c];

		if(attenuationChannel[c] < -Stepup)
		{
			attenuationChannel[c] += Stepup;
			monoMul = (MYFLOAT) pow(10.0, attenuationChannel[c] / 20.0);
		}
		else
		{
			attenuationChannel[c] = 0.0;
			monoMul = 1.0;
		}
		monoMul *= gainMultChannel[c];
		for(int i = 0; i < len; i++)
		{
			float mon = *ptr;
			ptr += channels;
			lastChannelFixedSample[c] = (mon + preEmphMultChannel[c] * lastChannelSample[c]) * monoMul;
			*(dstMono++) = (float) lastChannelFixedSample[c];
			dstMono++;
			MYFLOAT val = fabs(lastChannelFixedSample[c]);
			if(val > PeakChannel[c]) PeakChannel[c] = val;
			lastChannelSample[c] = mon;
		}
		PeakChannel[c] = CALCPEAK(PeakChannel[c]);
		if(PeakChannel[c] > PeakM) PeakM = PeakChannel[c];
		// USE PEAK INSTEAD
		if(PeakChannel[c] > LimitdBChannel[c])
		{
			MYFLOAT atten = LimitdBChannel[c] - PeakChannel[c];
			PeakChannel[c] += atten;
			attenuationChannel[c] += atten;
			monoMul = (float) pow(10.0, atten / 20.0);
		}
		else
		{
			monoMul = 1.0;
		}
		dstMono = outputMonoChannel[c];
		channelWillClip[c] = false;
		for(int i=0; i < len; i++)
		{
			MYFLOAT mono;
			if(deemphasis)
			{
			// CALCULATE RMS IN HERE
				float val = *dstMono;
				mono = (val - lastChannelFixedSample[c] * preEmphMultChannel[c]) * monoMul;
				MYFLOAT clipper = fabs(mono);
				if(clipper > 1.0)
				{
					monoWillClip = channelWillClip[c] = true;
					mono /= clipper;
				}
				*(dstMono++) = *(dstMono++) = (float) mono;
				lastChannelFixedSample[c] = val;
				sumMono += mono * mono;
			}
			else
			{
			// CALCULATE RMS IN HERE
				mono = lastChannelFixedSample[c] = (*(dstMono) * (float) monoMul);
				MYFLOAT clipper = fabs(mono);
				if(clipper > 1.0)
				{
					monoWillClip = channelWillClip[c] = true;
					mono /= clipper;
				}
				*(dstMono++) = *(dstMono++) = (float) mono;
				sumMono += mono * mono;
			}
		}
		RmsChannel[c] = CALCRMS(sumMono, len);
		if(RmsChannel[c] > RmsM)
		{
			RmsM = RmsChannel[c];
		}
	}
	RmsL = RmsR = RmsM;
	PeakL = PeakR = PeakM;
	leftWillClip = rightWillClip = monoWillClip;
	previousSampleRate = SampleRate;
}

void limitMultiMono::limit(float * samples, int len, int inSampleRate, MYFLOAT inPreEmphasis, MYFLOAT inLimitdB, MYFLOAT inGaindB)
{
	outputSize = len;
	int blockOffset = len * 2;
	if(dst == NULL || len > dst_size)
	{
		if(dst == NULL)
		{
			dst = (float *) malloc(len * sizeof(float) * channels * 2);
		}
		else
		{
			dst = (float *) realloc((void *) dst, len * sizeof(float) * channels * 2);
		}
		dst_size = len;
	}
	
	outputMono = dst;

	PeakM = -120.0;
	RmsM = -120.0;

	if(inSampleRate > 0) 
	{
		SampleRate = inSampleRate;
	}

	for(int c = 0; c < channels; c++)
	{
		outputMonoChannel[c] = outputMono + (blockOffset * c);
		PeakChannel[c] = 0.0;
		if(inGaindB)
		{
			if(inGaindB != GaindBChannel[c])
			{
				GaindBChannel[c] = inGaindB;
				gainMultChannel[c] = (MYFLOAT) pow(10.0, GaindBChannel[c] / 20.0);
			}
		}
		if(inPreEmphasis)
		{
			if(inPreEmphasis >= 0)
			{
				PreEmphasisChannel[c] = inPreEmphasis;
			}
		}
		else
		{
			PreEmphasisChannel[c] = 0.0;
		}
		if(inLimitdB)
		{
			if(inLimitdB < 20.0)
			{
				LimitdBChannel[c] = inLimitdB;
			}
		}
		else
		{
			LimitdBChannel[c] = 20.0;
		}
		MYFLOAT sumMono = 0.0;
		MYFLOAT monoMul = 1.0;
		
		if(SampleRate != previousSampleRate || PreEmphasisChannel[c] != previousPreEmphChannel[c])
		{
			if(PreEmphasisChannel[c] <= 0.0)
			{
				preEmphMultChannel[c] = 0.0;
			}
			else
			{
				preEmphMultChannel[c] = (MYFLOAT) exp(__PI2 / (PreEmphasisChannel[c] * SampleRate));
			}
			previousPreEmphChannel[c] = PreEmphasisChannel[c];
		}
		float * ptr = &samples[c];
		float * dstMono = outputMonoChannel[c];

		if(attenuationChannel[c] < -Stepup)
		{
			attenuationChannel[c] += Stepup;
			monoMul = (MYFLOAT) pow(10.0, attenuationChannel[c] / 20.0);
		}
		else
		{
			attenuationChannel[c] = 0.0;
			monoMul = 1.0;
		}
		monoMul *= gainMultChannel[c];
		for(int i = 0; i < len; i++)
		{
			float mon = *ptr;
			ptr += channels;
			lastChannelFixedSample[c] = (mon + preEmphMultChannel[c] * lastChannelSample[c]) * monoMul;
			*(dstMono++) = (float) lastChannelFixedSample[c];
			dstMono++;
			MYFLOAT val = fabs(lastChannelFixedSample[c]);
			if(val > PeakChannel[c]) PeakChannel[c] = val;
			lastChannelSample[c] = mon;
		}
		PeakChannel[c] = CALCPEAK(PeakChannel[c]);
		if(PeakChannel[c] > PeakM) PeakM = PeakChannel[c];
		// USE PEAK INSTEAD
		if(PeakChannel[c] > LimitdBChannel[c])
		{
			MYFLOAT atten = LimitdBChannel[c] - PeakChannel[c];
			PeakChannel[c] += atten;
			attenuationChannel[c] += atten;
			monoMul = (float) pow(10.0, atten / 20.0);
		}
		else
		{
			monoMul = 1.0;
		}
		dstMono = outputMonoChannel[c];
		channelWillClip[c] = false;
		for(int i=0; i < len; i++)
		{
			MYFLOAT mono;
			if(deemphasis)
			{
			// CALCULATE RMS IN HERE
				float val = *dstMono;
				mono = (val - lastChannelFixedSample[c] * preEmphMultChannel[c]) * monoMul;
				MYFLOAT clipper = fabs(mono);
				if(clipper > 1.0)
				{
					monoWillClip = channelWillClip[c] = true;
					mono /= clipper;
				}
				*(dstMono++) = *(dstMono++) = (float) mono;
				lastChannelFixedSample[c] = val;
				sumMono += mono * mono;
			}
			else
			{
			// CALCULATE RMS IN HERE
				mono = lastChannelFixedSample[c] = (*(dstMono) * (float) monoMul);
				MYFLOAT clipper = fabs(mono);
				if(clipper > 1.0)
				{
					monoWillClip = channelWillClip[c] = true;
					mono /= clipper;
				}
				*(dstMono++) = *(dstMono++) = (float) mono;
				sumMono += mono * mono;
			}
		}
		RmsChannel[c] = CALCRMS(sumMono, len);
		if(RmsChannel[c] > RmsM)
		{
			RmsM = RmsChannel[c];
		}
	}
	RmsL = RmsR = RmsM;
	PeakL = PeakR = PeakM;
	leftWillClip = rightWillClip = monoWillClip;
	previousSampleRate = SampleRate;
}
/*
void limitMultiMono::limit(float * samples, int len, int inSampleRate, MYFLOAT inPreEmphasis, MYFLOAT inLimitdB, MYFLOAT inGaindB)
{
	outputSize = len;
	if(dst == NULL || len > dst_size)
	{
		if(dst == NULL)
		{
			dst = (float *) malloc(len * sizeof(float) * channels * 2);
		}
		else
		{
			dst = (float *) realloc((void *) dst, len * sizeof(float) * channels * 2);
		}
		dst_size = len;
	}
	len *= channels;
	if(inSampleRate > 0) 
	{
		SampleRate = inSampleRate;
	}
	if(inPreEmphasis >= 0)
	{
		PreEmphasis = inPreEmphasis;
	}
	if(inLimitdB <= 0.0)
	{
		LimitdB = inLimitdB;
	}
	outputMono = dst;
	MYFLOAT sumMono = 0.0;
	float * dstMono = dst;
	float * srcMono = samples;
	PeakM = 0.0;
	for(int i = 0; i < len; i++)
	{
		MYFLOAT val = *(srcMono++);
		*(dstMono++) = *(dstMono++) = (float) val;
		sumMono += val * val;
		val = fabs(val);
		if (val > PeakM) PeakM = val;
		if (val > 1.0) monoWillClip = true;
	}

	RmsL = RmsR = CALCRMS(sumMono, len);
	PeakL = PeakR = CALCPEAK(PeakM);

	leftWillClip = rightWillClip = monoWillClip;
}
*/

//ShortInt
inline void makeShortIntMono(float * samples, short int * dst, int numSamples)
{
	while(numSamples--)
	{
		*(dst++) = (short int)(*(samples++) * 32767.0);
		samples++;
	}
}

inline void makeShortIntMonoMono(float * samplesLeft, float * samplesRight, short int * dst, int numSamples)
{
	while(numSamples--)
	{
		*(dst++) = (short int)(*(samplesLeft++) * 32767.0);
		samplesLeft++;
		samplesRight++;
		*(dst++) = (short int)(*(samplesRight++) * 32767.0);
	}
}

inline void makeShortIntStereo(float * samples, short int * dst, int numSamples)
{
	while(numSamples--)
	{
		*(dst++) = (short int)(*(samples++) * 32767.0);
		*(dst++) = (short int)(*(samples++) * 32767.0);
	}
}

void makeShortInt(Limiters * limiter, short int * dstSamples, int numSamples, int dstChannels, int leftChannel, int rightChannel)
{
	int srcChannels = limiter->channels;

	if(srcChannels < 3)
	{
		if(dstChannels == 1) // 1:1
		{
			makeShortIntMono(limiter->outputMono, dstSamples, numSamples);
		}
		else // if(dstChannels == 2) // 2:2
		{
			makeShortIntStereo(limiter->outputStereo, dstSamples, numSamples);
		}
	}
	else // channel x[y] -> mono[stereo]
	{
		if(dstChannels == 1) // 1:1
		{
			int srcoffsetM = limiter->outputSize * 2 * leftChannel;
			float * srcM = (limiter->outputMono) + srcoffsetM;

			makeShortIntMono(srcM, dstSamples, numSamples);
		}
		else // if(dstChannels == 2) // 2:2
		{
			int srcoffsetL = limiter->outputSize * 2 * leftChannel;
			float * srcL = (limiter->outputStereo) + srcoffsetL;

			int srcoffsetR = limiter->outputSize * 2 * rightChannel;
			float * srcR = (limiter->outputStereo) + srcoffsetR;

			makeShortIntMonoMono(srcL, srcR, dstSamples, numSamples);
		}
	}
}
// LongInt
inline void makeLongIntMono(float * samples, long * dst, int numSamples)
{
	while(numSamples--)
	{
		*(dst++) = (long)(*(samples++) * 32767.0);
		samples++;
	}
}

inline void makeLongIntMonoMono(float * samplesLeft, float * samplesRight, long * dst, int numSamples)
{
	while(numSamples--)
	{
		*(dst++) = (long)(*(samplesLeft++) * 32767.0);
		samplesLeft++;
		samplesRight++;
		*(dst++) = (long)(*(samplesRight++) * 32767.0);
	}
}

inline void makeLongIntStereo(float * samples, long * dst, int numSamples)
{
	while(numSamples--)
	{
		*(dst++) = (long)(*(samples++) * 32767.0);
		*(dst++) = (long)(*(samples++) * 32767.0);
	}
}

void makeLongInt(Limiters * limiter, long * dstSamples, int numSamples, int dstChannels, int leftChannel, int rightChannel)
{
	int srcChannels = limiter->channels;

	if(srcChannels < 3)
	{
		if(dstChannels == 1) // 1:1
		{
			makeLongIntMono(limiter->outputMono, dstSamples, numSamples);
		}
		else // if(dstChannels == 2) // 2:2
		{
			makeLongIntStereo(limiter->outputStereo, dstSamples, numSamples);
		}
	}
	else // channel x[y] -> mono[stereo]
	{
		if(dstChannels == 1) // 1:1
		{
			int srcoffsetM = limiter->outputSize * 2 * leftChannel;
			float * srcM = (limiter->outputMono) + srcoffsetM;

			makeLongIntMono(srcM, dstSamples, numSamples);
		}
		else // if(dstChannels == 2) // 2:2
		{
			int srcoffsetL = limiter->outputSize * 2 * leftChannel;
			float * srcL = (limiter->outputStereo) + srcoffsetL;

			int srcoffsetR = limiter->outputSize * 2 * rightChannel;
			float * srcR = (limiter->outputStereo) + srcoffsetR;

			makeLongIntMonoMono(srcL, srcR, dstSamples, numSamples);
		}
	}
}
//  faacFloat
inline void makeFaacFloatMono(float * samples, float * dst, int numSamples)
{
	while(numSamples--)
	{
		*(dst++) = (float)(*(samples++) * 32767.0);
		samples++;
	}
}

inline void makeFaacFloatMonoMono(float * samplesLeft, float * samplesRight, float * dst, int numSamples)
{
	while(numSamples--)
	{
		*(dst++) = (float)(*(samplesLeft++) * 32767.0);
		samplesLeft++;
		samplesRight++;
		*(dst++) = (float)(*(samplesRight++) * 32767.0);
	}
}

inline void makeFaacFloatStereo(float * samples, float * dst, int numSamples)
{
	while(numSamples--)
	{
		*(dst++) = (float)(*(samples++) * 32767.0);
		*(dst++) = (float)(*(samples++) * 32767.0);
	}
}

void makeFaacFloat(Limiters * limiter, float * dstSamples, int numSamples, int dstChannels, int leftChannel, int rightChannel)
{
	int srcChannels = limiter->channels;

	if(srcChannels < 3)
	{
		if(dstChannels == 1) // 1:1
		{
			makeFaacFloatMono(limiter->outputMono, dstSamples, numSamples);
		}
		else // if(dstChannels == 2) // 2:2
		{
			makeFaacFloatStereo(limiter->outputStereo, dstSamples, numSamples);
		}
	}
	else // channel x[y] -> mono[stereo]
	{
		if(dstChannels == 1) // 1:1
		{
			int srcoffsetM = limiter->outputSize * 2 * leftChannel;
			float * srcM = (limiter->outputMono) + srcoffsetM;

			makeFaacFloatMono(srcM, dstSamples, numSamples);
		}
		else // if(dstChannels == 2) // 2:2
		{
			int srcoffsetL = limiter->outputSize * 2 * leftChannel;
			float * srcL = (limiter->outputStereo) + srcoffsetL;

			int srcoffsetR = limiter->outputSize * 2 * rightChannel;
			float * srcR = (limiter->outputStereo) + srcoffsetR;

			makeFaacFloatMonoMono(srcL, srcR, dstSamples, numSamples);
		}
	}
}

//  flacFloat
inline void makeFlacFloatMono(float * samples, float * * ppdst, int numSamples)
{
	float * dst = ppdst[0];
	while(numSamples--)
	{
		*(dst++) = (float)(*(samples++) * 32767.0);
		samples++;
	}
}

inline void makeFlacFloatMonoMono(float * samplesLeft, float * samplesRight, float * * ppdst, int numSamples)
{
	float * left = ppdst[0];
	float * right = ppdst[1];

	while(numSamples--)
	{
		*(left++) = (float)(*(samplesLeft++) * 32767.0);
		samplesLeft++;
		samplesRight++;
		*(right++) = (float)(*(samplesRight++) * 32767.0);
	}
}

inline void makeFlacFloatStereo(float * samples, float * * ppdst, int numSamples)
{
	float * left = ppdst[0];
	float * right = ppdst[1];

	while(numSamples--)
	{
		*(left++) = (float)(*(samples++) * 32767.0);
		*(right++) = (float)(*(samples++) * 32767.0);
	}
}

void makeFlacFloat(Limiters * limiter, float * * dstSamples, int numSamples, int dstChannels, int leftChannel, int rightChannel)
{
	int srcChannels = limiter->channels;

	if(srcChannels < 3)
	{
		if(dstChannels == 1) // 1:1
		{
			makeFlacFloatMono(limiter->outputMono, dstSamples, numSamples);
		}
		else // if(dstChannels == 2) // 2:2
		{
			makeFlacFloatStereo(limiter->outputStereo, dstSamples, numSamples);
		}
	}
	else // channel x[y] -> mono[stereo]
	{
		if(dstChannels == 1) // 1:1
		{
			int srcoffsetM = limiter->outputSize * 2 * leftChannel;
			float * srcM = (limiter->outputMono) + srcoffsetM;

			makeFlacFloatMono(srcM, dstSamples, numSamples);
		}
		else // if(dstChannels == 2) // 2:2
		{
			int srcoffsetL = limiter->outputSize * 2 * leftChannel;
			float * srcL = (limiter->outputStereo) + srcoffsetL;

			int srcoffsetR = limiter->outputSize * 2 * rightChannel;
			float * srcR = (limiter->outputStereo) + srcoffsetR;

			makeFlacFloatMonoMono(srcL, srcR, dstSamples, numSamples);
		}
	}
}
