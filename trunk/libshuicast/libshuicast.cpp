#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/timeb.h>
#include <time.h>
#include <stdarg.h>
#include <math.h>
#include <ShlObj.h>

#ifdef HAVE_VORBIS
#include <vorbis/vorbisenc.h>
#endif

#include "libshuicast.h"
#include "libshuicast_socket.h"
#ifdef WIN32
//#include <bass.h>
#else
#ifdef HAVE_LAME
#include <lame/lame.h>
#endif
#include <errno.h>
#endif
#ifdef HAVE_FAAC
#include <faac.h>
#endif
#ifndef LAME_MAXMP3BUFFER
#define LAME_MAXMP3BUFFER	16384
#endif
#define GUINT16_TO_LE(val)		((unsigned short) (val))
#define GUINT32_TO_LE(val)		((unsigned int) (val))
#define GUINT16_FROM_LE(val)	(GUINT16_TO_LE(val))
#ifdef WIN32
//#pragma comment(linker, "/delayload:libfaac.dll")
#define FILE_SEPARATOR	"\\"
#else
#define FILE_SEPARATOR	"/"
typedef struct
{
	short	wFormatTag;
	short	nChannels;
	long	nSamplesPerSec;
	long	nAvgBytesPerSec;
	short	nBlockAlign;
	short	wBitsPerSample;
	short	cbSize;
} WAVEFORMATEX;
#define WAVE_FORMAT_PCM (0x0001)
#define DWORD			long
#endif
long	dwDataBlockPos = 0;
long	TotalWritten = 0;


#ifdef WIN32
#define INT32	__int32
#else
#define INT32	int
#endif

#define MAX_ENCODERS 10

extern shuicastGlobals			*g[MAX_ENCODERS];
extern shuicastGlobals			gMain;

int buffer_blocksize = 0;

//double _attenTable[11];
static char * AsciiToUtf8(char * ascii);

#ifdef HAVE_AACP

/*
 =======================================================================================================================
    uninteresting stuff ;
    for the input plugin
 =======================================================================================================================
 */
static void SAAdd(void *data, int timestamp, int csa)
{
}

static void VSAAdd(void *data, int timestamp)
{
}

static void SAVSAInit(int maxlatency_in_ms, int srate)
{
}

static void SAVSADeInit() 
{
}

static void SAAddPCMData(void *PCMData, int nch, int bps, int timestamp)
{
}

static int SAGetMode() 
{
	return 0;
}

static int VSAGetMode(int *specNch, int *waveNch)
{
	return 0;
}

static void VSAAddPCMData(void *PCMData, int nch, int bps, int timestamp)
{
}

static void VSASetInfo(int nch, int srate)
{
}

static int dsp_isactive() 
{
	return 0;
}

static int dsp_dosamples(short int *samples, int ns, int bps, int nch, int srate)
{
	return ns;
}

static void SetInfo(int bitrate0, int srate0, int stereo, int synched) 
{
}
#endif

typedef struct tagConfigFileValue
{
	char_t	Variable[256];
	char_t	Value[256];
	//char_t	Description[1024];
}
configFileValue;

typedef struct tagconfigFileDesc
{
	char_t	Variable[256];
	char_t	Description[1024];
}
configFileDesc;

static configFileValue	configFileValues[200];
static configFileDesc	configFileDescs[200];
static int				numConfigValues = 0;

static int				greconnectFlag = 0;
char_t	defaultLogFileName[MAX_PATH] = "shuicast.log";
char_t defaultConfigDir[MAX_PATH];

void setDefaultLogFileName(char_t *filename)
{
	strcpy(defaultLogFileName, filename);
}

void setConfigDir(char_t *dirname)
{
	strcpy(defaultConfigDir, dirname);
}

int getReconnectFlag(shuicastGlobals *g) 
{
	return g->gAutoReconnect;
}

int getReconnectSecs(shuicastGlobals *g) 
{
	return g->gReconnectSec;
}

void addVorbisComment(shuicastGlobals *g, char_t *comment)
{
	int commentLen = strlen(comment) + 1;

	g->vorbisComments[g->numVorbisComments] = (char_t *) calloc(1, commentLen);
	if (g->vorbisComments[g->numVorbisComments])
	{
		memset(g->vorbisComments[g->numVorbisComments], '\000', commentLen);
		strcpy(g->vorbisComments[g->numVorbisComments], comment);
		g->numVorbisComments++;
	}
}

void freeVorbisComments(shuicastGlobals *g) 
{
	for(int i = 0; i < g->numVorbisComments; i++)
	{
		if(g->vorbisComments[i]) 
		{
			free(g->vorbisComments[i]);
			g->vorbisComments[i] = NULL;
		}
	}

	g->numVorbisComments = 0;
}

void addConfigVariable(shuicastGlobals *g, char_t *variable) 
{
	g->configVariables[g->numConfigVariables] = _strdup(variable);
	g->numConfigVariables++;
}

long getWritten(shuicastGlobals *g) 
{
	return g->written;
}

void setWritten(shuicastGlobals *g, long writ)
{
	g->written = writ;
}

void setAutoConnect(shuicastGlobals *g, int flag)
{
	g->autoconnect = flag;
}

void setLimiterFlag(shuicastGlobals *g, int flag)
{
	g->gLimiter = flag;
}

void setStartMinimizedFlag(shuicastGlobals *g, int flag)
{
	g->gStartMinimized = flag;
}

int getStartMinimizedFlag(shuicastGlobals *g)
{
	return g->gStartMinimized;
}

void setLimiterValues(shuicastGlobals *g, int db, int pre, int gain)
{
	g->gLimitdb = db;
	g->gLimitpre = pre;
	g->gGaindb = gain;
}

FILE *getSaveFileP(shuicastGlobals *g) 
{
	return g->gSaveFile;
}

int getLiveRecordingSetFlag(shuicastGlobals *g)
{
	return g->gLiveRecordingFlag;
}

#if 0
bool getLiveRecordingFlag(shuicastGlobals *g) 
{
	return g->areLiveRecording;
}

void setLiveRecordingFlag(shuicastGlobals *g, bool flag) 
{
	g->areLiveRecording = flag;
}
#endif

#if 0
int getLiveInSamplerate(shuicastGlobals *g) 
{
	return g->gLiveInSamplerate;
}

void setLiveInSamplerate(shuicastGlobals *g, int rate)
{
	g->gLiveInSamplerate = rate;
}
#endif

int getOggFlag(shuicastGlobals *g)
{
	return g->gOggFlag;
}

char_t *getServerDesc(shuicastGlobals *g) 
{
	return g->gServDesc;
}

char_t *getSourceURL(shuicastGlobals *g)
{
	return g->gSourceURL;
}

void setSourceURL(shuicastGlobals *g, char_t *url)
{
	strcpy(g->gSourceURL, url);
}

int getIsConnected(shuicastGlobals *g)
{
	return g->weareconnected;
}

long getCurrentSamplerate(shuicastGlobals *g) 
{
	return g->currentSamplerate;
}

int getCurrentBitrate(shuicastGlobals *g)
{
	return g->currentBitrate;
}

int getCurrentChannels(shuicastGlobals *g)
{
	return g->currentChannels;
}

double getAttenuation(shuicastGlobals *g)
{
	return g->dAttenuation;
}

void setSourceDescription(shuicastGlobals *g, char_t *desc)
{
	strcpy(g->sourceDescription, desc);
}

long getVUShow(shuicastGlobals *g)
{
	return g->vuShow;
}

void setVUShow(shuicastGlobals *g, long x)
{
	g->vuShow = x;
}

long getLastXWindow(shuicastGlobals *g)
{
	return g->lastX;
}

long getLastYWindow(shuicastGlobals *g)
{
	return g->lastY;
}

void setLastXWindow(shuicastGlobals *g, long x) 
{
	g->lastX = x;
}

void setLastYWindow(shuicastGlobals *g, long y) 
{
	g->lastY = y;
}

int getSaveAsWAV(shuicastGlobals *g)
{
	return g->gSaveAsWAV;
}

void setSaveAsWAV(shuicastGlobals *g, int flag) 
{
	g->gSaveAsWAV = flag;
}

int getForceDSP(shuicastGlobals *g)
{
	return g->gForceDSPrecording;
}

void setForceDSP(shuicastGlobals *g, int flag) 
{
	g->gForceDSPrecording = flag;
}

int getThreeHourBug(shuicastGlobals *g)
{
	return g->gThreeHourBug;
}

void setThreeHourBug(shuicastGlobals *g, int flag) 
{
	g->gThreeHourBug = flag;
}

int getSkipCloseWarning(shuicastGlobals *g)
{
	return g->gSkipCloseWarning;
}

void setSkipCloseWarning(shuicastGlobals *g, int flag) 
{
	g->gSkipCloseWarning = flag;
}

int getAsioSelectChannel(shuicastGlobals *g)
{
	return g->gAsioSelectChannel;
}

void setAsioSelectChannel(shuicastGlobals *g, int flag)
{
	g->gAsioSelectChannel = flag;
}

char_t *getAsioChannel(shuicastGlobals *g)
{
	return(g->gAsioChannel);
}

void setAsioChannel(shuicastGlobals *g, char_t *name) 
{
	strcpy(g->gAsioChannel, name);
}

int getEnableEncoderScheduler(shuicastGlobals *g)
{
	return g->gEnableEncoderScheduler;
}

void setEnableEncoderScheduler(shuicastGlobals *g, int val)
{
	g->gEnableEncoderScheduler = val;
}

#define DAY_SCHEDULE(dow) \
int get##dow##Enable(shuicastGlobals *g) \
{ \
	return g->g##dow##Enable; \
} \
int get##dow##OnTime(shuicastGlobals *g) \
{ \
	return g->g##dow##OnTime; \
} \
int get##dow##OffTime(shuicastGlobals *g) \
{ \
	return g->g##dow##OffTime; \
} \
void set##dow##Enable(shuicastGlobals *g, int val) \
{ \
	g->g##dow##Enable = val; \
} \
void set##dow##OnTime(shuicastGlobals *g, int val) \
{ \
	g->g##dow##OnTime = val; \
} \
void set##dow##OffTime(shuicastGlobals *g, int val) \
{ \
	g->g##dow##OffTime = val; \
}

DAY_SCHEDULE(Monday)
DAY_SCHEDULE(Tuesday)
DAY_SCHEDULE(Wednesday)
DAY_SCHEDULE(Thursday)
DAY_SCHEDULE(Friday)
DAY_SCHEDULE(Saturday)
DAY_SCHEDULE(Sunday)

char_t *getCurrentRecordingName(shuicastGlobals *g)
{
	return(g->gCurrentRecordingName);
}

void setCurrentRecordingName(shuicastGlobals *g, char_t *name) 
{
	strcpy(g->gCurrentRecordingName, name);
}

int getFrontEndType(shuicastGlobals *g) 
{
	return(g->frontEndType);
}

void setFrontEndType(shuicastGlobals *g, int x) 
{
	g->frontEndType = x;
}

int getReconnectTrigger(shuicastGlobals *g) 
{
	return(g->ReconnectTrigger);
}

void setReconnectTrigger(shuicastGlobals *g, int x) 
{
	g->ReconnectTrigger = x;
}

char_t *getLockedMetadata(shuicastGlobals *g) 
{
	return g->gManualSongTitle;
}

void setLockedMetadata(shuicastGlobals *g, char_t *buf) 
{
	memset(g->gManualSongTitle, '\000', sizeof(g->gManualSongTitle));
	strncpy(g->gManualSongTitle, buf, sizeof(g->gManualSongTitle) - 1);
}

int getLockedMetadataFlag(shuicastGlobals *g) 
{
	return g->gLockSongTitle;
}

void setLockedMetadataFlag(shuicastGlobals *g, int flag) 
{
	g->gLockSongTitle = flag;
}

void setSaveDirectory(shuicastGlobals *g, char_t *saveDir) 
{
	memset(g->gSaveDirectory, '\000', sizeof(g->gSaveDirectory));
	strncpy(g->gSaveDirectory, saveDir, sizeof(g->gSaveDirectory) - 1);
}

char_t *getSaveDirectory(shuicastGlobals *g) 
{
	return(g->gSaveDirectory);
}

#if 0
void setSaveDirectoryFlag(shuicastGlobals *g, int flag) 
{
	g->gSaveDirectoryFlag = flag;
}

int getSaveDirectoryFlag(shuicastGlobals *g) 
{
	return(g->gSaveDirectoryFlag);
}
#endif

void setgLogFile(shuicastGlobals *g, char_t *logFile) 
{
	strcpy(g->gLogFile, logFile);
}

char_t *getgLogFile(shuicastGlobals *g) 
{
	return(g->gLogFile);
}

int resetResampler(shuicastGlobals *g) 
{
	if(g->initializedResampler) 
	{
		res_clear(&(g->resampler));
	}

	g->initializedResampler = 0;
	return 1;
}

/* Gratuitously ripped from util.c */
static char_t			base64table[64] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' };
static signed char_t	base64decode[256] = { -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, 62, -2, -2, -2, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -2, -2, -2, -1, -2, -2, -2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -2, -2, -2, -2, -2, -2, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2 };

/*
 =======================================================================================================================
    This isn't efficient, but it doesn't need to be
 =======================================================================================================================
 */
char_t *util_base64_encode(char_t *data) 
{
	int		len = strlen(data);
	char_t	*out = (char_t *) malloc(len * 4 / 3 + 4);
	char_t	*result = out;
	int		chunk;

	while(len > 0) 
	{
		chunk = (len > 3) ? 3 : len;
		*out++ = base64table[(*data & 0xFC) >> 2];
		*out++ = base64table[((*data & 0x03) << 4) | ((*(data + 1) & 0xF0) >> 4)];
		switch(chunk) 
		{
		case 3:
			*out++ = base64table[((*(data + 1) & 0x0F) << 2) | ((*(data + 2) & 0xC0) >> 6)];
			*out++ = base64table[(*(data + 2)) & 0x3F];
			break;

		case 2:
			*out++ = base64table[((*(data + 1) & 0x0F) << 2)];
			*out++ = '=';
			break;

		case 1:
			*out++ = '=';
			*out++ = '=';
			break;
		}

		data += chunk;
		len -= chunk;
	}

	*out = 0;

	return result;
}

char_t *util_base64_decode(unsigned char_t *input) 
{
	int			len = strlen((char_t *) input);
	char_t		*out = (char_t *) malloc(len * 3 / 4 + 5);
	char_t		*result = out;
	signed char_t vals[4];

	while(len > 0) 
	{
		if(len < 4) 
		{
			free(result);
			return NULL;	/* Invalid Base64 data */
		}

		vals[0] = base64decode[*input++];
		vals[1] = base64decode[*input++];
		vals[2] = base64decode[*input++];
		vals[3] = base64decode[*input++];

		if(vals[0] < 0 || vals[1] < 0 || vals[2] < -1 || vals[3] < -1) 
		{
			continue;
		}

		*out++ = vals[0] << 2 | vals[1] >> 4;
		if(vals[2] >= 0)
			*out++ = ((vals[1] & 0x0F) << 4) | (vals[2] >> 2);
		else
			*out++ = 0;

		if(vals[3] >= 0)
			*out++ = ((vals[2] & 0x03) << 6) | (vals[3]);
		else
			*out++ = 0;

		len -= 4;
	}

	*out = 0;

	return result;
}

#define HEADER_TYPE 1
#define CODEC_TYPE	2

void closeArchiveFile(shuicastGlobals *g) 
{
	if(g->gSaveFile) 
	{
		if(g->gSaveAsWAV) 
		{
			wav_header.length = GUINT32_TO_LE(g->written + sizeof(struct wavhead) - 8);
			wav_header.data_length = GUINT32_TO_LE(g->written);
			fseek(g->gSaveFile, 0, SEEK_SET);
			fwrite(&wav_header, sizeof(struct wavhead), 1, g->gSaveFile);
			g->written = 0;
		}

		fclose(g->gSaveFile);
		g->gSaveFile = 0;
	}
}

int openArchiveFile(shuicastGlobals *g) 
{
	char_t		outFilename[1024] = "";
	char_t		outputFile[1024] = "";
	struct tm	*newtime;
	time_t		aclock;

	time(&aclock);
	newtime = localtime(&aclock);

	wsprintf(outFilename, "%s_%s", g->gServDesc, asctime(newtime));

	memset(outputFile, '\000', sizeof(outputFile));

	memset(outputFile, '\000', sizeof(outputFile));
	ReplaceString(outFilename, outputFile, "\"", "'");
	memset(outFilename, '\000', sizeof(outFilename));
	ReplaceString(outputFile, outFilename, FILE_SEPARATOR, "");
	memset(outputFile, '\000', sizeof(outputFile));
	ReplaceString(outFilename, outputFile, "/", "");
	memset(outFilename, '\000', sizeof(outFilename));
	ReplaceString(outputFile, outFilename, ":", "");
	memset(outputFile, '\000', sizeof(outputFile));
	ReplaceString(outFilename, outputFile, "*", "");
	memset(outFilename, '\000', sizeof(outFilename));
	ReplaceString(outputFile, outFilename, "?", "");
	memset(outputFile, '\000', sizeof(outputFile));
	ReplaceString(outFilename, outputFile, "<", "");
	memset(outFilename, '\000', sizeof(outFilename));
	ReplaceString(outputFile, outFilename, ">", "");
	memset(outputFile, '\000', sizeof(outputFile));
	ReplaceString(outFilename, outputFile, "|", "");
	memset(outFilename, '\000', sizeof(outFilename));
	ReplaceString(outputFile, outFilename, "\n", "");

	memset(outputFile, '\000', sizeof(outputFile));
	strcpy(outputFile, outFilename);

    if ( g->gSaveAsWAV ) strcat( outputFile, ".wav" );
    else if ( g->gOggFlag ) strcat( outputFile, ".ogg" );
    else if ( g->gLAMEFlag ) strcat( outputFile, ".mp3" );
    else if ( g->gAACFlag || g->gAACPFlag || g->gFHAACPFlag ) strcat( outputFile, ".aac" );
	wsprintf(outFilename, "%s%s%s", g->gSaveDirectory, FILE_SEPARATOR, outputFile);

	g->gSaveFile = fopen(outFilename, "wb");
	if(!g->gSaveFile) 
	{
		char_t	buff[1024] = "";
		wsprintf(buff, "Cannot open %s", outputFile);
		LogMessage(g,LOG_ERROR, buff);
		return 0;
	}

	if(g->gSaveAsWAV) 
	{
		int nch = 2;
		int rate = 44100;

		memcpy(&wav_header.main_chunk, "RIFF", 4);
		wav_header.length = GUINT32_TO_LE(0);
		memcpy(&wav_header.chunk_type, "WAVE", 4);
		memcpy(&wav_header.sub_chunk, "fmt ", 4);
		wav_header.sc_len = GUINT32_TO_LE(16);
		wav_header.format = GUINT16_TO_LE(1);
		wav_header.modus = GUINT16_TO_LE(nch);
		wav_header.sample_fq = GUINT32_TO_LE(rate);
		wav_header.bit_p_spl = GUINT16_TO_LE(16);
		wav_header.byte_p_sec = GUINT32_TO_LE(rate * wav_header.modus * (GUINT16_FROM_LE(wav_header.bit_p_spl) / 8));
		wav_header.byte_p_spl = GUINT16_TO_LE((GUINT16_FROM_LE(wav_header.bit_p_spl) / (8 / nch)));
		memcpy(&wav_header.data_chunk, "data", 4);
		wav_header.data_length = GUINT32_TO_LE(0);
		fwrite(&wav_header, sizeof(struct wavhead), 1, g->gSaveFile);
	}

	return 1;
}

// this needs to be virtualized to support NSV encapsualtion if I ever get around to supporting NSV
int sendToServer(shuicastGlobals *g, int sd, char_t *data, int length, int type) 
{
	int ret = 0;
	int sendflags = 0;

	if(g->gSaveDirectoryFlag) 
	{
		if(!g->gSaveFile) 
		{
			openArchiveFile(g);
		}
	}

#if !defined(WIN32) && !defined(__FreeBSD__)
	sendflags = MSG_NOSIGNAL;
#endif
	switch(type) 
	{
	case HEADER_TYPE:
		ret = send(sd, data, length, sendflags);
		break;

	case CODEC_TYPE:
		ret = send(sd, data, length, sendflags);
		if(g->gSaveDirectoryFlag) 
		{
			if(g->gSaveFile) 
			{
				if(!g->gSaveAsWAV) 
				{
					fwrite(data, length, 1, g->gSaveFile);
				}
			}
		}
		break;
	}

	if(ret > 0) 
	{
		if(g->writeBytesCallback) 
		{
			if(g->weareconnected)
			{
				g->writeBytesCallback((void *) g, (void *) ret);
			}
		}
	}

	return ret;
}

bool firstRead = true;

int readConfigFile(shuicastGlobals *g, int readOnly) 
{
	FILE	*filep;
	char_t	buffer[1024];
	char_t	configFile[1024] = "";
	char_t	defaultConfigName[] = "shuicast";

	if(firstRead)
	{
		ZeroMemory(&configFileDescs, sizeof(configFileDescs));
		firstRead = false;
	}

	numConfigValues = 0;
	memset(&configFileValues, '\000', sizeof(configFileValues));

	if(readOnly) 
	{
		wsprintf(configFile, "%s", g->gConfigFileName);
	}
	else
	{
		if(strlen(g->gConfigFileName) == 0) 
		{
			wsprintf(configFile, "%s_%d.cfg", defaultConfigName, g->encoderNumber);
		}
		else 
		{
			wsprintf(configFile, "%s_%d.cfg", g->gConfigFileName, g->encoderNumber);
		}
	}

	filep = fopen(configFile, "r");
	if(filep == 0) 
	{
		/*
		 * LogMessage(g,g,LOG_ERROR, "Cannot open config file %s\n", configFile);
		 * strcpy(g->gConfigFileName, defaultConfigName);
		 */
	}
	else
	{
		while(!feof(filep)) 
		{
			char_t	*p2;

			memset(buffer, '\000', sizeof(buffer));
			fgets(buffer, sizeof(buffer) - 1, filep);
			p2 = strchr(buffer, '\r');
			if(p2) 
			{
				*p2 = '\000';
			}

			p2 = strchr(buffer, '\n');
			if(p2) 
			{
				*p2 = '\000';
			}

			if(buffer[0] != '#') 
			{
				char_t	*p1 = strchr(buffer, '=');
				if(p1) 
				{
					strncpy(configFileValues[numConfigValues].Variable, buffer, p1 - buffer);
					p1++;	/* Get past the = */
					strcpy(configFileValues[numConfigValues].Value, p1);
					numConfigValues++;
				}
			}
		}

		if(filep) 
		{
			fclose(filep);
		}
	}

	config_read(g);

	if(!readOnly) 
	{
		writeConfigFile(g);
	}

	return 1;
}

int deleteConfigFile(shuicastGlobals *g) 
{
	char_t	configFile[1024] = "";
	char_t	defaultConfigName[] = "shuicast";

	if(strlen(g->gConfigFileName) == 0) 
	{
		wsprintf(configFile, "%s_%d.cfg", defaultConfigName, g->encoderNumber);
	}
	else
	{
		wsprintf(configFile, "%s_%d.cfg", g->gConfigFileName, g->encoderNumber);
	}

	_unlink(configFile);

	return 1;
}

void setConfigFileName(shuicastGlobals *g, char_t *configFile) 
{
	strcpy(g->gConfigFileName, configFile);
}

char_t *getConfigFileName(shuicastGlobals *g) 
{
	return g->gConfigFileName;
}

char * getDescription(char * paramName)
{
	for(int i=0; i < 200; i++)
	{
		if(!strcmp(configFileDescs[i].Variable, paramName))
		{
			return configFileDescs[i].Description;
		}
	}
	return NULL;
}

int writeConfigFile(shuicastGlobals *g) 
{
	char_t	configFile[1024] = "";
	char_t	defaultConfigName[] = "shuicast";
	config_write(g);

	if(strlen(g->gConfigFileName) == 0) 
	{
		wsprintf(configFile, "%s_%d.cfg", defaultConfigName, g->encoderNumber);
	}
	else 
	{
		wsprintf(configFile, "%s_%d.cfg", g->gConfigFileName, g->encoderNumber);
	}

	FILE	*filep = fopen(configFile, "w");

	if(filep == 0) 
	{
		LogMessage(g,LOG_ERROR, "Cannot open config file %s\n", configFile);
		return 0;
	}

	for(int i = 0; i < numConfigValues; i++) 
	{
		int ok = 1;
		if (g->configVariables) 
		{
			ok = 0;
			for (int j=0;j<g->numConfigVariables;j++) 
			{
				if (!strcmp(g->configVariables[j], configFileValues[i].Variable)) 
				{
					ok = 1;
					break;
				}
			}
		}

		if (ok) 
		{
			char * desc = getDescription(configFileValues[i].Variable);
			if (desc) 
			{
				fprintf(filep, "# %s\n", desc);
			}
			fprintf(filep, "%s=%s\n", configFileValues[i].Variable, configFileValues[i].Value);
		}
	}

	fclose(filep);

	return 1;
}

#if 0
void printConfigFileValues() 
{
	for(int i = 0; i < numConfigValues; i++) 
	{
		LogMessage(g,LOG_DEBUG, "(%s) = (%s)\n", configFileValues[i].Variable, configFileValues[i].Value);
	}
}
#endif

void putDescription(char * paramName, char * desc)
{
	int f = -1;
	for(int i = 0; i < 200; i++)
	{
		if(f < 0 && !configFileDescs[i].Description[0])
			f = i;
		else if(!strcmp(configFileDescs[i].Variable, paramName))
		{
			strcpy(configFileDescs[i].Description, desc);
			return;
		}
	}
	if(f >= 0)
	{
		strcpy(configFileDescs[f].Variable, paramName);
		strcpy(configFileDescs[f].Description, desc);
	}
}

void GetConfigVariable(shuicastGlobals *g, char_t *appName, char_t *paramName, char_t *defaultvalue, char_t *destValue, int destSize, char_t *desc)
{
	if (g->configVariables) 
	{
		int ok = 0;
		for (int j=0;j<g->numConfigVariables;j++) 
		{
			if (!strcmp(g->configVariables[j], paramName)) 
			{
				ok = 1;
				break;
			}
		}
		if (!ok) 
		{
			strcpy(destValue, defaultvalue);
			return;
		}
	}
	for(int i = 0; i < numConfigValues; i++) 
	{
		if(!strcmp(paramName, configFileValues[i].Variable)) 
		{
			strcpy(destValue, configFileValues[i].Value);
			if (desc) 
			{
				putDescription(paramName, desc);
			}
			return;
		}
	}

	strcpy(configFileValues[numConfigValues].Variable, paramName);
	strcpy(configFileValues[numConfigValues].Value, defaultvalue);
	if (desc) 
	{
		putDescription(paramName, desc);
	}
	strcpy(destValue, configFileValues[numConfigValues].Value);
	numConfigValues++;
	return;
}

long GetConfigVariableLong(shuicastGlobals *g, char_t *appName, char_t *paramName, long defaultvalue, char_t *desc) 
{
	char_t	buf[1024] = "";
	char_t	defaultbuf[1024] = "";

	wsprintf(defaultbuf, "%d", defaultvalue);

	GetConfigVariable(g, appName, paramName, defaultbuf, buf, sizeof(buf), desc);

	return atol(buf);
}

void PutConfigVariable(shuicastGlobals *g, char_t *appName, char_t *paramName, char_t *destValue) 
{

	if (g->configVariables) 
	{
		int ok = 0;
		for (int j=0;j<g->numConfigVariables;j++) 
		{
			if (!strcmp(g->configVariables[j], paramName)) 
			{
				ok = 1;
				break;
			}
		}
		if (!ok) 
		{
			return;
		}
	}

	for(int i = 0; i < numConfigValues; i++) 
	{
		if(!strcmp(paramName, configFileValues[i].Variable)) 
		{
			strcpy(configFileValues[i].Value, destValue);
			return;
		}
	}

	strcpy(configFileValues[numConfigValues].Variable, paramName);
	strcpy(configFileValues[numConfigValues].Value, destValue);
	//strcpy(configFileValues[numConfigValues].Description, "");
	numConfigValues++;
	return;
}

void PutConfigVariableLong(shuicastGlobals *g, char_t *appName, char_t *paramName, long value) 
{
	char_t	buf[1024] = "";
	wsprintf(buf, "%d", value);
	PutConfigVariable(g, appName, paramName, buf);
	return;
}

int trimVariable(char_t *variable) 
{
	char_t	*p1;

	/* Trim off the back */
	for(p1 = variable + strlen(variable) - 1; p1 > variable; p1--) 
	{
		if((*p1 == ' ') || (*p1 == '\t')) 
		{
			*p1 = '\000';
		}
		else 
		{
			break;
		}
	}

	/* Trim off the front */
	char_t	tempVariable[1024] = "";

	memset(tempVariable, '\000', sizeof(tempVariable));
	for(p1 = variable; p1 < variable + strlen(variable) - 1; p1++) 
	{
		if((*p1 == ' ') || (*p1 == '\t')) 
		{
			;
		}
		else 
		{
			break;
		}
	}

	strcpy(tempVariable, p1);
	strcpy(variable, tempVariable);
	return 1;
}

void setDestURLCallback(shuicastGlobals *g, void (*pCallback) (void *, void *)) 
{
	g->destURLCallback = pCallback;
}

void setSourceURLCallback(shuicastGlobals *g, void (*pCallback) (void *, void *)) 
{
	g->sourceURLCallback = pCallback;
}

void setServerStatusCallback(shuicastGlobals *g, void (*pCallback) (void *, void *)) 
{
	g->serverStatusCallback = pCallback;
}

void setGeneralStatusCallback(shuicastGlobals *g, void (*pCallback) (void *, void *)) 
{
	g->generalStatusCallback = pCallback;
}

void setWriteBytesCallback(shuicastGlobals *g, void (*pCallback) (void *, void *)) 
{
	g->writeBytesCallback = pCallback;
}

void setServerTypeCallback(shuicastGlobals *g, void (*pCallback) (void *, void *)) 
{
	g->serverTypeCallback = pCallback;
}

void setServerNameCallback(shuicastGlobals *g, void (*pCallback) (void *, void *)) 
{
	g->serverNameCallback = pCallback;
}

void setStreamTypeCallback(shuicastGlobals *g, void (*pCallback) (void *, void *)) 
{
	g->streamTypeCallback = pCallback;
}

void setBitrateCallback(shuicastGlobals *g, void (*pCallback) (void *, void *)) 
{
	g->bitrateCallback = pCallback;
}

void setOggEncoderText(shuicastGlobals *g, char_t *text) 
{
	strcpy(g->gOggEncoderText, text);
}

void setVUCallback(shuicastGlobals *g, void (*pCallback) (double, double, double, double)) 
{
	g->VUCallback = pCallback;
}

void setForceStop(shuicastGlobals *g, int forceStop) 
{
	g->gForceStop = forceStop;
}

void initializeGlobals(shuicastGlobals *g) 
{
	g->gReconnectSec = 10;
	g->gAutoCountdown = 10;
	g->automaticconnect = 1;
	//_attenTable[0] = 1.0;
	//for(int i = 1; i < 11; i++)
	//{
	//	_attenTable[i] = pow((double)10.0,(double) (-i) / (double) 20.0);
	//}
	g->gLogLevel = LM_ERROR;
	pthread_mutex_init(&(g->mutex), NULL);
	g->LAMEJointStereoFlag = 1;
	g->oggflag = 1;
}

char_t *getCurrentlyPlaying(shuicastGlobals *g)
{
	return(g->gSongTitle);
}

int setCurrentSongTitle(shuicastGlobals *g, char_t *song)
{
	char_t	*pCurrent;

	if(g->gLockSongTitle)
	{
		pCurrent = g->gManualSongTitle;
	}
	else
	{
		pCurrent = song;
	}

	if(strcmp(g->gSongTitle, pCurrent))
	{
		strcpy(g->gSongTitle, pCurrent);
		updateSongTitle(g, 0);
		return 1;
	}

	return 0;
}

int setCurrentSongTitleURL(shuicastGlobals *g, char_t *song) 
{
	char_t	*pCurrent;

	if(g->gLockSongTitle) 
	{
		pCurrent = g->gManualSongTitle;
	}
	else 
	{
		pCurrent = song;
	}

	if(strcmp(g->gSongTitle, pCurrent)) 
	{
		strcpy(g->gSongTitle, pCurrent);
		updateSongTitle(g, 0);
		return 1;
	}

	return 0;
}

void getCurrentSongTitle(shuicastGlobals *g, char_t *song, char_t *artist, char_t *full) 
{
	char_t	songTitle[1024] = "";
	char_t	songTitle2[1024] = "";

	memset(songTitle2, '\000', sizeof(songTitle2));

	char_t	*pCurrent;

	if(g->gLockSongTitle) 
	{
		pCurrent = g->gManualSongTitle;
	}
	else 
	{
		pCurrent = g->gSongTitle;
	}

	strcpy(songTitle, pCurrent);
	strcpy(full, songTitle);

	char_t	*p1 = strchr(songTitle, '-');
	if(p1) 
	{
		if(*(p1 - 1) == ' ') 
		{
			p1--;
		}

		strncpy(artist, songTitle, p1 - songTitle);
		p1 = strchr(songTitle, '-');
		p1++;
		if(*p1 == ' ') 
		{
			p1++;
		}

		strcpy(song, p1);
	}
	else
	{
		strcpy(artist, "");
		strcpy(song, songTitle);
	}
}

void ReplaceString(char_t *source, char_t *dest, char_t *from, char_t *to) 
{
	int		loop = 1;
	char_t	*p2 = (char_t *) 1;
	char_t	*p1 = source;
	*dest = '\0';

	while(p2) 
	{
		p2 = strstr(p1, from);
		if(p2) 
		{
			strncat(dest, p1, p2 - p1);
			strcat(dest, to);
			p1 = p2 + strlen(from);
		}
		else
		{
			strcat(dest, p1);
		}
	}
}

/*
 =======================================================================================================================
    This function URLencodes strings for use in sending them thru ;
    the Shoutcast admin.cgi interface to update song titles..
 =======================================================================================================================
 */
char_t * URLize(char_t *input) 
{
	int maxlen =  strlen(input) * 3 + 1; // worst case, the string will be 3 times as long
	char_t *buff2 = (char_t *) malloc(maxlen);
	char_t *buff1 = (char_t *) malloc(maxlen);

	ReplaceString(input, buff1, "%", "%25");
	ReplaceString(buff1, buff2, ";", "%3B");
	ReplaceString(buff2, buff1, "/", "%2F");
	ReplaceString(buff1, buff2, "?", "%3F");
	ReplaceString(buff2, buff1, ":", "%3A");
	ReplaceString(buff1, buff2, "@", "%40");
	ReplaceString(buff2, buff1, "&", "%26");
	ReplaceString(buff1, buff2, "=", "%3D");
	ReplaceString(buff2, buff1, "+", "%2B");
	ReplaceString(buff1, buff2, " ", "%20");
	ReplaceString(buff2, buff1, "\"", "%22");
	ReplaceString(buff1, buff2, "#", "%23");
	ReplaceString(buff2, buff1, "<", "%3C");
	ReplaceString(buff1, buff2, ">", "%3E");
	ReplaceString(buff2, buff1, "!", "%21");
	ReplaceString(buff1, buff2, "*", "%2A");
	ReplaceString(buff2, buff1, "'", "%27");
	ReplaceString(buff1, buff2, "(", "%28");
	ReplaceString(buff2, buff1, ")", "%29");
	ReplaceString(buff1, buff2, ",", "%2C");
	free(buff1);
	return buff2; // freed by caller
}

static char_t reqHeaders[] = "User-Agent: (Mozilla Compatible)\r\n\r\n";

int updateSongTitle(shuicastGlobals *g, int forceURL) 
{

	char_t	contentString[16384] = "";

	if(getIsConnected(g)) 
	{
		if((!g->gOggFlag) || (forceURL)) 
		{
			if((g->gSCFlag) || (g->gIcecastFlag) || (g->gIcecast2Flag) || forceURL) 
			{
				char_t	* URLSong = URLize(g->gSongTitle);
				char_t	* URLPassword = URLize(g->gPassword);
				strcpy(g->gCurrentSong, g->gSongTitle);

				if(g->gIcecast2Flag) 
				{
					char_t	userAuth[4096] = "";

					sprintf(userAuth, "source:%s", g->gPassword);

					char_t	*puserAuthbase64 = util_base64_encode(userAuth);

					if(puserAuthbase64) 
					{
						sprintf( contentString, "GET /admin/metadata?pass=%s&mode=updinfo&mount=%s&song=%s HTTP/1.0\r\nAuthorization: Basic %s\r\n%s",
							URLPassword, g->gMountpoint, URLSong, puserAuthbase64, reqHeaders );
						free(puserAuthbase64);
					}
				}

				if(g->gIcecastFlag) 
				{
					sprintf( contentString, "GET /admin.cgi?pass=%s&mode=updinfo&mount=%s&song=%s HTTP/1.0\r\n%s",
						URLPassword, g->gMountpoint, URLSong, reqHeaders );
				}

				if(g->gSCFlag) 
				{
					if(strchr(g->gPassword, ':') == NULL) // use Basic Auth for non sc_trans 2 connections
					{
						char_t	userAuth[1024] = "";
						sprintf(userAuth, "admin:%s", g->gPassword);
						char_t	*puserAuthbase64 = util_base64_encode(userAuth);
						sprintf( contentString, "GET /admin.cgi?mode=updinfo&song=%s HTTP/1.0\r\nAuthorization: Basic %s\r\n%s",
							URLSong, puserAuthbase64, reqHeaders );
						free(puserAuthbase64);
					}
					else
					{
						sprintf( contentString, "GET /admin.cgi?pass=%s&mode=updinfo&song=%s HTTP/1.0\r\n%s",
							URLPassword, URLSong, reqHeaders );
					}
				}

				free(URLSong);
				free(URLPassword);

				g->gSCSocketControl = g->controlChannel.DoSocketConnect(g->gServer, atoi(g->gPort));
				if(g->gSCSocketControl != -1) 
				{
					int sent = send(g->gSCSocketControl, contentString, strlen(contentString), 0);
					//int sent = sendToServer(g, g->gSCSocketControl, contentString, strlen(contentString), HEADER_TYPE);
					closesocket(g->gSCSocketControl);
				}
				else 
				{
					LogMessage(g,LOG_ERROR, "Cannot connect to server");
				}
			}
		}
		else 
		{
			g->ice2songChange = true;
		}
		return 1;
	}
	return 0;
}

/*
 =======================================================================================================================
    This function does some magic in order to change the metadata in a vorbis stream....Vakor helped me with this, and
    it's pretty much all his idea anyway...and probably the reason why it actually does work..:)
 =======================================================================================================================
 */
void icecast2SendMetadata(shuicastGlobals *g)
{
#ifdef HAVE_VORBIS
	pthread_mutex_lock(&(g->mutex));
	vorbis_analysis_wrote(&g->vd, 0);
	ogg_encode_dataout(g);
	initializeencoder(g);
	pthread_mutex_unlock(&(g->mutex));
#endif
}


#ifdef HAVE_FLAC
extern "C"
{
	FLAC__StreamEncoderWriteStatus FLACWriteCallback ( const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[],
			unsigned bytes, unsigned samples, unsigned current_frame, void *client_data ) 
	{
		shuicastGlobals	*g = (shuicastGlobals *) client_data;
		int sentbytes = sendToServer(g, g->gSCSocket, (char_t *) buffer, bytes, CODEC_TYPE);
        g->flacFailure = (sentbytes < 0) ? 1 : 0;
		return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
	}

    void FLACMetadataCallback ( const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data ) 
	{
        shuicastGlobals	*g = (shuicastGlobals *) client_data;
		return;
	}
}
#endif

/*
 =======================================================================================================================
    This function will disconnect the DSP from the server (duh)
 =======================================================================================================================
 */
int disconnectFromServer(shuicastGlobals *g) 
{
	g->weareconnected = 0;
	if(g->serverStatusCallback)
	{
		g->serverStatusCallback(g, (char_t *) "Disconnecting");
	}
	int retry = 10;
	while(g->gCurrentlyEncoding && retry--)
	{
#ifdef _WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	}
	if(retry == 0 && g->serverStatusCallback) 
	{
		g->serverStatusCallback(g, (char_t *) "Disconnecting - encoder did not stop");
	}
	/* Close all open sockets */
	closesocket(g->gSCSocket);
	closesocket(g->gSCSocketControl);

	/*
	 * Reset the Status to Disconnected, and reenable the config ;
	 * button
	 */
	g->gSCSocket = 0;
	g->gSCSocketControl = 0;

#ifdef HAVE_VORBIS
	if(g->gOggFlag)
	{
		ogg_stream_clear(&g->os);
		vorbis_block_clear(&g->vb);
		vorbis_dsp_clear(&g->vd);
		vorbis_info_clear(&g->vi);
		memset(&(g->vi), '\000', sizeof(g->vi));
	}
#endif
#ifdef HAVE_LAME
#ifndef _WIN32
	if(g->gf)
	{
		lame_close(g->gf);
		g->gf = NULL;
	}
#endif
#endif
	if(g->serverStatusCallback) 
	{
		g->serverStatusCallback(g, (void *) "Disconnected");
	}

	closeArchiveFile(g);

	return 1;
}

/*
 =======================================================================================================================
 This funciton will connect to a server (Shoutcast/Icecast/Icecast2) and send the appropriate password info and check
 to make sure things are connected....

 basecode - derived server types: icecast, icecast2, shoutcast, ultravox2.1=shoutcast2, ultravox2, ultravox3 and my own outcast
 ultravox(2,2.1 and 3) will be a later addition
 outcast is same as shoutcast in many ways, just a convenience for entering the following:
 application
 instance
 stream_id
 path

 outcast streaming protocol, as mentioned, similar to shoutcast.

 shoutcast:
	password\r\n
	Content-type: ...\r\n
	icy-*: ...\r\n
	\r\n

 etc, outcast:
	cast \t app \t instance \t stream_id \t password\r\n
	Content-type: ...\r\n
	icy-*: ...\r\n
	\r\n
 - or -
	application/instance:stream_id:password\r\n
	Content-type: ...\r\n
	icy-*: ...\r\n
	\r\n

 outcast will return which port accepts metadata, and possibly what the path is
 outcast metadata update
 
 admin.cgi?pass=...&app=...&inst=...&stream=...&mode=updinfo&....
 or
 admin.cgi?pass=application/instance:stream_id:password&mode=updinfo&....
 app, inst and id can NOT have / or : in them anyway, so there should be no issues with deciphering the second format
 =======================================================================================================================
 */
int connectToServer(shuicastGlobals *g)
{
	int		s_socket = 0;
	char_t	buffer[1024] = "";
	char_t	contentString[1024] = "";
	char_t	brate[25] = "";
	char_t	ypbrate[25] = "";

	LogMessage(g,LOG_DEBUG, "Connecting encoder %d", g->encoderNumber);

	sprintf(brate, "%d", g->currentBitrate);

	if(g->gOggFlag)
	{
		if(!g->gOggBitQualFlag)
		{
			sprintf(ypbrate, "Quality %s", g->gOggQuality);
		}
		else 
		{
			strcpy(ypbrate, brate);
		}
	}
	else
	{
		strcpy(ypbrate, brate);
	}

	g->gSCFlag = 0;
	greconnectFlag = 0;

	if(g->serverStatusCallback) 
	{
		g->serverStatusCallback(g, (void *) "Connecting");
	}

#ifdef WIN32
	g->dataChannel.initWinsockLib();
#endif

	/* If we are Icecast/Icecast2, then connect to specified port */
	if(g->gIcecastFlag || g->gIcecast2Flag)
	{
		g->gSCSocket = g->dataChannel.DoSocketConnect(g->gServer, atoi(g->gPort));
	}
	else
	{
		/* If we are Shoutcast, then the control socket (used for password) is port+1. */
		g->gSCSocket = g->dataChannel.DoSocketConnect(g->gServer, atoi(g->gPort) + 1);
	}

	/* Check to see if we connected okay */
	if(g->gSCSocket == -1)
	{
		if(g->serverStatusCallback)
		{
			g->serverStatusCallback(g, (void *) "Unable to connect to socket");
		}

		return 0;
	}

	int pswdok = 1;

	/* Yup, we did. */
	if(g->serverStatusCallback)
	{
		g->serverStatusCallback(g, (void *) "Socket connected");
	}

	char_t	contentType[255] = "";

	if(g->gOggFlag) 
	{
		strcpy(contentType, "application/ogg");
	} 
	else if(g->gAACFlag)
	{
		strcpy(contentType, "audio/aac");
	}
	else if(g->gAACPFlag)
	{
		switch(g->gAACPFlag) 
		{
		case 1: // HE-AAC, AAC Plus
			strcpy(contentType, "audio/aacp");
			break;
		case 2: // HE-AAC High Bitrate
			strcpy(contentType, "audio/aach");
			break;
		case 3: // LC-AAC
			// strcpy(contentType, "audio/aacr");
			strcpy(contentType, "audio/aac");
			break;
		}
	}
	else if(g->gFHAACPFlag)
	{
		strcpy(contentType, "audio/aacp");
	}
	else if(g->gFLACFlag)
	{
		strcpy(contentType, "application/ogg");
	}
	else 
	{
		strcpy(contentType, "audio/mpeg");
	}

	/*
	 * Here are all the variations of sending the password to ;
	 * a server..This if statement really is ugly...must fix.
	 */
	if(g->gIcecastFlag || g->gIcecast2Flag) 
	{

		/* The Icecast/Icecast2 Way */
		if(g->gIcecastFlag) 
		{
			sprintf(contentString,
					"SOURCE %s %s\r\ncontent-type: %s\r\nx-audiocast-name: %s\r\nx-audiocast-url: %s\r\nx-audiocast-genre: %s\r\nx-audiocast-bitrate: %s\r\nx-audiocast-public: %d\r\nx-audiocast-description: %s\r\n\r\n",
				g->gPassword, g->gMountpoint, contentType, g->gServDesc, g->gServURL, g->gServGenre, brate, g->gPubServ, g->gServDesc);
		}

		if(g->gIcecast2Flag)
		{
			char_t	audioInfo[1024] = "";
			sprintf(audioInfo, "ice-samplerate=%d;ice-bitrate=%s;ice-channels=%d", getCurrentSamplerate(g), ypbrate, getCurrentChannels(g));
			char_t	userAuth[1024] = "";
			sprintf(userAuth, "source:%s", g->gPassword);
			char_t	*puserAuthbase64 = util_base64_encode(userAuth);
			if(puserAuthbase64)
			{
				sprintf(contentString,
						"SOURCE %s ICE/1.0\ncontent-type: %s\nAuthorization: Basic %s\nice-name: %s\nice-url: %s\nice-genre: %s\nice-bitrate: %s\nice-private: %d\nice-public: %d\nice-description: %s\nice-audio-info: %s\n\n",
					g->gMountpoint, contentType, puserAuthbase64, g->gServName, g->gServURL, g->gServGenre, ypbrate, !g->gPubServ, g->gPubServ, g->gServDesc, audioInfo);
				free(puserAuthbase64);
			}
		}
	}
	else
	{

		/* The Shoutcast way */
		sendToServer(g, g->gSCSocket, g->gPassword, strlen(g->gPassword), HEADER_TYPE);
		sendToServer(g, g->gSCSocket, "\r\n", strlen("\r\n"), HEADER_TYPE);

		recv(g->gSCSocket, buffer, sizeof(buffer), (int) 0);

		// if we get an OK, then we are not a Shoutcast server (could be live365 or other variant)..And OK2 means it's
		// Shoutcast and we can safely send in metadata via the admin.cgi interface.
		if(!strncmp(buffer, "OK", strlen("OK"))) 
		{
			if(!strncmp(buffer, "OK2", strlen("OK2")))
			{
				g->gSCFlag = 1;
			}
			else
			{
				g->gSCFlag = 0;
			}

			if(g->serverStatusCallback)
			{
				g->serverStatusCallback(g, (void *) "Password OK");
			}
		}
		else
		{
			if(g->serverStatusCallback)
			{
				g->serverStatusCallback(g, (void *) "Password Failed");
			}

			closesocket(g->gSCSocket);
			return 0;
		}

		memset(contentString, '\000', sizeof(contentString));
		if(strlen(g->gServICQ) == 0) 
		{
			strcpy(g->gServICQ, "N/A");
		}

		if(strlen(g->gServAIM) == 0) 
		{
			strcpy(g->gServAIM, "N/A");
		}

		if(strlen(g->gServIRC) == 0) 
		{
			strcpy(g->gServIRC, "N/A");
		}

		sprintf(contentString,
				"content-type:%s\r\nicy-name:%s\r\nicy-genre:%s\r\nicy-url:%s\r\nicy-pub:%d\r\nicy-irc:%s\r\nicy-icq:%s\r\nicy-aim:%s\r\nicy-br:%s\r\n\r\n",
			contentType, g->gServName, g->gServGenre, g->gServURL, g->gPubServ, g->gServIRC, g->gServICQ, g->gServAIM, brate);
	}

	sendToServer(g, g->gSCSocket, contentString, strlen(contentString), HEADER_TYPE);

	if(g->gIcecastFlag)
	{
		/*
		 * Here we are checking the response from Icecast/Icecast2 ;
		 * from when we sent in the password...OK means we are good..if the ;
		 * password is bad, Icecast just disconnects the socket.
		 */
		if(g->gOggFlag)
		{
			recv(g->gSCSocket, buffer, sizeof(buffer), 0);
			if(!strncmp(buffer, "OK", strlen("OK")))
			{
				/* I don't think this check is needed.. */
				if(!strncmp(buffer, "OK2", strlen("OK2")))
				{
					g->gSCFlag = 1;
				}
				else 
				{
					g->gSCFlag = 0;
				}

				if(g->serverStatusCallback)
				{
					g->serverStatusCallback(g, (void *) "Password OK");
				}
			}
			else
			{
				if(g->serverStatusCallback) 
				{
					g->serverStatusCallback(g, (void *) "Password Failed");
				}

				closesocket(g->gSCSocket);
				return 0;
			}
		}
	}

	/* We are connected */
	char_t		outFilename[1024] = "";
	char_t		outputFile[1024] = "";
	struct tm	*newtime;
	time_t		aclock;

	time(&aclock);
	newtime = localtime(&aclock);

	int ret = 0;

	ret = initializeencoder(g);
	g->forcedDisconnect = false;
	if(ret)
	{
		g->weareconnected = 1;
		g->automaticconnect = 1;

		if(g->serverStatusCallback) 
		{
			g->serverStatusCallback(g, (void *) "Success");
		}

		/* Start up song title check */
	}
	else
	{
		disconnectFromServer(g);
		if(g->serverStatusCallback)
		{
#ifdef _WIN32
			if(g->gLAMEFlag) 
			{
				g->serverStatusCallback(g, (void *) "error with lame_enc.dll");
			}
			else 
			{
				if(g->gAACFlag) 
				{
					g->serverStatusCallback(g, (void *) "cannot find libfaac.dll");
				}
				else 
				{
					g->serverStatusCallback(g, (void *) "Encoder init failed");
				}
			}

#else
			g->serverStatusCallback(g, (void *) "Encoder init failed");
#endif
		}

		return 0;
	}

	if(g->serverStatusCallback)
	{
		g->serverStatusCallback(g, (void *) "Connected");
	}

	setCurrentSongTitle(g, g->gSongTitle);
	updateSongTitle(g, 0);
	return 1;
}

/*
 =======================================================================================================================
    These are some ogg routines that are used for Icecast2
 =======================================================================================================================
 */
int ogg_encode_dataout(shuicastGlobals *g)
{
#ifdef HAVE_VORBIS
	ogg_packet	op;
	ogg_page	og;
	int			result;
	int			sentbytes = 0;

	if(g->in_header) 
	{
		result = ogg_stream_flush(&g->os, &og);
		g->in_header = 0;
	}

	while(vorbis_analysis_blockout(&g->vd, &g->vb) == 1) 
	{
		vorbis_analysis(&g->vb, NULL);
		vorbis_bitrate_addblock(&g->vb);
		int packetsdone = 0;

		while(vorbis_bitrate_flushpacket(&g->vd, &op)) 
		{
			/* Add packet to bitstream */
			ogg_stream_packetin(&g->os, &op);
			packetsdone++;

			/* If we've gone over a page boundary, we can do actual output, so do so (for however many pages are available) */
			int eos = 0;
			while(!eos) 
			{
				int result = ogg_stream_pageout(&g->os, &og);
				if(!result) break;
				sentbytes = sendToServer(g, g->gSCSocket, (char_t *) og.header, og.header_len, CODEC_TYPE);
				if(sentbytes < 0) 
				{
					return sentbytes;
				}
				sentbytes += sendToServer(g, g->gSCSocket, (char_t *) og.body, og.body_len, CODEC_TYPE);
				if(sentbytes < 0) 
				{
					return sentbytes;
				}
				if(ogg_page_eos(&og)) 
				{
					eos = 1;
				}
			}
		}
	}

	return sentbytes;
#else
	return 0;
#endif
}

void oddsock_error_handler_function(const char_t *format, va_list ap) 
{
}

int initializeResampler(shuicastGlobals *g, long inSampleRate, long inNCH) 
{
	if(!g->initializedResampler) 
	{
		long	in_samplerate = inSampleRate;
		long	out_samplerate = getCurrentSamplerate(g);
		long	in_nch = inNCH;
		long	out_nch = 2;

		if(res_init(&(g->resampler), out_nch, out_samplerate, in_samplerate, RES_END)) 
		{
			LogMessage(g,LOG_ERROR, "Error initializing resampler");
			return 0;
		}

		g->initializedResampler = 1;
	}

	return 1;
}

int ocConvertAudio(shuicastGlobals *g, float *in_samples, float *out_samples, int num_in_samples, int num_out_samples) 
{
	int max_num_samples = res_push_max_input(&(g->resampler), num_out_samples);
	int ret_samples = res_push_interleaved(&(g->resampler), (SAMPLE *) out_samples, (const SAMPLE *) in_samples, max_num_samples);
	return ret_samples;
}

int initializeencoder(shuicastGlobals *g) 
{
	int		ret = 0;
	char_t	outFilename[1024] = "";
	char_t	message[1024] = "";

	resetResampler(g);

	if(g->gLAMEFlag)
	{
#ifdef HAVE_LAME
#ifdef _WIN32
		BE_ERR		err = 0;
		BE_VERSION	Version = { 0, };
		BE_CONFIG	beConfig = { 0, };

		if(g->hDLL) 
		{
			FreeLibrary(g->hDLL);
		}

		g->hDLL = LoadLibrary("lame_enc.dll");

		if(g->hDLL == NULL) 
		{
			wsprintf(message,
				"Unable to load DLL (lame_enc.dll)\n\
You have selected encoding with LAME, but apparently the plugin cannot find LAME installed. \
Due to legal issues, ShuiCast cannot distribute LAME directly, and so you'll have to download it separately. \
You will need to put the LAME DLL (lame_enc.dll) \
into the same directory as the application in order to get it working-> \
To download the LAME DLL, check out http://www.rarewares.org/mp3-lame-bundle.php");
			LogMessage(g,LOG_ERROR, message);
			if(g->serverStatusCallback)
			{
				g->serverStatusCallback(g, (void *) "can't find lame_enc.dll");
			}

			return 0;
		}

		/* Get Interface functions from the DLL */
		g->beInitStream = (BEINITSTREAM) GetProcAddress(g->hDLL, TEXT_BEINITSTREAM);
		g->beEncodeChunk = (BEENCODECHUNK) GetProcAddress(g->hDLL, TEXT_BEENCODECHUNK);
		g->beDeinitStream = (BEDEINITSTREAM) GetProcAddress(g->hDLL, TEXT_BEDEINITSTREAM);
		g->beCloseStream = (BECLOSESTREAM) GetProcAddress(g->hDLL, TEXT_BECLOSESTREAM);
		g->beVersion = (BEVERSION) GetProcAddress(g->hDLL, TEXT_BEVERSION);
		g->beWriteVBRHeader = (BEWRITEVBRHEADER) GetProcAddress(g->hDLL, TEXT_BEWRITEVBRHEADER);

		if ( !g->beInitStream || !g->beEncodeChunk || !g->beDeinitStream || !g->beCloseStream || !g->beVersion || !g->beWriteVBRHeader )
		{
			wsprintf(message, "Unable to get LAME interfaces - This DLL (lame_enc.dll) doesn't appear to be LAME?!?!?");
			LogMessage(g,LOG_ERROR, message);
			return 0;
		}

		/* Get the version number */
		g->beVersion(&Version);
		if(Version.byMajorVersion < 3)
		{
			wsprintf(message,
				"This version of ShuiCast expects at least version 3.91 of the LAME DLL, the DLL found is at %u.%02u, please consider upgrading",
				Version.byDLLMajorVersion, Version.byDLLMinorVersion);
			LogMessage(g,LOG_ERROR, message);
		}
		else
		{
			if(Version.byMinorVersion < 91)
			{
				wsprintf(message,
					"This version of ShuiCast expects at least version 3.91 of the LAME DLL, the DLL found is at %u.%02u, please consider upgrading",
					Version.byDLLMajorVersion, Version.byDLLMinorVersion);
				LogMessage(g,LOG_ERROR, message);
			}
		}

		/* Check if all interfaces are present */
		memset(&beConfig, 0, sizeof(beConfig)); /* clear all fields */

		/* use the LAME config structure */
		beConfig.dwConfig = BE_CONFIG_LAME;

		if(g->currentChannels == 1) 
		{
			beConfig.format.LHV1.nMode = BE_MP3_MODE_MONO;
		}
		else 
		{
			if (g->LAMEJointStereoFlag) 
			{
				beConfig.format.LHV1.nMode = BE_MP3_MODE_JSTEREO;
			}
			else 
			{
				beConfig.format.LHV1.nMode = BE_MP3_MODE_STEREO;
			}
		}

		// this are the default settings for testcase.wav 
		beConfig.format.LHV1.dwStructVersion = 1;
		beConfig.format.LHV1.dwStructSize = sizeof(beConfig);
		beConfig.format.LHV1.dwSampleRate = g->currentSamplerate;	// INPUT FREQUENCY 
		beConfig.format.LHV1.dwReSampleRate = g->currentSamplerate; // DON"T RESAMPLE 
		// beConfig.format.LHV1.dwReSampleRate = 0;
		beConfig.format.LHV1.dwMpegVersion = MPEG1;					// MPEG VERSION (I or II) 
		beConfig.format.LHV1.dwPsyModel = 0;						// USE DEFAULT PSYCHOACOUSTIC MODEL 
		beConfig.format.LHV1.dwEmphasis = 0;						// NO EMPHASIS TURNED ON 
		beConfig.format.LHV1.bWriteVBRHeader = TRUE;				// YES, WRITE THE XING VBR HEADER 
		//beConfig.format.LHV1.bNoRes = TRUE;						// No Bit resorvoir 
		beConfig.format.LHV1.bStrictIso = g->gLAMEOptions.strict_ISO;
		beConfig.format.LHV1.bCRC = FALSE;							//
		beConfig.format.LHV1.bCopyright = g->gLAMEOptions.copywrite;
		beConfig.format.LHV1.bOriginal = g->gLAMEOptions.original;
		beConfig.format.LHV1.bPrivate = FALSE;						//
		beConfig.format.LHV1.bNoRes = g->gLAMEOptions.disable_reservoir;
		beConfig.format.LHV1.nQuality = g->gLAMEOptions.quality | ((~g->gLAMEOptions.quality) << 8);
		beConfig.format.LHV1.dwBitrate = g->currentBitrate;		// BIT RATE

		if((g->gLAMEOptions.cbrflag) || !strcmp(g->gLAMEOptions.VBR_mode, "vbr_none") || g->gLAMEpreset == LQP_CBR)
		{
			beConfig.format.LHV1.nVbrMethod = VBR_METHOD_NONE;
			beConfig.format.LHV1.bEnableVBR = FALSE;
			beConfig.format.LHV1.nVBRQuality = 0;
		}
		else if(!strcmp(g->gLAMEOptions.VBR_mode, "vbr_abr") || g->gLAMEpreset == LQP_ABR)
		{
			beConfig.format.LHV1.nVbrMethod = VBR_METHOD_ABR;
			beConfig.format.LHV1.bEnableVBR = TRUE;
			beConfig.format.LHV1.dwVbrAbr_bps = g->currentBitrate * 1000;
			beConfig.format.LHV1.dwMaxBitrate = g->currentBitrateMax;
			beConfig.format.LHV1.nVBRQuality = g->gLAMEOptions.quality;
		}
		else
		{
			beConfig.format.LHV1.bEnableVBR = TRUE;
			beConfig.format.LHV1.dwMaxBitrate = g->currentBitrateMax;
			beConfig.format.LHV1.nVBRQuality = g->gLAMEOptions.quality;

			if(!strcmp(g->gLAMEOptions.VBR_mode, "vbr_rh")) 
			{
				beConfig.format.LHV1.nVbrMethod = VBR_METHOD_OLD;
			}
			else if(!strcmp(g->gLAMEOptions.VBR_mode, "vbr_new")) 
			{
				beConfig.format.LHV1.nVbrMethod = VBR_METHOD_NEW;
			}
			else if(!strcmp(g->gLAMEOptions.VBR_mode, "vbr_mtrh")) 
			{
				beConfig.format.LHV1.nVbrMethod = VBR_METHOD_MTRH;
			}
            else if(!strcmp(g->gLAMEOptions.VBR_mode, "vbr_abr"))
            {
                beConfig.format.LHV1.nVbrMethod = VBR_METHOD_ABR;
            }
            else
			{
				beConfig.format.LHV1.nVbrMethod = VBR_METHOD_DEFAULT;
			}
		}

		//if(g->gLAMEpreset != LQP_NOPRESET) 
		//{
			beConfig.format.LHV1.nPreset = g->gLAMEpreset;
		//}

		err = g->beInitStream(&beConfig, &(g->dwSamples), &(g->dwMP3Buffer), &(g->hbeStream));

		if(err != BE_ERR_SUCCESSFUL)
		{
			wsprintf(message, "Error opening encoding stream (%lu)", err);
			LogMessage(g,LOG_ERROR, message);
			return 0;
		}

#else
		g->gf = lame_init();
		lame_set_errorf(g->gf, oddsock_error_handler_function);
		lame_set_debugf(g->gf, oddsock_error_handler_function);
		lame_set_msgf(g->gf, oddsock_error_handler_function);
		lame_set_brate(g->gf, g->currentBitrate);
		lame_set_quality(g->gf, g->gLAMEOptions.quality);
		lame_set_num_channels(g->gf, 2);

		if(g->currentChannels == 1)
		{
			lame_set_mode(g->gf, MONO);

			/*
			 * lame_set_num_channels(g->gf, 1);
			 */
		}
		else
		{
			lame_set_mode(g->gf, STEREO);
		}

		/*
		 * Make the input sample rate the same as output..i.e. don't make lame do ;
		 * any resampling->..cause we are handling it ourselves...
		 */
		lame_set_in_samplerate(g->gf, g->currentSamplerate);
		lame_set_out_samplerate(g->gf, g->currentSamplerate);
		lame_set_copyright(g->gf, g->gLAMEOptions.copywrite);
		lame_set_strict_ISO(g->gf, g->gLAMEOptions.strict_ISO);
		lame_set_disable_reservoir(g->gf, g->gLAMEOptions.disable_reservoir);

		if(!g->gLAMEOptions.cbrflag)
		{
			if(!strcmp(g->gLAMEOptions.VBR_mode, "vbr_rh"))
			{
				lame_set_VBR(g->gf, vbr_rh);
			}
			else if(!strcmp(g->gLAMEOptions.VBR_mode, "vbr_mtrh"))
			{
				lame_set_VBR(g->gf, vbr_mtrh);
			}
			else if(!strcmp(g->gLAMEOptions.VBR_mode, "vbr_abr"))
			{
				lame_set_VBR(g->gf, vbr_abr);
			}

			lame_set_VBR_mean_bitrate_kbps(g->gf, g->currentBitrate);
			lame_set_VBR_min_bitrate_kbps(g->gf, g->currentBitrateMin);
			lame_set_VBR_max_bitrate_kbps(g->gf, g->currentBitrateMax);
		}

		if(strlen(g->gLAMEbasicpreset) > 0)
		{
			if(!strcmp(g->gLAMEbasicpreset, "r3mix"))
			{

				/*
				 * presets_set_r3mix(g->gf, g->gLAMEbasicpreset, stdout);
				 */
			}
			else
			{

				/*
				 * presets_set_basic(g->gf, g->gLAMEbasicpreset, stdout);
				 */
			}
		}

		if(strlen(g->gLAMEaltpreset) > 0)
		{
			int altbitrate = atoi(g->gLAMEaltpreset);

			/*
			 * dm_presets(g->gf, 0, altbitrate, g->gLAMEaltpreset, "shuicast");
			 */
		}

		/* do internal inits... */
		lame_set_lowpassfreq(g->gf, g->gLAMEOptions.lowpassfreq);
		lame_set_highpassfreq(g->gf, g->gLAMEOptions.highpassfreq);

		int lame_ret = lame_init_params(g->gf);

		if(lame_ret != 0)
		{
			printf("Error initializing LAME");
		}
#endif
#else
		if(g->serverStatusCallback)
		{
			g->serverStatusCallback(g, (void *) "Not compiled with LAME support");
		}

		LogMessage(g,LOG_ERROR, "Not compiled with LAME support");
		return 0;
#endif
	}

	if(g->gAACFlag)
	{
#ifdef HAVE_FAAC
		faacEncConfigurationPtr m_pConfig;

#ifdef WIN32
		g->hFAACDLL = LoadLibrary("libfaac.dll");
		if(g->hFAACDLL == NULL)
		{
			wsprintf(message, "Unable to load AAC DLL (libfaac.dll)");
			LogMessage(g,LOG_ERROR, message);
			if(g->serverStatusCallback)
			{
				g->serverStatusCallback(g, (void *) "can't find libfaac.dll");
			}

			return 0;
		}

		FreeLibrary(g->hFAACDLL);
#endif
		if(g->aacEncoder)
		{
			faacEncClose(g->aacEncoder);
			g->aacEncoder = NULL;
		}

		g->aacEncoder = faacEncOpen(g->currentSamplerate, g->currentChannels, &g->samplesInput, &g->maxBytesOutput);

		if(g->faacFIFO)
		{
			free(g->faacFIFO);
		}

		g->faacFIFO = (float *) malloc(g->samplesInput * sizeof(float) * 16);
		g->faacFIFOendpos = 0;

		m_pConfig = faacEncGetCurrentConfiguration(g->aacEncoder);

		m_pConfig->mpegVersion = MPEG2;

		m_pConfig->quantqual = atoi(g->gAACQuality);

		int cutoff = atoi(g->gAACCutoff);

		if(cutoff > 0)
		{
			m_pConfig->bandWidth = cutoff;
		}

		/*
		 * m_pConfig->bitRate = (g->currentBitrate * 1000) / g->currentChannels;
		 */
		m_pConfig->allowMidside = 1;
		m_pConfig->useLfe = 0;
		m_pConfig->useTns = 1;
		m_pConfig->aacObjectType = LOW;
		m_pConfig->outputFormat = 1;
		m_pConfig->inputFormat = FAAC_INPUT_FLOAT;

		/* set new config */
		faacEncSetConfiguration(g->aacEncoder, m_pConfig);
#else
		if(g->serverStatusCallback)
		{
			g->serverStatusCallback(g, (void *) "Not compiled with AAC support");
		}

		LogMessage(g,LOG_ERROR, "Not compiled with AAC support");
		return 0;
#endif
	}

	if(g->gFHAACPFlag)
	{
#ifdef HAVE_FHGAACP
		g->hFHGAACPDLL = LoadLibrary("enc_fhgaac.dll");
		if(g->hFHGAACPDLL == NULL)
		{
			LogMessage(g,LOG_ERROR, "Searching in plugins");
			g->hFHGAACPDLL = LoadLibrary("plugins\\enc_fhgaac.dll");
		}

		if(g->hFHGAACPDLL == NULL)
		{
			wsprintf(message, "Unable to load FHAAC Plus DLL (enc_fhgaac.dll)");
			LogMessage(g,LOG_ERROR, message);
			if(g->serverStatusCallback)
			{
				g->serverStatusCallback(g, (void *) "can't find enc_fhgaac.dll");
			}

			return 0;
		}
		g->fhCreateAudio3 = (CREATEAUDIO3TYPE) GetProcAddress(g->hFHGAACPDLL, "CreateAudio3");
		if(!g->fhCreateAudio3)
		{
			wsprintf(message, "Invalid DLL (enc_fhgaac.dll)");
			LogMessage(g,LOG_ERROR, message);
			if(g->serverStatusCallback)
			{
				g->serverStatusCallback(g, (void *) "invalid enc_fhgaac.dll");
			}

			return 0;
		}
		g->fhGetAudioTypes3 = (GETAUDIOTYPES3TYPE) GetProcAddress(g->hFHGAACPDLL, "GetAudioTypes3");
		*(void **) &(g->fhFinishAudio3) = (void *) GetProcAddress(g->hFHGAACPDLL, "FinishAudio3");
		*(void **) &(g->fhPrepareToFinish) = (void *) GetProcAddress(g->hFHGAACPDLL, "PrepareToFinish");
		if(g->fhaacpEncoder)
		{
			delete g->fhaacpEncoder;
			g->fhaacpEncoder = NULL;
		}
		unsigned int	outt = mmioFOURCC('A', 'D', 'T', 'S');//1346584897;
		char_t			conf_file[MAX_PATH] = "";	/* Default ini file */
		char_t			sectionName[255] = "audio_adtsaac";
		wsprintf(conf_file, "%s\\edcast_fhaacp_%d.ini", defaultConfigDir, g->encoderNumber);
		/*
		switch(g->gFHAACPFlag)
		{
			case 1:
				outt = mmioFOURCC('A', 'A', 'C', 'P');
				break;
			case 2:
				outt = mmioFOURCC('A', 'A', 'C', 'r');
				break;
			case 3:
				outt = mmioFOURCC('A', 'A', 'C', 'P');
				break;
			case 4:
				outt = mmioFOURCC('A', 'A', 'C', 'P');
				break;
		}
		*/
		/*
		xyzzy - this should be different if using enc_fhgaac.dll
		g->gAACPFlag will be 10(auto) 11(LC) 12(HE-AAC) 13(HE-AACv2) config file is far simpler, i.e.:
			[audio_adtsaac] | [audio_fhgaac]
			profile=g->gAACPFlag-10
			bitrate=sampleRate/1000!!
			surround=0
			shoutcast=1
			//preset=?
			//mode=?
		*/
		char_t tmp[2048];
		wsprintf(tmp, "%d", g->gFHAACPFlag - 1);
		WritePrivateProfileString(sectionName, "profile", tmp, conf_file);
		wsprintf(tmp, "%d", g->currentBitrate);
		WritePrivateProfileString(sectionName, "bitrate", tmp, conf_file);
		WritePrivateProfileString(sectionName, "surround", "0", conf_file);
		WritePrivateProfileString(sectionName, "shoutcast", "1", conf_file);
		//WritePrivateProfileString(sectionName, "preset", "0", conf_file);
		g->fhaacpEncoder = g->fhCreateAudio3((int) g->currentChannels,
										 (int) g->currentSamplerate,
										 16,
										 mmioFOURCC('P', 'C', 'M', ' '),
										 &outt,
										 conf_file);
		if(!g->fhaacpEncoder)
		{
			if(g->serverStatusCallback)
			{
				g->serverStatusCallback(g, (void *) "Invalid FHGAAC+ settings");
			}

			LogMessage(g,LOG_ERROR, "Invalid FHGAAC+ settings");
			return 0;
		}

#endif
	}
	if(g->gAACPFlag)
	{
#ifdef HAVE_AACP

#ifdef _WIN32
		g->hAACPDLL = LoadLibrary("enc_aacplus.dll");
		if(g->hAACPDLL == NULL)
		{
			g->hAACPDLL = LoadLibrary("plugins\\enc_aacplus.dll");
		}

		if(g->hAACPDLL == NULL)
		{
			wsprintf(message, "Unable to load AAC Plus DLL (enc_aacplus.dll)");
			LogMessage(g,LOG_ERROR, message);
			if(g->serverStatusCallback)
			{
				g->serverStatusCallback(g, (void *) "can't find enc_aacplus.dll");
			}

			return 0;
		}

		g->CreateAudio3 = (CREATEAUDIO3TYPE) GetProcAddress(g->hAACPDLL, "CreateAudio3");
		if(!g->CreateAudio3)
		{
			wsprintf(message, "Invalid DLL (enc_aacplus.dll)");
			LogMessage(g,LOG_ERROR, message);
			if(g->serverStatusCallback)
			{
				g->serverStatusCallback(g, (void *) "invalid enc_aacplus.dll");
			}

			return 0;
		}

		g->GetAudioTypes3 = (GETAUDIOTYPES3TYPE) GetProcAddress(g->hAACPDLL, "GetAudioTypes3");
		*(void **) &(g->finishAudio3) = (void *) GetProcAddress(g->hAACPDLL, "FinishAudio3");
		*(void **) &(g->PrepareToFinish) = (void *) GetProcAddress(g->hAACPDLL, "PrepareToFinish");

		/*
		 * FreeLibrary(g->hAACPDLL);
		 */
#endif
		if(g->aacpEncoder)
		{
			delete g->aacpEncoder;
			g->aacpEncoder = NULL;
		}

		unsigned int	outt = mmioFOURCC('A', 'A', 'C', 'P');//1346584897;
		char_t			conf_file[MAX_PATH] = "";	/* Default ini file */
		char_t			sectionName[255] = "audio_aacplus";

		/* 1 - Mono 2 - Stereo 3 - Stereo Independent 4 - Parametric 5 - Dual Channel */
		char_t			sampleRate[255] = "";
		char_t			channelMode[255] = "";
		char_t			bitrateValue[255] = "";
		char_t			aacpV2Enable[255] = "1";
		long			bitrateLong = g->currentBitrate * 1000;

		wsprintf(conf_file, "%s\\edcast_aacp_%d.ini", defaultConfigDir, g->encoderNumber);
		sprintf(bitrateValue, "%d", bitrateLong);
		switch(g->gAACPFlag)
		{
			case 1:
				strcpy(channelMode, "2");
				outt = mmioFOURCC('A', 'A', 'C', 'P');
				//strcpy(aacpV2Enable, "0");
				strcpy(sectionName, "audio_aacplus");
				if(bitrateLong > 64000) 
				{
					strcpy(channelMode, "2");
				}
				if(g->currentChannels == 2) 
				{
					if(g->LAMEJointStereoFlag && bitrateLong >=16000 && bitrateLong <= 56000) 
					{
						strcpy(channelMode, "4");
						//strcpy(aacpV2Enable, "1");
					}
					if(bitrateLong >=12000 && bitrateLong < 16000)
					{
						strcpy(channelMode, "4");
						//strcpy(aacpV2Enable, "1");
					}
				}
				if(g->currentChannels == 1 || bitrateLong < 12000)
				{
					strcpy(channelMode, "1");
				}
				break;
			case 2:
				outt = mmioFOURCC('A', 'A', 'C', 'H');
				//strcpy(aacpV2Enable, "0");
				strcpy(sectionName, "audio_aacplushigh");
				break;
			case 3:
				outt = mmioFOURCC('A', 'A', 'C', 'r');
				//strcpy(aacpV2Enable, "0");
				strcpy(sectionName, "audio_aac");
				break;
		}
/*
		if(bitrateLong > 128000)
		{
			outt = mmioFOURCC('A', 'A', 'C', 'H');
			strcpy(aacpV2Enable, "1");
			strcpy(sectionName, "audio_aacplushigh");
		}
		if(bitrateLong >= 56000 || g->gAACPFlag > 1)
		{
			if(g->currentChannels == 2)
			{
				strcpy(channelMode, "2");
			}
			else
			{
				strcpy(channelMode, "1");
			}
		}
		else if(bitrateLong >= 16000)
		{
			if(g->currentChannels == 2)
			{
				strcpy(channelMode, "4");
			}
			else
			{
				strcpy(channelMode, "1");
			}
		}
		else
		{
			strcpy(channelMode, "1");
		}
*/
		sprintf(sampleRate, "%d", g->currentSamplerate);

		WritePrivateProfileString(sectionName, "samplerate", sampleRate, conf_file);
		WritePrivateProfileString(sectionName, "channelmode", channelMode, conf_file);
		WritePrivateProfileString(sectionName, "bitrate", bitrateValue, conf_file);
		WritePrivateProfileString(sectionName, "v2enable", aacpV2Enable, conf_file);
		WritePrivateProfileString(sectionName, "bitstream", "0", conf_file);
		WritePrivateProfileString(sectionName, "signallingmode", "0", conf_file);
		WritePrivateProfileString(sectionName, "speech", "0", conf_file);
		WritePrivateProfileString(sectionName, "pns", "0", conf_file);

		g->aacpEncoder = g->CreateAudio3((int) g->currentChannels,
										 (int) g->currentSamplerate,
										 16,
										 mmioFOURCC('P', 'C', 'M', ' '),
										 &outt,
										 conf_file);
		if(!g->aacpEncoder)
		{
			if(g->serverStatusCallback)
			{
				g->serverStatusCallback(g, (void *) "Invalid AAC+ settings");
			}

			LogMessage(g,LOG_ERROR, "Invalid AAC+ settings");
			return 0;
		}

#else
		if(g->serverStatusCallback)
		{
			g->serverStatusCallback(g, (void *) "Not compiled with AAC Plus support");
		}

		LogMessage(g,LOG_ERROR, "Not compiled with AAC Plus support");
		return 0;
#endif
	}

	if(g->gOggFlag)
	{
#ifdef HAVE_VORBIS
		/* Ogg Vorbis Initialization */
		ogg_stream_clear(&g->os);
		vorbis_block_clear(&g->vb);
		vorbis_dsp_clear(&g->vd);
		vorbis_info_clear(&g->vi);

		int bitrate = 0;

		vorbis_info_init(&g->vi);

		int encode_ret = 0;

		if(!g->gOggBitQualFlag)
		{
			encode_ret = vorbis_encode_setup_vbr(&g->vi, g->currentChannels, g->currentSamplerate, ((float) atof(g->gOggQuality) * (float) .1));
			if(encode_ret)
			{
				vorbis_info_clear(&g->vi);
			}
		}
		else
		{
			int maxbit = -1;
			int minbit = -1;

			if(g->currentBitrateMax > 0)
			{
				maxbit = g->currentBitrateMax;
			}

			if(g->currentBitrateMin > 0)
			{
				minbit = g->currentBitrateMin;
			}

			encode_ret = vorbis_encode_setup_managed(&g->vi, g->currentChannels, g->currentSamplerate, g->currentBitrate * 1000, g->currentBitrate * 1000, g->currentBitrate * 1000);
			if(encode_ret)
			{
				vorbis_info_clear(&g->vi);
			}
		}

		if(encode_ret == OV_EIMPL)
		{
			LogMessage(g,LOG_ERROR, "Sorry, but this vorbis mode is not supported currently...");
			return 0;
		}

		if(encode_ret == OV_EINVAL)
		{
			LogMessage(g,LOG_ERROR, "Sorry, but this is an illegal vorbis mode...");
			return 0;
		}

		ret = vorbis_encode_setup_init(&g->vi);

		/*
		 * Now, set up the analysis engine, stream encoder, and other preparation before
		 * the encoding begins
		 */
		ret = vorbis_analysis_init(&g->vd, &g->vi);
		ret = vorbis_block_init(&g->vd, &g->vb);

		g->serialno = 0;
		srand((unsigned int)time(0));
		ret = ogg_stream_init(&g->os, rand());

		/*
		 * Now, build the three header packets and send through to the stream output stage
		 * (but defer actual file output until the main encode loop)
		 */
		ogg_packet		header_main;
		ogg_packet		header_comments;
		ogg_packet		header_codebooks;
		vorbis_comment	vc;
		char_t			title[1024] = "";
		char_t			artist[1024] = "";
		char_t			FullTitle[1024] = "";
		char_t			SongTitle[1024] = "";
		char_t			Artist[1024] = "";
		char_t			Streamed[1024] = "";
#if 0
		wchar_t			widestring[4096];
		char			tempstring[4096];
#endif
		memset(Artist, '\000', sizeof(Artist));
		memset(SongTitle, '\000', sizeof(SongTitle));
		memset(FullTitle, '\000', sizeof(FullTitle));
		memset(Streamed, '\000', sizeof(Streamed));

		vorbis_comment_init(&vc);

		bool	bypass = false;

		if(!getLockedMetadataFlag(g))
		{
			if(g->numVorbisComments) 
			{
				for(int i = 0; i < g->numVorbisComments; i++)
				{
#ifdef _WIN32
#if 1
					char * utf = AsciiToUtf8(g->vorbisComments[i]);
					vorbis_comment_add(&vc, utf);
					free(utf);
#else
					MultiByteToWideChar(CP_ACP, 0, g->vorbisComments[i], strlen(g->vorbisComments[i]) + 1, widestring, 4096);
					memset(tempstring, '\000', sizeof(tempstring));
					WideCharToMultiByte(CP_UTF8, 0, widestring, wcslen(widestring) + 1, tempstring, sizeof(tempstring), 0, NULL);
					vorbis_comment_add(&vc, tempstring);
#endif
#else
					vorbis_comment_add(&vc, g->vorbisComments[i]);
#endif
				}

				bypass = true;
			}
		}

		if(!bypass)
		{
			getCurrentSongTitle(g, SongTitle, Artist, FullTitle);
			if((strlen(SongTitle) == 0) && (strlen(Artist) == 0))
			{
				sprintf(title, "TITLE=%s", FullTitle);
			}
			else 
			{
				sprintf(title, "TITLE=%s", SongTitle);
			}

#ifdef _WIN32
#if 1
			{
				char * utf = AsciiToUtf8(title);
				vorbis_comment_add(&vc, utf);
				free(utf);
			}
#else
			MultiByteToWideChar(CP_ACP, 0, title, strlen(title) + 1, widestring, 4096);
			memset(tempstring, '\000', sizeof(tempstring));
			WideCharToMultiByte(CP_UTF8, 0, widestring, wcslen(widestring) + 1, tempstring, sizeof(tempstring), 0, NULL);
			vorbis_comment_add(&vc, tempstring);
#endif
#else
			vorbis_comment_add(&vc, title);
#endif
			sprintf(artist, "ARTIST=%s", Artist);
#ifdef WIN32
#if 1
			{
				char * utf = AsciiToUtf8(artist);
				vorbis_comment_add(&vc, utf);
				free(utf);
			}
#else
			MultiByteToWideChar(CP_ACP, 0, artist, strlen(artist) + 1, widestring, 4096);
			memset(tempstring, '\000', sizeof(tempstring));
			WideCharToMultiByte(CP_UTF8, 0, widestring, wcslen(widestring) + 1, tempstring, sizeof(tempstring), 0, NULL);
			vorbis_comment_add(&vc, tempstring);
#endif
#else
			vorbis_comment_add(&vc, artist);
#endif
		}

		sprintf(Streamed, "ENCODEDBY=shuicast");
		vorbis_comment_add(&vc, Streamed);
		if(strlen(g->sourceDescription) > 0)
		{
			sprintf(Streamed, "TRANSCODEDFROM=%s", g->sourceDescription);
			vorbis_comment_add(&vc, Streamed);
		}

		/* Build the packets */
		memset(&header_main, '\000', sizeof(header_main));
		memset(&header_comments, '\000', sizeof(header_comments));
		memset(&header_codebooks, '\000', sizeof(header_codebooks));

		vorbis_analysis_headerout(&g->vd, &vc, &header_main, &header_comments, &header_codebooks);

		ogg_stream_packetin(&g->os, &header_main);
		ogg_stream_packetin(&g->os, &header_comments);
		ogg_stream_packetin(&g->os, &header_codebooks);

		g->in_header = 1;

		ogg_page	og;
		int			eos = 0;
		int			sentbytes = 0;
		int			ret = 0;

		while(!eos) 
		{
			int result = ogg_stream_flush(&g->os, &og);
			if(result == 0) break;
			sentbytes += sendToServer(g, g->gSCSocket, (char *) og.header, og.header_len, CODEC_TYPE);
			sentbytes += sendToServer(g, g->gSCSocket, (char *) og.body, og.body_len, CODEC_TYPE);
		}

		vorbis_comment_clear(&vc);
		if(g->numVorbisComments) 
		{
			freeVorbisComments(g);
		}

#else
		if(g->serverStatusCallback)
		{
			g->serverStatusCallback(g, (void *) "Not compiled with Ogg Vorbis support");
		}

		LogMessage(g,LOG_ERROR, "Not compiled with Ogg Vorbis support");
		return 0;
#endif
	}

	if(g->gFLACFlag)
	{
#ifdef HAVE_FLAC
		char			FullTitle[1024] = "";
		char			SongTitle[1024] = "";
		char			Artist[1024] = "";
		char			Streamed[1024] = "";

		memset(Artist, '\000', sizeof(Artist));
		memset(SongTitle, '\000', sizeof(SongTitle));
		memset(FullTitle, '\000', sizeof(FullTitle));
		memset(Streamed, '\000', sizeof(Streamed));

		if(g->flacEncoder)
		{
			FLAC__stream_encoder_finish(g->flacEncoder);
			FLAC__stream_encoder_delete(g->flacEncoder);
			FLAC__metadata_object_delete(g->flacMetadata);
			g->flacEncoder = NULL;
			g->flacMetadata = NULL;
		}

		g->flacEncoder = FLAC__stream_encoder_new();
		g->flacMetadata = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);

		FLAC__stream_encoder_set_streamable_subset(g->flacEncoder, false);
//		FLAC__stream_encoder_set_client_data(g->flacEncoder, (void*)g);
		FLAC__stream_encoder_set_channels(g->flacEncoder, g->currentChannels);
/*
		FLAC__stream_encoder_set_write_callback(g->flacEncoder,(FLAC__StreamEncoderWriteCallback) FLACWriteCallback,
												   (FLAC__StreamEncoderWriteCallback) FLACWriteCallback);
		FLAC__stream_encoder_set_metadata_callback(g->flacEncoder,
												   (FLAC__StreamEncoderMetadataCallback) FLACMetadataCallback);
*/
		srand((unsigned int)time(0));

		if(!getLockedMetadataFlag(g))
		{
			FLAC__StreamMetadata_VorbisComment_Entry entry;
			FLAC__StreamMetadata_VorbisComment_Entry entry3;
			FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ENCODEDBY", "shuicast");
			FLAC__metadata_object_vorbiscomment_append_comment(g->flacMetadata, entry, true);
			if(strlen(g->sourceDescription) > 0)
			{
				FLAC__StreamMetadata_VorbisComment_Entry entry2;
				FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry2, "TRANSCODEDFROM", g->sourceDescription);
				FLAC__metadata_object_vorbiscomment_append_comment(g->flacMetadata, entry2, true);
			}
			getCurrentSongTitle(g, SongTitle, Artist, FullTitle);
			FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry3, "TITLE", FullTitle);
			FLAC__metadata_object_vorbiscomment_append_comment(g->flacMetadata, entry3, true);

		}
		FLAC__stream_encoder_set_ogg_serial_number(g->flacEncoder, rand());

		FLAC__StreamEncoderInitStatus ret = FLAC__stream_encoder_init_ogg_stream(g->flacEncoder, NULL, (FLAC__StreamEncoderWriteCallback) FLACWriteCallback, NULL, NULL, (FLAC__StreamEncoderMetadataCallback) FLACMetadataCallback, (void*)g);
		if(ret == FLAC__STREAM_ENCODER_INIT_STATUS_OK) 
		{
			if(g->serverStatusCallback)
			{
				g->serverStatusCallback(g, (void *) "FLAC initialized");
			}
		}
		else
		{
			if(g->serverStatusCallback)
			{
				g->serverStatusCallback(g, (void *) "Error Initializing FLAC");
			}

			LogMessage(g,LOG_ERROR, "Error Initializing FLAC");
			return 0;
		}
#endif
	}

	return 1;
}

void addToFIFO(shuicastGlobals *g, float *samples, int numsamples)
{
	int currentFIFO = g->faacFIFOendpos;

	for(int i = 0; i < numsamples; i++)
	{
		g->faacFIFO[currentFIFO] = samples[i];
		currentFIFO++;
	}

	g->faacFIFOendpos = currentFIFO;
}

void FloatScale(float *destination, float *source, int numsamples, int destchannels) 
{
	int i;

	if(destchannels == 2) 
	{
		for(i = 0; i < numsamples; i++)
		{
			*destination++ = *source++ *32767.f;
		}
	}
	else 
	{
		for(i = 0; i < numsamples >> 1; i++) 
		{
			*destination++ = (*source++ +*source++) * 16383.f;
		}
	}
}

void ExtractFromFIFO(float *destination, float *source, int numsamples)
{
	for(int i = 0; i < numsamples; i++) 
	{
		*destination++ = *source++;
	}
}

// This ALWAYS gets 2 channels!!
int do_encoding(shuicastGlobals *g, float *samples, int numsamples, Limiters * limiter = NULL) 
{
	int				count = 0;
	unsigned char	mp3buffer[LAME_MAXMP3BUFFER];
	int				imp3;
	short int		*int_samples;
	int				eos = 0;
	int				ret = 0;
	int				sentbytes = 0;
	char			buf[255] = "";

	if(g->weareconnected) 
	{
		g->gCurrentlyEncoding = 1;

		int		samplecounter = 0;
		if(g->VUCallback) 
		{
			if(limiter)
			{
				g->VUCallback(limiter->PeakL, limiter->PeakR, limiter->RmsL, limiter->RmsR);
			}
			else
			{
				long	leftMax = 0;
				long	rightMax = 0;
				LogMessage(g,LOG_DEBUG, "determining left/right max...");
				for(int i = 0; i < numsamples * 2; i = i + 2) 
				{
					leftMax += abs((int) ((float) samples[i] * 32767.f));
					rightMax += abs((int) ((float) samples[i + 1] * 32767.f));
				}

				if(numsamples > 0) 
				{
					leftMax = leftMax / (numsamples * 2);
					rightMax = rightMax / (numsamples * 2);
					g->VUCallback(leftMax, rightMax, leftMax, rightMax);
				}
			}
		}
		if(g->gOggFlag)
		{
#ifdef HAVE_VORBIS
			/*
			 * If a song change was detected, close the stream and resend new ;
			 * vorbis headers (with new comments) - all done by icecast2SendMetadata();
			 */
			if(g->ice2songChange) 
			{
				LogMessage(g,LOG_DEBUG, "Song change processing...");
				g->ice2songChange = false;
				icecast2SendMetadata(g);
			}

			LogMessage(g,LOG_DEBUG, "vorbis_analysis_buffer...");

			float	**buffer = vorbis_analysis_buffer(&g->vd, numsamples);
			int		samplecount = 0;
			int		i;

			samplecounter = 0;

			for(i = 0; i < numsamples * 2; i = i + 2) 
			{
				buffer[0][samplecounter] = samples[i];
				if(g->currentChannels == 2) 
				{
					buffer[1][samplecounter] = samples[i + 1];
				}

				samplecounter++;
			}
			LogMessage(g,LOG_DEBUG, "vorbis_analysis_wrote...");

			ret = vorbis_analysis_wrote(&g->vd, numsamples);

			pthread_mutex_lock(&(g->mutex));
			LogMessage(g,LOG_DEBUG, "ogg_encode_dataout...");
			/* Stream out what we just prepared for Vorbis... */
			sentbytes = ogg_encode_dataout(g);
			LogMessage(g,LOG_DEBUG, "done ogg_ecndoe_dataout...");
			pthread_mutex_unlock(&(g->mutex));
#endif
		}

		if(g->gAACFlag)
		{
#ifdef HAVE_FAAC
			float	*buffer = (float *) malloc(numsamples * 2 * sizeof(float));
			FloatScale(buffer, samples, numsamples * 2, g->currentChannels);

			addToFIFO(g, buffer, numsamples * g->currentChannels);

			while(g->faacFIFOendpos > (int)g->samplesInput) 
			{
				float	*buffer2 = (float *) malloc(g->samplesInput * 2 * sizeof(float));

				ExtractFromFIFO(buffer2, g->faacFIFO, g->samplesInput);

				int counter = 0;

				for(int i = g->samplesInput; i < g->faacFIFOendpos; i++) 
				{
					g->faacFIFO[counter] = g->faacFIFO[i];
					counter++;
				}

				g->faacFIFOendpos = counter;

				unsigned long	dwWrite = 0;
				unsigned char	*aacbuffer = (unsigned char *) malloc(g->maxBytesOutput);

				imp3 = faacEncEncode(g->aacEncoder, (int32_t *) buffer2, g->samplesInput, aacbuffer, g->maxBytesOutput);

				if(imp3) 
				{
					sentbytes = sendToServer(g, g->gSCSocket, (char *) aacbuffer, imp3, CODEC_TYPE);
				}

				if(buffer2) free(buffer2);
				if(aacbuffer) free(aacbuffer);
			}

			if(buffer) free(buffer);
#endif
		}

		if(g->gFHAACPFlag)
		{
#ifdef HAVE_FHGAACP
			static char outbuffer[32768];
			int			len = numsamples * g->currentChannels * sizeof(short);

			int_samples = (short *) malloc(len);

			int samplecount = 0;

			if(g->currentChannels == 1) 
			{
				for(int i = 0; i < numsamples * 2; i = i + 2) 
				{
					int_samples[samplecount] = (short int) (samples[i] * 32767.0);
					samplecount++;
				}
			}
			else
			{
				for(int i = 0; i < numsamples * 2; i++) 
				{
					int_samples[i] = (short int) (samples[i] * 32767.0);
					samplecount++;
				}
			}

			char	*bufcounter = (char *) int_samples;

			for(;;)
			{
				int in_used = 0;

				if(len <= 0) break;

				int enclen = g->fhaacpEncoder->Encode(in_used, bufcounter, len, &in_used, outbuffer, sizeof(outbuffer));

				if(enclen > 0) 
				{
					// can be part of NSV stream
					sentbytes = sendToServer(g, g->gSCSocket, (char *) outbuffer, enclen, CODEC_TYPE);
				}
				else 
				{
					break;
				}

				if(in_used > 0) 
				{
					bufcounter += in_used;
					len -= in_used;
				}
			}

			if(int_samples) 
			{
				free(int_samples);
			}
#endif
		}

		if(g->gAACPFlag)
		{
#ifdef HAVE_AACP
			static char outbuffer[32768];
			//static char inbuffer[32768];
			//static int	inbufferused = 0;
			int			len = numsamples * g->currentChannels * sizeof(short);

			int_samples = (short *) malloc(len);

			int samplecount = 0;

			if(g->currentChannels == 1) 
			{
				for(int i = 0; i < numsamples * 2; i = i + 2) 
				{
					int_samples[samplecount] = (short int) (samples[i] * 32767.0);
					samplecount++;
				}
			}
			else
			{
				for(int i = 0; i < numsamples * 2; i++) 
				{
					int_samples[i] = (short int) (samples[i] * 32767.0);
					samplecount++;
				}
			}

			char	*bufcounter = (char *) int_samples;

			for(;;)
			{
				int in_used = 0;

				if(len <= 0) break;

				int enclen = g->aacpEncoder->Encode(in_used, bufcounter, len, &in_used, outbuffer, sizeof(outbuffer));

				if(enclen > 0) 
				{
					// can be part of NSV stream
					sentbytes = sendToServer(g, g->gSCSocket, (char *) outbuffer, enclen, CODEC_TYPE);
				}
				else 
				{
					break;
				}

				if(in_used > 0) 
				{
					bufcounter += in_used;
					len -= in_used;
				}
			}

			if(int_samples) 
			{
				free(int_samples);
			}
#endif
		}

		if(g->gLAMEFlag)
		{
#ifdef HAVE_LAME
			/* Lame encoding is simple, we are passing it interleaved samples */
			int_samples = (short int *) malloc(numsamples * 2 * sizeof(short int));

			int samplecount = 0;

			if(g->currentChannels == 1) 
			{
				for(int i = 0; i < numsamples * 2; i = i + 2) 
				{
					int_samples[samplecount] = (int) (samples[i] * 32767.0);
					samplecount++;
				}
			}
			else
			{
				for(int i = 0; i < numsamples * 2; i++) 
				{
					int_samples[i] = (int) (samples[i] * 32767.0);
					samplecount++;
				}
			}

#ifdef WIN32
			unsigned long	dwWrite = 0;
			int				err = g->beEncodeChunk(g->hbeStream,
												   samplecount,
												   (short *) int_samples,
												   (PBYTE) mp3buffer,
												   &dwWrite);

			imp3 = dwWrite;
#else
			float	*samples_left;
			float	*samples_right;

			samples_left = (float *) malloc(numsamples * (sizeof(float)));
			samples_right = (float *) malloc(numsamples * (sizeof(float)));

			for(int i = 0; i < numsamples; i++)
			{
				samples_left[i] = samples[2 * i] * 32767.0;
				samples_right[i] = samples[2 * i + 1] * 32767.0;
			}

			imp3 = lame_encode_buffer_float(g->gf,
											(float *) samples_left,
											(float *) samples_right,
											numsamples,
											mp3buffer,
											sizeof(mp3buffer));
			if(samples_left)
			{
				free(samples_left);
			}

			if(samples_right)
			{
				free(samples_right);
			}
#endif
			if(int_samples) 
			{
				free(int_samples);
			}

			if(imp3 == -1) 
			{
				LogMessage(g,LOG_ERROR, "mp3 buffer is not big enough!");
				g->gCurrentlyEncoding = 0;
				return -1;
			}

			/* Send out the encoded buffer */
			// can be part of NSV stream
			sentbytes = sendToServer(g, g->gSCSocket, (char *) mp3buffer, imp3, CODEC_TYPE);
#endif
		}

		if(g->gFLACFlag)
		{
#ifdef HAVE_FLAC
			INT32		*int32_samples;

			int32_samples = (INT32 *) malloc(numsamples * 2 * sizeof(INT32));

			int samplecount = 0;

			if(g->currentChannels == 1) 
			{
				for(int i = 0; i < numsamples * 2; i = i + 2) 
				{
					int32_samples[samplecount] = (INT32) (samples[i] * 32767.0);
					samplecount++;
				}
			}
			else
			{
				for(int i = 0; i < numsamples * 2; i++) 
				{
					int32_samples[i] = (INT32) (samples[i] * 32767.0);
					samplecount++;
				}
			}

			FLAC__stream_encoder_process_interleaved(g->flacEncoder, int32_samples, numsamples);

			if(int32_samples) 
			{
				free(int32_samples);
			}

			if(g->flacFailure) 
			{
				sentbytes = 0;
			}
			else 
			{
				sentbytes = 1;
			}
#endif
		}

		/*
		 * Generic error checking, if there are any socket problems, the trigger ;
		 * a disconnection handling->..
		 */
		if(sentbytes < 0) 
		{
			g->gCurrentlyEncoding = 0;
			int rret = triggerDisconnect(g);
			if (rret == 0) return 0;
		}
	}

	g->gCurrentlyEncoding = 0;
	return 1;
}

int do_encoding_faster(shuicastGlobals *g, float *samples, int numsamples, int nchannels) 
{
	int				count = 0;
	unsigned char	mp3buffer[LAME_MAXMP3BUFFER];
	int				imp3;
	short int		*int_samples;
	int				eos = 0;
	int				ret = 0;
	int				sentbytes = 0;
	char			buf[255] = "";
    
	if(g->weareconnected) 
	{
		g->gCurrentlyEncoding = 1;

		int		samplecounter = 0;
		if(g->gOggFlag)
		{
#ifdef HAVE_VORBIS
			/*
			 * If a song change was detected, close the stream and resend new ;
			 * vorbis headers (with new comments) - all done by icecast2SendMetadata();
			 */
			if(g->ice2songChange) 
			{
				LogMessage(g,LOG_DEBUG, "Song change processing...");
				g->ice2songChange = false;
				icecast2SendMetadata(g);
			}

			LogMessage(g,LOG_DEBUG, "vorbis_analysis_buffer...");

			float	**buffer = vorbis_analysis_buffer(&g->vd, numsamples);
			samplecounter = 0;

			float * src = samples;
			if(g->currentChannels == 1)
			{
				while(samplecounter < numsamples)
				{
					buffer[0][samplecounter] = *(src++);
					samplecounter++;
				}
			}
			else
			{
				while(samplecounter < numsamples)
				{
					buffer[0][samplecounter] = *(src++);
					buffer[1][samplecounter] = *(src++);
					samplecounter++;
				}
			}
			LogMessage(g,LOG_DEBUG, "vorbis_analysis_wrote...");

			ret = vorbis_analysis_wrote(&g->vd, numsamples);

			pthread_mutex_lock(&(g->mutex));
			LogMessage(g,LOG_DEBUG, "ogg_encode_dataout...");
			/* Stream out what we just prepared for Vorbis... */
			sentbytes = ogg_encode_dataout(g);
			LogMessage(g,LOG_DEBUG, "done ogg_ecndoe_dataout...");
			pthread_mutex_unlock(&(g->mutex));
#endif
		}

		if(g->gAACFlag)
		{
#ifdef HAVE_FAAC // always always always stereo!!!
			int cnt = numsamples * g->currentChannels;
			int len = cnt * sizeof(float);

			float	*buffer = (float *) malloc(len);
			// this needs to be changed
			float * src = samples;
			float * dst = buffer;
			for(int i = 0; i < cnt; i ++)
			{
				*(dst++) = *(src++) * 32767.f;
			}
			//FloatScale(buffer, samples, numsamples * 2, g->currentChannels);

			addToFIFO(g, buffer, numsamples * g->currentChannels);

			while(g->faacFIFOendpos > (long) g->samplesInput) 
			{
				float	*buffer2 = (float *) malloc(g->samplesInput * 2 * sizeof(float));

				ExtractFromFIFO(buffer2, g->faacFIFO, g->samplesInput);

				int counter = 0;

				for(int i = g->samplesInput; i < g->faacFIFOendpos; i++) 
				{
					g->faacFIFO[counter] = g->faacFIFO[i];
					counter++;
				}

				g->faacFIFOendpos = counter;

				unsigned long	dwWrite = 0;
				unsigned char	*aacbuffer = (unsigned char *) malloc(g->maxBytesOutput);

				imp3 = faacEncEncode(g->aacEncoder, (int32_t *) buffer2, g->samplesInput, aacbuffer, g->maxBytesOutput);

				if(imp3) 
				{
					sentbytes = sendToServer(g, g->gSCSocket, (char *) aacbuffer, imp3, CODEC_TYPE);
				}

				if(buffer2) free(buffer2);
				if(aacbuffer) free(aacbuffer);
			}

			if(buffer) free(buffer);
#endif
		}

		if(g->gFHAACPFlag)
		{
#ifdef HAVE_FHGAACP
			static char outbuffer[32768];
			int cnt = numsamples * g->currentChannels;
			int len = cnt * sizeof(short);

			int_samples = (short *) malloc(len);
			float * src = samples;
			short * dst = int_samples;

			for(int i = 0; i < cnt; i++)
			{
				*(dst++) = (short int) (*(src++) * 32767.0);
			}

			char	*bufcounter = (char *) int_samples;

			for(;;)
			{
				int in_used = 0;

				if(len <= 0) break;

				int enclen = g->fhaacpEncoder->Encode(in_used, bufcounter, len, &in_used, outbuffer, sizeof(outbuffer));

				if(enclen > 0) 
				{
					// can be part of NSV stream
					sentbytes = sendToServer(g, g->gSCSocket, (char *) outbuffer, enclen, CODEC_TYPE);
				}
				else 
				{
					break;
				}

				if(in_used > 0) 
				{
					bufcounter += in_used;
					len -= in_used;
				}
			}

			if(int_samples) 
			{
				free(int_samples);
			}
#endif
		}

		if(g->gAACPFlag)
		{
#ifdef HAVE_AACP
			static char outbuffer[32768];
			int cnt = numsamples * g->currentChannels;
			int len = cnt * sizeof(short);

			int_samples = (short *) malloc(len);
			float * src = samples;
			short * dst = int_samples;

			for(int i = 0; i < cnt; i++)
			{
				*(dst++) = (short int) (*(src++) * 32767.0);
			}

			char	*bufcounter = (char *) int_samples;

			for(;;)
			{
				int in_used = 0;

				if(len <= 0) break;

				int enclen = g->aacpEncoder->Encode(in_used, bufcounter, len, &in_used, outbuffer, sizeof(outbuffer));

				if(enclen > 0) 
				{
					// can be part of NSV stream
					sentbytes = sendToServer(g, g->gSCSocket, (char *) outbuffer, enclen, CODEC_TYPE);
				}
				else 
				{
					break;
				}

				if(in_used > 0) 
				{
					bufcounter += in_used;
					len -= in_used;
				}
			}

			if(int_samples) 
			{
				free(int_samples);
			}
#endif
		}

		if(g->gLAMEFlag)
		{
#ifdef HAVE_LAME
			/* Lame encoding is simple, we are passing it interleaved samples */
			int cnt = numsamples * g->currentChannels;
			int len = cnt * sizeof(short);
			int_samples = (short int *) malloc(len);

			float * src = samples;
			short * dst = int_samples;

			for(int i = 0; i < cnt; i++) 
			{
				*(dst++) = (short int) (*(src++) * 32767.0);
			}

#ifdef WIN32
			unsigned long	dwWrite = 0;
			int				err = g->beEncodeChunk(g->hbeStream,
												   numsamples,
												   (short *) int_samples,
												   (PBYTE) mp3buffer,
												   &dwWrite);

			imp3 = dwWrite;
#else
			float	*samples_left;
			float	*samples_right;

			samples_left = (float *) malloc(numsamples * (sizeof(float)));
			samples_right = (float *) malloc(numsamples * (sizeof(float)));

			for(int i = 0; i < numsamples; i++)
			{
				samples_left[i] = samples[2 * i] * 32767.0;
				samples_right[i] = samples[2 * i + 1] * 32767.0;
			}

			imp3 = lame_encode_buffer_float(g->gf,
											(float *) samples_left,
											(float *) samples_right,
											numsamples,
											mp3buffer,
											sizeof(mp3buffer));
			if(samples_left)
			{
				free(samples_left);
			}

			if(samples_right)
			{
				free(samples_right);
			}
#endif
			if(int_samples) 
			{
				free(int_samples);
			}

			if(imp3 == -1) 
			{
				LogMessage(g,LOG_ERROR, "mp3 buffer is not big enough!");
				g->gCurrentlyEncoding = 0;
				return -1;
			}

			/* Send out the encoded buffer */
			// can be part of NSV stream
			sentbytes = sendToServer(g, g->gSCSocket, (char *) mp3buffer, imp3, CODEC_TYPE);
#endif
		}

		if(g->gFLACFlag)
		{
#ifdef HAVE_FLAC
			int cnt = numsamples * g->currentChannels;
			INT32		*int32_samples;

			int32_samples = (INT32 *) malloc(cnt * sizeof(INT32));
			float * src = samples;
			INT32 * dst = int32_samples;

			for(int i = 0; i < cnt; i++) 
			{
				*(dst++) = (INT32) (*(src++) * 32767.0);
			}

			FLAC__stream_encoder_process_interleaved(g->flacEncoder, int32_samples, numsamples);

			if(int32_samples) 
			{
				free(int32_samples);
			}

			if(g->flacFailure) 
			{
				sentbytes = 0;
			}
			else 
			{
				sentbytes = 1;
			}
#endif
		}

		/*
		 * Generic error checking, if there are any socket problems, the trigger ;
		 * a disconnection handling->..
		 */
		if(sentbytes < 0) 
		{
			g->gCurrentlyEncoding = 0;
			int rret = triggerDisconnect(g);
			if (rret == 0) 
			{
				return 0;
			}
		}
	}

	g->gCurrentlyEncoding = 0;
	return 1;
}

int triggerDisconnect(shuicastGlobals *g) 
{
	char buf[2046] = "";

	disconnectFromServer(g);
	if(g->gForceStop) 
	{
		g->gForceStop = 0;
		return 0;
	}


	wsprintf(buf, "Disconnected from server");
	g->forcedDisconnect = true;
	//g->forcedDisconnectSecs = time(&(g->forcedDisconnectSecs));
	g->forcedDisconnectSecs = time(NULL);
	g->serverStatusCallback(g, (void *) buf);
	return 1;
}

void config_read(shuicastGlobals *g) 
{
	strcpy(g->gAppName, "shuicast");

	char	buf[255] = "";
	char	desc[1024] = "";

#ifdef XMMS_PLUGIN
	wsprintf(desc, "This is the named pipe used to communicate with the XMMS effect plugin. Make sure it matches the settings in that plugin");
	GetConfigVariable(g, g->gAppName, "SourceURL", "/tmp/shuicastFIFO", g->gSourceURL, sizeof(g->gSourceURL), desc);
#else
	wsprintf(desc, "The source URL for the broadcast. It must be in the form http://server:port/mountpoint.  For those servers without a mountpoint (Shoutcast) use http://server:port.");
	GetConfigVariable(g, g->gAppName, "SourceURL", "http://localhost/", g->gSourceURL, sizeof(g->gSourceURL), desc);
#endif
	if(g->sourceURLCallback)
	{
		g->sourceURLCallback(g, (char *) g->gSourceURL);
	}

	wsprintf(desc, "Destination server details (to where you are encoding).  Valid server types : Shoutcast, Icecast, Icecast2");
	GetConfigVariable(g, g->gAppName, "ServerType", "Icecast2", g->gServerType, sizeof(g->gServerType), desc);
//	wsprintf(desc, "The server to which the stream is sent. It can be a hostname  or IP (example: www.stream.com, 192.168.1.100)");
	GetConfigVariable(g, g->gAppName, "Server", "localhost", g->gServer, sizeof(g->gServer), NULL);
//	wsprintf(desc, "The port to which the stream is sent. Must be a number (example: 8000)");
	GetConfigVariable(g, g->gAppName, "Port", "8000", g->gPort, sizeof(g->gPort), NULL);
//	wsprintf(desc, "This is the encoder password for the destination server (example: hackme)");
	GetConfigVariable(g, g->gAppName, "ServerPassword", "changemenow", g->gPassword, sizeof(g->gPassword), NULL);
//	wsprintf(desc,"Used for Icecast/Icecast2 servers, The mountpoint must end in .ogg for Vorbis streams and have NO extention for MP3 streams.  If you are sending to a Shoutcast server, this MUST be blank. (example: /mp3, /myvorbis.ogg)");
	GetConfigVariable(g, g->gAppName, "ServerMountpoint", "/stream.ogg", g->gMountpoint, sizeof(g->gMountpoint), NULL);
//	wsprintf(desc,"This setting tells the destination server to list on any available YP listings. Not all servers support this (Shoutcast does, Icecast2 doesn't) (example: 1 for YES, 0 for NO)");
	wsprintf(desc,"YP (Stream Directory) Settings");
	g->gPubServ = GetConfigVariableLong(g, g->gAppName, "ServerPublic", 1, desc);
//	wsprintf(desc, "This is used in the YP listing, I think only Shoutcast supports this (example: #mystream)");
	GetConfigVariable(g, g->gAppName, "ServerIRC", "", g->gServIRC, sizeof(g->gServIRC), NULL);
//	wsprintf(desc, "This is used in the YP listing, I think only Shoutcast supports this (example: myAIMaccount)");
	GetConfigVariable(g, g->gAppName, "ServerAIM", "", g->gServAIM, sizeof(g->gServAIM), NULL);
//	wsprintf(desc, "This is used in the YP listing, I think only Shoutcast supports this (example: 332123132)");
	GetConfigVariable(g, g->gAppName, "ServerICQ", "", g->gServICQ, sizeof(g->gServICQ), NULL);
//	wsprintf(desc, "The URL that is associated with your stream. (example: http://www.mystream.com)");
	GetConfigVariable(g, g->gAppName, "ServerStreamURL", "http://www.shoutcast.com", g->gServURL, sizeof(g->gServURL), NULL);
//	wsprintf(desc, "The Stream Name");
	GetConfigVariable(g, g->gAppName, "ServerName", "This is my server name", g->gServName, sizeof(g->gServName), NULL);
//	wsprintf(desc, "A short description of the stream (example: Stream House on Fire!)");
	GetConfigVariable(g, g->gAppName, "ServerDescription", "This is my server description", g->gServDesc, sizeof(g->gServDesc), NULL);
//	wsprintf(desc, "Genre of music, can be anything you want... (example: Rock)");
	GetConfigVariable(g, g->gAppName, "ServerGenre", "Rock", g->gServGenre, sizeof(g->gServGenre), NULL);
//	wsprintf(desc,"Wether or not ShuiCast will reconnect if it is disconnected from the destination server (example: 1 for YES, 0 for NO)");
	wsprintf(desc,"Misc encoder properties");
	g->gAutoReconnect = GetConfigVariableLong(g, g->gAppName, "AutomaticReconnect", 1, desc);
//	wsprintf(desc, "How long it will wait (in seconds) between reconnect attempts. (example: 10)");
	g->gReconnectSec = GetConfigVariableLong(g, g->gAppName, "AutomaticReconnectSecs", 10, NULL);

	g->autoconnect = GetConfigVariableLong(g, g->gAppName, "AutoConnect", 0, NULL);

	wsprintf(desc,"Set to 1 to start minimized");
	g->gStartMinimized = GetConfigVariableLong(g, g->gAppName, "StartMinimized", 0, desc);
	wsprintf(desc,"Set to 1 to enable limiter");
	g->gLimiter = GetConfigVariableLong(g, g->gAppName, "Limiter", 0, desc);
	wsprintf(desc,"Limiter dB");
	g->gLimitdb = GetConfigVariableLong(g, g->gAppName, "LimitDB", 0, desc);
	wsprintf(desc,"Limiter GAIN dB");
	g->gGaindb = GetConfigVariableLong(g, g->gAppName, "LimitGainDB", 0, desc);
	wsprintf(desc,"Limiter pre-emphasis");
	g->gLimitpre = GetConfigVariableLong(g, g->gAppName, "LimitPRE", 0, desc);

	//	wsprintf(desc, "What format to encode to. Valid values are (OGG, LAME) (example: OGG, LAME)");
	wsprintf(desc, "Output codec selection (Valid selections : MP3, OggVorbis, Ogg FLAC, AAC, AAC Plus, HE-AAC, HE-AAC High, LC-AAC, FHGAAC-AUTO, FHGAAC-LC, FHGAAC-HE, FHGAAC-HEv2)");
	GetConfigVariable(g, g->gAppName, "Encode", "OggVorbis", g->gEncodeType, sizeof(g->gEncodeType), desc);

    g->gAACPFlag   = 0;
    g->gOggFlag    = 0;
    g->gLAMEFlag   = 0;
    g->gAACFlag    = 0;
    g->gFLACFlag   = 0;
    g->gFHAACPFlag = 0;

    if ( !strncmp( g->gEncodeType, "MP3", 3 ) )  // LAME
	{
		g->gLAMEFlag = 1;
	}
    else if ( !strcmp( g->gEncodeType, "FHGAAC-AUTO" ) )
    {
		g->gFHAACPFlag = 1;
	}

    else if ( !strcmp( g->gEncodeType, "FHGAAC-LC" ) )
	{
		g->gFHAACPFlag = 2;
	}
    else if ( !strcmp( g->gEncodeType, "FHGAAC-HE" ) )
	{
		g->gFHAACPFlag = 3;
	}
    else if ( !strcmp( g->gEncodeType, "FHGAAC-HEv2" ) )
	{
		g->gFHAACPFlag = 4;
	}
    else if ( !strcmp( g->gEncodeType, "HE-AAC" ) )
	{
		g->gAACPFlag = 1;
	}
    else if ( !strcmp( g->gEncodeType, "HE-AAC High" ) )
	{
		g->gAACPFlag = 2;
	}
    else if ( !strcmp( g->gEncodeType, "LC-AAC" ) )
	{
		g->gAACPFlag = 3;
	}
    else if ( !strncmp( g->gEncodeType, "AAC Plus", 8 ) )
    {
		g->gAACPFlag = 1;
	}
    else if ( !strcmp( g->gEncodeType, "AAC" ) )
	{
		g->gAACFlag = 1;
	}
    else if ( !strcmp( g->gEncodeType, "OggVorbis" ) )
	{
		g->gOggFlag = 1;
	}
    else if ( !strcmp( g->gEncodeType, "Ogg FLAC" ) )
	{
		g->gFLACFlag = 1;
	}

	if(g->streamTypeCallback)
	{
		if(g->gOggFlag) g->streamTypeCallback(g, (void *) "OggVorbis");
		if(g->gLAMEFlag) g->streamTypeCallback(g, (void *) "MP3");
		if(g->gAACFlag) g->streamTypeCallback(g, (void *) "AAC");
		if(g->gAACPFlag) g->streamTypeCallback(g, (void *) "AAC+");
		if(g->gFHAACPFlag) g->streamTypeCallback(g, (void *) "FHGAAC");
		if(g->gFLACFlag) g->streamTypeCallback(g, (void *) "OggFLAC");
	}

	if(g->destURLCallback) 
	{
		wsprintf(buf, "http://%s:%s%s", g->gServer, g->gPort, g->gMountpoint);
		g->destURLCallback(g, (char *) buf);
	}

//	wsprintf(desc, "Bitrate. This is the mean bitrate if using VBR.");
	wsprintf(desc, "General settings (non-codec related).  Note : NumberChannels = 1 for MONO, 2 for STEREO");
	g->currentBitrate = GetConfigVariableLong(g, g->gAppName, "BitrateNominal", 128, desc);

//	wsprintf(desc,"Minimum Bitrate. Used only if using Bitrate Management (not recommended) or LAME VBR(example: 64, 128)");
	g->currentBitrateMin = GetConfigVariableLong(g, g->gAppName, "BitrateMin", 128, NULL);

//	wsprintf(desc,"Maximum Bitrate. Used only if using Bitrate Management (not recommended) or LAME VBR (example: 64, 128)");
	g->currentBitrateMax = GetConfigVariableLong(g, g->gAppName, "BitrateMax", 128, NULL);

	wsprintf(desc, "Number of channels. Valid values are (1, 2). 1 means Mono, 2 means Stereo (example: 2,1)");
	g->currentChannels = GetConfigVariableLong(g, g->gAppName, "NumberChannels", 2, desc);

	{
		wsprintf(desc, "Per encoder Attenuation");
		GetConfigVariable(g, g->gAppName, "Attenuation", "0.0", g->attenuation, sizeof(g->attenuation), desc);
		double atten = -fabs(atof(g->attenuation));
		g->dAttenuation = pow(10.0, atten/20.0);
	}
	//	wsprintf(desc, "Sample rate for the stream. Valid values depend on wether using lame or vorbis. Vorbis supports odd samplerates such as 32kHz and 48kHz, but lame appears to not.feel free to experiment (example: 44100, 22050, 11025)");
	g->currentSamplerate = GetConfigVariableLong(g, g->gAppName, "Samplerate", 44100, NULL);

//	wsprintf(desc, "Vorbis Quality Level. Valid values are between -1 (lowest quality) and 10 (highest).  The lower the quality the lower the output bitrate. (example: -1, 3)");
	wsprintf(desc, "Ogg Vorbis specific settings.  Note: Valid settings for BitrateQuality flag are (Quality, Bitrate Management)");
	GetConfigVariable(g, g->gAppName, "OggQuality", "0", g->gOggQuality, sizeof(g->gOggQuality), desc);


//	wsprintf(desc,"This flag specifies if you want Vorbis Quality or Bitrate Management.  Quality is always recommended. Valid values are (Bitrate, Quality). (example: Quality, Bitrate Management)");
	GetConfigVariable(g, g->gAppName, "OggBitrateQualityFlag", "Quality", g->gOggBitQual, sizeof(g->gOggBitQual), NULL);
	g->gOggBitQualFlag = 0;
	if(!strncmp(g->gOggBitQual, "Q", 1))
	{
		/* Quality */
		g->gOggBitQualFlag = 0;
	}

	if(!strncmp(g->gOggBitQual, "B", 1))
	{
		/* Bitrate */
		g->gOggBitQualFlag = 1;
	}

	g->gAutoCountdown = atoi(g->gAutoStartSec);
	if(strlen(g->gMountpoint) > 0)
	{
		strcpy(g->gIceFlag, "1");
	}
	else
	{
		strcpy(g->gIceFlag, "0");
	}

	char	tempString[255] = "";

	memset(tempString, '\000', sizeof(tempString));
	ReplaceString(g->gServer, tempString, " ", "");
	strcpy(g->gServer, tempString);

	memset(tempString, '\000', sizeof(tempString));
	ReplaceString(g->gPort, tempString, " ", "");
	strcpy(g->gPort, tempString);

//	wsprintf(desc,"LAME specific settings.  Note: Setting the low/highpass freq to 0 will disable them.");
	wsprintf(desc,"This LAME flag indicates that CBR encoding is desired. If this flag is set then LAME with use CBR, if not set then it will use VBR (and you must then specify a VBR mode). Valid values are (1 for SET, 0 for NOT SET) (example: 1)");
	g->gLAMEOptions.cbrflag = GetConfigVariableLong(g, g->gAppName, "LameCBRFlag", 1, desc);
	wsprintf(desc,"A number between 0 and 9 which indicates the desired quality level of the stream.  0 = highest to 9 = lowest");
	g->gLAMEOptions.quality = GetConfigVariableLong(g, g->gAppName, "LameQuality", 0, desc);

	wsprintf(desc, "Copywrite flag-> Not used for much. Valid values (1 for YES, 0 for NO)");
	g->gLAMEOptions.copywrite = GetConfigVariableLong(g, g->gAppName, "LameCopywrite", 0, desc);
	wsprintf(desc, "Original flag-> Not used for much. Valid values (1 for YES, 0 for NO)");
	g->gLAMEOptions.original = GetConfigVariableLong(g, g->gAppName, "LameOriginal", 0, desc);
	wsprintf(desc, "Strict ISO flag-> Not used for much. Valid values (1 for YES, 0 for NO)");
	g->gLAMEOptions.strict_ISO = GetConfigVariableLong(g, g->gAppName, "LameStrictISO", 0, desc);
	wsprintf(desc, "Disable Reservior flag-> Not used for much. Valid values (1 for YES, 0 for NO)");
	g->gLAMEOptions.disable_reservoir = GetConfigVariableLong(g, g->gAppName, "LameDisableReservior", 1, desc);
	wsprintf(desc, "This specifies the type of VBR encoding LAME will perform if VBR encoding is set (CBRFlag is NOT SET). See the LAME documention for more on what these mean. Valid values are (vbr_rh, vbr_mt, vbr_mtrh, vbr_abr, vbr_cbr)");
	GetConfigVariable(g, g->gAppName, "LameVBRMode", "vbr_cbr",  g->gLAMEOptions.VBR_mode, sizeof(g->gLAMEOptions.VBR_mode), desc);

	wsprintf(desc, "Use LAMEs lowpass filter. If you set this to 0, then no filtering is done - not implemented");
	g->gLAMEOptions.lowpassfreq = GetConfigVariableLong(g, g->gAppName, "LameLowpassfreq", 0, desc);
	wsprintf(desc, "Use LAMEs highpass filter. If you set this to 0, then no filtering is done - not implemented");
	g->gLAMEOptions.highpassfreq = GetConfigVariableLong(g, g->gAppName, "LameHighpassfreq", 0, desc);

	if(g->gLAMEOptions.lowpassfreq > 0)
	{
		g->gLAMELowpassFlag = 1;
	}

	if(g->gLAMEOptions.highpassfreq > 0)
	{
		g->gLAMELowpassFlag = 1;
	}
	wsprintf(desc, "LAME Preset");
	int defaultPreset = 0;
#ifdef WIN32
	defaultPreset = LQP_NOPRESET;
#endif
	wsprintf(desc, "LAME Preset - Interesting ones are: -1 = None, 0 = Normal Quality, 1 = Low Quality, 2 = High Quality ... 5 = Very High Quality, 11 = ABR, 12 = CBR");
	g->gLAMEpreset = GetConfigVariableLong(g, g->gAppName, "LAMEPreset", defaultPreset, desc);

	wsprintf(desc, "AAC (FAAC) specific settings.");
//	wsprintf(desc, "AAC Quality Level. Valid values are between 10 (lowest quality) and 500 (highest).");
	GetConfigVariable(g, g->gAppName, "AACQuality", "100", g->gAACQuality, sizeof(g->gAACQuality), desc);
//	wsprintf(desc, "AAC Cutoff Frequency.");
	GetConfigVariable(g, g->gAppName, "AACCutoff", "", g->gAACCutoff, sizeof(g->gAACCutoff), NULL);

	if(!strcmp(g->gServerType, "KasterBlaster"))
	{
		g->gShoutcastFlag = 1;
		g->gIcecastFlag = 0;
		g->gIcecast2Flag = 0;
	}

	if(!strcmp(g->gServerType, "Shoutcast"))
	{
		g->gShoutcastFlag = 1;
		g->gIcecastFlag = 0;
		g->gIcecast2Flag = 0;
	}

	if(!strcmp(g->gServerType, "Icecast")) 
	{
		g->gShoutcastFlag = 0;
		g->gIcecastFlag = 1;
		g->gIcecast2Flag = 0;
	}

	if(!strcmp(g->gServerType, "Icecast2"))
	{
		g->gShoutcastFlag = 0;
		g->gIcecastFlag = 0;
		g->gIcecast2Flag = 1;
	}

	if(g->serverTypeCallback) 
	{
		g->serverTypeCallback(g, (void *) g->gServerType);
	}

	if(g->serverNameCallback)
	{
		char	*pdata = NULL;
		int		pdatalen = strlen(g->gServDesc) + strlen(g->gServName) + strlen(" () ") + 1;

		pdata = (char *) calloc(1, pdatalen);
		wsprintf(pdata, "%s (%s)", g->gServName, g->gServDesc);
		g->serverNameCallback(g, (void *) pdata);
		free(pdata);
	}

	wsprintf(desc, "If recording from linein, what device to use (not needed for win32) (example: /dev/dsp)");
	GetConfigVariable(g, g->gAppName, "AdvRecDevice", "/dev/dsp", buf, sizeof(buf), desc);
	strcpy(g->gAdvRecDevice, buf);

	wsprintf(desc, "If recording from linein, what sample rate to open the device with. (example: 44100, 48000)");
	GetConfigVariable(g, g->gAppName, "LiveInSamplerate", "44100", buf, sizeof(buf), desc);
	g->gLiveInSamplerate = atoi(buf);

	wsprintf(desc, "Used for any window positions (X value)");
	g->lastX = GetConfigVariableLong(g, g->gAppName, "lastX", 0, desc);
	wsprintf(desc, "Used for any window positions (Y value)");
	g->lastY = GetConfigVariableLong(g, g->gAppName, "lastY", 0, desc);
/*
	wsprintf(desc, "Used for dummy window positions (X value)");
	g->lastDummyX = GetConfigVariableLong(g, g->gAppName, "lastDummyX", 0, desc);
	wsprintf(desc, "Used for dummy window positions (Y value)");
	g->lastDummyY = GetConfigVariableLong(g, g->gAppName, "lastDummyY", 0, desc);
*/
	wsprintf(desc, "Used for plugins that show the VU meter");
	g->vuShow = GetConfigVariableLong(g, g->gAppName, "showVU", 0, desc);

	wsprintf(desc, "Flag which indicates we are recording from line in");

	int lineInDefault = 0;

#ifdef SHUICASTSTANDALONE
	lineInDefault = 1;
#endif
	g->gLiveRecordingFlag = GetConfigVariableLong(g, g->gAppName, "LineInFlag", lineInDefault, desc);

	wsprintf(desc, "Locked Metadata");
	GetConfigVariable(g, g->gAppName, "LockMetadata", "", g->gManualSongTitle, sizeof(g->gManualSongTitle), desc);
	wsprintf(desc, "Flag which indicates if we are using locked metadata");
	g->gLockSongTitle = GetConfigVariableLong(g, g->gAppName, "LockMetadataFlag", 0, desc);

	wsprintf(desc, "Save directory for archive streams");
	GetConfigVariable(g, g->gAppName, "SaveDirectory", "", g->gSaveDirectory, sizeof(g->gSaveDirectory), desc);
	wsprintf(desc, "Flag which indicates if we are saving archives");
	g->gSaveDirectoryFlag = GetConfigVariableLong(g, g->gAppName, "SaveDirectoryFlag", 0, desc);
	wsprintf(desc, "Log Level 1 = LOG_ERROR, 2 = LOG_ERROR+LOG_INFO, 3 = LOG_ERROR+LOG_INFO+LOG_DEBUG");
	g->gLogLevel = GetConfigVariableLong(g, g->gAppName, "LogLevel", 2, desc);
	wsprintf(desc, "Log File");
	GetConfigVariable(g, g->gAppName, "LogFile", defaultLogFileName, g->gLogFile, sizeof(g->gLogFile), desc);

	setgLogFile(g, g->gLogFile);

	wsprintf(desc, "Save Archives in WAV format");
	g->gSaveAsWAV = GetConfigVariableLong(g, g->gAppName, "SaveAsWAV", 0, desc);

	wsprintf(desc, "ASIO channel selection 0 1 or 2 only");
	g->gAsioSelectChannel = GetConfigVariableLong(g, g->gAppName, "AsioSelectChannel", 0, desc);

	wsprintf(desc, "ASIO channel");
	GetConfigVariable(g, g->gAppName, "AsioChannel", "", g->gAsioChannel, sizeof(g->gAsioChannel),  desc);

	wsprintf(desc, "Encoder Scheduler");
	g->gEnableEncoderScheduler = GetConfigVariableLong(g, g->gAppName, "EnableEncoderScheduler", 0, desc);


#define DOW_GETCONFIG(dow, dows) \
	wsprintf(desc, dows##" Schedule"); \
	g->g##dow##Enable = GetConfigVariableLong(g, g->gAppName, dows##"Enable", 1, desc); \
	g->g##dow##OnTime = GetConfigVariableLong(g, g->gAppName, dows##"OnTime", 0, NULL); \
	g->g##dow##OffTime = GetConfigVariableLong(g, g->gAppName, dows##"OffTime", 24, NULL); 

	DOW_GETCONFIG(Monday, "Monday");
	DOW_GETCONFIG(Tuesday, "Tuesday");
	DOW_GETCONFIG(Wednesday, "Wednesday");
	DOW_GETCONFIG(Thursday, "Thursday");
	DOW_GETCONFIG(Friday, "Friday");
	DOW_GETCONFIG(Saturday, "Saturday");
	DOW_GETCONFIG(Sunday, "Sunday");

	wsprintf(desc, "Append this string to all metadata");
	GetConfigVariable(g, g->gAppName, "MetadataAppend", "", g->metadataAppendString, sizeof(g->metadataAppendString),  desc);
	wsprintf(desc, "Remove this string (and everything after) from the window title of the window class the metadata is coming from");
	GetConfigVariable(g, g->gAppName, "MetadataRemoveAfter", "", g->metadataRemoveStringAfter, sizeof(g->metadataRemoveStringAfter), desc);
	wsprintf(desc,"Remove this string (and everything before) from the window title of the window class the metadata is coming from");
	GetConfigVariable(g, g->gAppName, "MetadataRemoveBefore", "", g->metadataRemoveStringBefore,  sizeof(g->metadataRemoveStringBefore), desc);
	wsprintf(desc, "Window classname to grab metadata from (uses window title)");
	GetConfigVariable(g, g->gAppName, "MetadataWindowClass", "", g->metadataWindowClass, sizeof(g->metadataWindowClass), desc);
	wsprintf(desc, "Indicator which tells ShuiCast to grab metadata from a defined window class");
	
	wsprintf(desc, "LAME Joint Stereo Flag");
	g->LAMEJointStereoFlag = GetConfigVariableLong(g, g->gAppName, "LAMEJointStereo", 1, desc);
	wsprintf(desc, "Set to 1, this encoder will record from DSP regardless of live record state");
	g->gForceDSPrecording = GetConfigVariableLong(g, g->gAppName, "ForceDSPrecording", 0, desc);
	wsprintf(desc, "Set ThreeHourBug=1 if your stream distorts after 3 hours 22 minutes and 56 seconds");
	g->gThreeHourBug = GetConfigVariableLong(g, g->gAppName, "ThreeHourBug", 0, desc);
	wsprintf(desc, "Set SkipCloseWarning=1 to remove the windows close warning");
	g->gSkipCloseWarning = GetConfigVariableLong(g, g->gAppName, "SkipCloseWarning", 0, desc);
	wsprintf(desc, "Set ASIO rate to 44100 or 48000");
	g->gAsioRate = GetConfigVariableLong(g, g->gAppName, "AsioRate", 48000, desc);

	g->metadataWindowClassInd = GetConfigVariableLong(g, g->gAppName, "MetadataWindowClassInd", 0, NULL) != 0;

	/* Set some derived values */
	char	localBitrate[255] = "";
	char	mode[50] = "";

	if(g->currentChannels == 1)
	{
		strcpy(mode, "Mono");
	}

	if(g->currentChannels == 2)
	{
		strcpy(mode, "Stereo");
	}

	if(g->gOggFlag) 
	{
		if(g->bitrateCallback)
		{
			if(g->gOggBitQualFlag == 0)  /* Quality */
			{
				wsprintf(localBitrate, "Vorbis: Quality %s/%s/%d", g->gOggQuality, mode, g->currentSamplerate);
			}
			else 
			{
				wsprintf(localBitrate, "Vorbis: %dkbps/%s/%d", g->currentBitrate, mode, g->currentSamplerate);
			}
			g->bitrateCallback(g, (void *) localBitrate);
		}
	}

	if(g->gLAMEFlag)
	{
		if(g->bitrateCallback)
		{
			if(g->gLAMEOptions.cbrflag)
			{
				wsprintf(localBitrate, "MP3: %dkbps/%dHz/%s", g->currentBitrate, g->currentSamplerate, mode);
			}
			else 
			{
				wsprintf(localBitrate, "MP3: (%d/%d/%d)/%s/%d", g->currentBitrateMin, g->currentBitrate, g->currentBitrateMax, mode, g->currentSamplerate);
			}

			g->bitrateCallback(g, (void *) localBitrate);
		}
	}

	if(g->gAACFlag)
	{
		if(g->bitrateCallback) 
		{
			wsprintf(localBitrate, "AAC: Quality %s/%dHz/%s", g->gAACQuality, g->currentSamplerate, mode);
			g->bitrateCallback(g, (void *) localBitrate);
		}
	}

	if(g->gFHAACPFlag)
	{
		if(g->bitrateCallback) 
		{
			char enc[20];
			switch(g->gFHAACPFlag) 
			{
			case 1: strcpy(enc, "AACP-AUTO(fh)"); break;
			case 2: strcpy(enc, "LC-AAC(fh)"); break;
			case 3: strcpy(enc, "HE-AAC(fh)"); break;
			case 4: strcpy(enc, "HE-AACv2(fh)"); break;
			}
			wsprintf(localBitrate, "%s: %dkbps/%dHz", enc, g->currentBitrate, g->currentSamplerate);
			g->bitrateCallback(g, (void *) localBitrate);
		}
	}

	if(g->gAACPFlag)
	{
		if(g->bitrateCallback) 
		{
			long	bitrateLong = g->currentBitrate * 1000;
			char enc[20];

			switch(g->gAACPFlag) 
			{
			case 1:
				strcpy(enc, "HE-AAC(ct)");
				if(bitrateLong > 64000)
				{
					strcpy(mode, "Stereo");
					if(g->currentChannels == 1) 
					{
						strcat(mode, "*");
					}
				}
				else if(bitrateLong > 56000) 
				{
					if(g->currentChannels == 2) 
					{
						strcpy(mode, "Stereo");
					}
					else 
					{
						strcpy(mode, "Mono");
					}
				}
				else if(bitrateLong >= 16000) 
				{
					if(g->currentChannels == 2) 
					{
						if (g->LAMEJointStereoFlag && bitrateLong <= 56000) 
						{
							strcpy(mode, "PS");
						}
						else
						{
							strcpy(mode, "Stereo");
						}
					}
					else 
					{
						strcpy(mode, "Mono");
					}
				}
				else if(bitrateLong >= 12000) 
				{
					if(g->currentChannels == 2) 
					{
						strcpy(mode, "PS");
						if (!g->LAMEJointStereoFlag)
						{
							strcat(mode, "*");
						}
					}
					else 
					{
						strcpy(mode, "Mono");
					}
				}
				else 
				{
					strcpy(mode, "Mono");
					if(g->currentChannels != 1) 
					{
						strcat(mode, "*");
					}
				}
				break;
			case 2:
				strcpy(enc, "HE-AAC High(ct)");
				strcpy(mode, "Stereo");
				break;
			case 3:
				strcpy(enc, "LC-AAC(ct)");
				strcpy(mode, "Stereo");
				break;
			}
			wsprintf(localBitrate, "%s: %dkbps/%dHz/%s", enc, g->currentBitrate, g->currentSamplerate, mode);
			g->bitrateCallback(g, (void *) localBitrate);
		}
	}

	if(g->gFLACFlag) 
	{
		if(g->bitrateCallback) 
		{
			wsprintf(localBitrate, "FLAC: %dHz/%s", g->currentSamplerate, mode);
			g->bitrateCallback(g, (void *) localBitrate);
		}
	}

	if(g->serverStatusCallback)
	{
		g->serverStatusCallback(g, (void *) "Disconnected" );
	}

	wsprintf(desc, "Number of encoders to use");
	g->gNumEncoders = GetConfigVariableLong(g, g->gAppName, "NumEncoders", 0, desc);

	wsprintf(desc, "Enable external metadata calls (DISABLED, URL, FILE)");
	GetConfigVariable(g, g->gAppName, "ExternalMetadata", "DISABLED", g->externalMetadata, sizeof(g->gLogFile), desc);
	wsprintf(desc, "URL to retrieve for external metadata");
	GetConfigVariable(g, g->gAppName, "ExternalURL", "", g->externalURL, sizeof(g->externalURL), desc);
	wsprintf(desc, "File to retrieve for external metadata");
	GetConfigVariable(g, g->gAppName, "ExternalFile", "", g->externalFile, sizeof(g->externalFile), desc);
	wsprintf(desc, "Interval for retrieving external metadata");
	GetConfigVariable(g, g->gAppName, "ExternalInterval", "60", g->externalInterval, sizeof(g->externalInterval), desc);
	wsprintf(desc, "Advanced setting");
	GetConfigVariable(g, g->gAppName, "OutputControl", "", g->outputControl, sizeof(g->outputControl), desc);

	wsprintf(desc, "Windows Recording Device");
	GetConfigVariable(g, g->gAppName, "WindowsRecDevice", "", buf, sizeof(buf), desc);
	strcpy(g->WindowsRecDevice, buf);

	wsprintf(desc, "Windows Recording Sub Device");
	GetConfigVariable(g, g->gAppName, "WindowsRecSubDevice", "", buf, sizeof(buf), desc);
	strcpy(g->WindowsRecSubDevice, buf);

	wsprintf(desc, "LAME Joint Stereo Flag");
	g->LAMEJointStereoFlag = GetConfigVariableLong(g, g->gAppName, "LAMEJointStereo", 1, desc);

}

void config_write(shuicastGlobals *g) 
{
	strcpy(g->gAppName, "shuicast");

	char	buf[255] = "";
	char	desc[1024] = "";
	char	tempString[1024] = "";

	memset(tempString, '\000', sizeof(tempString));
	ReplaceString(g->gServer, tempString, " ", "");
	strcpy(g->gServer, tempString);

	memset(tempString, '\000', sizeof(tempString));
	ReplaceString(g->gPort, tempString, " ", "");
	strcpy(g->gPort, tempString);

	PutConfigVariable(g, g->gAppName, "SourceURL", g->gSourceURL);
	PutConfigVariable(g, g->gAppName, "ServerType", g->gServerType);
	PutConfigVariable(g, g->gAppName, "Server", g->gServer);
	PutConfigVariable(g, g->gAppName, "Port", g->gPort);
	PutConfigVariable(g, g->gAppName, "ServerMountpoint", g->gMountpoint);
	PutConfigVariable(g, g->gAppName, "ServerPassword", g->gPassword);
	PutConfigVariableLong(g, g->gAppName, "ServerPublic", g->gPubServ);
	PutConfigVariable(g, g->gAppName, "ServerIRC", g->gServIRC);
	PutConfigVariable(g, g->gAppName, "ServerAIM", g->gServAIM);
	PutConfigVariable(g, g->gAppName, "ServerICQ", g->gServICQ);
	PutConfigVariable(g, g->gAppName, "ServerStreamURL", g->gServURL);
	PutConfigVariable(g, g->gAppName, "ServerDescription", g->gServDesc);
	PutConfigVariable(g, g->gAppName, "ServerName", g->gServName);
	PutConfigVariable(g, g->gAppName, "ServerGenre", g->gServGenre);
	PutConfigVariableLong(g, g->gAppName, "AutomaticReconnect", g->gAutoReconnect);
	PutConfigVariableLong(g, g->gAppName, "AutomaticReconnectSecs", g->gReconnectSec);
	PutConfigVariableLong(g, g->gAppName, "AutoConnect", g->autoconnect);
	PutConfigVariableLong(g, g->gAppName, "StartMinimized", g->gStartMinimized);
	PutConfigVariableLong(g, g->gAppName, "Limiter", g->gLimiter);
	PutConfigVariableLong(g, g->gAppName, "LimitDB", g->gLimitdb);
	PutConfigVariableLong(g, g->gAppName, "LimitGainDB", g->gGaindb);
	PutConfigVariableLong(g, g->gAppName, "LimitPRE", g->gLimitpre);
	PutConfigVariable(g, g->gAppName, "Encode", g->gEncodeType);

	PutConfigVariableLong(g, g->gAppName, "BitrateNominal", g->currentBitrate);
	PutConfigVariableLong(g, g->gAppName, "BitrateMin", g->currentBitrateMin);
	PutConfigVariableLong(g, g->gAppName, "BitrateMax", g->currentBitrateMax);
	PutConfigVariableLong(g, g->gAppName, "NumberChannels", g->currentChannels);
	PutConfigVariable(g, g->gAppName, "Attenuation", g->attenuation);
	PutConfigVariableLong(g, g->gAppName, "Samplerate", g->currentSamplerate);
	PutConfigVariable(g, g->gAppName, "OggQuality", g->gOggQuality);
	if(g->gOggBitQualFlag)
	{
		strcpy(g->gOggBitQual, "Bitrate");
	}
	else 
	{
		strcpy(g->gOggBitQual, "Quality");
	}

	PutConfigVariable(g, g->gAppName, "OggBitrateQualityFlag", g->gOggBitQual);
	PutConfigVariableLong(g, g->gAppName, "LameCBRFlag", g->gLAMEOptions.cbrflag);
	PutConfigVariableLong(g, g->gAppName, "LameQuality", g->gLAMEOptions.quality);
	PutConfigVariableLong(g, g->gAppName, "LameCopywrite", g->gLAMEOptions.copywrite);
	PutConfigVariableLong(g, g->gAppName, "LameOriginal", g->gLAMEOptions.original);
	PutConfigVariableLong(g, g->gAppName, "LameStrictISO", g->gLAMEOptions.strict_ISO);
	PutConfigVariableLong(g, g->gAppName, "LameDisableReservior", g->gLAMEOptions.disable_reservoir);
	PutConfigVariable(g, g->gAppName, "LameVBRMode", g->gLAMEOptions.VBR_mode);
	PutConfigVariableLong(g, g->gAppName, "LameLowpassfreq", g->gLAMEOptions.lowpassfreq);
	PutConfigVariableLong(g, g->gAppName, "LameHighpassfreq", g->gLAMEOptions.highpassfreq);
	PutConfigVariableLong(g, g->gAppName, "LAMEPreset", g->gLAMEpreset);
	PutConfigVariable(g, g->gAppName, "AACQuality", g->gAACQuality);
	PutConfigVariable(g, g->gAppName, "AACCutoff", g->gAACCutoff);

	PutConfigVariable(g, g->gAppName, "AdvRecDevice", g->gAdvRecDevice);
	PutConfigVariableLong(g, g->gAppName, "LiveInSamplerate", g->gLiveInSamplerate);
	PutConfigVariableLong(g, g->gAppName, "LineInFlag", g->gLiveRecordingFlag);

	PutConfigVariableLong(g, g->gAppName, "lastX", g->lastX);
	PutConfigVariableLong(g, g->gAppName, "lastY", g->lastY);
	PutConfigVariableLong(g, g->gAppName, "showVU", g->vuShow);

	PutConfigVariable(g, g->gAppName, "LockMetadata", g->gManualSongTitle);
	PutConfigVariableLong(g, g->gAppName, "LockMetadataFlag", g->gLockSongTitle);

	PutConfigVariable(g, g->gAppName, "SaveDirectory", g->gSaveDirectory);
	PutConfigVariableLong(g, g->gAppName, "SaveDirectoryFlag", g->gSaveDirectoryFlag);
	PutConfigVariableLong(g, g->gAppName, "SaveAsWAV", g->gSaveAsWAV);
	PutConfigVariable(g, g->gAppName, "LogFile", g->gLogFile);
	PutConfigVariableLong(g, g->gAppName, "LogLevel", g->gLogLevel);

	PutConfigVariableLong(g, g->gAppName, "AsioSelectChannel", g->gAsioSelectChannel);
	PutConfigVariable(g, g->gAppName, "AsioChannel", g->gAsioChannel);

	PutConfigVariableLong(g, g->gAppName, "EnableEncoderScheduler", g->gEnableEncoderScheduler);
#define PUTDOWVARS(dow, dows) \
	PutConfigVariableLong(g, g->gAppName, dows##"Enable", g->g##dow##Enable); \
	PutConfigVariableLong(g, g->gAppName, dows##"OnTime", g->g##dow##OnTime); \
	PutConfigVariableLong(g, g->gAppName, dows##"OffTime", g->g##dow##OffTime);

	PUTDOWVARS(Monday, "Monday");
	PUTDOWVARS(Tuesday, "Tuesday");
	PUTDOWVARS(Wednesday, "Wednesday");
	PUTDOWVARS(Thursday, "Thursday");
	PUTDOWVARS(Friday, "Friday");
	PUTDOWVARS(Saturday, "Saturday");
	PUTDOWVARS(Sunday, "Sunday");

	PutConfigVariableLong(g, g->gAppName, "NumEncoders", g->gNumEncoders);

	PutConfigVariable(g, g->gAppName, "ExternalMetadata", g->externalMetadata);
	PutConfigVariable(g, g->gAppName, "ExternalURL", g->externalURL);
	PutConfigVariable(g, g->gAppName, "ExternalFile", g->externalFile);
	PutConfigVariable(g, g->gAppName, "ExternalInterval", g->externalInterval);

	PutConfigVariable(g, g->gAppName, "OutputControl", g->outputControl);

	PutConfigVariable(g, g->gAppName, "MetadataAppend", g->metadataAppendString);
	PutConfigVariable(g, g->gAppName, "MetadataRemoveBefore", g->metadataRemoveStringBefore);
	PutConfigVariable(g, g->gAppName, "MetadataRemoveAfter", g->metadataRemoveStringAfter);
	PutConfigVariable(g, g->gAppName, "MetadataWindowClass", g->metadataWindowClass);
	PutConfigVariableLong(g, g->gAppName, "MetadataWindowClassInd", g->metadataWindowClassInd);

	PutConfigVariable(g, g->gAppName, "WindowsRecDevice", g->WindowsRecDevice);
	PutConfigVariable(g, g->gAppName, "WindowsRecSubDevice", g->WindowsRecSubDevice);
	PutConfigVariableLong(g, g->gAppName, "LAMEJointStereo", g->LAMEJointStereoFlag);
	PutConfigVariableLong(g, g->gAppName, "ForceDSPrecording", g->gForceDSPrecording);
	PutConfigVariableLong(g, g->gAppName, "ThreeHourBug", g->gThreeHourBug);
	PutConfigVariableLong(g, g->gAppName, "SkipCloseWarning", g->gSkipCloseWarning);
	PutConfigVariableLong(g, g->gAppName, "AsioRate", g->gAsioRate);

}

/*
 =======================================================================================================================
    Input is in interleaved float samples
    we could have 16 or more channels here
    each encoder can only handle 1 or 2 channels
	here we can copy the required one or two source channels
 =======================================================================================================================
*/
int handle_output(shuicastGlobals *g, float *samples, int nsamples, int nchannels, int in_samplerate, int asioChannel, int asioChannel2) 
{
	int			ret = 1;
	static int	current_insamplerate = 0;
	static int	current_nchannels = 0;
	long		out_samplerate = 0;
	long		out_nch = 0;
	int			samplecount = 0;
	float		*samplePtr = 0;
	int			in_nch = nchannels;
	int			sampleChannels = nchannels;
	int			leftChan = 0;
	int			rightChan = 1;
	float		*samples_resampled = NULL;
//	short		*samples_resampled_int = NULL;
	float		*samples_rechannel = NULL;
	float		*working_samples = NULL;

	if(asioChannel >= 0)
	{
		//nchannels = 1;
		//if(nchannels == 0) return 1;
		in_nch = nchannels;
		leftChan = rightChan = asioChannel;
		if(nchannels == 2)
			rightChan = asioChannel2;
	}

	nchannels = 2;
	if(g == NULL)
	{
		return 1;
	}

	if(g->weareconnected) 
	{
		LogMessage(g,LOG_DEBUG, "%d Calling handle output - attenuation = %g", g->encoderNumber, g->dAttenuation);
		if(g->dAttenuation != 1.0)
		{
			double atten = g->dAttenuation;
			int sizeofdata = nsamples * in_nch * sizeof(float);
			working_samples = (float *) malloc(sizeofdata);
			sizeofdata = nsamples * in_nch;
			while(sizeofdata--)
				working_samples[sizeofdata] = samples[sizeofdata] * (float) atten;
			samples = working_samples;
		}
		out_samplerate = getCurrentSamplerate(g);
		out_nch = getCurrentChannels(g);
		if (g->gSaveFile) 
		{
			if(g->gSaveAsWAV) 
			{
				if(sampleChannels < 3)
				{
					int			sizeofData = nsamples * in_nch * sizeof(short int);
					short int	*int_samples;

					int_samples = (short int *) malloc(sizeofData);

					for(int i = 0; i < nsamples * in_nch; i = i + 1)
					{
						int_samples[i] = (short int) (samples[i] * 32767.f);
					}

					fwrite(int_samples, sizeofData, 1, g->gSaveFile);
					g->written += sizeofData;
					free(int_samples);

					/*
					 * int sizeofData = nsamples*nchannels*sizeof(float);
					 * fwrite(samples, sizeofData, 1, g->gSaveFile);
					 * g->written += sizeofData;
					 * ;
					 * Write to WAV file
					 */
				}
				else
				{
					int			sizeofData = nsamples * nchannels * sizeof(short int);
					short int	*int_samples;

					int_samples = (short int *) malloc(sizeofData);
					int k = 0;
					for(int i = 0; i < nsamples; i += sampleChannels) 
					{
						int_samples[k++] = (short int) (samples[i+leftChan] * 32767.f);
						if(nchannels > 1)
							int_samples[k++] = (short int) (samples[i+rightChan] * 32767.f);
					}
					fwrite(int_samples, sizeofData, 1, g->gSaveFile);
					g->written += sizeofData;
					free(int_samples);
				}
			}
		}
		if(current_insamplerate != in_samplerate)
		{
			resetResampler(g);
			current_insamplerate = in_samplerate;
		}

		if(current_nchannels != nchannels) 
		{
			resetResampler(g);
			current_nchannels = nchannels;
		}

		samples_rechannel = (float *) malloc(sizeof(float) * nsamples * nchannels);
		memset(samples_rechannel, '\000', sizeof(float) * nsamples * nchannels);

		samplePtr = samples;

		int make_mono = 0;
		int make_stereo = 0;

		if((in_nch == 2) && (out_nch == 1)) 
		{
			make_mono = 1;
		}

		if((in_nch == 1) && (out_nch == 2))
		{
			make_stereo = 1;
		}

		if((in_nch == 1) && (out_nch == 1))
		{
			make_stereo = 1;
		}

		int samplecounter = 0;

		if(make_mono)
		{
			int k = 0;
			for(int i = 0; i < nsamples * sampleChannels; i += sampleChannels)
			{
				float v = (samples[i+leftChan] + samples[i+rightChan]) / 2;
				samples_rechannel[k++] = v;
				samples_rechannel[k++] = v;
			}
		}

		if(make_stereo) 
		{
			for(int i = 0; i < nsamples * sampleChannels; i += sampleChannels)
			{
				samples_rechannel[samplecounter++] = (samples[i+leftChan]);
				samples_rechannel[samplecounter++] = (samples[i+leftChan]);
			}
		}

		if(!(make_mono) && !(make_stereo))
		{
			int k = 0;
			for(int i = 0; i < nsamples * sampleChannels; i += sampleChannels)
			{
				samples_rechannel[k++] = (samples[i+leftChan]);
				if(in_nch == 2)
					samples_rechannel[k++] = (samples[i+rightChan]);
			}
		}

		LogMessage(g,LOG_DEBUG, "In samplerate = %d, Out = %d", in_samplerate, out_samplerate);
		samplePtr = samples_rechannel;
		if(in_samplerate != out_samplerate) 
		{
			nchannels = 2;

			/* Call the resampler */
			int buf_samples = ((nsamples * out_samplerate) / in_samplerate);

			LogMessage(g,LOG_DEBUG, "Initializing resampler");

			initializeResampler(g, in_samplerate, nchannels);

			samples_resampled = (float *) malloc(sizeof(float) * buf_samples * nchannels);
			memset(samples_resampled, '\000', sizeof(float) * buf_samples * nchannels);

			LogMessage(g,LOG_DEBUG, "calling ocConvertAudio");
			long	out_samples = ocConvertAudio(g,
												 (float *) samplePtr,
												 (float *) samples_resampled,
												 nsamples,
												 buf_samples);

//			samples_resampled_int = (short *) malloc(sizeof(short) * out_samples * nchannels);
//			memset(samples_resampled_int, '\000', sizeof(short) * out_samples * nchannels);

			LogMessage(g,LOG_DEBUG, "ready to do encoding");

			if(out_samples > 0) 
			{
				samplecount = 0;

				/* Here is the call to actually do the encoding->... */
				LogMessage(g,LOG_DEBUG, "do_encoding (resampled) start");
				// de-emphasis could go here
				ret = do_encoding(g, (float *) (samples_resampled), out_samples);
				LogMessage(g,LOG_DEBUG, "do_encoding end (%d)", ret);
			}

//			if(samples_resampled_int)
//			{
//				free(samples_resampled_int);
//				samples_resampled_int = NULL;
//			}

			if(samples_resampled) 
			{
				free(samples_resampled);
				samples_resampled = NULL;
			}
		}
		else 
		{
			LogMessage(g,LOG_DEBUG, "do_encoding start");
			ret = do_encoding(g, (float *) samples_rechannel, nsamples, NULL);
			LogMessage(g,LOG_DEBUG, "do_encoding end (%d)", ret);
		}

		if(samples_rechannel) 
		{
			free(samples_rechannel);
			samples_rechannel = NULL;
		}

		if(working_samples)
			free(working_samples);
		
		LogMessage(g,LOG_DEBUG, "%d Calling handle output - Ret = %d", g->encoderNumber, ret);
	}

	return ret;
}

int handle_output_fast(shuicastGlobals *g, Limiters *limiter, int dataoffset) 
{
	float *samples;
	int nsamples = limiter->outputSize;
	const int nchannels = 2;
	int in_samplerate = limiter->SampleRate;


	int			ret = 1;
	static int	current_insamplerate = 0;
	static int	current_nchannels = 0;
	long		out_samplerate = 0;
	long		out_nch = 0;
//	int			samplecount = 0;
	float		*samplePtr = 0;
	int			in_nch = nchannels;
	int			sampleChannels = nchannels;
	int			leftChan = 0;
	int			rightChan = 1;
	float		*samples_resampled = NULL;
//	float		*samples_rechannel = NULL;
	float		*working_samples = NULL;

	if(g == NULL)
	{
		return 1;
	}

	if(g->weareconnected)
	{
		LogMessage(g,LOG_DEBUG, "%d Calling handle output - attenuation = %g", g->encoderNumber, g->dAttenuation);
		out_nch = getCurrentChannels(g);
		if(out_nch == 1)
			samples = limiter->outputMono + dataoffset * limiter->outputSize * 2;
		else
			samples = limiter->outputStereo + dataoffset * limiter->outputSize * 2;

		if(g->dAttenuation != 1.0)
		{
			double atten = g->dAttenuation;
			int sizeofdata = nsamples * in_nch * sizeof(float);
			working_samples = (float *) malloc(sizeofdata);
			sizeofdata = nsamples * in_nch;
			while(sizeofdata--)
			{
				working_samples[sizeofdata] = samples[sizeofdata] * (float) atten;
			}
			samples = working_samples;
		}
		out_samplerate = getCurrentSamplerate(g);

		if (g->gSaveFile) 
		{
			if(g->gSaveAsWAV) 
			{
				if(sampleChannels < 3)
				{
					int			sizeofData = nsamples * in_nch * sizeof(short int);
					short int	*int_samples;

					int_samples = (short int *) malloc(sizeofData);

					for(int i = 0; i < nsamples * in_nch; i++)
					{
						int_samples[i] = (short int) (samples[i] * 32767.f);
					}

					fwrite(int_samples, sizeofData, 1, g->gSaveFile);
					g->written += sizeofData;
					free(int_samples);

					/*
					 * int sizeofData = nsamples*nchannels*sizeof(float);
					 * fwrite(samples, sizeofData, 1, g->gSaveFile);
					 * g->written += sizeofData;
					 * ;
					 * Write to WAV file
					 */
				}
				else
				{
					int			sizeofData = nsamples * sampleChannels * sizeof(short int);
					short int	*int_samples;

					int_samples = (short int *) malloc(sizeofData);
					int k = 0;
					for(int i = 0; i < nsamples; i += sampleChannels) 
					{
						int_samples[k++] = (short int) (samples[i+leftChan] * 32767.f);
						if(nchannels > 1)
							int_samples[k++] = (short int) (samples[i+rightChan] * 32767.f);
					}
					fwrite(int_samples, sizeofData, 1, g->gSaveFile);
					g->written += sizeofData;
					free(int_samples);
				}
			}
		}

		if(current_insamplerate != in_samplerate)
		{
			resetResampler(g);
			current_insamplerate = in_samplerate;
		}

		if(current_nchannels != nchannels) 
		{
			resetResampler(g);
			current_nchannels = nchannels;
		}

		LogMessage(g,LOG_DEBUG, "In samplerate = %d, Out = %d", in_samplerate, out_samplerate);

		if(in_samplerate != out_samplerate) 
		{
			int buf_samples = ((nsamples * out_samplerate) / in_samplerate);
#if 0
			float * samples_rechannel = (float *) malloc(sizeof(float) * nsamples * nchannels);
			CopyMemory(samples_rechannel, samples, sizeof(float) * nsamples * nchannels);
			samplePtr = samples_rechannel;
#else
			samplePtr = samples;
#endif
			LogMessage(g,LOG_DEBUG, "Initializing resampler");

			initializeResampler(g, in_samplerate, nchannels);

			samples_resampled = (float *) malloc(sizeof(float) * buf_samples * nchannels);

			LogMessage(g,LOG_DEBUG, "calling ocConvertAudio");
			long	out_samples = ocConvertAudio(g,
												 (float *) samplePtr,
												 (float *) samples_resampled,
												 nsamples,
												 buf_samples);


			LogMessage(g,LOG_DEBUG, "ready to do encoding");

			if(out_samples > 0) 
			{
				/* Here is the call to actually do the encoding->... */
				LogMessage(g,LOG_DEBUG, "do_encoding (resampled) start");
				ret = do_encoding(g, (float *) (samples_resampled), out_samples, limiter);
				LogMessage(g,LOG_DEBUG, "do_encoding end (%d)", ret);
			}

			if(samples_resampled) 
			{
				free(samples_resampled);
				samples_resampled = NULL;
			}
#if 0
			if(samples_rechannel) 
			{
				free(samples_rechannel);
				samples_rechannel = NULL;
			}
#endif
		}
		else 
		{
			LogMessage(g,LOG_DEBUG, "do_encoding start");
			ret = do_encoding(g, samples, nsamples, limiter);
			LogMessage(g,LOG_DEBUG, "do_encoding end (%d)", ret);
		}

		if(working_samples)
			free(working_samples);
		
		LogMessage(g,LOG_DEBUG, "%d Calling handle output - Ret = %d", g->encoderNumber, ret);
	}

	return ret;
}

#ifdef WIN32
void freeupGlobals(shuicastGlobals *g)
{
	if(g->hDLL) 
	{
		FreeLibrary(g->hDLL);
	}

	if(g->faacFIFO)
	{
		free(g->faacFIFO);
	}
}
#endif

void addUISettings(shuicastGlobals *g)
{
	addConfigVariable(g, "AutomaticReconnect");
	addConfigVariable(g, "AutomaticReconnectSecs");
	addConfigVariable(g, "AutoConnect");
	addConfigVariable(g, "Limiter");
	addConfigVariable(g, "LimitDB");
	addConfigVariable(g, "LimitGainDB");
	addConfigVariable(g, "LimitPRE");
	addConfigVariable(g, "AdvRecDevice");
	addConfigVariable(g, "LiveInSamplerate");
	addConfigVariable(g, "LineInFlag");
	addConfigVariable(g, "lastX");
	addConfigVariable(g, "lastY");
	addConfigVariable(g, "showVU");
	addConfigVariable(g, "LockMetadata");
	addConfigVariable(g, "LockMetadataFlag");
//	addConfigVariable(g, "SaveDirectory");
//	addConfigVariable(g, "SaveDirectoryFlag");
//	addConfigVariable(g, "SaveAsWAV");
	addConfigVariable(g, "LogLevel");
	addConfigVariable(g, "LogFile");
	addConfigVariable(g, "NumEncoders");
	addConfigVariable(g, "ExternalMetadata");
	addConfigVariable(g, "ExternalURL");
	addConfigVariable(g, "ExternalFile");
	addConfigVariable(g, "ExternalInterval");
	addConfigVariable(g, "OutputControl");
	addConfigVariable(g, "MetadataAppend");
	addConfigVariable(g, "MetadataRemoveBefore");
	addConfigVariable(g, "MetadataRemoveAfter");
	addConfigVariable(g, "MetadataWindowClass");
	addConfigVariable(g, "MetadataWindowClassInd");
	addConfigVariable(g, "WindowsRecDevice");
	addConfigVariable(g, "StartMinimized");
}
void addASIOUISettings(shuicastGlobals *g)
{
	addConfigVariable(g, "WindowsRecSubDevice");
}
void addASIOExtraSettings(shuicastGlobals *g)
{
	addConfigVariable(g, "AsioRate");
}
void addOtherUISettings(shuicastGlobals *g)
{
	addConfigVariable(g, "WindowsRecSubDevice");
}

void addBasicEncoderSettings(shuicastGlobals *g)
{
// server
//
    addConfigVariable(g, "ServerType");
    addConfigVariable(g, "Server");
    addConfigVariable(g, "Port");
    addConfigVariable(g, "ServerMountpoint");
    addConfigVariable(g, "ServerPassword");
    addConfigVariable(g, "ServerPublic");
    addConfigVariable(g, "ServerIRC");
    addConfigVariable(g, "ServerAIM");
    addConfigVariable(g, "ServerICQ");
    addConfigVariable(g, "ServerStreamURL");
    addConfigVariable(g, "ServerDescription");
    addConfigVariable(g, "Attenuation");
    addConfigVariable(g, "ServerName");
    addConfigVariable(g, "ServerGenre");
//    addConfigVariable(g, "AutomaticReconnect");
//    addConfigVariable(g, "AutomaticReconnectSecs");
//    addConfigVariable(g, "AutoConnect");
//
// encoder
    addConfigVariable(g, "Encode");
    addConfigVariable(g, "BitrateNominal");
    addConfigVariable(g, "BitrateMin");
    addConfigVariable(g, "BitrateMax");
    addConfigVariable(g, "NumberChannels");
    addConfigVariable(g, "Samplerate");
    addConfigVariable(g, "OggQuality");
    addConfigVariable(g, "OggBitrateQualityFlag");
    addConfigVariable(g, "LameCBRFlag");
    addConfigVariable(g, "LameQuality");
    addConfigVariable(g, "LameCopywrite");
    addConfigVariable(g, "LameOriginal");
    addConfigVariable(g, "LameStrictISO");
    addConfigVariable(g, "LameDisableReservior");
    addConfigVariable(g, "LameVBRMode");
    addConfigVariable(g, "LameLowpassfreq");
    addConfigVariable(g, "LameHighpassfreq");
    addConfigVariable(g, "LAMEPreset");
    addConfigVariable(g, "AACQuality");
    addConfigVariable(g, "AACCutoff");
	addConfigVariable(g, "LogLevel");
	addConfigVariable(g, "LogFile");
	addConfigVariable(g, "LAMEJointStereo");
	addConfigVariable(g, "SaveDirectory");
	addConfigVariable(g, "SaveDirectoryFlag");
	addConfigVariable(g, "SaveAsWAV");
}

void addMultiStereoEncoderSettings(shuicastGlobals *g)
{
	addConfigVariable(g, "AsioChannel2");
}

void addDSPONLYsettings(shuicastGlobals *g)
{
	addConfigVariable(g, "ForceDSPrecording");
}

void addStandaloneONLYsettings(shuicastGlobals *g)
{
	addConfigVariable(g, "SkipCloseWarning");
}

void addBASSONLYsettings(shuicastGlobals *g)
{
	addConfigVariable(g, "ThreeHourBug");
}

void addMultiEncoderSettings(shuicastGlobals *g)
{
	addConfigVariable(g, "AsioSelectChannel");
	addConfigVariable(g, "AsioChannel");
	addConfigVariable(g, "EnableEncoderScheduler");

#define ADDDOWVARS(dows) \
	addConfigVariable(g, dows##"Enable"); \
	addConfigVariable(g, dows##"OnTime"); \
	addConfigVariable(g, dows##"OffTime");

	ADDDOWVARS("Monday");
	ADDDOWVARS("Tuesday");
	ADDDOWVARS("Wednesday");
	ADDDOWVARS("Thursday");
	ADDDOWVARS("Friday");
	ADDDOWVARS("Saturday");
	ADDDOWVARS("Sunday");
}

void LogMessage(shuicastGlobals *g, int type, char *source, int line, char *fmt, ...)
{
	va_list parms;
	char	errortype[25] = "";
	int	addNewline = 1;
	struct tm *tp;
	time_t t;
	int parseableOutput = 0;
	char    timeStamp[255];

	char	sourceLine[1024] = "";

	char *p1 = NULL;

#ifdef WIN32
	p1 = strrchr(source, '\\');
#else
	p1 = strrchr(source, '/');
#endif

	if (p1) 
	{
		strcpy(sourceLine, p1+1);
	}
	else 
	{
		strcpy(sourceLine, source);
	}
	memset(timeStamp, '\000', sizeof(timeStamp));

	time(&t);
	tp = localtime(&t);
	strftime(timeStamp, sizeof(timeStamp), "%m/%d/%y %H:%M:%S", tp);

	
	switch (type)
	{
	case LM_ERROR:
		strcpy(errortype, "Error");
		break;
	case LM_INFO:
		strcpy(errortype, "Info");
		break;
	case LM_DEBUG:
		strcpy(errortype, "Debug");
		break;
	default:
		strcpy(errortype, "Unknown");
		break;
	}

	if (fmt[strlen(fmt)-1] == '\n')
	{
		addNewline = 0;
	}

	if (type <= g->gLogLevel)
	{
		va_start(parms, fmt);
		char_t	logfile[1024] = "";

		if (g->logFilep == 0)
		{
			wsprintf(logfile, "%s.log", g->gLogFile);
			g->logFilep = fopen(logfile, "a");
		}
		
		if (!g->logFilep)
		{
			fprintf(stdout, "Cannot open logfile: %s(%s:%d): ", logfile, sourceLine, line);
			vfprintf(stdout, fmt, parms);
			va_end(parms);
			if (addNewline)
			{
				fprintf(stdout, "\n");
			}
		}
		else
		{
			fprintf(g->logFilep,  "%s %s(%s:%d): ", timeStamp, errortype, sourceLine, line);
			vfprintf(g->logFilep, fmt, parms);
			va_end(parms);
			if (addNewline)
			{
				fprintf(g->logFilep, "\n");
			}
			fflush(g->logFilep);
		}
	}
}

char_t *getWindowsRecordingDevice(shuicastGlobals *g)
{
	return g->WindowsRecDevice;
}
void	setWindowsRecordingDevice(shuicastGlobals *g, char_t *device)
{
	strcpy(g->WindowsRecDevice, device);
}
char_t *getWindowsRecordingSubDevice(shuicastGlobals *g) 
{
	return g->WindowsRecSubDevice;
}
void	setWindowsRecordingSubDevice(shuicastGlobals *g, char_t *device)
{
	strcpy(g->WindowsRecSubDevice, device);
}
int getLAMEJointStereoFlag(shuicastGlobals *g)
{
	return g->LAMEJointStereoFlag;
}
void	setLAMEJointStereoFlag(shuicastGlobals *g, int flag)
{
	g->LAMEJointStereoFlag = flag;
}
/*
	return - 
		0 : exists, config exists
		1 : exists, no config
		2 : locn exists, subfolder does not
		3 : locn does not exist
		4 : other error

*/
int getAppdata(bool checkonly, int locn, DWORD flags, LPCSTR subdir, LPCSTR configname, LPSTR strdestn)
{
	char destn[MAX_PATH] = "";
	int retVal = 1;

	HRESULT res = SHGetFolderPathAndSubDir(NULL, locn, NULL, flags, subdir, destn);
	if(!SUCCEEDED(res)) 
	{
		retVal = 4;
		if (HRESULT_CODE(res) == ERROR_PATH_NOT_FOUND) 
		{
			char tmpdir[MAX_PATH];
			res = SHGetFolderPath(NULL, locn, NULL, flags, tmpdir);
			if(SUCCEEDED(res))
			{
				retVal = 2;
				if(!checkonly) 
				{
					wsprintf(destn, "%s\\%s", tmpdir, subdir);
					res = SHCreateDirectoryEx(NULL, destn, NULL);
					if(SUCCEEDED(res)) 
					{
						res = SHGetFolderPathAndSubDir(NULL, locn, NULL, flags, subdir, destn);
						retVal = 1;
						if(!SUCCEEDED(res)) 
						{
							retVal = 4;
							wsprintf(destn, "Error 0x%08X getting created LOCAL_APPDATA\\%s", res, subdir);
						}
					}
					else 
					{
						retVal = 4;
						wsprintf(destn, "Error 0x%08X creating LOCAL_APPDATA\\%s", res, subdir);
					}
				}
			}
			else 
			{
				retVal = 3;
				wsprintf(destn, "Error 0x%08X getting LOCAL_APPDATA", res);
			}
		}
		else
		{
			retVal = 4;
			wsprintf(destn, "Error 0x%08X getting LOCAL_APPDATA\\%s", res, subdir);
		}
	}
	if(retVal == 1)
	{
		strcpy(strdestn, destn);
		if(testLocal(strdestn, configname))
		{
			retVal = 0;
		}
		// check if configname exists, set retVal = 0 if true
	}
	return retVal;
}

bool testLocal(LPCSTR dir, LPCSTR file)
{
	char tmpfile[MAX_PATH] = "";
	FILE *filep;
	if(file == NULL)
	{
		wsprintf(tmpfile, "%s\\.tmp", dir);
		filep = fopen(tmpfile, "w");
	}
	else
	{
		wsprintf(tmpfile, "%s\\%s", dir, file);
		filep = fopen(tmpfile, "r");
	}
	if (filep == NULL) 
	{
		return false;
	}
	else
	{
		fclose(filep);
		if(file == NULL)
		{
			_unlink(tmpfile);
		}
	}
	return true;
}

bool _getDirName(LPCSTR inDir, LPSTR dst, int lvl=1) // base
{
	// inDir = ...\winamp\Plugins
	char * dir = _strdup(inDir);
	bool retval = false;
	// remove trailing slash

	if(dir[strlen(dir)-1] == '\\')
	{
		dir[strlen(dir)-1] = '\0';
	}

	char *p1;

	for(p1 = dir + strlen(dir) - 1; p1 >= dir; p1--)
	{
		if(*p1 == '\\')
		{
			if(--lvl > 0)
			{
				*p1 = '\0';
			}
			else
			{
				strcpy(dst, p1);
				retval = true;
				break;
			}
		}
	}
	free(dir);
	return retval;
}

void LoadConfigs(char *currentDir, char *subdir, char * logPrefix, char *currentConfigDir, bool inWinamp) // different for DSP
{
	char	configFile[1024] = "";
	char	currentlogFile[1024] = "";

	char tmpfile[MAX_PATH] = "";
	char tmp2file[MAX_PATH] = "";

	char cfgfile[MAX_PATH];
	wsprintf(cfgfile, "%s_0.cfg", logPrefix);

	bool canUseCurrent = testLocal(currentDir, NULL);
	bool hasCurrentData = false;
	bool hasAppData = false;
	bool canUseAppData = false;
	bool hasProgramData = false;
	bool canUseProgramData = false;
	if(canUseCurrent)
	{
		hasCurrentData = testLocal(currentDir, cfgfile);
	}
	if(!hasCurrentData)
	{
		int iHasAppData = -1;
		if(inWinamp)
		{
			int iHasWinampDir = -1;
			int iHasPluginDir = -1;
			int iHasEdcastDir = -1;
			int iHasEdcastCfg = -1;
			char wasubdir[MAX_PATH] = "";
			char wa_instance[MAX_PATH] = "";
		
			_getDirName(currentDir, wa_instance, 1); //...../{winamp name}/ - we want {winamp name}
			if(!strcmp(wa_instance, "plugins"))
			{
				_getDirName(currentDir, wa_instance, 2); //...../{winamp name}/plugins - we want {winamp name}
			}

			wsprintf(wasubdir, "%s", wa_instance);
			iHasWinampDir = getAppdata(false, CSIDL_APPDATA, SHGFP_TYPE_CURRENT, wasubdir, cfgfile, tmpfile);
			if(iHasWinampDir < 2)
			{
				wsprintf(wasubdir, "%s\\Plugins", wa_instance);
				iHasPluginDir = getAppdata(false, CSIDL_APPDATA, SHGFP_TYPE_CURRENT, wasubdir, cfgfile, tmpfile);
				if(iHasPluginDir < 2)
				{
					wsprintf(wasubdir, "%s\\Plugins\\%s", wa_instance, subdir);
					iHasAppData = getAppdata(true, CSIDL_APPDATA, SHGFP_TYPE_CURRENT, wasubdir, cfgfile, tmpfile);
				}
			}
		}
		else
		{
			/*
			int iHasInstanceDir = -1;
			int iHasPluginDir = -1;
			int iHasEdcastDir = -1;
			int iHasEdcastCfg = -1;
			char instance_subdir[MAX_PATH] = "";
			char _instance[MAX_PATH] = "";
		
			_getDirName(currentDir, _instance, 1); //...../{winamp name}/ - we want {winamp name}
			if(!strcmp(_instance, "plugins"))
			{
				_getDirName(currentDir, _instance, 2); //...../{winamp name}/plugins - we want {winamp name}
			}

			wsprintf(instance_subdir, "%s", _instance);
			iHasInstanceDir = getAppdata(false, CSIDL_LOCAL_APPDATA, SHGFP_TYPE_CURRENT, instance_subdir, cfgfile, tmpfile);
			if(iHasInstanceDir < 2)
			{
				wsprintf(instance_subdir, "%s\\Plugins", _instance);
				iHasPluginDir = getAppdata(false, CSIDL_LOCAL_APPDATA, SHGFP_TYPE_CURRENT, instance_subdir, cfgfile, tmpfile);
				if(iHasPluginDir < 2)
				{
					wsprintf(instance_subdir, "%s\\Plugins\\%s", _instance, subdir);
					iHasAppData = getAppdata(true, CSIDL_LOCAL_APPDATA, SHGFP_TYPE_CURRENT, instance_subdir, cfgfile, tmpfile);
				}
			}
			*/
			iHasAppData = getAppdata(true, CSIDL_LOCAL_APPDATA, SHGFP_TYPE_CURRENT, subdir, cfgfile, tmpfile);
		}

		hasAppData = (iHasAppData == 0);
		canUseAppData = (iHasAppData < 2);
		if(!hasAppData)
		{
			int iHasProgramData = getAppdata(true, CSIDL_COMMON_APPDATA, SHGFP_TYPE_CURRENT, subdir, cfgfile, tmp2file);
			hasProgramData = (iHasProgramData == 0);
			canUseProgramData = (iHasProgramData < 2);
		}
	}
	if(hasAppData && hasCurrentData)
	{
		// tmpfile already has the right value
	}
	else if (hasAppData)
	{
		// tmpfile already has the right value
	}
	else if (hasCurrentData)
	{
		strcpy(tmpfile, currentDir);
	}
	else if(canUseAppData)
	{
		// tmpfile alread has the right value
	}
	else if(canUseCurrent)
	{
		strcpy(tmpfile, currentDir);
	}
	else if(canUseProgramData)
	{
		strcpy(tmpfile, tmp2file);
	}
	else
	{
		// fail!!
	}
	strcpy(currentConfigDir, tmpfile);
}

static char * AsciiToUtf8(char * ascii) // caller MUST free returned buffer when done
{
	int needlen = MultiByteToWideChar(CP_ACP, 0, ascii, strlen(ascii) + 1, NULL, 0);
	wchar_t * temp = (wchar_t *) malloc(needlen * sizeof(wchar_t));
	MultiByteToWideChar(CP_ACP, 0, ascii, strlen(ascii) + 1, temp, needlen);

	needlen = WideCharToMultiByte(CP_UTF8, 0, temp, wcslen(temp) + 1, NULL, 0, NULL, NULL);
	char *utf = (char *) malloc(needlen * sizeof(char));
	WideCharToMultiByte(CP_UTF8, 0, temp, wcslen(temp) + 1, utf, needlen, NULL, NULL);

	free(temp);
	return utf;
}

static char * UnicodeToUtf8(wchar_t * unicode) // caller MUST free returned buffer when done
{

	int needlen = WideCharToMultiByte(CP_UTF8, 0, unicode, wcslen(unicode) + 1, NULL, 0, NULL, NULL);
	char *utf = (char *) malloc(needlen * sizeof(char));
	WideCharToMultiByte(CP_UTF8, 0, unicode, wcslen(unicode) + 1, utf, needlen, NULL, NULL);

	return utf;
}
