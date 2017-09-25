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

//extern CEncoder			*g[MAX_ENCODERS];
//extern CEncoder			gMain;

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

void CEncoder::AddConfigVariable(char_t *variable) 
{
	configVariables[numConfigVariables] = _strdup(variable);
	numConfigVariables++;
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

void setSourceDescription(shuicastGlobals *g, char_t *desc)
{
	strcpy(g->sourceDescription, desc);
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
    g->ReplaceString( outFilename, outputFile, "\"", "'" );
	memset(outFilename, '\000', sizeof(outFilename));
    g->ReplaceString( outputFile, outFilename, FILE_SEPARATOR, "" );
	memset(outputFile, '\000', sizeof(outputFile));
    g->ReplaceString( outFilename, outputFile, "/", "" );
	memset(outFilename, '\000', sizeof(outFilename));
    g->ReplaceString( outputFile, outFilename, ":", "" );
	memset(outputFile, '\000', sizeof(outputFile));
    g->ReplaceString( outFilename, outputFile, "*", "" );
	memset(outFilename, '\000', sizeof(outFilename));
    g->ReplaceString( outputFile, outFilename, "?", "" );
	memset(outputFile, '\000', sizeof(outputFile));
    g->ReplaceString( outFilename, outputFile, "<", "" );
	memset(outFilename, '\000', sizeof(outFilename));
    g->ReplaceString( outputFile, outFilename, ">", "" );
	memset(outputFile, '\000', sizeof(outputFile));
    g->ReplaceString( outFilename, outputFile, "|", "" );
	memset(outFilename, '\000', sizeof(outFilename));
    g->ReplaceString( outputFile, outFilename, "\n", "" );

	memset(outputFile, '\000', sizeof(outputFile));
	strcpy(outputFile, outFilename);

    if ( g->gSaveAsWAV ) strcat( outputFile, ".wav" );
    else if ( g->m_Type == ENCODER_OGG  ) strcat( outputFile, ".ogg" );
    else if ( g->m_Type == ENCODER_LAME ) strcat( outputFile, ".mp3" );
    else if ( g->gAACFlag || g->gAACPFlag || g->gFHAACPFlag ) strcat( outputFile, ".aac" );
	wsprintf(outFilename, "%s%s%s", g->gSaveDirectory, FILE_SEPARATOR, outputFile);

	g->gSaveFile = fopen(outFilename, "wb");
	if(!g->gSaveFile) 
	{
		char_t	buff[1024] = "";
		wsprintf(buff, "Cannot open %s", outputFile);
		g->LogMessage( LOG_ERROR, buff );
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
            if ( g->m_IsConnected )
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
		 * g->LogMessage(LOG_ERROR, "Cannot open config file %s\n", configFile);
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

    g->LoadConfig();

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
	g->StoreConfig();

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
		g->LogMessage(LOG_ERROR, "Cannot open config file %s\n", configFile);
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
		g->LogMessage(LOG_DEBUG, "(%s) = (%s)\n", configFileValues[i].Variable, configFileValues[i].Value);
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

#if 0
int trimVariable(char_t *variable) 
{
	char_t	*p1;
	/* Trim off the back */
	for(p1 = variable + strlen(variable) - 1; p1 > variable; p1--) 
	{
		if((*p1 == ' ') || (*p1 == '\t')) *p1 = '\000';
		else break;
	}

	/* Trim off the front */
	char_t	tempVariable[1024] = "";
	memset(tempVariable, '\000', sizeof(tempVariable));
	for(p1 = variable; p1 < variable + strlen(variable) - 1; p1++) 
	{
		if((*p1 == ' ') || (*p1 == '\t')) ;
		else break;
	}

	strcpy(tempVariable, p1);
	strcpy(variable, tempVariable);
	return 1;
}
#endif

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

CEncoder::CEncoder ( int encoderNumber ) :
    encoderNumber(encoderNumber), gReconnectSec(10), gLogLevel(LM_ERROR), LAMEJointStereoFlag(1), oggflag(1)
{
	//_attenTable[0] = 1.0;
	//for(int i = 1; i < 11; i++)
	//{
	//	_attenTable[i] = pow((double)10.0,(double) (-i) / (double) 20.0);
	//}
	pthread_mutex_init(&mutex, NULL);
}

int CEncoder::SetCurrentSongTitle ( char_t *song )
{
    char_t *pCurrent = gLockSongTitle ? gManualSongTitle : song;
    if ( strcmp( gSongTitle, pCurrent ) )
    {
        strcpy( gSongTitle, pCurrent );
        UpdateSongTitle( 0 );
        return 1;
    }
    return 0;
}

void CEncoder::GetCurrentSongTitle ( char_t *song, char_t *artist, char_t *full ) const
{
	char_t	songTitle[1024] = "";
    strcpy( songTitle, gLockSongTitle ? gManualSongTitle : gSongTitle );
	strcpy( full, songTitle );
	char_t	*p1 = strchr(songTitle, '-');
	if(p1) 
	{
		if(*(p1 - 1) == ' ') p1--;
		strncpy(artist, songTitle, p1 - songTitle);
		p1 = strchr(songTitle, '-');
		p1++;
		if(*p1 == ' ') p1++;
		strcpy(song, p1);
	}
	else
	{
		strcpy(artist, "");
		strcpy(song, songTitle);
	}
}

void CEncoder::ReplaceString ( char_t *source, char_t *dest, char_t *from, char_t *to ) const
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
char_t* CEncoder::URLize ( char_t *input ) const
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

int CEncoder::UpdateSongTitle ( int forceURL )
{
	char_t contentString[16384] = "";

	if ( IsConnected() ) 
	{
        if ( (m_Type != ENCODER_OGG) || forceURL )
		{
			if((gSCFlag) || (gIcecastFlag) || (gIcecast2Flag) || forceURL) 
			{
				char_t	* URLSong = URLize(gSongTitle);
				char_t	* URLPassword = URLize(gPassword);
				strcpy(gCurrentSong, gSongTitle);

				if(gIcecast2Flag) 
				{
					char_t	userAuth[4096] = "";
					sprintf(userAuth, "source:%s", gPassword);
					char_t	*puserAuthbase64 = util_base64_encode(userAuth);
					if(puserAuthbase64) 
					{
						sprintf( contentString, "GET /admin/metadata?pass=%s&mode=updinfo&mount=%s&song=%s HTTP/1.0\r\nAuthorization: Basic %s\r\n%s",
							URLPassword, gMountpoint, URLSong, puserAuthbase64, reqHeaders );
						free(puserAuthbase64);
					}
				}

				if(gIcecastFlag) 
				{
					sprintf( contentString, "GET /admin.cgi?pass=%s&mode=updinfo&mount=%s&song=%s HTTP/1.0\r\n%s",
						URLPassword, gMountpoint, URLSong, reqHeaders );
				}

				if(gSCFlag) 
				{
					if(strchr(gPassword, ':') == NULL) // use Basic Auth for non sc_trans 2 connections
					{
						char_t	userAuth[1024] = "";
						sprintf(userAuth, "admin:%s", gPassword);
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

                m_SCSocketCtrl = m_CtrlChannel.DoSocketConnect( gServer, atoi( gPort ) );
                if ( m_SCSocketCtrl != -1 )
				{
                    int sent = send( m_SCSocketCtrl, contentString, strlen( contentString ), 0 );
					//int sent = sendToServer(this, m_SCSocketCtrl, contentString, strlen(contentString), HEADER_TYPE);
                    closesocket( m_SCSocketCtrl );
				}
				else 
				{
					LogMessage( LOG_ERROR, "Cannot connect to server" );
				}
			}
		}
		else 
		{
			ice2songChange = true;
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
    g->Load();
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
        int sentbytes = sendToServer( g, g->m_SCSocket, (char_t *)buffer, bytes, CODEC_TYPE );
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
int CEncoder::DisconnectFromServer()
{
    m_IsConnected = 0;
	if(serverStatusCallback)
	{
		serverStatusCallback(this, (char_t *) "Disconnecting");
	}
	int retry = 10;
	while(gCurrentlyEncoding && retry--)
	{
#ifdef _WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	}
	if(retry == 0 && serverStatusCallback) 
	{
		serverStatusCallback(this, (char_t *) "Disconnecting - encoder did not stop");
	}
	/* Close all open sockets */
    closesocket( m_SCSocket );
    closesocket( m_SCSocketCtrl );

	/*
	 * Reset the Status to Disconnected, and reenable the config ;
	 * button
	 */
    m_SCSocket = 0;
    m_SCSocketCtrl = 0;

#ifdef HAVE_VORBIS
    if ( m_Type == ENCODER_OGG )
	{
		ogg_stream_clear(&os);
		vorbis_block_clear(&vb);
		vorbis_dsp_clear(&vd);
        vorbis_info_clear( &m_VorbisInfo );
        memset( &m_VorbisInfo, '\000', sizeof( m_VorbisInfo ) );
	}
#endif
#ifdef HAVE_LAME
#ifndef _WIN32
	if(gf)
	{
		lame_close(gf);
		gf = NULL;
	}
#endif
#endif
	if(serverStatusCallback) 
	{
		serverStatusCallback(this, (void *) "Disconnected");
	}

	closeArchiveFile(this);

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
int CEncoder::ConnectToServer()
{
	int		s_socket = 0;
	char_t	buffer[1024] = "";
	char_t	contentString[1024] = "";
	char_t	brate[25] = "";
	char_t	ypbrate[25] = "";

	LogMessage( LOG_DEBUG, "Connecting encoder %d", encoderNumber );

    sprintf( brate, "%d", m_CurrentBitrate );

    if ( m_Type == ENCODER_OGG )
	{
		if(!gOggBitQualFlag)
		{
			sprintf(ypbrate, "Quality %s", gOggQuality);
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

	gSCFlag = 0;
	greconnectFlag = 0;

	if(serverStatusCallback) 
	{
		serverStatusCallback(this, (void *) "Connecting");
	}

#ifdef WIN32
    m_DataChannel.initWinsockLib();
#endif

	/* If we are Icecast/Icecast2, then connect to specified port */
	if(gIcecastFlag || gIcecast2Flag)
	{
        m_SCSocket = m_DataChannel.DoSocketConnect( gServer, atoi( gPort ) );
	}
	else
	{
		/* If we are Shoutcast, then the control socket (used for password) is port+1. */
        m_SCSocket = m_DataChannel.DoSocketConnect( gServer, atoi( gPort ) + 1 );
	}

	/* Check to see if we connected okay */
    if ( m_SCSocket == -1 )
	{
		if(serverStatusCallback)
		{
			serverStatusCallback(this, (void *) "Unable to connect to socket");
		}

		return 0;
	}

	int pswdok = 1;

	/* Yup, we did. */
	if(serverStatusCallback)
	{
		serverStatusCallback(this, (void *) "Socket connected");
	}

	char_t	contentType[255] = "";

    if ( m_Type == ENCODER_OGG )
	{
		strcpy(contentType, "application/ogg");
	} 
	else if(gAACFlag)
	{
		strcpy(contentType, "audio/aac");
	}
	else if(gAACPFlag)
	{
		switch(gAACPFlag) 
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
	else if(gFHAACPFlag)
	{
		strcpy(contentType, "audio/aacp");
	}
    else if ( m_Type == ENCODER_FLAC )
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
	if(gIcecastFlag || gIcecast2Flag) 
	{

		/* The Icecast/Icecast2 Way */
		if(gIcecastFlag) 
		{
			sprintf(contentString,
					"SOURCE %s %s\r\ncontent-type: %s\r\nx-audiocast-name: %s\r\nx-audiocast-url: %s\r\nx-audiocast-genre: %s\r\nx-audiocast-bitrate: %s\r\nx-audiocast-public: %d\r\nx-audiocast-description: %s\r\n\r\n",
				gPassword, gMountpoint, contentType, gServDesc, gServURL, gServGenre, brate, gPubServ, gServDesc);
		}

		if(gIcecast2Flag)
		{
			char_t	audioInfo[1024] = "";
			sprintf(audioInfo, "ice-samplerate=%d;ice-bitrate=%s;ice-channels=%d", GetCurrentSamplerate(), ypbrate, GetCurrentChannels());
			char_t	userAuth[1024] = "";
			sprintf(userAuth, "source:%s", gPassword);
			char_t	*puserAuthbase64 = util_base64_encode(userAuth);
			if(puserAuthbase64)
			{
				sprintf(contentString,
						"SOURCE %s ICE/1.0\ncontent-type: %s\nAuthorization: Basic %s\nice-name: %s\nice-url: %s\nice-genre: %s\nice-bitrate: %s\nice-private: %d\nice-public: %d\nice-description: %s\nice-audio-info: %s\n\n",
					gMountpoint, contentType, puserAuthbase64, gServName, gServURL, gServGenre, ypbrate, !gPubServ, gPubServ, gServDesc, audioInfo);
				free(puserAuthbase64);
			}
		}
	}
	else
	{

		/* The Shoutcast way */
        sendToServer( this, m_SCSocket, gPassword, strlen( gPassword ), HEADER_TYPE );
        sendToServer( this, m_SCSocket, "\r\n", strlen( "\r\n" ), HEADER_TYPE );

        recv( m_SCSocket, buffer, sizeof( buffer ), (int)0 );

		// if we get an OK, then we are not a Shoutcast server (could be live365 or other variant)..And OK2 means it's
		// Shoutcast and we can safely send in metadata via the admin.cgi interface.
		if(!strncmp(buffer, "OK", strlen("OK"))) 
		{
			if(!strncmp(buffer, "OK2", strlen("OK2")))
			{
				gSCFlag = 1;
			}
			else
			{
				gSCFlag = 0;
			}

			if(serverStatusCallback)
			{
				serverStatusCallback(this, (void *) "Password OK");
			}
		}
		else
		{
			if(serverStatusCallback)
			{
				serverStatusCallback(this, (void *) "Password Failed");
			}

            closesocket( m_SCSocket );
			return 0;
		}

		memset(contentString, '\000', sizeof(contentString));
		if(strlen(gServICQ) == 0) 
		{
			strcpy(gServICQ, "N/A");
		}

		if(strlen(gServAIM) == 0) 
		{
			strcpy(gServAIM, "N/A");
		}

		if(strlen(gServIRC) == 0) 
		{
			strcpy(gServIRC, "N/A");
		}

		sprintf(contentString,
				"content-type:%s\r\nicy-name:%s\r\nicy-genre:%s\r\nicy-url:%s\r\nicy-pub:%d\r\nicy-irc:%s\r\nicy-icq:%s\r\nicy-aim:%s\r\nicy-br:%s\r\n\r\n",
			contentType, gServName, gServGenre, gServURL, gPubServ, gServIRC, gServICQ, gServAIM, brate);
	}

    sendToServer( this, m_SCSocket, contentString, strlen( contentString ), HEADER_TYPE );

	if(gIcecastFlag)
	{
		/*
		 * Here we are checking the response from Icecast/Icecast2 ;
		 * from when we sent in the password...OK means we are good..if the ;
		 * password is bad, Icecast just disconnects the socket.
		 */
        if ( m_Type == ENCODER_OGG )
		{
            recv( m_SCSocket, buffer, sizeof( buffer ), 0 );
			if(!strncmp(buffer, "OK", strlen("OK")))
			{
				/* I don't think this check is needed.. */
				if(!strncmp(buffer, "OK2", strlen("OK2")))
				{
					gSCFlag = 1;
				}
				else 
				{
					gSCFlag = 0;
				}

				if(serverStatusCallback)
				{
					serverStatusCallback(this, (void *) "Password OK");
				}
			}
			else
			{
				if(serverStatusCallback) 
				{
					serverStatusCallback(this, (void *) "Password Failed");
				}

                closesocket( m_SCSocket );
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

    ret = Load();
	forcedDisconnect = false;
	if(ret)
	{
        m_IsConnected = 1;
		if(serverStatusCallback) 
		{
			serverStatusCallback(this, (void *) "Success");
		}

		/* Start up song title check */
	}
	else
	{
		DisconnectFromServer();
		if(serverStatusCallback)
		{
#ifdef _WIN32
            if ( m_Type == ENCODER_LAME )
			{
				serverStatusCallback(this, (void *) "error with lame_enc.dll");
			}
			else if(gAACFlag) 
			{
				serverStatusCallback(this, (void *) "cannot find libfaac.dll");
			}
			else 
			{
				serverStatusCallback(this, (void *) "Encoder init failed");
			}

#else
			serverStatusCallback(this, (void *) "Encoder init failed");
#endif
		}

		return 0;
	}

	if(serverStatusCallback)
	{
		serverStatusCallback(this, (void *) "Connected");
	}

	SetCurrentSongTitle(gSongTitle);
	UpdateSongTitle( 0 );
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
                sentbytes = sendToServer( g, g->m_SCSocket, (char_t *)og.header, og.header_len, CODEC_TYPE );
				if(sentbytes < 0) 
				{
					return sentbytes;
				}
                sentbytes += sendToServer( g, g->m_SCSocket, (char_t *)og.body, og.body_len, CODEC_TYPE );
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
		long	out_samplerate = g->GetCurrentSamplerate();
		long	in_nch = inNCH;
		long	out_nch = 2;

		if(res_init(&(g->resampler), out_nch, out_samplerate, in_samplerate, RES_END)) 
		{
			g->LogMessage( LOG_ERROR, "Error initializing resampler" );
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

int CEncoder::Load()
{
	int		ret = 0;
	char_t	outFilename[1024] = "";
	char_t	message[1024] = "";

	resetResampler(this);

    if ( m_Type == ENCODER_LAME )
	{
#ifdef HAVE_LAME
#ifdef _WIN32
		BE_ERR		err = 0;
		BE_VERSION	Version = { 0, };
		BE_CONFIG	beConfig = { 0, };

		if(hDLL) 
		{
			FreeLibrary(hDLL);
		}

		hDLL = LoadLibrary("lame_enc.dll");

		if(hDLL == NULL) 
		{
			wsprintf(message,
				"Unable to load DLL (lame_enc.dll)\n\
You have selected encoding with LAME, but apparently the plugin cannot find LAME installed. \
Due to legal issues, ShuiCast cannot distribute LAME directly, and so you'll have to download it separately. \
You will need to put the LAME DLL (lame_enc.dll) \
into the same directory as the application in order to get it working-> \
To download the LAME DLL, check out http://www.rarewares.org/mp3-lame-bundle.php");
			LogMessage( LOG_ERROR, message );
			if(serverStatusCallback)
			{
				serverStatusCallback(this, (void *) "can't find lame_enc.dll");
			}

			return 0;
		}

		/* Get Interface functions from the DLL */
		beInitStream = (BEINITSTREAM) GetProcAddress(hDLL, TEXT_BEINITSTREAM);
		beEncodeChunk = (BEENCODECHUNK) GetProcAddress(hDLL, TEXT_BEENCODECHUNK);
		beDeinitStream = (BEDEINITSTREAM) GetProcAddress(hDLL, TEXT_BEDEINITSTREAM);
		beCloseStream = (BECLOSESTREAM) GetProcAddress(hDLL, TEXT_BECLOSESTREAM);
		beVersion = (BEVERSION) GetProcAddress(hDLL, TEXT_BEVERSION);
		beWriteVBRHeader = (BEWRITEVBRHEADER) GetProcAddress(hDLL, TEXT_BEWRITEVBRHEADER);

		if ( !beInitStream || !beEncodeChunk || !beDeinitStream || !beCloseStream || !beVersion || !beWriteVBRHeader )
		{
			wsprintf(message, "Unable to get LAME interfaces - This DLL (lame_enc.dll) doesn't appear to be LAME?!?!?");
			LogMessage( LOG_ERROR, message );
			return 0;
		}

		/* Get the version number */
		beVersion(&Version);
		if(Version.byMajorVersion < 3)
		{
			wsprintf(message,
				"This version of ShuiCast expects at least version 3.91 of the LAME DLL, the DLL found is at %u.%02u, please consider upgrading",
				Version.byDLLMajorVersion, Version.byDLLMinorVersion);
			LogMessage( LOG_ERROR, message );
		}
		else
		{
			if(Version.byMinorVersion < 91)
			{
				wsprintf(message,
					"This version of ShuiCast expects at least version 3.91 of the LAME DLL, the DLL found is at %u.%02u, please consider upgrading",
					Version.byDLLMajorVersion, Version.byDLLMinorVersion);
				LogMessage( LOG_ERROR, message );
			}
		}

		/* Check if all interfaces are present */
		memset(&beConfig, 0, sizeof(beConfig)); /* clear all fields */

		/* use the LAME config structure */
		beConfig.dwConfig = BE_CONFIG_LAME;

        if ( m_CurrentChannels == 1 )
		{
			beConfig.format.LHV1.nMode = BE_MP3_MODE_MONO;
		}
		else 
		{
			if (LAMEJointStereoFlag) 
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
        beConfig.format.LHV1.dwSampleRate = m_CurrentSamplerate;	// INPUT FREQUENCY 
        beConfig.format.LHV1.dwReSampleRate = m_CurrentSamplerate; // DON"T RESAMPLE 
		// beConfig.format.LHV1.dwReSampleRate = 0;
		beConfig.format.LHV1.dwMpegVersion = MPEG1;					// MPEG VERSION (I or II) 
		beConfig.format.LHV1.dwPsyModel = 0;						// USE DEFAULT PSYCHOACOUSTIC MODEL 
		beConfig.format.LHV1.dwEmphasis = 0;						// NO EMPHASIS TURNED ON 
		beConfig.format.LHV1.bWriteVBRHeader = TRUE;				// YES, WRITE THE XING VBR HEADER 
		//beConfig.format.LHV1.bNoRes = TRUE;						// No Bit resorvoir 
		beConfig.format.LHV1.bStrictIso = gLAMEOptions.strict_ISO;
		beConfig.format.LHV1.bCRC = FALSE;							//
		beConfig.format.LHV1.bCopyright = gLAMEOptions.copywrite;
		beConfig.format.LHV1.bOriginal = gLAMEOptions.original;
		beConfig.format.LHV1.bPrivate = FALSE;						//
		beConfig.format.LHV1.bNoRes = gLAMEOptions.disable_reservoir;
		beConfig.format.LHV1.nQuality = gLAMEOptions.quality | ((~gLAMEOptions.quality) << 8);
        beConfig.format.LHV1.dwBitrate = m_CurrentBitrate;		// BIT RATE

		if((gLAMEOptions.cbrflag) || !strcmp(gLAMEOptions.VBR_mode, "vbr_none") || gLAMEpreset == LQP_CBR)
		{
			beConfig.format.LHV1.nVbrMethod = VBR_METHOD_NONE;
			beConfig.format.LHV1.bEnableVBR = FALSE;
			beConfig.format.LHV1.nVBRQuality = 0;
		}
		else if(!strcmp(gLAMEOptions.VBR_mode, "vbr_abr") || gLAMEpreset == LQP_ABR)
		{
			beConfig.format.LHV1.nVbrMethod = VBR_METHOD_ABR;
			beConfig.format.LHV1.bEnableVBR = TRUE;
            beConfig.format.LHV1.dwVbrAbr_bps = m_CurrentBitrate * 1000;
            beConfig.format.LHV1.dwMaxBitrate = m_CurrentBitrateMax;
			beConfig.format.LHV1.nVBRQuality = gLAMEOptions.quality;
		}
		else
		{
			beConfig.format.LHV1.bEnableVBR = TRUE;
            beConfig.format.LHV1.dwMaxBitrate = m_CurrentBitrateMax;
			beConfig.format.LHV1.nVBRQuality = gLAMEOptions.quality;

			if(!strcmp(gLAMEOptions.VBR_mode, "vbr_rh")) 
			{
				beConfig.format.LHV1.nVbrMethod = VBR_METHOD_OLD;
			}
			else if(!strcmp(gLAMEOptions.VBR_mode, "vbr_new")) 
			{
				beConfig.format.LHV1.nVbrMethod = VBR_METHOD_NEW;
			}
			else if(!strcmp(gLAMEOptions.VBR_mode, "vbr_mtrh")) 
			{
				beConfig.format.LHV1.nVbrMethod = VBR_METHOD_MTRH;
			}
            else if(!strcmp(gLAMEOptions.VBR_mode, "vbr_abr"))
            {
                beConfig.format.LHV1.nVbrMethod = VBR_METHOD_ABR;
            }
            else
			{
				beConfig.format.LHV1.nVbrMethod = VBR_METHOD_DEFAULT;
			}
		}

		//if(gLAMEpreset != LQP_NOPRESET) 
		//{
			beConfig.format.LHV1.nPreset = gLAMEpreset;
		//}

		err = beInitStream(&beConfig, &(dwSamples), &(dwMP3Buffer), &(hbeStream));

		if(err != BE_ERR_SUCCESSFUL)
		{
			wsprintf(message, "Error opening encoding stream (%lu)", err);
			LogMessage( LOG_ERROR, message );
			return 0;
		}

#else
		gf = lame_init();
		lame_set_errorf(gf, oddsock_error_handler_function);
		lame_set_debugf(gf, oddsock_error_handler_function);
		lame_set_msgf(gf, oddsock_error_handler_function);
        lame_set_brate(gf, m_CurrentBitrate);
		lame_set_quality(gf, gLAMEOptions.quality);
		lame_set_num_channels(gf, 2);

        if(m_CurrentChannels == 1)
		{
			lame_set_mode(gf, MONO);

			/*
			 * lame_set_num_channels(gf, 1);
			 */
		}
		else
		{
			lame_set_mode(gf, STEREO);
		}

		/*
		 * Make the input sample rate the same as output..i.e. don't make lame do ;
		 * any resampling->..cause we are handling it ourselves...
		 */
        lame_set_in_samplerate(gf, m_CurrentSamplerate);
        lame_set_out_samplerate(gf, m_CurrentSamplerate);
		lame_set_copyright(gf, gLAMEOptions.copywrite);
		lame_set_strict_ISO(gf, gLAMEOptions.strict_ISO);
		lame_set_disable_reservoir(gf, gLAMEOptions.disable_reservoir);

		if(!gLAMEOptions.cbrflag)
		{
			if(!strcmp(gLAMEOptions.VBR_mode, "vbr_rh"))
			{
				lame_set_VBR(gf, vbr_rh);
			}
			else if(!strcmp(gLAMEOptions.VBR_mode, "vbr_mtrh"))
			{
				lame_set_VBR(gf, vbr_mtrh);
			}
			else if(!strcmp(gLAMEOptions.VBR_mode, "vbr_abr"))
			{
				lame_set_VBR(gf, vbr_abr);
			}

            lame_set_VBR_mean_bitrate_kbps(gf, m_CurrentBitrate);
            lame_set_VBR_min_bitrate_kbps(gf, m_CurrentBitrateMin);
            lame_set_VBR_max_bitrate_kbps(gf, m_CurrentBitrateMax);
		}

		if(strlen(gLAMEbasicpreset) > 0)
		{
			if(!strcmp(gLAMEbasicpreset, "r3mix"))
			{

				/*
				 * presets_set_r3mix(gf, gLAMEbasicpreset, stdout);
				 */
			}
			else
			{

				/*
				 * presets_set_basic(gf, gLAMEbasicpreset, stdout);
				 */
			}
		}

		if(strlen(gLAMEaltpreset) > 0)
		{
			int altbitrate = atoi(gLAMEaltpreset);

			/*
			 * dm_presets(gf, 0, altbitrate, gLAMEaltpreset, "shuicast");
			 */
		}

		/* do internal inits... */
		lame_set_lowpassfreq(gf, gLAMEOptions.lowpassfreq);
		lame_set_highpassfreq(gf, gLAMEOptions.highpassfreq);

		int lame_ret = lame_init_params(gf);

		if(lame_ret != 0)
		{
			printf("Error initializing LAME");
		}
#endif
#else
		if(serverStatusCallback)
		{
			serverStatusCallback(this, (void *) "Not compiled with LAME support");
		}

		LogMessage( LOG_ERROR, "Not compiled with LAME support" );
		return 0;
#endif
	}

	if(gAACFlag)
	{
#ifdef HAVE_FAAC
		faacEncConfigurationPtr m_pConfig;

#ifdef WIN32
		hFAACDLL = LoadLibrary("libfaac.dll");
		if(hFAACDLL == NULL)
		{
			wsprintf(message, "Unable to load AAC DLL (libfaac.dll)");
			LogMessage( LOG_ERROR, message );
			if(serverStatusCallback)
			{
				serverStatusCallback(this, (void *) "can't find libfaac.dll");
			}

			return 0;
		}

		FreeLibrary(hFAACDLL);
#endif
		if(aacEncoder)
		{
			faacEncClose(aacEncoder);
			aacEncoder = NULL;
		}

        aacEncoder = faacEncOpen( m_CurrentSamplerate, m_CurrentChannels, &samplesInput, &maxBytesOutput );

		if(faacFIFO)
		{
			free(faacFIFO);
		}

		faacFIFO = (float *) malloc(samplesInput * sizeof(float) * 16);
		faacFIFOendpos = 0;
		m_pConfig = faacEncGetCurrentConfiguration(aacEncoder);
		m_pConfig->mpegVersion = MPEG2;
		m_pConfig->quantqual = atoi(gAACQuality);

		int cutoff = atoi(gAACCutoff);
		if(cutoff > 0)
		{
			m_pConfig->bandWidth = cutoff;
		}

		/*
		 * m_pConfig->bitRate = (m_CurrentBitrate * 1000) / m_CurrentChannels;
		 */
		m_pConfig->allowMidside = 1;
		m_pConfig->useLfe = 0;
		m_pConfig->useTns = 1;
		m_pConfig->aacObjectType = LOW;
		m_pConfig->outputFormat = 1;
		m_pConfig->inputFormat = FAAC_INPUT_FLOAT;

		/* set new config */
		faacEncSetConfiguration(aacEncoder, m_pConfig);
#else
		if(serverStatusCallback)
		{
			serverStatusCallback(this, (void *) "Not compiled with AAC support");
		}

		LogMessage( LOG_ERROR, "Not compiled with AAC support" );
		return 0;
#endif
	}

	if(gFHAACPFlag)
	{
#ifdef HAVE_FHGAACP
		hFHGAACPDLL = LoadLibrary("enc_fhgaac.dll");
		if(hFHGAACPDLL == NULL)
		{
			LogMessage( LOG_ERROR, "Searching in plugins" );
			hFHGAACPDLL = LoadLibrary("plugins\\enc_fhgaac.dll");
		}

		if(hFHGAACPDLL == NULL)
		{
			wsprintf(message, "Unable to load FHAAC Plus DLL (enc_fhgaac.dll)");
			LogMessage( LOG_ERROR, message );
			if(serverStatusCallback)
			{
				serverStatusCallback(this, (void *) "can't find enc_fhgaac.dll");
			}

			return 0;
		}
		fhCreateAudio3 = (CREATEAUDIO3TYPE) GetProcAddress(hFHGAACPDLL, "CreateAudio3");
		if(!fhCreateAudio3)
		{
			wsprintf(message, "Invalid DLL (enc_fhgaac.dll)");
			LogMessage( LOG_ERROR, message );
			if(serverStatusCallback)
			{
				serverStatusCallback(this, (void *) "invalid enc_fhgaac.dll");
			}

			return 0;
		}
		fhGetAudioTypes3 = (GETAUDIOTYPES3TYPE) GetProcAddress(hFHGAACPDLL, "GetAudioTypes3");
		*(void **) &(fhFinishAudio3) = (void *) GetProcAddress(hFHGAACPDLL, "FinishAudio3");
		*(void **) &(fhPrepareToFinish) = (void *) GetProcAddress(hFHGAACPDLL, "PrepareToFinish");
		if(fhaacpEncoder)
		{
			delete fhaacpEncoder;
			fhaacpEncoder = NULL;
		}
		unsigned int	outt = mmioFOURCC('A', 'D', 'T', 'S');//1346584897;
		char_t			conf_file[MAX_PATH] = "";	/* Default ini file */
		char_t			sectionName[255] = "audio_adtsaac";
		wsprintf(conf_file, "%s\\edcast_fhaacp_%d.ini", defaultConfigDir, encoderNumber);
		/*
		switch(gFHAACPFlag)
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
		gAACPFlag will be 10(auto) 11(LC) 12(HE-AAC) 13(HE-AACv2) config file is far simpler, i.e.:
			[audio_adtsaac] | [audio_fhgaac]
			profile=gAACPFlag-10
			bitrate=sampleRate/1000!!
			surround=0
			shoutcast=1
			//preset=?
			//mode=?
		*/
		char_t tmp[2048];
		wsprintf(tmp, "%d", gFHAACPFlag - 1);
		WritePrivateProfileString(sectionName, "profile", tmp, conf_file);
        wsprintf(tmp, "%d", m_CurrentBitrate);
		WritePrivateProfileString(sectionName, "bitrate", tmp, conf_file);
		WritePrivateProfileString(sectionName, "surround", "0", conf_file);
		WritePrivateProfileString(sectionName, "shoutcast", "1", conf_file);
		//WritePrivateProfileString(sectionName, "preset", "0", conf_file);
        fhaacpEncoder = fhCreateAudio3((int) m_CurrentChannels,
            (int) m_CurrentSamplerate,
										 16,
										 mmioFOURCC('P', 'C', 'M', ' '),
										 &outt,
										 conf_file);
		if(!fhaacpEncoder)
		{
			if(serverStatusCallback)
			{
				serverStatusCallback(this, (void *) "Invalid FHGAAC+ settings");
			}

			LogMessage( LOG_ERROR, "Invalid FHGAAC+ settings" );
			return 0;
		}

#endif
	}
	if(gAACPFlag)
	{
#ifdef HAVE_AACP

#ifdef _WIN32
		hAACPDLL = LoadLibrary("enc_aacplus.dll");
		if(hAACPDLL == NULL)
		{
			hAACPDLL = LoadLibrary("plugins\\enc_aacplus.dll");
		}

		if(hAACPDLL == NULL)
		{
			wsprintf(message, "Unable to load AAC Plus DLL (enc_aacplus.dll)");
			LogMessage( LOG_ERROR, message );
			if(serverStatusCallback)
			{
				serverStatusCallback(this, (void *) "can't find enc_aacplus.dll");
			}

			return 0;
		}

		CreateAudio3 = (CREATEAUDIO3TYPE) GetProcAddress(hAACPDLL, "CreateAudio3");
		if(!CreateAudio3)
		{
			wsprintf(message, "Invalid DLL (enc_aacplus.dll)");
			LogMessage( LOG_ERROR, message );
			if(serverStatusCallback)
			{
				serverStatusCallback(this, (void *) "invalid enc_aacplus.dll");
			}

			return 0;
		}

		GetAudioTypes3 = (GETAUDIOTYPES3TYPE) GetProcAddress(hAACPDLL, "GetAudioTypes3");
		*(void **) &(finishAudio3) = (void *) GetProcAddress(hAACPDLL, "FinishAudio3");
		*(void **) &(PrepareToFinish) = (void *) GetProcAddress(hAACPDLL, "PrepareToFinish");

		/*
		 * FreeLibrary(hAACPDLL);
		 */
#endif
		if(aacpEncoder)
		{
			delete aacpEncoder;
			aacpEncoder = NULL;
		}

		unsigned int	outt = mmioFOURCC('A', 'A', 'C', 'P');//1346584897;
		char_t			conf_file[MAX_PATH] = "";	/* Default ini file */
		char_t			sectionName[255] = "audio_aacplus";

		/* 1 - Mono 2 - Stereo 3 - Stereo Independent 4 - Parametric 5 - Dual Channel */
		char_t			sampleRate[255] = "";
		char_t			channelMode[255] = "";
		char_t			bitrateValue[255] = "";
		char_t			aacpV2Enable[255] = "1";
        long			bitrateLong = m_CurrentBitrate * 1000;

		wsprintf(conf_file, "%s\\edcast_aacp_%d.ini", defaultConfigDir, encoderNumber);
		sprintf(bitrateValue, "%d", bitrateLong);
		switch(gAACPFlag)
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
                if ( m_CurrentChannels == 2 )
				{
					if(LAMEJointStereoFlag && bitrateLong >=16000 && bitrateLong <= 56000) 
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
                if ( m_CurrentChannels == 1 || bitrateLong < 12000 )
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
		if(bitrateLong >= 56000 || gAACPFlag > 1)
		{
			if(m_CurrentChannels == 2)
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
			if(m_CurrentChannels == 2)
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
        sprintf( sampleRate, "%d", m_CurrentSamplerate );

		WritePrivateProfileString(sectionName, "samplerate", sampleRate, conf_file);
		WritePrivateProfileString(sectionName, "channelmode", channelMode, conf_file);
		WritePrivateProfileString(sectionName, "bitrate", bitrateValue, conf_file);
		WritePrivateProfileString(sectionName, "v2enable", aacpV2Enable, conf_file);
		WritePrivateProfileString(sectionName, "bitstream", "0", conf_file);
		WritePrivateProfileString(sectionName, "signallingmode", "0", conf_file);
		WritePrivateProfileString(sectionName, "speech", "0", conf_file);
		WritePrivateProfileString(sectionName, "pns", "0", conf_file);

        aacpEncoder = CreateAudio3( (int)m_CurrentChannels, (int)m_CurrentSamplerate, 16, mmioFOURCC( 'P', 'C', 'M', ' ' ), &outt, conf_file );
		if(!aacpEncoder)
		{
			if(serverStatusCallback)
			{
				serverStatusCallback(this, (void *) "Invalid AAC+ settings");
			}

			LogMessage( LOG_ERROR, "Invalid AAC+ settings" );
			return 0;
		}

#else
		if(serverStatusCallback)
		{
			serverStatusCallback(this, (void *) "Not compiled with AAC Plus support");
		}

		LogMessage( LOG_ERROR, "Not compiled with AAC Plus support" );
		return 0;
#endif
	}

    if ( m_Type == ENCODER_OGG )
	{
#ifdef HAVE_VORBIS
		/* Ogg Vorbis Initialization */
		ogg_stream_clear(&os);
		vorbis_block_clear(&vb);
		vorbis_dsp_clear(&vd);
        vorbis_info_clear( &m_VorbisInfo );

		int bitrate = 0;

        vorbis_info_init( &m_VorbisInfo );

		int encode_ret = 0;

		if(!gOggBitQualFlag)
		{
            encode_ret = vorbis_encode_setup_vbr( &m_VorbisInfo, m_CurrentChannels, m_CurrentSamplerate, ((float)atof( gOggQuality ) * (float) .1) );
			if(encode_ret)
			{
                vorbis_info_clear( &m_VorbisInfo );
			}
		}
		else
		{
			int maxbit = -1;
			int minbit = -1;

            if ( m_CurrentBitrateMax > 0 )
			{
                maxbit = m_CurrentBitrateMax;
			}

            if ( m_CurrentBitrateMin > 0 )
			{
                minbit = m_CurrentBitrateMin;
			}

            encode_ret = vorbis_encode_setup_managed( &m_VorbisInfo, m_CurrentChannels, m_CurrentSamplerate, m_CurrentBitrate * 1000, m_CurrentBitrate * 1000, m_CurrentBitrate * 1000 );
			if(encode_ret)
			{
                vorbis_info_clear( &m_VorbisInfo );
			}
		}

		if(encode_ret == OV_EIMPL)
		{
			LogMessage( LOG_ERROR, "Sorry, but this vorbis mode is not supported currently..." );
			return 0;
		}

		if(encode_ret == OV_EINVAL)
		{
			LogMessage( LOG_ERROR, "Sorry, but this is an illegal vorbis mode..." );
			return 0;
		}

        ret = vorbis_encode_setup_init( &m_VorbisInfo );

		/*
		 * Now, set up the analysis engine, stream encoder, and other preparation before
		 * the encoding begins
		 */
        ret = vorbis_analysis_init( &vd, &m_VorbisInfo );
		ret = vorbis_block_init(&vd, &vb);

		serialno = 0;
		srand((unsigned int)time(0));
		ret = ogg_stream_init(&os, rand());

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

		if(!getLockedMetadataFlag(this))
		{
			if(numVorbisComments) 
			{
				for(int i = 0; i < numVorbisComments; i++)
				{
#ifdef _WIN32
#if 1
					char * utf = AsciiToUtf8(vorbisComments[i]);
					vorbis_comment_add(&vc, utf);
					free(utf);
#else
					MultiByteToWideChar(CP_ACP, 0, vorbisComments[i], strlen(vorbisComments[i]) + 1, widestring, 4096);
					memset(tempstring, '\000', sizeof(tempstring));
					WideCharToMultiByte(CP_UTF8, 0, widestring, wcslen(widestring) + 1, tempstring, sizeof(tempstring), 0, NULL);
					vorbis_comment_add(&vc, tempstring);
#endif
#else
					vorbis_comment_add(&vc, vorbisComments[i]);
#endif
				}

				bypass = true;
			}
		}

		if(!bypass)
		{
            GetCurrentSongTitle( SongTitle, Artist, FullTitle );
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
		if(strlen(sourceDescription) > 0)
		{
			sprintf(Streamed, "TRANSCODEDFROM=%s", sourceDescription);
			vorbis_comment_add(&vc, Streamed);
		}

		/* Build the packets */
		memset(&header_main, '\000', sizeof(header_main));
		memset(&header_comments, '\000', sizeof(header_comments));
		memset(&header_codebooks, '\000', sizeof(header_codebooks));

		vorbis_analysis_headerout(&vd, &vc, &header_main, &header_comments, &header_codebooks);

		ogg_stream_packetin(&os, &header_main);
		ogg_stream_packetin(&os, &header_comments);
		ogg_stream_packetin(&os, &header_codebooks);

		in_header = 1;

		ogg_page	og;
		int			eos = 0;
		int			sentbytes = 0;
		int			ret = 0;

		while(!eos) 
		{
			int result = ogg_stream_flush(&os, &og);
			if(result == 0) break;
            sentbytes += sendToServer( this, m_SCSocket, (char *)og.header, og.header_len, CODEC_TYPE );
            sentbytes += sendToServer( this, m_SCSocket, (char *)og.body, og.body_len, CODEC_TYPE );
		}

		vorbis_comment_clear(&vc);
		if(numVorbisComments) 
		{
			freeVorbisComments(this);
		}

#else
		if(serverStatusCallback)
		{
			serverStatusCallback(this, (void *) "Not compiled with Ogg Vorbis support");
		}

		LogMessage( LOG_ERROR, "Not compiled with Ogg Vorbis support" );
		return 0;
#endif
	}

    if ( m_Type == ENCODER_FLAC )
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

		if(flacEncoder)
		{
			FLAC__stream_encoder_finish(flacEncoder);
			FLAC__stream_encoder_delete(flacEncoder);
			FLAC__metadata_object_delete(flacMetadata);
			flacEncoder = NULL;
			flacMetadata = NULL;
		}

		flacEncoder = FLAC__stream_encoder_new();
		flacMetadata = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);

		FLAC__stream_encoder_set_streamable_subset(flacEncoder, false);
//		FLAC__stream_encoder_set_client_data(flacEncoder, (void*)this);
        FLAC__stream_encoder_set_channels( flacEncoder, m_CurrentChannels );
/*
		FLAC__stream_encoder_set_write_callback(flacEncoder,(FLAC__StreamEncoderWriteCallback) FLACWriteCallback,
												   (FLAC__StreamEncoderWriteCallback) FLACWriteCallback);
		FLAC__stream_encoder_set_metadata_callback(flacEncoder,
												   (FLAC__StreamEncoderMetadataCallback) FLACMetadataCallback);
*/
		srand((unsigned int)time(0));

		if(!getLockedMetadataFlag(this))
		{
			FLAC__StreamMetadata_VorbisComment_Entry entry;
			FLAC__StreamMetadata_VorbisComment_Entry entry3;
			FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ENCODEDBY", "shuicast");
			FLAC__metadata_object_vorbiscomment_append_comment(flacMetadata, entry, true);
			if(strlen(sourceDescription) > 0)
			{
				FLAC__StreamMetadata_VorbisComment_Entry entry2;
				FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry2, "TRANSCODEDFROM", sourceDescription);
				FLAC__metadata_object_vorbiscomment_append_comment(flacMetadata, entry2, true);
			}
            GetCurrentSongTitle( SongTitle, Artist, FullTitle );
			FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry3, "TITLE", FullTitle);
			FLAC__metadata_object_vorbiscomment_append_comment(flacMetadata, entry3, true);

		}
		FLAC__stream_encoder_set_ogg_serial_number(flacEncoder, rand());

		FLAC__StreamEncoderInitStatus ret = FLAC__stream_encoder_init_ogg_stream(flacEncoder, NULL, (FLAC__StreamEncoderWriteCallback) FLACWriteCallback, NULL, NULL, (FLAC__StreamEncoderMetadataCallback) FLACMetadataCallback, (void*)this);
		if(ret == FLAC__STREAM_ENCODER_INIT_STATUS_OK) 
		{
			if(serverStatusCallback)
			{
				serverStatusCallback(this, (void *) "FLAC initialized");
			}
		}
		else
		{
			if(serverStatusCallback)
			{
				serverStatusCallback(this, (void *) "Error Initializing FLAC");
			}

			LogMessage( LOG_ERROR, "Error Initializing FLAC" );
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
int CEncoder::DoEncoding ( float *samples, int numsamples, Limiters *limiter )
{
	int				count = 0;
	unsigned char	mp3buffer[LAME_MAXMP3BUFFER];
	int				imp3;
	short int		*int_samples;
	int				eos = 0;
	int				ret = 0;
	int				sentbytes = 0;
	char			buf[255] = "";

    if ( m_IsConnected )
	{
		gCurrentlyEncoding = 1;

		int		samplecounter = 0;
		if(VUCallback) 
		{
			if(limiter)
			{
				VUCallback(limiter->PeakL, limiter->PeakR, limiter->RmsL, limiter->RmsR);
			}
			else
			{
				long	leftMax = 0;
				long	rightMax = 0;
				LogMessage( LOG_DEBUG, "determining left/right max..." );
				for(int i = 0; i < numsamples * 2; i = i + 2) 
				{
					leftMax += abs((int) ((float) samples[i] * 32767.f));
					rightMax += abs((int) ((float) samples[i + 1] * 32767.f));
				}

				if(numsamples > 0) 
				{
					leftMax = leftMax / (numsamples * 2);
					rightMax = rightMax / (numsamples * 2);
					VUCallback(leftMax, rightMax, leftMax, rightMax);
				}
			}
		}
        if ( m_Type == ENCODER_OGG )
		{
#ifdef HAVE_VORBIS
			/*
			 * If a song change was detected, close the stream and resend new ;
			 * vorbis headers (with new comments) - all done by icecast2SendMetadata();
			 */
			if(ice2songChange) 
			{
				LogMessage( LOG_DEBUG, "Song change processing..." );
				ice2songChange = false;
				icecast2SendMetadata(this);
			}

			LogMessage( LOG_DEBUG, "vorbis_analysis_buffer..." );

			float	**buffer = vorbis_analysis_buffer(&vd, numsamples);
			int		samplecount = 0;
			int		i;

			samplecounter = 0;

			for(i = 0; i < numsamples * 2; i = i + 2) 
			{
				buffer[0][samplecounter] = samples[i];
                if ( m_CurrentChannels == 2 )
				{
					buffer[1][samplecounter] = samples[i + 1];
				}

				samplecounter++;
			}
			LogMessage( LOG_DEBUG, "vorbis_analysis_wrote..." );

			ret = vorbis_analysis_wrote(&vd, numsamples);

			pthread_mutex_lock(&(mutex));
			LogMessage( LOG_DEBUG, "ogg_encode_dataout..." );
			/* Stream out what we just prepared for Vorbis... */
			sentbytes = ogg_encode_dataout(this);
			LogMessage( LOG_DEBUG, "done ogg_ecndoe_dataout..." );
			pthread_mutex_unlock(&mutex);
#endif
		}

		if(gAACFlag)
		{
#ifdef HAVE_FAAC
			float	*buffer = (float *) malloc(numsamples * 2 * sizeof(float));
            FloatScale( buffer, samples, numsamples * 2, m_CurrentChannels );

            addToFIFO( this, buffer, numsamples * m_CurrentChannels );

			while(faacFIFOendpos > (int)samplesInput) 
			{
				float	*buffer2 = (float *) malloc(samplesInput * 2 * sizeof(float));

				ExtractFromFIFO(buffer2, faacFIFO, samplesInput);

				int counter = 0;

				for(int i = samplesInput; i < faacFIFOendpos; i++) 
				{
					faacFIFO[counter] = faacFIFO[i];
					counter++;
				}

				faacFIFOendpos = counter;

				unsigned long	dwWrite = 0;
				unsigned char	*aacbuffer = (unsigned char *) malloc(maxBytesOutput);

				imp3 = faacEncEncode(aacEncoder, (int32_t *) buffer2, samplesInput, aacbuffer, maxBytesOutput);

				if(imp3) 
				{
                    sentbytes = sendToServer( this, m_SCSocket, (char *)aacbuffer, imp3, CODEC_TYPE );
				}

				if(buffer2) free(buffer2);
				if(aacbuffer) free(aacbuffer);
			}

			if(buffer) free(buffer);
#endif
		}

		if(gFHAACPFlag)
		{
#ifdef HAVE_FHGAACP
			static char outbuffer[32768];
            int			len = numsamples * m_CurrentChannels * sizeof(short);

			int_samples = (short *) malloc(len);

			int samplecount = 0;

            if(m_CurrentChannels == 1) 
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

				int enclen = fhaacpEncoder->Encode(in_used, bufcounter, len, &in_used, outbuffer, sizeof(outbuffer));

				if(enclen > 0) 
				{
					// can be part of NSV stream
                    sentbytes = sendToServer(this, m_SCSocket, (char *) outbuffer, enclen, CODEC_TYPE);
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

		if(gAACPFlag)
		{
#ifdef HAVE_AACP
			static char outbuffer[32768];
			//static char inbuffer[32768];
			//static int	inbufferused = 0;
            int			len = numsamples * m_CurrentChannels * sizeof( short );

			int_samples = (short *) malloc(len);

			int samplecount = 0;

            if ( m_CurrentChannels == 1 )
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

				int enclen = aacpEncoder->Encode(in_used, bufcounter, len, &in_used, outbuffer, sizeof(outbuffer));

				if(enclen > 0) 
				{
					// can be part of NSV stream
                    sentbytes = sendToServer( this, m_SCSocket, (char *)outbuffer, enclen, CODEC_TYPE );
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

        if ( m_Type == ENCODER_LAME )
		{
#ifdef HAVE_LAME
			/* Lame encoding is simple, we are passing it interleaved samples */
			int_samples = (short int *) malloc(numsamples * 2 * sizeof(short int));

			int samplecount = 0;

            if ( m_CurrentChannels == 1 )
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
			int				err = beEncodeChunk(hbeStream, samplecount, (short *) int_samples, (PBYTE) mp3buffer, &dwWrite);

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

			imp3 = lame_encode_buffer_float(gf, (float *) samples_left, (float *) samples_right, numsamples, mp3buffer, sizeof(mp3buffer));
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
				LogMessage( LOG_ERROR, "mp3 buffer is not big enough!" );
				gCurrentlyEncoding = 0;
				return -1;
			}

			/* Send out the encoded buffer */
			// can be part of NSV stream
            sentbytes = sendToServer( this, m_SCSocket, (char *)mp3buffer, imp3, CODEC_TYPE );
#endif
		}

        if ( m_Type == ENCODER_FLAC )
		{
#ifdef HAVE_FLAC
			INT32		*int32_samples;

			int32_samples = (INT32 *) malloc(numsamples * 2 * sizeof(INT32));

			int samplecount = 0;

            if ( m_CurrentChannels == 1 )
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

			FLAC__stream_encoder_process_interleaved(flacEncoder, int32_samples, numsamples);

			if(int32_samples) 
			{
				free(int32_samples);
			}

			sentbytes = flacFailure ? 0 : 1;
#endif
		}

		/*
		 * Generic error checking, if there are any socket problems, the trigger ;
		 * a disconnection handling->..
		 */
		if(sentbytes < 0) 
		{
			gCurrentlyEncoding = 0;
			int rret = TriggerDisconnect();
			if (rret == 0) return 0;
		}
	}

	gCurrentlyEncoding = 0;
	return 1;
}

#if 0
int CEncoder::DoEncodingFaster ( float *samples, int numsamples, int nchannels )
{
	int				count = 0;
	unsigned char	mp3buffer[LAME_MAXMP3BUFFER];
	int				imp3;
	short int		*int_samples;
	int				eos = 0;
	int				ret = 0;
	int				sentbytes = 0;
	char			buf[255] = "";
    
    if(m_IsConnected) 
	{
		gCurrentlyEncoding = 1;

		int		samplecounter = 0;
        if ( m_Type == ENCODER_OGG )
		{
#ifdef HAVE_VORBIS
			/*
			 * If a song change was detected, close the stream and resend new ;
			 * vorbis headers (with new comments) - all done by icecast2SendMetadata();
			 */
			if(ice2songChange) 
			{
				LogMessage( LOG_DEBUG, "Song change processing..." );
				ice2songChange = false;
				icecast2SendMetadata(this);
			}

			LogMessage( LOG_DEBUG, "vorbis_analysis_buffer..." );

			float	**buffer = vorbis_analysis_buffer(&vd, numsamples);
			samplecounter = 0;

			float * src = samples;
            if ( m_CurrentChannels == 1 )
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
			LogMessage( LOG_DEBUG, "vorbis_analysis_wrote..." );

			ret = vorbis_analysis_wrote(&vd, numsamples);

			pthread_mutex_lock(&mutex);
			LogMessage( LOG_DEBUG, "ogg_encode_dataout..." );
			/* Stream out what we just prepared for Vorbis... */
			sentbytes = ogg_encode_dataout(this);
			LogMessage( LOG_DEBUG, "done ogg_encode_dataout..." );
			pthread_mutex_unlock(&mutex);
#endif
		}

		if(gAACFlag)
		{
#ifdef HAVE_FAAC // always always always stereo!!!
            int cnt = numsamples * m_CurrentChannels;
			int len = cnt * sizeof(float);

			float	*buffer = (float *) malloc(len);
			// this needs to be changed
			float * src = samples;
			float * dst = buffer;
			for(int i = 0; i < cnt; i ++)
			{
				*(dst++) = *(src++) * 32767.f;
			}
			//FloatScale(buffer, samples, numsamples * 2, m_CurrentChannels);

            addToFIFO( this, buffer, numsamples * m_CurrentChannels );

			while(faacFIFOendpos > (long) samplesInput) 
			{
				float	*buffer2 = (float *) malloc(samplesInput * 2 * sizeof(float));

				ExtractFromFIFO(buffer2, faacFIFO, samplesInput);

				int counter = 0;

				for(int i = samplesInput; i < faacFIFOendpos; i++) 
				{
					faacFIFO[counter] = faacFIFO[i];
					counter++;
				}

				faacFIFOendpos = counter;

				unsigned long	dwWrite = 0;
				unsigned char	*aacbuffer = (unsigned char *) malloc(maxBytesOutput);

				imp3 = faacEncEncode(aacEncoder, (int32_t *) buffer2, samplesInput, aacbuffer, maxBytesOutput);

				if(imp3) 
				{
                    sentbytes = sendToServer(this, m_SCSocket, (char *) aacbuffer, imp3, CODEC_TYPE);
				}

				if(buffer2) free(buffer2);
				if(aacbuffer) free(aacbuffer);
			}

			if(buffer) free(buffer);
#endif
		}

		if(gFHAACPFlag)
		{
#ifdef HAVE_FHGAACP
			static char outbuffer[32768];
            int cnt = numsamples * m_CurrentChannels;
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

				int enclen = fhaacpEncoder->Encode(in_used, bufcounter, len, &in_used, outbuffer, sizeof(outbuffer));

				if(enclen > 0) 
				{
					// can be part of NSV stream
                    sentbytes = sendToServer(this, m_SCSocket, (char *) outbuffer, enclen, CODEC_TYPE);
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

		if(gAACPFlag)
		{
#ifdef HAVE_AACP
			static char outbuffer[32768];
            int cnt = numsamples * m_CurrentChannels;
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

				int enclen = aacpEncoder->Encode(in_used, bufcounter, len, &in_used, outbuffer, sizeof(outbuffer));

				if(enclen > 0) 
				{
					// can be part of NSV stream
                    sentbytes = sendToServer(this, m_SCSocket, (char *) outbuffer, enclen, CODEC_TYPE);
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

        if ( m_Type == ENCODER_LAME )
		{
#ifdef HAVE_LAME
			/* Lame encoding is simple, we are passing it interleaved samples */
            int cnt = numsamples * m_CurrentChannels;
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
			int				err = beEncodeChunk(hbeStream, numsamples, (short *) int_samples, (PBYTE) mp3buffer, &dwWrite);

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

			imp3 = lame_encode_buffer_float(gf, (float *) samples_left, (float *) samples_right, numsamples, mp3buffer, sizeof(mp3buffer));
			if(samples_left) free(samples_left);
			if(samples_right) free(samples_right);
#endif
			if(int_samples) 
			{
				free(int_samples);
			}

			if(imp3 == -1) 
			{
				LogMessage( LOG_ERROR, "mp3 buffer is not big enough!" );
				gCurrentlyEncoding = 0;
				return -1;
			}

			/* Send out the encoded buffer */
			// can be part of NSV stream
            sentbytes = sendToServer(this, m_SCSocket, (char *) mp3buffer, imp3, CODEC_TYPE);
#endif
		}

        if ( m_Type == ENCODER_FLAC )
		{
#ifdef HAVE_FLAC
            int cnt = numsamples * m_CurrentChannels;
			INT32		*int32_samples;

			int32_samples = (INT32 *) malloc(cnt * sizeof(INT32));
			float * src = samples;
			INT32 * dst = int32_samples;

			for(int i = 0; i < cnt; i++) 
			{
				*(dst++) = (INT32) (*(src++) * 32767.0);
			}

			FLAC__stream_encoder_process_interleaved(flacEncoder, int32_samples, numsamples);

			if(int32_samples) free(int32_samples);
            sentbytes = flacFailure ? 0 : 1;
#endif
		}

		/*
		 * Generic error checking, if there are any socket problems, the trigger ;
		 * a disconnection handling->..
		 */
		if(sentbytes < 0) 
		{
			gCurrentlyEncoding = 0;
			int rret = TriggerDisconnect();
			if (rret == 0) return 0;
		}
	}

	gCurrentlyEncoding = 0;
	return 1;
}
#endif

int CEncoder::TriggerDisconnect ()
{
    char buf[2046] = "";
    DisconnectFromServer();
    if ( gForceStop )
    {
        gForceStop = 0;
        return 0;
    }
    wsprintf( buf, "Disconnected from server" );
    forcedDisconnect = true;
    forcedDisconnectSecs = time( NULL );
    serverStatusCallback( this, (void *)buf );
    return 1;
}

void CEncoder::LoadConfig ()
{
	strcpy(gAppName, "shuicast");

	char	buf[255] = "";
	char	desc[1024] = "";

#ifdef XMMS_PLUGIN
	wsprintf(desc, "This is the named pipe used to communicate with the XMMS effect plugin. Make sure it matches the settings in that plugin");
	GetConfigVariable(this, gAppName, "SourceURL", "/tmp/shuicastFIFO", gSourceURL, sizeof(gSourceURL), desc);
#else
	wsprintf(desc, "The source URL for the broadcast. It must be in the form http://server:port/mountpoint.  For those servers without a mountpoint (Shoutcast) use http://server:port.");
	GetConfigVariable(this, gAppName, "SourceURL", "http://localhost/", gSourceURL, sizeof(gSourceURL), desc);
#endif
	if(sourceURLCallback)
	{
		sourceURLCallback(this, (char *)gSourceURL);
	}

	wsprintf(desc, "Destination server details (to where you are encoding).  Valid server types : Shoutcast, Icecast, Icecast2");
	GetConfigVariable(this, gAppName, "ServerType", "Icecast2", gServerType, sizeof(gServerType), desc);
//	wsprintf(desc, "The server to which the stream is sent. It can be a hostname  or IP (example: www.stream.com, 192.168.1.100)");
	GetConfigVariable(this, gAppName, "Server", "localhost", gServer, sizeof(gServer), NULL);
//	wsprintf(desc, "The port to which the stream is sent. Must be a number (example: 8000)");
	GetConfigVariable(this, gAppName, "Port", "8000", gPort, sizeof(gPort), NULL);
//	wsprintf(desc, "This is the encoder password for the destination server (example: hackme)");
	GetConfigVariable(this, gAppName, "ServerPassword", "changemenow", gPassword, sizeof(gPassword), NULL);
//	wsprintf(desc,"Used for Icecast/Icecast2 servers, The mountpoint must end in .ogg for Vorbis streams and have NO extention for MP3 streams.  If you are sending to a Shoutcast server, this MUST be blank. (example: /mp3, /myvorbis.ogg)");
	GetConfigVariable(this, gAppName, "ServerMountpoint", "/stream.ogg", gMountpoint, sizeof(gMountpoint), NULL);
//	wsprintf(desc,"This setting tells the destination server to list on any available YP listings. Not all servers support this (Shoutcast does, Icecast2 doesn't) (example: 1 for YES, 0 for NO)");
	wsprintf(desc,"YP (Stream Directory) Settings");
	gPubServ = GetConfigVariableLong(this, gAppName, "ServerPublic", 1, desc);
//	wsprintf(desc, "This is used in the YP listing, I think only Shoutcast supports this (example: #mystream)");
	GetConfigVariable(this, gAppName, "ServerIRC", "", gServIRC, sizeof(gServIRC), NULL);
//	wsprintf(desc, "This is used in the YP listing, I think only Shoutcast supports this (example: myAIMaccount)");
    GetConfigVariable( this, gAppName, "ServerAIM", "", gServAIM, sizeof( gServAIM ), NULL );
//	wsprintf(desc, "This is used in the YP listing, I think only Shoutcast supports this (example: 332123132)");
    GetConfigVariable( this, gAppName, "ServerICQ", "", gServICQ, sizeof( gServICQ ), NULL );
//	wsprintf(desc, "The URL that is associated with your stream. (example: http://www.mystream.com)");
    GetConfigVariable( this, gAppName, "ServerStreamURL", "http://www.shoutcast.com", gServURL, sizeof( gServURL ), NULL );
//	wsprintf(desc, "The Stream Name");
    GetConfigVariable( this, gAppName, "ServerName", "This is my server name", gServName, sizeof( gServName ), NULL );
//	wsprintf(desc, "A short description of the stream (example: Stream House on Fire!)");
    GetConfigVariable( this, gAppName, "ServerDescription", "This is my server description", gServDesc, sizeof( gServDesc ), NULL );
//	wsprintf(desc, "Genre of music, can be anything you want... (example: Rock)");
    GetConfigVariable( this, gAppName, "ServerGenre", "Rock", gServGenre, sizeof( gServGenre ), NULL );
	wsprintf(desc, "Reconnect if disconnected from the destination server: 1 for YES, 0 for NO");
    gAutoReconnect = GetConfigVariableLong( this, gAppName, "AutomaticReconnect", 1, desc );
	wsprintf(desc, "How long to wait (in seconds) between reconnect attempts, e.g.: 10");
    gReconnectSec = GetConfigVariableLong( this, gAppName, "AutomaticReconnectSecs", 10, desc );

    autoconnect = GetConfigVariableLong( this, gAppName, "AutoConnect", 0, NULL );

	wsprintf(desc,"Set to 1 to start minimized");
    gStartMinimized = GetConfigVariableLong( this, gAppName, "StartMinimized", 0, desc );
	wsprintf(desc,"Set to 1 to enable limiter");
    gLimiter = GetConfigVariableLong( this, gAppName, "Limiter", 0, desc );
	wsprintf(desc,"Limiter dB");
    gLimitdb = GetConfigVariableLong( this, gAppName, "LimitDB", 0, desc );
	wsprintf(desc,"Limiter GAIN dB");
    gGaindb = GetConfigVariableLong( this, gAppName, "LimitGainDB", 0, desc );
	wsprintf(desc,"Limiter pre-emphasis");
    gLimitpre = GetConfigVariableLong( this, gAppName, "LimitPRE", 0, desc );

	wsprintf(desc, "Output codec selection. Valid selections: MP3, OggVorbis, Ogg FLAC, AAC, AAC Plus, HE-AAC, HE-AAC High, LC-AAC, FHGAAC-AUTO, FHGAAC-LC, FHGAAC-HE, FHGAAC-HEv2");
    GetConfigVariable( this, gAppName, "Encode", "OggVorbis", gEncodeType, sizeof( gEncodeType ), desc );

    m_Type = ENCODER_NONE;
    gAACPFlag   = 0;
    gAACFlag    = 0;
    gFHAACPFlag = 0;

    if ( !strncmp( gEncodeType, "MP3", 3 ) ) m_Type = ENCODER_LAME;
    else if ( !strcmp( gEncodeType, "FHGAAC-AUTO" ) ) gFHAACPFlag = 1;
    else if ( !strcmp( gEncodeType, "FHGAAC-LC" ) ) gFHAACPFlag = 2;
    else if ( !strcmp( gEncodeType, "FHGAAC-HE" ) ) gFHAACPFlag = 3;
    else if ( !strcmp( gEncodeType, "FHGAAC-HEv2" ) ) gFHAACPFlag = 4;
    else if ( !strcmp( gEncodeType, "HE-AAC" ) ) gAACPFlag = 1;
    else if ( !strcmp( gEncodeType, "HE-AAC High" ) ) gAACPFlag = 2;
    else if ( !strcmp( gEncodeType, "LC-AAC" ) ) gAACPFlag = 3;
    else if ( !strncmp( gEncodeType, "AAC Plus", 8 ) ) gAACPFlag = 1;
    else if ( !strcmp( gEncodeType, "AAC" ) ) gAACFlag = 1;
    else if ( !strcmp( gEncodeType, "OggVorbis" ) ) m_Type = ENCODER_OGG;
    else if ( !strcmp( gEncodeType, "Ogg FLAC" ) ) m_Type = ENCODER_FLAC;

	if(streamTypeCallback)
	{
        if ( m_Type == ENCODER_OGG  ) streamTypeCallback( this, (void *) "OggVorbis" );
        if ( m_Type == ENCODER_LAME ) streamTypeCallback( this, (void *) "MP3" );
		if(gAACFlag) streamTypeCallback(this, (void *) "AAC");
		if(gAACPFlag) streamTypeCallback(this, (void *) "AAC+");
		if(gFHAACPFlag) streamTypeCallback(this, (void *) "FHGAAC");
        if ( m_Type == ENCODER_FLAC ) streamTypeCallback( this, (void *) "OggFLAC" );
	}

	if(destURLCallback) 
	{
		wsprintf(buf, "http://%s:%s%s", gServer, gPort, gMountpoint);
		destURLCallback(this, (char *) buf);
	}

	wsprintf(desc, "Bitrate. This is the mean bitrate if using VBR.");
    m_CurrentBitrate = GetConfigVariableLong( this, gAppName, "BitrateNominal", 128, desc );

	wsprintf(desc,"Minimum Bitrate. Used only if using Bitrate Management (not recommended) or LAME VBR(example: 64, 128)");
    m_CurrentBitrateMin = GetConfigVariableLong( this, gAppName, "BitrateMin", 128, desc );

	wsprintf(desc,"Maximum Bitrate. Used only if using Bitrate Management (not recommended) or LAME VBR (example: 64, 128)");
    m_CurrentBitrateMax = GetConfigVariableLong( this, gAppName, "BitrateMax", 128, desc );

	wsprintf(desc, "Number of channels. Valid values are (1, 2). 1 means Mono, 2 means Stereo (example: 2,1)");
    m_CurrentChannels = GetConfigVariableLong( this, gAppName, "NumberChannels", 2, desc );

	{
		wsprintf(desc, "Per encoder Attenuation");
        GetConfigVariable( this, gAppName, "Attenuation", "0.0", m_AttenuationTable, sizeof( m_AttenuationTable ), desc );
        double atten = -fabs( atof( m_AttenuationTable ) );
        m_Attenuation = pow( 10.0, atten/20.0 );
	}
	//	wsprintf(desc, "Sample rate for the stream. Valid values depend on wether using lame or vorbis. Vorbis supports odd samplerates such as 32kHz and 48kHz, but lame appears to not.feel free to experiment (example: 44100, 22050, 11025)");
    m_CurrentSamplerate = GetConfigVariableLong( this, gAppName, "Samplerate", 44100, NULL );

//	wsprintf(desc, "Vorbis Quality Level. Valid values are between -1 (lowest quality) and 10 (highest).  The lower the quality the lower the output bitrate. (example: -1, 3)");
	wsprintf(desc, "Ogg Vorbis specific settings.  Note: Valid settings for BitrateQuality flag are (Quality, Bitrate Management)");
	GetConfigVariable(this, gAppName, "OggQuality", "0", gOggQuality, sizeof(gOggQuality), desc);


//	wsprintf(desc,"This flag specifies if you want Vorbis Quality or Bitrate Management.  Quality is always recommended. Valid values are (Bitrate, Quality). (example: Quality, Bitrate Management)");
	GetConfigVariable(this, gAppName, "OggBitrateQualityFlag", "Quality", gOggBitQual, sizeof(gOggBitQual), NULL);
	gOggBitQualFlag = 0;
	if(!strncmp(gOggBitQual, "Q", 1))
	{
		/* Quality */
		gOggBitQualFlag = 0;
	}

	if(!strncmp(gOggBitQual, "B", 1))
	{
		/* Bitrate */
		gOggBitQualFlag = 1;
	}

	if(strlen(gMountpoint) > 0)
	{
		strcpy(gIceFlag, "1");
	}
	else
	{
		strcpy(gIceFlag, "0");
	}

	char	tempString[255] = "";

	memset(tempString, '\000', sizeof(tempString));
	ReplaceString(gServer, tempString, " ", "");
	strcpy(gServer, tempString);

	memset(tempString, '\000', sizeof(tempString));
	ReplaceString(gPort, tempString, " ", "");
	strcpy(gPort, tempString);

//	wsprintf(desc,"LAME specific settings.  Note: Setting the low/highpass freq to 0 will disable them.");
	wsprintf(desc,"This LAME flag indicates that CBR encoding is desired. If this flag is set then LAME with use CBR, if not set then it will use VBR (and you must then specify a VBR mode). Valid values are (1 for SET, 0 for NOT SET) (example: 1)");
	gLAMEOptions.cbrflag = GetConfigVariableLong(this, gAppName, "LameCBRFlag", 1, desc);
	wsprintf(desc,"A number between 0 and 9 which indicates the desired quality level of the stream.  0 = highest to 9 = lowest");
	gLAMEOptions.quality = GetConfigVariableLong(this, gAppName, "LameQuality", 0, desc);

	wsprintf(desc, "Copywrite flag-> Not used for much. Valid values (1 for YES, 0 for NO)");
	gLAMEOptions.copywrite = GetConfigVariableLong(this, gAppName, "LameCopywrite", 0, desc);
	wsprintf(desc, "Original flag-> Not used for much. Valid values (1 for YES, 0 for NO)");
	gLAMEOptions.original = GetConfigVariableLong(this, gAppName, "LameOriginal", 0, desc);
	wsprintf(desc, "Strict ISO flag-> Not used for much. Valid values (1 for YES, 0 for NO)");
	gLAMEOptions.strict_ISO = GetConfigVariableLong(this, gAppName, "LameStrictISO", 0, desc);
	wsprintf(desc, "Disable Reservior flag-> Not used for much. Valid values (1 for YES, 0 for NO)");
	gLAMEOptions.disable_reservoir = GetConfigVariableLong(this, gAppName, "LameDisableReservior", 1, desc);
	wsprintf(desc, "This specifies the type of VBR encoding LAME will perform if VBR encoding is set (CBRFlag is NOT SET). See the LAME documention for more on what these mean. Valid values are (vbr_rh, vbr_mt, vbr_mtrh, vbr_abr, vbr_cbr)");
	GetConfigVariable(this, gAppName, "LameVBRMode", "vbr_cbr",  gLAMEOptions.VBR_mode, sizeof(gLAMEOptions.VBR_mode), desc);

	wsprintf(desc, "Use LAMEs lowpass filter. If you set this to 0, then no filtering is done - not implemented");
	gLAMEOptions.lowpassfreq = GetConfigVariableLong(this, gAppName, "LameLowpassfreq", 0, desc);
	wsprintf(desc, "Use LAMEs highpass filter. If you set this to 0, then no filtering is done - not implemented");
	gLAMEOptions.highpassfreq = GetConfigVariableLong(this, gAppName, "LameHighpassfreq", 0, desc);

	if(gLAMEOptions.lowpassfreq > 0) gLAMELowpassFlag = 1;
	if(gLAMEOptions.highpassfreq > 0) gLAMEHighpassFlag = 1;

    wsprintf(desc, "LAME Preset");
	int defaultPreset = 0;
#ifdef _WIN32
	defaultPreset = LQP_NOPRESET;
#endif
	wsprintf(desc, "LAME Preset - Interesting ones are: -1 = None, 0 = Normal Quality, 1 = Low Quality, 2 = High Quality ... 5 = Very High Quality, 11 = ABR, 12 = CBR");
	gLAMEpreset = GetConfigVariableLong(this, gAppName, "LAMEPreset", defaultPreset, desc);

	wsprintf(desc, "AAC Quality Level. Valid values are between 10 (lowest quality) and 500 (highest).");
	GetConfigVariable(this, gAppName, "AACQuality", "100", gAACQuality, sizeof(gAACQuality), desc);
	wsprintf(desc, "AAC Cutoff Frequency.");
	GetConfigVariable(this, gAppName, "AACCutoff", "", gAACCutoff, sizeof(gAACCutoff), desc);

	if(!strcmp(gServerType, "KasterBlaster"))
	{
		gShoutcastFlag = 1;
		gIcecastFlag = 0;
		gIcecast2Flag = 0;
	}

	if(!strcmp(gServerType, "Shoutcast"))
	{
		gShoutcastFlag = 1;
		gIcecastFlag = 0;
		gIcecast2Flag = 0;
	}

	if(!strcmp(gServerType, "Icecast")) 
	{
		gShoutcastFlag = 0;
		gIcecastFlag = 1;
		gIcecast2Flag = 0;
	}

	if(!strcmp(gServerType, "Icecast2"))
	{
		gShoutcastFlag = 0;
		gIcecastFlag = 0;
		gIcecast2Flag = 1;
	}

	if(serverTypeCallback) 
	{
		serverTypeCallback(this, (void *) gServerType);
	}

	if(serverNameCallback)
	{
		char	*pdata = NULL;
		int		pdatalen = strlen(gServDesc) + strlen(gServName) + strlen(" () ") + 1;

		pdata = (char *) calloc(1, pdatalen);
		wsprintf(pdata, "%s (%s)", gServName, gServDesc);
		serverNameCallback(this, (void *) pdata);
		free(pdata);
	}

	wsprintf(desc, "If recording from linein, what device to use (not needed for win32) (example: /dev/dsp)");
	GetConfigVariable(this, gAppName, "AdvRecDevice", "/dev/dsp", buf, sizeof(buf), desc);
	strcpy(gAdvRecDevice, buf);
	wsprintf(desc, "If recording from linein, what sample rate to open the device with. (example: 44100, 48000)");
	GetConfigVariable(this, gAppName, "LiveInSamplerate", "44100", buf, sizeof(buf), desc);
	gLiveInSamplerate = atoi(buf);
	wsprintf(desc, "Used for any window positions (X value)");
	lastX = GetConfigVariableLong(this, gAppName, "lastX", 0, desc);
	wsprintf(desc, "Used for any window positions (Y value)");
	lastY = GetConfigVariableLong(this, gAppName, "lastY", 0, desc);
	wsprintf(desc, "Used for plugins that show the VU meter");
	vuShow = GetConfigVariableLong(this, gAppName, "showVU", 0, desc);

    wsprintf(desc, "Flag which indicates we are recording from line in");
	int lineInDefault = 0;
#ifdef SHUICASTSTANDALONE  // TODO: get rid of this
	lineInDefault = 1;
#endif
	gLiveRecordingFlag = GetConfigVariableLong(this, gAppName, "LineInFlag", lineInDefault, desc);

	wsprintf(desc, "Locked Metadata");
	GetConfigVariable(this, gAppName, "LockMetadata", "", gManualSongTitle, sizeof(gManualSongTitle), desc);
	wsprintf(desc, "Flag which indicates if we are using locked metadata");
	gLockSongTitle = GetConfigVariableLong(this, gAppName, "LockMetadataFlag", 0, desc);
	wsprintf(desc, "Save directory for archive streams");
	GetConfigVariable(this, gAppName, "SaveDirectory", "", gSaveDirectory, sizeof(gSaveDirectory), desc);
	wsprintf(desc, "Flag which indicates if we are saving archives");
	gSaveDirectoryFlag = GetConfigVariableLong(this, gAppName, "SaveDirectoryFlag", 0, desc);
	wsprintf(desc, "Log Level 1 = LOG_ERROR, 2 = LOG_ERROR+LOG_INFO, 3 = LOG_ERROR+LOG_INFO+LOG_DEBUG");
	gLogLevel = GetConfigVariableLong(this, gAppName, "LogLevel", 2, desc);
	wsprintf(desc, "Log File");
	GetConfigVariable(this, gAppName, "LogFile", defaultLogFileName, gLogFile, sizeof(gLogFile), desc);

	setgLogFile(this, gLogFile);

	wsprintf(desc, "Save Archives in WAV format");
	gSaveAsWAV = GetConfigVariableLong(this, gAppName, "SaveAsWAV", 0, desc);
	wsprintf(desc, "ASIO channel selection 0 1 or 2 only");
	gAsioSelectChannel = GetConfigVariableLong(this, gAppName, "AsioSelectChannel", 0, desc);
	wsprintf(desc, "ASIO channel");
	GetConfigVariable(this, gAppName, "AsioChannel", "", gAsioChannel, sizeof(gAsioChannel),  desc);
	wsprintf(desc, "Encoder Scheduler");
	gEnableEncoderScheduler = GetConfigVariableLong(this, gAppName, "EnableEncoderScheduler", 0, desc);


#define DOW_GETCONFIG(dow, dows) \
	wsprintf(desc, dows##" Schedule"); \
	g##dow##Enable = GetConfigVariableLong(this, gAppName, dows##"Enable", 1, desc); \
	g##dow##OnTime = GetConfigVariableLong(this, gAppName, dows##"OnTime", 0, NULL); \
	g##dow##OffTime = GetConfigVariableLong(this, gAppName, dows##"OffTime", 24, NULL); 

	DOW_GETCONFIG(Monday, "Monday");
	DOW_GETCONFIG(Tuesday, "Tuesday");
	DOW_GETCONFIG(Wednesday, "Wednesday");
	DOW_GETCONFIG(Thursday, "Thursday");
	DOW_GETCONFIG(Friday, "Friday");
	DOW_GETCONFIG(Saturday, "Saturday");
	DOW_GETCONFIG(Sunday, "Sunday");

	wsprintf(desc, "Append this string to all metadata");
	GetConfigVariable(this, gAppName, "MetadataAppend", "", metadataAppendString, sizeof(metadataAppendString),  desc);
	wsprintf(desc, "Remove this string (and everything after) from the window title of the window class the metadata is coming from");
    GetConfigVariable( this, gAppName, "MetadataRemoveAfter", "", metadataRemoveStringAfter, sizeof( metadataRemoveStringAfter ), desc );
	wsprintf(desc,"Remove this string (and everything before) from the window title of the window class the metadata is coming from");
    GetConfigVariable( this, gAppName, "MetadataRemoveBefore", "", metadataRemoveStringBefore, sizeof( metadataRemoveStringBefore ), desc );
	wsprintf(desc, "Window classname to grab metadata from (uses window title)");
    GetConfigVariable( this, gAppName, "MetadataWindowClass", "", metadataWindowClass, sizeof( metadataWindowClass ), desc );
	wsprintf(desc, "Indicator which tells ShuiCast to grab metadata from a defined window class");
    metadataWindowClassInd = GetConfigVariableLong( this, gAppName, "MetadataWindowClassInd", 0, NULL ) != 0;
	wsprintf(desc, "LAME Joint Stereo Flag");
    LAMEJointStereoFlag = GetConfigVariableLong( this, gAppName, "LAMEJointStereo", 1, desc );
	wsprintf(desc, "Set to 1, this encoder will record from DSP regardless of live record state");
    gForceDSPrecording = GetConfigVariableLong( this, gAppName, "ForceDSPrecording", 0, desc );
	wsprintf(desc, "Set ThreeHourBug=1 if your stream distorts after 3 hours 22 minutes and 56 seconds");
    gThreeHourBug = GetConfigVariableLong( this, gAppName, "ThreeHourBug", 0, desc );
	wsprintf(desc, "Set SkipCloseWarning=1 to remove the windows close warning");
    gSkipCloseWarning = GetConfigVariableLong( this, gAppName, "SkipCloseWarning", 0, desc );
	wsprintf(desc, "Set ASIO rate to 44100 or 48000");
    gAsioRate = GetConfigVariableLong( this, gAppName, "AsioRate", 48000, desc );

	/* Set some derived values */
	char	localBitrate[255] = "";
	char	mode[50] = "";

    if ( m_CurrentChannels == 1 )
	{
		strcpy(mode, "Mono");
	}

    if ( m_CurrentChannels == 2 )
	{
		strcpy(mode, "Stereo");
	}

    if ( m_Type == ENCODER_OGG )
	{
		if(bitrateCallback)
		{
			if(gOggBitQualFlag == 0)  /* Quality */
			{
                wsprintf( localBitrate, "Vorbis: Quality %s/%s/%d", gOggQuality, mode, m_CurrentSamplerate );
			}
			else 
			{
                wsprintf( localBitrate, "Vorbis: %dkbps/%s/%d", m_CurrentBitrate, mode, m_CurrentSamplerate );
			}
			bitrateCallback(this, (void *) localBitrate);
		}
	}

    if ( m_Type == ENCODER_LAME )
	{
		if(bitrateCallback)
		{
			if(gLAMEOptions.cbrflag)
			{
                wsprintf( localBitrate, "MP3: %dkbps/%dHz/%s", m_CurrentBitrate, m_CurrentSamplerate, mode );
			}
			else 
			{
                wsprintf( localBitrate, "MP3: (%d/%d/%d)/%s/%d", m_CurrentBitrateMin, m_CurrentBitrate, m_CurrentBitrateMax, mode, m_CurrentSamplerate );
			}

			bitrateCallback(this, (void *) localBitrate);
		}
	}

	if(gAACFlag)
	{
		if(bitrateCallback) 
		{
            wsprintf( localBitrate, "AAC: Quality %s/%dHz/%s", gAACQuality, m_CurrentSamplerate, mode );
			bitrateCallback(this, (void *) localBitrate);
		}
	}

	if(gFHAACPFlag)
	{
		if(bitrateCallback) 
		{
			char enc[20];
			switch(gFHAACPFlag) 
			{
			case 1: strcpy(enc, "AACP-AUTO(fh)"); break;
			case 2: strcpy(enc, "LC-AAC(fh)"); break;
			case 3: strcpy(enc, "HE-AAC(fh)"); break;
			case 4: strcpy(enc, "HE-AACv2(fh)"); break;
			}
            wsprintf( localBitrate, "%s: %dkbps/%dHz", enc, m_CurrentBitrate, m_CurrentSamplerate );
			bitrateCallback(this, (void *) localBitrate);
		}
	}

	if(gAACPFlag)
	{
		if(bitrateCallback) 
		{
            long	bitrateLong = m_CurrentBitrate * 1000;
			char enc[20];

			switch(gAACPFlag) 
			{
			case 1:
				strcpy(enc, "HE-AAC(ct)");
				if(bitrateLong > 64000)
				{
					strcpy(mode, "Stereo");
                    if ( m_CurrentChannels == 1 )
					{
						strcat(mode, "*");
					}
				}
				else if(bitrateLong > 56000) 
				{
                    if ( m_CurrentChannels == 2 )
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
                    if ( m_CurrentChannels == 2 )
					{
						if (LAMEJointStereoFlag && bitrateLong <= 56000) 
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
                    if ( m_CurrentChannels == 2 )
					{
						strcpy(mode, "PS");
						if (!LAMEJointStereoFlag)
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
                    if ( m_CurrentChannels != 1 )
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
            wsprintf( localBitrate, "%s: %dkbps/%dHz/%s", enc, m_CurrentBitrate, m_CurrentSamplerate, mode );
			bitrateCallback(this, (void *) localBitrate);
		}
	}

    if ( m_Type == ENCODER_FLAC )
	{
		if(bitrateCallback) 
		{
            wsprintf( localBitrate, "FLAC: %dHz/%s", m_CurrentSamplerate, mode );
			bitrateCallback(this, (void *) localBitrate);
		}
	}

	if(serverStatusCallback)
	{
		serverStatusCallback(this, (void *) "Disconnected" );
	}

	wsprintf(desc, "Number of encoders to use");
	gNumEncoders = GetConfigVariableLong(this, gAppName, "NumEncoders", 0, desc);
	wsprintf(desc, "Enable external metadata calls (DISABLED, URL, FILE)");
    GetConfigVariable( this, gAppName, "ExternalMetadata", "DISABLED", externalMetadata, sizeof( gLogFile ), desc );
	wsprintf(desc, "URL to retrieve for external metadata");
    GetConfigVariable( this, gAppName, "ExternalURL", "", externalURL, sizeof( externalURL ), desc );
	wsprintf(desc, "File to retrieve for external metadata");
    GetConfigVariable( this, gAppName, "ExternalFile", "", externalFile, sizeof( externalFile ), desc );
	wsprintf(desc, "Interval for retrieving external metadata");
    GetConfigVariable( this, gAppName, "ExternalInterval", "60", externalInterval, sizeof( externalInterval ), desc );
	wsprintf(desc, "Advanced setting");
    GetConfigVariable( this, gAppName, "OutputControl", "", outputControl, sizeof( outputControl ), desc );
	wsprintf(desc, "Windows Recording Device");
    GetConfigVariable( this, gAppName, "WindowsRecDevice", "", buf, sizeof( buf ), desc );
	strcpy(WindowsRecDevice, buf);
	wsprintf(desc, "Windows Recording Sub Device");
    GetConfigVariable( this, gAppName, "WindowsRecSubDevice", "", buf, sizeof( buf ), desc );
	strcpy(WindowsRecSubDevice, buf);
	wsprintf(desc, "LAME Joint Stereo Flag");
    LAMEJointStereoFlag = GetConfigVariableLong( this, gAppName, "LAMEJointStereo", 1, desc );
}

void CEncoder::StoreConfig ()
{
	strcpy(gAppName, "shuicast");

	char	buf[255] = "";
	char	desc[1024] = "";
	char	tempString[1024] = "";

	memset(tempString, '\000', sizeof(tempString));
	ReplaceString(gServer, tempString, " ", "");
	strcpy(gServer, tempString);

	memset(tempString, '\000', sizeof(tempString));
	ReplaceString(gPort, tempString, " ", "");
	strcpy(gPort, tempString);

	PutConfigVariable(this, gAppName, "SourceURL", gSourceURL);
	PutConfigVariable(this, gAppName, "ServerType", gServerType);
	PutConfigVariable(this, gAppName, "Server", gServer);
	PutConfigVariable(this, gAppName, "Port", gPort);
	PutConfigVariable(this, gAppName, "ServerMountpoint", gMountpoint);
	PutConfigVariable(this, gAppName, "ServerPassword", gPassword);
	PutConfigVariableLong(this, gAppName, "ServerPublic", gPubServ);
	PutConfigVariable(this, gAppName, "ServerIRC", gServIRC);
	PutConfigVariable(this, gAppName, "ServerAIM", gServAIM);
	PutConfigVariable(this, gAppName, "ServerICQ", gServICQ);
	PutConfigVariable(this, gAppName, "ServerStreamURL", gServURL);
	PutConfigVariable(this, gAppName, "ServerDescription", gServDesc);
	PutConfigVariable(this, gAppName, "ServerName", gServName);
	PutConfigVariable(this, gAppName, "ServerGenre", gServGenre);
	PutConfigVariableLong(this, gAppName, "AutomaticReconnect", gAutoReconnect);
	PutConfigVariableLong(this, gAppName, "AutomaticReconnectSecs", gReconnectSec);
	PutConfigVariableLong(this, gAppName, "AutoConnect", autoconnect);
	PutConfigVariableLong(this, gAppName, "StartMinimized", gStartMinimized);
	PutConfigVariableLong(this, gAppName, "Limiter", gLimiter);
	PutConfigVariableLong(this, gAppName, "LimitDB", gLimitdb);
	PutConfigVariableLong(this, gAppName, "LimitGainDB", gGaindb);
	PutConfigVariableLong(this, gAppName, "LimitPRE", gLimitpre);
	PutConfigVariable(this, gAppName, "Encode", gEncodeType);

    PutConfigVariableLong( this, gAppName, "BitrateNominal", m_CurrentBitrate );
    PutConfigVariableLong( this, gAppName, "BitrateMin", m_CurrentBitrateMin );
    PutConfigVariableLong( this, gAppName, "BitrateMax", m_CurrentBitrateMax );
    PutConfigVariableLong( this, gAppName, "NumberChannels", m_CurrentChannels );
    PutConfigVariable( this, gAppName, "Attenuation", m_AttenuationTable );
    PutConfigVariableLong( this, gAppName, "Samplerate", m_CurrentSamplerate );
    PutConfigVariable( this, gAppName, "OggQuality", gOggQuality );
	if(gOggBitQualFlag)
	{
		strcpy(gOggBitQual, "Bitrate");
	}
	else 
	{
		strcpy(gOggBitQual, "Quality");
	}

    PutConfigVariable( this, gAppName, "OggBitrateQualityFlag", gOggBitQual );
	PutConfigVariableLong(this, gAppName, "LameCBRFlag", gLAMEOptions.cbrflag);
	PutConfigVariableLong(this, gAppName, "LameQuality", gLAMEOptions.quality);
	PutConfigVariableLong(this, gAppName, "LameCopywrite", gLAMEOptions.copywrite);
	PutConfigVariableLong(this, gAppName, "LameOriginal", gLAMEOptions.original);
	PutConfigVariableLong(this, gAppName, "LameStrictISO", gLAMEOptions.strict_ISO);
	PutConfigVariableLong(this, gAppName, "LameDisableReservior", gLAMEOptions.disable_reservoir);
    PutConfigVariable( this, gAppName, "LameVBRMode", gLAMEOptions.VBR_mode );
	PutConfigVariableLong(this, gAppName, "LameLowpassfreq", gLAMEOptions.lowpassfreq);
	PutConfigVariableLong(this, gAppName, "LameHighpassfreq", gLAMEOptions.highpassfreq);
	PutConfigVariableLong(this, gAppName, "LAMEPreset", gLAMEpreset);
    PutConfigVariable( this, gAppName, "AACQuality", gAACQuality );
    PutConfigVariable( this, gAppName, "AACCutoff", gAACCutoff );
    PutConfigVariable( this, gAppName, "AdvRecDevice", gAdvRecDevice );
    PutConfigVariableLong( this, gAppName, "LiveInSamplerate", gLiveInSamplerate );
    PutConfigVariableLong( this, gAppName, "LineInFlag", gLiveRecordingFlag );
	PutConfigVariableLong(this, gAppName, "lastX", lastX);
	PutConfigVariableLong(this, gAppName, "lastY", lastY);
	PutConfigVariableLong(this, gAppName, "showVU", vuShow);
    PutConfigVariable( this, gAppName, "LockMetadata", gManualSongTitle );
    PutConfigVariableLong( this, gAppName, "LockMetadataFlag", gLockSongTitle );
    PutConfigVariable( this, gAppName, "SaveDirectory", gSaveDirectory );
    PutConfigVariableLong( this, gAppName, "SaveDirectoryFlag", gSaveDirectoryFlag );
    PutConfigVariableLong( this, gAppName, "SaveAsWAV", gSaveAsWAV );
    PutConfigVariable( this, gAppName, "LogFile", gLogFile );
    PutConfigVariableLong( this, gAppName, "LogLevel", gLogLevel );
    PutConfigVariableLong( this, gAppName, "AsioSelectChannel", gAsioSelectChannel );
    PutConfigVariable( this, gAppName, "AsioChannel", gAsioChannel );
    PutConfigVariableLong( this, gAppName, "EnableEncoderScheduler", gEnableEncoderScheduler );

#define PUTDOWVARS(dow, dows) \
	PutConfigVariableLong(this, gAppName, dows##"Enable", g##dow##Enable); \
	PutConfigVariableLong(this, gAppName, dows##"OnTime", g##dow##OnTime); \
	PutConfigVariableLong(this, gAppName, dows##"OffTime", g##dow##OffTime);

	PUTDOWVARS(Monday, "Monday");
	PUTDOWVARS(Tuesday, "Tuesday");
	PUTDOWVARS(Wednesday, "Wednesday");
	PUTDOWVARS(Thursday, "Thursday");
	PUTDOWVARS(Friday, "Friday");
	PUTDOWVARS(Saturday, "Saturday");
	PUTDOWVARS(Sunday, "Sunday");

    PutConfigVariableLong( this, gAppName, "NumEncoders", gNumEncoders );
	PutConfigVariable(this, gAppName, "ExternalMetadata", externalMetadata);
	PutConfigVariable(this, gAppName, "ExternalURL", externalURL);
	PutConfigVariable(this, gAppName, "ExternalFile", externalFile);
	PutConfigVariable(this, gAppName, "ExternalInterval", externalInterval);
    PutConfigVariable( this, gAppName, "OutputControl", outputControl );
	PutConfigVariable(this, gAppName, "MetadataAppend", metadataAppendString);
	PutConfigVariable(this, gAppName, "MetadataRemoveBefore", metadataRemoveStringBefore);
	PutConfigVariable(this, gAppName, "MetadataRemoveAfter", metadataRemoveStringAfter);
	PutConfigVariable(this, gAppName, "MetadataWindowClass", metadataWindowClass);
    PutConfigVariableLong( this, gAppName, "MetadataWindowClassInd", metadataWindowClassInd );
	PutConfigVariable(this, gAppName, "WindowsRecDevice", WindowsRecDevice);
	PutConfigVariable(this, gAppName, "WindowsRecSubDevice", WindowsRecSubDevice);
	PutConfigVariableLong(this, gAppName, "LAMEJointStereo", LAMEJointStereoFlag);
	PutConfigVariableLong(this, gAppName, "ForceDSPrecording", gForceDSPrecording);
	PutConfigVariableLong(this, gAppName, "ThreeHourBug", gThreeHourBug);
	PutConfigVariableLong(this, gAppName, "SkipCloseWarning", gSkipCloseWarning);
	PutConfigVariableLong(this, gAppName, "AsioRate", gAsioRate);
}

/*
 =======================================================================================================================
    Input is in interleaved float samples
    we could have 16 or more channels here
    each encoder can only handle 1 or 2 channels
	here we can copy the required one or two source channels
 =======================================================================================================================
*/
int CEncoder::HandleOutput ( float *samples, int nsamples, int nchannels, int in_samplerate, int asioChannel, int asioChannel2 )
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

    if ( m_IsConnected )
	{
        LogMessage( LOG_DEBUG, "%d Calling handle output - attenuation = %g", encoderNumber, m_Attenuation );
        if ( m_Attenuation != 1.0 )
		{
            double atten = m_Attenuation;
			int sizeofdata = nsamples * in_nch * sizeof(float);
			working_samples = (float *) malloc(sizeofdata);
			sizeofdata = nsamples * in_nch;
			while(sizeofdata--)
				working_samples[sizeofdata] = samples[sizeofdata] * (float) atten;
			samples = working_samples;
		}
		out_samplerate = GetCurrentSamplerate();
		out_nch = GetCurrentChannels();
		if (gSaveFile) 
		{
			if(gSaveAsWAV) 
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

					fwrite(int_samples, sizeofData, 1, gSaveFile);
					written += sizeofData;
					free(int_samples);

					/*
					 * int sizeofData = nsamples*nchannels*sizeof(float);
					 * fwrite(samples, sizeofData, 1, gSaveFile);
					 * written += sizeofData;
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
					fwrite(int_samples, sizeofData, 1, gSaveFile);
					written += sizeofData;
					free(int_samples);
				}
			}
		}
		if(current_insamplerate != in_samplerate)
		{
			resetResampler(this);
			current_insamplerate = in_samplerate;
		}

		if(current_nchannels != nchannels) 
		{
			resetResampler(this);
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

		LogMessage( LOG_DEBUG, "In samplerate = %d, Out = %d", in_samplerate, out_samplerate );
		samplePtr = samples_rechannel;
		if(in_samplerate != out_samplerate) 
		{
			nchannels = 2;

			/* Call the resampler */
			int buf_samples = ((nsamples * out_samplerate) / in_samplerate);

			LogMessage( LOG_DEBUG, "Initializing resampler" );

			initializeResampler(this, in_samplerate, nchannels);

			samples_resampled = (float *) malloc(sizeof(float) * buf_samples * nchannels);
			memset(samples_resampled, '\000', sizeof(float) * buf_samples * nchannels);

			LogMessage( LOG_DEBUG, "calling ocConvertAudio" );
			long	out_samples = ocConvertAudio(this, (float *) samplePtr, (float *) samples_resampled, nsamples, buf_samples);

//			samples_resampled_int = (short *) malloc(sizeof(short) * out_samples * nchannels);
//			memset(samples_resampled_int, '\000', sizeof(short) * out_samples * nchannels);

			LogMessage( LOG_DEBUG, "ready to do encoding" );

			if(out_samples > 0) 
			{
				samplecount = 0;

				/* Here is the call to actually do the encoding->... */
				LogMessage( LOG_DEBUG, "DoEncoding (resampled) start" );
				// de-emphasis could go here
                ret = DoEncoding( (float *)(samples_resampled), out_samples );
				LogMessage( LOG_DEBUG, "DoEncoding end (%d)", ret );
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
			LogMessage( LOG_DEBUG, "DoEncoding start" );
            ret = DoEncoding( (float *)samples_rechannel, nsamples, NULL );
			LogMessage( LOG_DEBUG, "DoEncoding end (%d)", ret );
		}

		if(samples_rechannel) 
		{
			free(samples_rechannel);
			samples_rechannel = NULL;
		}

		if(working_samples)
			free(working_samples);
		
		LogMessage( LOG_DEBUG, "%d Calling handle output - Ret = %d", encoderNumber, ret );
	}

	return ret;
}

int CEncoder::HandleOutputFast ( Limiters *limiter, int dataoffset )
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

    if ( m_IsConnected )
	{
        LogMessage( LOG_DEBUG, "%d Calling handle output - attenuation = %g", encoderNumber, m_Attenuation );
		out_nch = GetCurrentChannels();
		if(out_nch == 1)
			samples = limiter->outputMono + dataoffset * limiter->outputSize * 2;
		else
			samples = limiter->outputStereo + dataoffset * limiter->outputSize * 2;

        if ( m_Attenuation != 1.0 )
		{
            double atten = m_Attenuation;
			int sizeofdata = nsamples * in_nch * sizeof(float);
			working_samples = (float *) malloc(sizeofdata);
			sizeofdata = nsamples * in_nch;
			while(sizeofdata--)
			{
				working_samples[sizeofdata] = samples[sizeofdata] * (float) atten;
			}
			samples = working_samples;
		}
		out_samplerate = GetCurrentSamplerate();

		if (gSaveFile) 
		{
			if(gSaveAsWAV) 
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

					fwrite(int_samples, sizeofData, 1, gSaveFile);
					written += sizeofData;
					free(int_samples);

					/*
					 * int sizeofData = nsamples*nchannels*sizeof(float);
					 * fwrite(samples, sizeofData, 1, gSaveFile);
					 * written += sizeofData;
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
					fwrite(int_samples, sizeofData, 1, gSaveFile);
					written += sizeofData;
					free(int_samples);
				}
			}
		}

		if(current_insamplerate != in_samplerate)
		{
			resetResampler(this);
			current_insamplerate = in_samplerate;
		}

		if(current_nchannels != nchannels) 
		{
			resetResampler(this);
			current_nchannels = nchannels;
		}

		LogMessage( LOG_DEBUG, "In samplerate = %d, Out = %d", in_samplerate, out_samplerate );

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
			LogMessage( LOG_DEBUG, "Initializing resampler" );
			initializeResampler(this, in_samplerate, nchannels);
			samples_resampled = (float *) malloc(sizeof(float) * buf_samples * nchannels);
			LogMessage( LOG_DEBUG, "calling ocConvertAudio" );
			long	out_samples = ocConvertAudio(this, (float *) samplePtr, (float *) samples_resampled, nsamples, buf_samples);
			LogMessage( LOG_DEBUG, "ready to do encoding" );

			if(out_samples > 0) 
			{
				/* Here is the call to actually do the encoding->... */
				LogMessage( LOG_DEBUG, "DoEncoding (resampled) start" );
                ret = DoEncoding( (float *)(samples_resampled), out_samples, limiter );
				LogMessage( LOG_DEBUG, "DoEncoding end (%d)", ret );
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
			LogMessage( LOG_DEBUG, "DoEncoding start" );
            ret = DoEncoding( samples, nsamples, limiter );
			LogMessage( LOG_DEBUG, "DoEncoding end (%d)", ret );
		}

		if(working_samples)
			free(working_samples);
		
		LogMessage( LOG_DEBUG, "%d Calling handle output - Ret = %d", encoderNumber, ret );
	}

	return ret;
}

#ifdef _WIN32
void freeupGlobals(shuicastGlobals *g)
{
	if(g->hDLL) 
	{
		FreeLibrary(g->hDLL);
        g->hDLL = NULL;
	}

	if(g->faacFIFO)
	{
		free(g->faacFIFO);
        g->faacFIFO = NULL;
	}
}
#endif

void CEncoder::AddUISettings ()
{
    AddConfigVariable( "AutomaticReconnect" );
    AddConfigVariable( "AutomaticReconnectSecs" );
    AddConfigVariable( "AutoConnect" );
    AddConfigVariable( "Limiter" );
    AddConfigVariable( "LimitDB" );
    AddConfigVariable( "LimitGainDB" );
    AddConfigVariable( "LimitPRE" );
    AddConfigVariable( "AdvRecDevice" );
    AddConfigVariable( "LiveInSamplerate" );
    AddConfigVariable( "LineInFlag" );
    AddConfigVariable( "lastX" );
    AddConfigVariable( "lastY" );
    AddConfigVariable( "showVU" );
    AddConfigVariable( "LockMetadata" );
    AddConfigVariable( "LockMetadataFlag" );
    AddConfigVariable( "LogLevel" );
    AddConfigVariable( "LogFile" );
    AddConfigVariable( "NumEncoders" );
    AddConfigVariable( "ExternalMetadata" );
    AddConfigVariable( "ExternalURL" );
    AddConfigVariable( "ExternalFile" );
    AddConfigVariable( "ExternalInterval" );
    AddConfigVariable( "OutputControl" );
    AddConfigVariable( "MetadataAppend" );
    AddConfigVariable( "MetadataRemoveBefore" );
    AddConfigVariable( "MetadataRemoveAfter" );
    AddConfigVariable( "MetadataWindowClass" );
    AddConfigVariable( "MetadataWindowClassInd" );
    AddConfigVariable( "WindowsRecDevice" );
    AddConfigVariable( "StartMinimized" );
    AddConfigVariable( "WindowsRecSubDevice" );
    AddConfigVariable( "ThreeHourBug" );
}

void CEncoder::AddASIOSettings ()
{
    AddConfigVariable( "WindowsRecSubDevice" );
    AddConfigVariable( "AsioRate" );
}

void CEncoder::AddBasicEncoderSettings ()
{
    // Server
    AddConfigVariable( "ServerType" );
    AddConfigVariable( "Server" );
    AddConfigVariable( "Port" );
    AddConfigVariable( "ServerMountpoint" );
    AddConfigVariable( "ServerPassword" );
    AddConfigVariable( "ServerPublic" );
    AddConfigVariable( "ServerIRC" );
    AddConfigVariable( "ServerAIM" );
    AddConfigVariable( "ServerICQ" );
    AddConfigVariable( "ServerStreamURL" );
    AddConfigVariable( "ServerDescription" );
    AddConfigVariable( "Attenuation" );
    AddConfigVariable( "ServerName" );
    AddConfigVariable( "ServerGenre" );

    // Encoder
    AddConfigVariable( "Encode" );
    AddConfigVariable( "BitrateNominal" );
    AddConfigVariable( "BitrateMin" );
    AddConfigVariable( "BitrateMax" );
    AddConfigVariable( "NumberChannels" );
    AddConfigVariable( "Samplerate" );
    AddConfigVariable( "OggQuality" );
    AddConfigVariable( "OggBitrateQualityFlag" );
    AddConfigVariable( "LameCBRFlag" );
    AddConfigVariable( "LameQuality" );
    AddConfigVariable( "LameCopywrite" );
    AddConfigVariable( "LameOriginal" );
    AddConfigVariable( "LameStrictISO" );
    AddConfigVariable( "LameDisableReservior" );
    AddConfigVariable( "LameVBRMode" );
    AddConfigVariable( "LameLowpassfreq" );
    AddConfigVariable( "LameHighpassfreq" );
    AddConfigVariable( "LAMEPreset" );
    AddConfigVariable( "AACQuality" );
    AddConfigVariable( "AACCutoff" );
    AddConfigVariable( "LogLevel" );
    AddConfigVariable( "LogFile" );
    AddConfigVariable( "LAMEJointStereo" );
    AddConfigVariable( "SaveDirectory" );
    AddConfigVariable( "SaveDirectoryFlag" );
    AddConfigVariable( "SaveAsWAV" );
}

void CEncoder::AddDSPSettings ()
{
	AddConfigVariable( "ForceDSPrecording" );
}

void CEncoder::AddStandaloneSettings ()
{
    AddConfigVariable( "SkipCloseWarning" );
}

void CEncoder::AddMultiEncoderSettings ()
{
	AddConfigVariable( "AsioSelectChannel" );
	AddConfigVariable( "AsioChannel" );
    AddConfigVariable( "AsioChannel2" );
    AddConfigVariable( "EnableEncoderScheduler" );

#define ADDDOWVARS( dows ) \
	AddConfigVariable( dows##"Enable"  ); \
	AddConfigVariable( dows##"OnTime"  ); \
	AddConfigVariable( dows##"OffTime" )

	ADDDOWVARS( "Monday"    );
	ADDDOWVARS( "Tuesday"   );
	ADDDOWVARS( "Wednesday" );
	ADDDOWVARS( "Thursday"  );
	ADDDOWVARS( "Friday"    );
	ADDDOWVARS( "Saturday"  );
	ADDDOWVARS( "Sunday"    );
}

void CEncoder::LogMessage ( int type, char *source, int line, char *fmt, ...)
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

	if (type <= gLogLevel)
	{
		va_start(parms, fmt);
		char_t	logfile[1024] = "";

		if (logFilep == 0)
		{
			wsprintf(logfile, "%s.log", gLogFile);
			logFilep = fopen(logfile, "a");
		}
		
		if (!logFilep)
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
			fprintf(logFilep,  "%s %s(%s:%d): ", timeStamp, errortype, sourceLine, line);
			vfprintf(logFilep, fmt, parms);
			va_end(parms);
			if (addNewline)
			{
				fprintf(logFilep, "\n");
			}
			fflush(logFilep);
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
