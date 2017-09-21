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
class CEncoder
{
public:

    void Init();
    int  Load();
    int  ConnectToServer();
    int  DisconnectFromServer();

    inline long GetCurrentSamplerate() const
    {
        return m_CurrentSamplerate;
    }

    long      m_CurrentSamplerate;  // TODO: add m_ for all, make private
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

#ifdef _WIN32
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

#ifdef HAVE_VORBIS  // TODO: these things are candidates for child classes
    ogg_stream_state os;
    vorbis_dsp_state vd;
    vorbis_block     vb;
    vorbis_info      m_VorbisInfo;
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
};

#define shuicastGlobals CEncoder  // TODO: replace everywhere and add functions as methods

void    addConfigVariable( CEncoder *g, char_t *variable );
void    getCurrentSongTitle( CEncoder *g, char_t *song, char_t *artist, char_t *full );
void    ReplaceString( char_t *source, char_t *dest, char_t *from, char_t *to );
void    config_read( CEncoder *g );
void    config_write( CEncoder *g );
int     do_encoding( CEncoder *g, short int *samples, int numsamples, Limiters * limiter = NULL );
void    URLize( char_t *input, char_t *output, int inputlen, int outputlen );
int     updateSongTitle( CEncoder *g, int forceURL );
int     setCurrentSongTitleURL( CEncoder *g, char_t *song );
void    icecast2SendMetadata( CEncoder *g );
int     ogg_encode_dataout( CEncoder *g );
int     trimVariable( char_t *variable );
int     readConfigFile( CEncoder *g, int readOnly = 0 );
int     writeConfigFile( CEncoder *g );
//void  printConfigFileValues();
void    ErrorMessage( char_t *title, char_t *fmt, ... );
int     setCurrentSongTitle( CEncoder *g, char_t *song );
char_t *getSourceURL( CEncoder *g );
void    setSourceURL( CEncoder *g, char_t *url );
int     getCurrentBitrate( CEncoder *g );
int     getCurrentChannels( CEncoder *g );
double  getAttenuation( CEncoder *g );
int     ocConvertAudio( CEncoder *g, float *in_samples, float *out_samples, int num_in_samples, int num_out_samples );
int     initializeResampler( CEncoder *g, long inSampleRate, long inNCH );
int     handle_output( CEncoder *g, float *samples, int nsamples, int nchannels, int in_samplerate, int asioChannel = -1, int asioChannel2 = -1 );
int     handle_output_fast( CEncoder *g, Limiters *limiter, int dataoffset=0 );
void    setServerStatusCallback( CEncoder *g, void( *pCallback )(void *, void *) );
void    setGeneralStatusCallback( CEncoder *g, void( *pCallback )(void *, void *) );
void    setWriteBytesCallback( CEncoder *g, void( *pCallback )(void *, void *) );
void    setServerTypeCallback( CEncoder *g, void( *pCallback )(void *, void *) );
void    setServerNameCallback( CEncoder *g, void( *pCallback )(void *, void *) );
void    setStreamTypeCallback( CEncoder *g, void( *pCallback )(void *, void *) );
void    setBitrateCallback( CEncoder *g, void( *pCallback )(void *, void *) );
void    setVUCallback( CEncoder *g, void( *pCallback )(int, int) );
void    setSourceURLCallback( CEncoder *g, void( *pCallback )(void *, void *) );
void    setDestURLCallback( CEncoder *g, void( *pCallback )(void *, void *) );
void    setSourceDescription( CEncoder *g, char_t *desc );
int     getOggFlag( CEncoder *g );
bool    getLiveRecordingFlag( CEncoder *g );
void    setLiveRecordingFlag( CEncoder *g, bool flag );
void    setDumpData( CEncoder *g, int dump );
void    setConfigFileName( CEncoder *g, char_t *configFile );
char_t *getConfigFileName( CEncoder *g );
char_t *getServerDesc( CEncoder *g );
int     getReconnectFlag( CEncoder *g );
int     getReconnectSecs( CEncoder *g );
int     getIsConnected( CEncoder *g );
int     resetResampler( CEncoder *g );
void    setOggEncoderText( CEncoder *g, char_t *text );
int     getLiveRecordingSetFlag( CEncoder *g );
char_t *getCurrentRecordingName( CEncoder *g );
void    setCurrentRecordingName( CEncoder *g, char_t *name );
void    setForceStop( CEncoder *g, int forceStop );
long    getLastXWindow( CEncoder *g );
long    getLastYWindow( CEncoder *g );
void    setLastXWindow( CEncoder *g, long x );
void    setLastYWindow( CEncoder *g, long y );
long    getVUShow( CEncoder *g );
void    setVUShow( CEncoder *g, long x );
int     getFrontEndType( CEncoder *g );
void    setFrontEndType( CEncoder *g, int x );
int     getReconnectTrigger( CEncoder *g );
void    setReconnectTrigger( CEncoder *g, int x );
char_t *getCurrentlyPlaying( CEncoder *g );
//long  GetConfigVariableLong(char_t *appName, char_t *paramName, long defaultvalue, char_t *desc);
long    GetConfigVariableLong( CEncoder *g, char_t *appName, char_t *paramName, long defaultvalue, char_t *desc );
char_t *getLockedMetadata( CEncoder *g );
void    setLockedMetadata( CEncoder *g, char_t *buf );
int     getLockedMetadataFlag( CEncoder *g );
void    setLockedMetadataFlag( CEncoder *g, int flag );
void    setSaveDirectory( CEncoder *g, char_t *saveDir );
char_t *getSaveDirectory( CEncoder *g );
char_t *getgLogFile( CEncoder *g );
void    setgLogFile( CEncoder *g, char_t *logFile );
int     getSaveAsWAV( CEncoder *g );
void    setSaveAsWAV( CEncoder *g, int flag );
int     getForceDSP( CEncoder *g );
void    setForceDSP( CEncoder *g, int flag );
FILE   *getSaveFileP( CEncoder *g );
long    getWritten( CEncoder *g );
void    setWritten( CEncoder *g, long writ );
int     deleteConfigFile( CEncoder *g );
void    setAutoConnect( CEncoder *g, int flag );
void    setStartMinimizedFlag( CEncoder *g, int flag );
int     getStartMinimizedFlag( CEncoder *g );
void    setLimiterFlag( CEncoder *g, int flag );
void    setLimiterValues( CEncoder *g, int db, int pre, int gain );
void    addVorbisComment( CEncoder *g, char_t *comment );
void    freeVorbisComments( CEncoder *g );
void    addBasicEncoderSettings( CEncoder *g );
void    addMultiEncoderSettings( CEncoder *g );
void    addMultiStereoEncoderSettings( CEncoder *g );
void    addDSPONLYsettings( CEncoder *g );
void    addStandaloneONLYsettings( CEncoder *g );
void    addBASSONLYsettings( CEncoder *g );
void    addUISettings( CEncoder *g );
void    addASIOUISettings( CEncoder *g );
void    addASIOExtraSettings( CEncoder *g );
void    addOtherUISettings( CEncoder *g );
void    setDefaultLogFileName( char_t *filename );
void    setConfigDir( char_t *dirname );
void    LogMessage( CEncoder *g, int type, char_t *source, int line, char_t *fmt, ... );
char_t *getWindowsRecordingDevice( CEncoder *g );
void    setWindowsRecordingDevice( CEncoder *g, char_t *device );
char_t *getWindowsRecordingSubDevice( CEncoder *g );
void    setWindowsRecordingSubDevice( CEncoder *g, char_t *device );
int     getLAMEJointStereoFlag( CEncoder *g );
void    setLAMEJointStereoFlag( CEncoder *g, int flag );
int     triggerDisconnect( CEncoder *g );
int     getAppdata( bool checkonly, int locn, DWORD flags, LPCSTR subdir, LPCSTR configname, LPSTR strdestn );
bool    testLocal( LPCSTR dir, LPCSTR file );
