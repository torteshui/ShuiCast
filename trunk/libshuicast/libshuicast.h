#pragma once

#ifdef _WIN32
#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <pthread.h>

#include "libshuicast_cbuffer.h"
#include "libshuicast_limiters.h"
#include "libshuicast_socket.h"
#include "libshuicast_resample.h"
#ifdef HAVE_VORBIS
#include <vorbis/vorbisenc.h>
#endif

#include <stdio.h>
#include <time.h>

#ifdef _DMALLOC_
#include <dmalloc.h>
#endif

/*
#ifdef _UNICODE
#define char_t wchar_t
#define atoi _wtoi
#define LPCSTR LPCWSTR
#define strcpy wcscpy
#define strcmp wcscmp
#define strlen wcslen
#define fopen _wfopen
#define strstr wcsstr
#define sprintf swprintf
#define strcat wcscat
#define fgets fgetws
#else
#define char_t char
#endif
*/

#define char_t char

#ifdef _WIN32
#include <BladeMP3EncDLL.h>
#else
#ifdef HAVE_LAME
#include <lame/lame.h>
#endif
#endif

#ifdef HAVE_FAAC
#undef MPEG2 // hack *cough* hack
typedef signed int  int32_t;
#include <faac.h>
#endif

#ifdef HAVE_AACP
#include "libshuicast_enc_if.h"
typedef AudioCoder* (*CREATEAUDIO3TYPE)(int, int, int, unsigned int, unsigned int *, char_t *);
typedef unsigned int( *GETAUDIOTYPES3TYPE )(int, char_t *);
#endif

#define LM_FORCE 0
#define LM_ERROR 1
#define LM_INFO 2
#define LM_DEBUG 3
#ifdef _WIN32
#define LOG_FORCE LM_FORCE, TEXT(__FILE__), __LINE__
#define LOG_ERROR LM_ERROR, TEXT(__FILE__), __LINE__
#define LOG_INFO  LM_INFO, TEXT(__FILE__), __LINE__
#define LOG_DEBUG LM_DEBUG, TEXT(__FILE__), __LINE__
#define FILE_LINE TEXT(__FILE__), __LINE__
#else
#define LOG_FORCE LM_FORCE, __FILE__, __LINE__
#define LOG_ERROR LM_ERROR, __FILE__, __LINE__
#define LOG_INFO  LM_INFO, __FILE__, __LINE__
#define LOG_DEBUG LM_DEBUG, __FILE__, __LINE__
#define FILE_LINE __FILE__, __LINE__
#endif


#ifdef HAVE_FLAC
#include <FLAC/stream_encoder.h>
extern "C" {
#include <FLAC/metadata.h>
}
#endif

//#define FormatID 'fmt '   /* chunkID for Format Chunk. NOTE: There is a space at the end of this ID. */
// For skin stuff
//#define WINDOW_WIDTH      276
//#define WINDOW_HEIGHT     150

#ifndef FALSE
#define FALSE false
#endif
#ifndef TRUE
#define TRUE true
#endif
#ifndef _WIN32
#include <sys/ioctl.h>
#else
#include <mmsystem.h>
#endif

// Callbacks
//#define   BYTES_PER_SECOND 1
#define FRONT_END_SHUICAST_PLUGIN 1
#define FRONT_END_TRANSCODER 2

typedef struct tagLAMEOptions
{
    int     cbrflag;
    int     out_samplerate;
    int     quality;
#ifdef _WIN32
    int     mode;
#else
#ifdef HAVE_LAME
    MPEG_mode   mode;
#endif
#endif
    int     brate;
    int     copywrite;
    int     original;
    int     strict_ISO;
    int     disable_reservoir;
    char_t  VBR_mode[25];
    int     VBR_mean_bitrate_kbps;
    int     VBR_min_bitrate_kbps;
    int     VBR_max_bitrate_kbps;
    int     lowpassfreq;
    int     highpassfreq;
}
LAMEOptions;

typedef struct
{
    char_t  RIFF[4];
    long    chunkSize;
    char_t  WAVE[4];
}
RIFFChunk;

typedef struct
{
    char_t         chunkID[4];
    long           chunkSize;
    short          wFormatTag;
    unsigned short wChannels;
    unsigned long  dwSamplesPerSec;
    unsigned long  dwAvgBytesPerSec;
    unsigned short wBlockAlign;
    unsigned short wBitsPerSample;

    /* Note: there may be additional fields here, depending upon wFormatTag. */
}
FormatChunk;

typedef struct
{
    char_t  chunkID[4];
    long    chunkSize;
    short * waveformData;
}
DataChunk;

struct wavhead
{
    unsigned int   main_chunk;
    unsigned int   length;
    unsigned int   chunk_type;
    unsigned int   sub_chunk;
    unsigned int   sc_len;
    unsigned short format;
    unsigned short modus;
    unsigned int   sample_fq;
    unsigned int   byte_p_sec;
    unsigned short byte_p_spl;
    unsigned short bit_p_spl;
    unsigned int   data_chunk;
    unsigned int   data_length;
};


static struct wavhead   wav_header;

// Global variables....gotta love em...
typedef struct
{
    long      currentSamplerate;
    int       currentBitrate;
    int       currentBitrateMin;
    int       currentBitrateMax;
    int       currentChannels;
    char_t    attenuation[30];
    double    dAttenuation;
    int       gSCSocket;
    int       gSCSocket2;
    int       gSCSocketControl;
    CMySocket dataChannel;
    CMySocket controlChannel;
    int       gSCFlag;
    int       gCountdown;
    int       gAutoCountdown;
    int       automaticconnect;
    char_t    gSourceURL[1024];
    char_t    gServer[256];
    char_t    gPort[10];
    char_t    gPassword[256];
    int       weareconnected;
    char_t    gIniFile[1024];
    char_t    gAppName[256];
    char_t    gCurrentSong[1024];
    int       gSongUpdateCounter;
    char_t    gMetadataUpdate[10];
    int       gPubServ;
    char_t    gServIRC[20];
    char_t    gServICQ[20];
    char_t    gServAIM[20];
    char_t    gServURL[1024];
    char_t    gServDesc[1024];
    char_t    gServName[1024];
    char_t    gServGenre[100];
    char_t    gMountpoint[100];
    char_t    gFrequency[10];
    char_t    gChannels[10];
    int       gAutoReconnect;
    int       gReconnectSec;
    char_t    gAutoStart[10];
    char_t    gAutoStartSec[20];
    char_t    gQuality[5];
#ifndef _WIN32
#ifdef HAVE_LAME
    lame_global_flags *gf;
#endif
#endif
    int       gCurrentlyEncoding;
    int       gFLACFlag;
    int       gAACFlag;
    int       gAACPFlag;
    int       gFHAACPFlag;
    int       gOggFlag;
    char_t    gIceFlag[10];
    int       gLAMEFlag;
    char_t    gOggQuality[25];
    int       gLiveRecordingFlag;
    int       gLimiter;
    int       gLimitdb;
    int       gGaindb;
    int       gLimitpre;
    int       gStartMinimized;
    int       gOggBitQualFlag;
    char_t    gOggBitQual[40];
    char_t    gEncodeType[25];
    int       gAdvancedRecording;
    int       gNOggchannels;
    char_t    gModes[4][255];
    int       gShoutcastFlag;
    int       gIcecastFlag;
    int       gIcecast2Flag;
    char_t    gSaveDirectory[1024];
    char_t    gLogFile[1024];
    int       gLogLevel;
    FILE     *logFilep;
    int       gSaveDirectoryFlag;
    int       gSaveAsWAV;
    int       gAsioSelectChannel;
    char_t    gAsioChannel[255];

    int       gEnableEncoderScheduler;
    int       gMondayEnable;
    int       gMondayOnTime;
    int       gMondayOffTime;
    int       gTuesdayEnable;
    int       gTuesdayOnTime;
    int       gTuesdayOffTime;
    int       gWednesdayEnable;
    int       gWednesdayOnTime;
    int       gWednesdayOffTime;
    int       gThursdayEnable;
    int       gThursdayOnTime;
    int       gThursdayOffTime;
    int       gFridayEnable;
    int       gFridayOnTime;
    int       gFridayOffTime;
    int       gSaturdayEnable;
    int       gSaturdayOnTime;
    int       gSaturdayOffTime;
    int       gSundayEnable;
    int       gSundayOnTime;
    int       gSundayOffTime;

    FILE       *gSaveFile;
    LAMEOptions gLAMEOptions;
    int         gLAMEHighpassFlag;
    int         gLAMELowpassFlag;

    int       oggflag;
    int       serialno;
#ifdef HAVE_VORBIS
    ogg_sync_state oy_stream;
    ogg_packet     header_main_save;
    ogg_packet     header_comments_save;
    ogg_packet     header_codebooks_save;
#endif
    bool      ice2songChange;
    int       in_header;
    long      written;
    int       vuShow;

    int       gLAMEpreset;
    char_t    gLAMEbasicpreset[255];
    char_t    gLAMEaltpreset[255];
    char_t    gSongTitle[1024];
    char_t    gManualSongTitle[1024];
    int       gLockSongTitle;
    int       gNumEncoders;

    res_state resampler;
    int       initializedResampler;
    void      (*sourceURLCallback)     ( void *, void * );
    void      (*destURLCallback)       ( void *, void * );
    void      (*serverStatusCallback)  ( void *, void * );
    void      (*generalStatusCallback) ( void *, void * );
    void      (*writeBytesCallback)    ( void *, void * );
    void      (*serverTypeCallback)    ( void *, void * );
    void      (*serverNameCallback)    ( void *, void * );
    void      (*streamTypeCallback)    ( void *, void * );
    void      (*bitrateCallback)       ( void *, void * );
    void      (*VUCallback)            ( double, double, double, double );
    long      startTime;
    long      endTime;
    char_t    sourceDescription[255];
    char_t    gServerType[25];

#ifdef WIN32
    WAVEFORMATEX waveFormat;
    HWAVEIN      inHandle;
    WAVEHDR      WAVbuffer1;
    WAVEHDR      WAVbuffer2;
#else
    int inHandle; // for advanced recording
#endif

    unsigned long result;
    short int     WAVsamplesbuffer1[1152*2];
    short int     WAVsamplesbuffer2[1152*2];
    bool          areLiveRecording;
    char_t        gAdvRecDevice[255];
#ifndef _WIN32
    char_t        gAdvRecServerTitle[255];
#endif
    int           gLiveInSamplerate;

#ifdef _WIN32
    // These are for the LAME DLL
    BEINITSTREAM     beInitStream;
    BEENCODECHUNK    beEncodeChunk;
    BEDEINITSTREAM   beDeinitStream;
    BECLOSESTREAM    beCloseStream;
    BEVERSION        beVersion;
    BEWRITEVBRHEADER beWriteVBRHeader;
    HINSTANCE        hDLL;
    HINSTANCE        hFAACDLL;
    HINSTANCE        hAACPDLL;
    HINSTANCE        hFHGAACPDLL;
    DWORD            dwSamples;
    DWORD            dwMP3Buffer;
    HBE_STREAM       hbeStream;
#endif

    char_t    gConfigFileName[255];
    char_t    gOggEncoderText[255];
    int       gForceStop;
    char_t    gCurrentRecordingName[1024];
    long      lastX;
    long      lastY;

#ifdef HAVE_VORBIS
    ogg_stream_state os;
    vorbis_dsp_state vd;
    vorbis_block     vb;
    vorbis_info      vi;
#endif

    int       frontEndType;
    int       ReconnectTrigger;

#ifdef HAVE_AACP
    CREATEAUDIO3TYPE   fhCreateAudio3;
    GETAUDIOTYPES3TYPE fhGetAudioTypes3;

    AudioCoder *(*fhFinishAudio3)    ( char_t *fn, AudioCoder *c );
    void        (*fhPrepareToFinish) ( const char_t *filename, AudioCoder *coder );
    AudioCoder *fhaacpEncoder;

    CREATEAUDIO3TYPE   CreateAudio3;
    GETAUDIOTYPES3TYPE GetAudioTypes3;

    AudioCoder *(*finishAudio3)(char_t *fn, AudioCoder *c);
    void( *PrepareToFinish )(const char_t *filename, AudioCoder *coder);
    AudioCoder * aacpEncoder;
#endif

#ifdef HAVE_FAAC
    faacEncHandle aacEncoder;
#endif

    unsigned long samplesInput, maxBytesOutput;
    float    *faacFIFO;
    long      faacFIFOendpos;
    char_t    gAACQuality[25];
    char_t    gAACCutoff[25];
    int       encoderNumber;
    bool      forcedDisconnect;
    time_t    forcedDisconnectSecs;
    int       autoconnect;
    char_t    externalMetadata[255];
    char_t    externalURL[255];
    char_t    externalFile[255];
    char_t    externalInterval[25];
    char_t   *vorbisComments[30];
    int       numVorbisComments;
    char_t    outputControl[255];
    char_t    metadataAppendString[255];
    char_t    metadataRemoveStringBefore[255];
    char_t    metadataRemoveStringAfter[255];
    char_t    metadataWindowClass[255];
    bool      metadataWindowClassInd;
#ifdef HAVE_FLAC
    FLAC__StreamEncoder  *flacEncoder;
    FLAC__StreamMetadata *flacMetadata;
    int       flacFailure;
#endif
    char_t   *configVariables[255];
    int       numConfigVariables;
    pthread_mutex_t mutex;

    char_t    WindowsRecSubDevice[255];
    char_t    WindowsRecDevice[255];

    int       LAMEJointStereoFlag;
    int       gForceDSPrecording;
    int       gThreeHourBug;
    int       gSkipCloseWarning;
    int       gAsioRate;
    //CBUFFER circularBuffer;
}
shuicastGlobals;

void    addConfigVariable( shuicastGlobals *g, char_t *variable );
int     initializeencoder( shuicastGlobals *g );
void    getCurrentSongTitle( shuicastGlobals *g, char_t *song, char_t *artist, char_t *full );
void    initializeGlobals( shuicastGlobals *g );
void    ReplaceString( char_t *source, char_t *dest, char_t *from, char_t *to );
void    config_read( shuicastGlobals *g );
void    config_write( shuicastGlobals *g );
int     connectToServer( shuicastGlobals *g );
int     disconnectFromServer( shuicastGlobals *g );
int     do_encoding( shuicastGlobals *g, short int *samples, int numsamples, Limiters * limiter = NULL );
void    URLize( char_t *input, char_t *output, int inputlen, int outputlen );
int     updateSongTitle( shuicastGlobals *g, int forceURL );
int     setCurrentSongTitleURL( shuicastGlobals *g, char_t *song );
void    icecast2SendMetadata( shuicastGlobals *g );
int     ogg_encode_dataout( shuicastGlobals *g );
int     trimVariable( char_t *variable );
int     readConfigFile( shuicastGlobals *g, int readOnly = 0 );
int     writeConfigFile( shuicastGlobals *g );
//void  printConfigFileValues();
void    ErrorMessage( char_t *title, char_t *fmt, ... );
int     setCurrentSongTitle( shuicastGlobals *g, char_t *song );
char_t *getSourceURL( shuicastGlobals *g );
void    setSourceURL( shuicastGlobals *g, char_t *url );
long    getCurrentSamplerate( shuicastGlobals *g );
int     getCurrentBitrate( shuicastGlobals *g );
int     getCurrentChannels( shuicastGlobals *g );
double  getAttenuation( shuicastGlobals *g );
int     ocConvertAudio( shuicastGlobals *g, float *in_samples, float *out_samples, int num_in_samples, int num_out_samples );
int     initializeResampler( shuicastGlobals *g, long inSampleRate, long inNCH );
int     handle_output( shuicastGlobals *g, float *samples, int nsamples, int nchannels, int in_samplerate, int asioChannel = -1, int asioChannel2 = -1 );
int     handle_output_fast( shuicastGlobals *g, Limiters *limiter, int dataoffset=0 );
void    setServerStatusCallback( shuicastGlobals *g, void( *pCallback )(void *, void *) );
void    setGeneralStatusCallback( shuicastGlobals *g, void( *pCallback )(void *, void *) );
void    setWriteBytesCallback( shuicastGlobals *g, void( *pCallback )(void *, void *) );
void    setServerTypeCallback( shuicastGlobals *g, void( *pCallback )(void *, void *) );
void    setServerNameCallback( shuicastGlobals *g, void( *pCallback )(void *, void *) );
void    setStreamTypeCallback( shuicastGlobals *g, void( *pCallback )(void *, void *) );
void    setBitrateCallback( shuicastGlobals *g, void( *pCallback )(void *, void *) );
void    setVUCallback( shuicastGlobals *g, void( *pCallback )(int, int) );
void    setSourceURLCallback( shuicastGlobals *g, void( *pCallback )(void *, void *) );
void    setDestURLCallback( shuicastGlobals *g, void( *pCallback )(void *, void *) );
void    setSourceDescription( shuicastGlobals *g, char_t *desc );
int     getOggFlag( shuicastGlobals *g );
bool    getLiveRecordingFlag( shuicastGlobals *g );
void    setLiveRecordingFlag( shuicastGlobals *g, bool flag );
void    setDumpData( shuicastGlobals *g, int dump );
void    setConfigFileName( shuicastGlobals *g, char_t *configFile );
char_t *getConfigFileName( shuicastGlobals *g );
char_t *getServerDesc( shuicastGlobals *g );
int     getReconnectFlag( shuicastGlobals *g );
int     getReconnectSecs( shuicastGlobals *g );
int     getIsConnected( shuicastGlobals *g );
int     resetResampler( shuicastGlobals *g );
void    setOggEncoderText( shuicastGlobals *g, char_t *text );
int     getLiveRecordingSetFlag( shuicastGlobals *g );
char_t *getCurrentRecordingName( shuicastGlobals *g );
void    setCurrentRecordingName( shuicastGlobals *g, char_t *name );
void    setForceStop( shuicastGlobals *g, int forceStop );
long    getLastXWindow( shuicastGlobals *g );
long    getLastYWindow( shuicastGlobals *g );
void    setLastXWindow( shuicastGlobals *g, long x );
void    setLastYWindow( shuicastGlobals *g, long y );
long    getVUShow( shuicastGlobals *g );
void    setVUShow( shuicastGlobals *g, long x );
int     getFrontEndType( shuicastGlobals *g );
void    setFrontEndType( shuicastGlobals *g, int x );
int     getReconnectTrigger( shuicastGlobals *g );
void    setReconnectTrigger( shuicastGlobals *g, int x );
char_t *getCurrentlyPlaying( shuicastGlobals *g );
//long  GetConfigVariableLong(char_t *appName, char_t *paramName, long defaultvalue, char_t *desc);
long    GetConfigVariableLong( shuicastGlobals *g, char_t *appName, char_t *paramName, long defaultvalue, char_t *desc );
char_t *getLockedMetadata( shuicastGlobals *g );
void    setLockedMetadata( shuicastGlobals *g, char_t *buf );
int     getLockedMetadataFlag( shuicastGlobals *g );
void    setLockedMetadataFlag( shuicastGlobals *g, int flag );
void    setSaveDirectory( shuicastGlobals *g, char_t *saveDir );
char_t *getSaveDirectory( shuicastGlobals *g );
char_t *getgLogFile( shuicastGlobals *g );
void    setgLogFile( shuicastGlobals *g, char_t *logFile );
int     getSaveAsWAV( shuicastGlobals *g );
void    setSaveAsWAV( shuicastGlobals *g, int flag );
int     getForceDSP( shuicastGlobals *g );
void    setForceDSP( shuicastGlobals *g, int flag );
FILE   *getSaveFileP( shuicastGlobals *g );
long    getWritten( shuicastGlobals *g );
void    setWritten( shuicastGlobals *g, long writ );
int     deleteConfigFile( shuicastGlobals *g );
void    setAutoConnect( shuicastGlobals *g, int flag );
void    setStartMinimizedFlag( shuicastGlobals *g, int flag );
int     getStartMinimizedFlag( shuicastGlobals *g );
void    setLimiterFlag( shuicastGlobals *g, int flag );
void    setLimiterValues( shuicastGlobals *g, int db, int pre, int gain );
void    addVorbisComment( shuicastGlobals *g, char_t *comment );
void    freeVorbisComments( shuicastGlobals *g );
void    addBasicEncoderSettings( shuicastGlobals *g );
void    addMultiEncoderSettings( shuicastGlobals *g );
void    addMultiStereoEncoderSettings( shuicastGlobals *g );
void    addDSPONLYsettings( shuicastGlobals *g );
void    addStandaloneONLYsettings( shuicastGlobals *g );
void    addBASSONLYsettings( shuicastGlobals *g );
void    addUISettings( shuicastGlobals *g );
void    addASIOUISettings( shuicastGlobals *g );
void    addASIOExtraSettings( shuicastGlobals *g );
void    addOtherUISettings( shuicastGlobals *g );
void    setDefaultLogFileName( char_t *filename );
void    setConfigDir( char_t *dirname );
void    LogMessage( shuicastGlobals *g, int type, char_t *source, int line, char_t *fmt, ... );
char_t *getWindowsRecordingDevice( shuicastGlobals *g );
void    setWindowsRecordingDevice( shuicastGlobals *g, char_t *device );
char_t *getWindowsRecordingSubDevice( shuicastGlobals *g );
void    setWindowsRecordingSubDevice( shuicastGlobals *g, char_t *device );
int     getLAMEJointStereoFlag( shuicastGlobals *g );
void    setLAMEJointStereoFlag( shuicastGlobals *g, int flag );
int     triggerDisconnect( shuicastGlobals *g );
int     getAppdata( bool checkonly, int locn, DWORD flags, LPCSTR subdir, LPCSTR configname, LPSTR strdestn );
bool    testLocal( LPCSTR dir, LPCSTR file );
