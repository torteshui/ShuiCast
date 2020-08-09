#pragma once
#pragma warning( disable : 4100 )  // unreferenced params

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define MYFLOAT double

class Limiters
{
public:
	Limiters();
	~Limiters();
	Limiters(MYFLOAT inPreEmphasis, int inSampleRate = 0, MYFLOAT inLimitdB = 1.0, MYFLOAT inGaindB = 0.0);
	Limiters(int inSampleRate, double inPreEmphasis = 0.0);
	virtual void limit(float * samples, int len, int inSampleRate = 0, MYFLOAT inPreEmphasis = -1.0, MYFLOAT inLimitdB = 1.0, MYFLOAT inGaindB = 0.0) = 0;
	virtual void multiLimit(float * samples, int len, int inSampleRate, MYFLOAT * inPreEmphasis, MYFLOAT * inLimitdB, MYFLOAT * inGaindB);
	float * outputStereo;
	float * outputMono;
	long outputSize;
	MYFLOAT PeakS;
	MYFLOAT PeakL;
	MYFLOAT PeakR;
	MYFLOAT PeakM;
	MYFLOAT RmsS;
	MYFLOAT RmsL;
	MYFLOAT RmsR;
	MYFLOAT RmsM;
	BOOL monoWillClip;
	BOOL leftWillClip;
	BOOL rightWillClip;
	BOOL stereoWillClip;
	BOOL sourceIsStereo;
	BOOL deemphasis;
	int SampleRate;
	int channels;
protected:
	float * dst;
	MYFLOAT PreEmphasis;
	MYFLOAT LimitdB;
	MYFLOAT GaindB;
	long dst_size;

	MYFLOAT lastLeftSample;
	MYFLOAT lastRightSample;
	MYFLOAT lastMonoSample;
	MYFLOAT lastLeftFixedSample;
	MYFLOAT lastRightFixedSample;
	MYFLOAT lastMonoFixedSample;

	int previousSampleRate;
	MYFLOAT previousPreEmph;
	MYFLOAT stereoAttenuation;
	MYFLOAT monoAttenuation;
	MYFLOAT preEmphMult;
	MYFLOAT gainMult;

	void SafeFree(void * b);
};
class limitStereoToStereoMono : public Limiters
{
public:
	limitStereoToStereoMono() { sourceIsStereo = true; channels = 2; }
	~limitStereoToStereoMono() {};
	void limit(float * samples, int len, int inSampleRate = 0, MYFLOAT inPreEmphasis = -1.0, MYFLOAT inLimitdB = 1.0, MYFLOAT inGaindB = 0.0);
};
class limitMonoToStereoMono : public Limiters
{
public:
	limitMonoToStereoMono() { sourceIsStereo = false; channels = 1; }
	~limitMonoToStereoMono() {};
	void limit(float * samples, int len, int inSampleRate = 0, MYFLOAT inPreEmphasis = -1.0, MYFLOAT inLimitdB = 1.0, MYFLOAT inGaindB = 0.0);
};

struct multiChannelInfo
{
	MYFLOAT lastChannelSample;
	MYFLOAT lastChannelFixedSample;
	MYFLOAT PreEmphasisChannel;
	MYFLOAT LimitdBChannel;
	MYFLOAT GaindBChannel;
	MYFLOAT attenuationChannel;
	MYFLOAT previousPreEmphChannel;
	MYFLOAT gainMultChannel;
};

class limitMultiMono : public Limiters
{
public:
	limitMultiMono(int numChannels);
	~limitMultiMono();
	void limit(float * samples, int len, int inSampleRate = 0, MYFLOAT inPreEmphasis = -1.0, MYFLOAT inLimitdB = 1.0, MYFLOAT inGaindB = 0.0);
	void multiLimit(float * samples, int len, int inSampleRate, MYFLOAT * inPreEmphasis, MYFLOAT * inLimitdB, MYFLOAT * inGaindB);

	MYFLOAT * PeakChannel;
	MYFLOAT * RmsChannel;
	BOOL * channelWillClip;
	float ** outputMonoChannel;
private:
	MYFLOAT * lastChannelSample;
	MYFLOAT * lastChannelFixedSample;
	MYFLOAT * PreEmphasisChannel;
	MYFLOAT * LimitdBChannel;
	MYFLOAT * GaindBChannel;
	MYFLOAT * previousPreEmphChannel;
	MYFLOAT * attenuationChannel;
	MYFLOAT * preEmphMultChannel;
	MYFLOAT * gainMultChannel;
};
