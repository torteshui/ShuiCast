#pragma once
#pragma warning( disable : 4351 )  // default initialization of array members

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

typedef enum
{
    ENCODER_NONE,
    ENCODER_LAME,
    ENCODER_OGG,
    ENCODER_AAC  // TODO
}
EncoderType;

typedef struct
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

class CEncoder
{
public:

    CEncoder( int encoderNumber );
    ~CEncoder(){}

    int  Load();
    void LoadConfig();
    void StoreConfig();
    void AddConfigVariable( char_t *variable );
    int  ConnectToServer();
    int  DisconnectFromServer();
    void GetCurrentSongTitle( char_t *song, char_t *artist, char_t *full ) const;
    int  SetCurrentSongTitle( char_t *song );

    void ReplaceString ( char_t *source, char_t *dest, char_t *from, char_t *to ) const;
    char_t* URLize ( char_t *input ) const;

    inline long GetCurrentSamplerate () const
    {
        return m_CurrentSamplerate;
    }

    inline int GetCurrentBitrate () const
    {
        return m_CurrentBitrate;
    }

    inline int GetCurrentChannels() const
    {
        return m_CurrentChannels;
    }

    EncoderType m_Type;

    long      m_CurrentSamplerate = 0;  // TODO: add m_ for all, make private
    int       m_CurrentBitrate    = 0;
    int       m_CurrentBitrateMin = 0;
    int       m_CurrentBitrateMax = 0;
    int       m_CurrentChannels   = 0;

    char_t    attenuation[30] = {};
    double    dAttenuation = 0;
    int       gSCSocket = 0;
    int       gSCSocket2 = 0;
    int       gSCSocketControl = 0;
    CMySocket dataChannel;
    CMySocket controlChannel;
    int       gSCFlag = 0;
    int       gCountdown = 0;
    int       gAutoCountdown = 0;
    int       automaticconnect = 0;
    char_t    gSourceURL[1024] ={};
    char_t    gServer[256] ={};
    char_t    gPort[10] ={};
    char_t    gPassword[256] ={};
    int       weareconnected = 0;
    char_t    gIniFile[1024] ={};
    char_t    gAppName[256] ={};
    char_t    gCurrentSong[1024] ={};
    int       gSongUpdateCounter = 0;
    char_t    gMetadataUpdate[10] ={};
    int       gPubServ = 0;
    char_t    gServIRC[20] ={};
    char_t    gServICQ[20] ={};
    char_t    gServAIM[20] ={};
    char_t    gServURL[1024] ={};
    char_t    gServDesc[1024] ={};
    char_t    gServName[1024] ={};
    char_t    gServGenre[100] ={};
    char_t    gMountpoint[100] ={};
    char_t    gFrequency[10] ={};
    char_t    gChannels[10] ={};
    int       gAutoReconnect = 0;
    int       gReconnectSec = 0;
    char_t    gAutoStart[10] ={};
    char_t    gAutoStartSec[20] ={};
    char_t    gQuality[5] ={};
#ifndef _WIN32
#ifdef HAVE_LAME
    lame_global_flags *gf = NULL;
#endif
#endif
    int       gCurrentlyEncoding = 0;
    int       gFLACFlag = 0;
    int       gAACFlag = 0;
    int       gAACPFlag = 0;
    int       gFHAACPFlag = 0;
    char_t    gIceFlag[10] ={};
    char_t    gOggQuality[25] ={};
    int       gLiveRecordingFlag = 0;
    int       gLimiter = 0;
    int       gLimitdb = 0;
    int       gGaindb = 0;
    int       gLimitpre = 0;
    int       gStartMinimized = 0;
    int       gOggBitQualFlag = 0;
    char_t    gOggBitQual[40] ={};
    char_t    gEncodeType[25] ={};
    int       gAdvancedRecording = 0;
    int       gNOggchannels = 0;
    char_t    gModes[4][255] ={};
    int       gShoutcastFlag = 0;
    int       gIcecastFlag = 0;
    int       gIcecast2Flag = 0;
    char_t    gSaveDirectory[1024] ={};
    char_t    gLogFile[1024] ={};
    int       gLogLevel = 0;
    FILE     *logFilep = NULL;
    int       gSaveDirectoryFlag = 0;
    int       gSaveAsWAV = 0;
    int       gAsioSelectChannel = 0;
    char_t    gAsioChannel[255] ={};

    int       gEnableEncoderScheduler = 0;
    int       gMondayEnable = 0;
    int       gMondayOnTime = 0;
    int       gMondayOffTime = 0;
    int       gTuesdayEnable = 0;
    int       gTuesdayOnTime = 0;
    int       gTuesdayOffTime = 0;
    int       gWednesdayEnable = 0;
    int       gWednesdayOnTime = 0;
    int       gWednesdayOffTime = 0;
    int       gThursdayEnable = 0;
    int       gThursdayOnTime = 0;
    int       gThursdayOffTime = 0;
    int       gFridayEnable = 0;
    int       gFridayOnTime = 0;
    int       gFridayOffTime = 0;
    int       gSaturdayEnable = 0;
    int       gSaturdayOnTime = 0;
    int       gSaturdayOffTime = 0;
    int       gSundayEnable = 0;
    int       gSundayOnTime = 0;
    int       gSundayOffTime = 0;

    FILE       *gSaveFile = NULL;
    LAMEOptions gLAMEOptions ={};
    int         gLAMEHighpassFlag = 0;
    int         gLAMELowpassFlag = 0;

    int       oggflag;
    int       serialno;
#ifdef HAVE_VORBIS
    ogg_sync_state oy_stream ={};
    ogg_packet     header_main_save ={};
    ogg_packet     header_comments_save ={};
    ogg_packet     header_codebooks_save ={};
#endif
    bool      ice2songChange = false;
    int       in_header = 0;
    long      written = 0;
    int       vuShow = 0;

    int       gLAMEpreset = 0;
    char_t    gLAMEbasicpreset[255] ={};
    char_t    gLAMEaltpreset[255] ={};
    char_t    gSongTitle[1024] ={};
    char_t    gManualSongTitle[1024] ={};
    int       gLockSongTitle = 0;
    int gNumEncoders;  // TODO: make static s_NumEncoders

    res_state resampler ={};
    int       initializedResampler = 0;
    void      (*sourceURLCallback)     ( void *, void * ) = NULL;
    void      (*destURLCallback)       ( void *, void * ) = NULL;
    void      (*serverStatusCallback)  ( void *, void * ) = NULL;
    void      (*generalStatusCallback) ( void *, void * ) = NULL;
    void      (*writeBytesCallback)    ( void *, void * ) = NULL;
    void      (*serverTypeCallback)    ( void *, void * ) = NULL;
    void      (*serverNameCallback)    ( void *, void * ) = NULL;
    void      (*streamTypeCallback)    ( void *, void * ) = NULL;
    void      (*bitrateCallback)       ( void *, void * ) = NULL;
    void( *VUCallback )            (double, double, double, double) = NULL;
    long      startTime = 0;
    long      endTime = 0;
    char_t    sourceDescription[255] ={};
    char_t    gServerType[25] ={};

#ifdef _WIN32
    //WAVEFORMATEX waveFormat ={};
    //HWAVEIN      inHandle = NULL;
    //WAVEHDR      WAVbuffer1 ={};
    //WAVEHDR      WAVbuffer2 ={};
#else
    //int inHandle = 0; // for advanced recording
#endif

    unsigned long result = 0;
    //short int     WAVsamplesbuffer1[1152*2] ={};
    //short int     WAVsamplesbuffer2[1152*2] ={};
    char_t        gAdvRecDevice[255] ={};
#ifndef _WIN32
    char_t        gAdvRecServerTitle[255] = {};
#endif
    int           gLiveInSamplerate = 0;

#ifdef _WIN32
    // These are for the LAME DLL
    BEINITSTREAM     beInitStream = NULL;
    BEENCODECHUNK    beEncodeChunk = NULL;
    BEDEINITSTREAM   beDeinitStream = NULL;
    BECLOSESTREAM    beCloseStream = NULL;
    BEVERSION        beVersion = NULL;
    BEWRITEVBRHEADER beWriteVBRHeader = NULL;
    HINSTANCE        hDLL = NULL;
    HINSTANCE        hFAACDLL = NULL;
    HINSTANCE        hAACPDLL = NULL;
    HINSTANCE        hFHGAACPDLL = NULL;
    DWORD            dwSamples = 0;
    DWORD            dwMP3Buffer = 0;
    HBE_STREAM       hbeStream = 0;
#endif

    char_t    gConfigFileName[255] ={};
    char_t    gOggEncoderText[255] ={};
    int       gForceStop = 0;
    char_t    gCurrentRecordingName[1024] ={};
    long      lastX = 0;
    long      lastY = 0;

#ifdef HAVE_VORBIS  // TODO: these things are candidates for child classes
    ogg_stream_state os ={};
    vorbis_dsp_state vd ={};
    vorbis_block     vb ={};
    vorbis_info      m_VorbisInfo ={};
#endif

    int       frontEndType = 0;
    int       ReconnectTrigger = 0;

#ifdef HAVE_AACP
    CREATEAUDIO3TYPE   fhCreateAudio3 = NULL;
    GETAUDIOTYPES3TYPE fhGetAudioTypes3 = NULL;

    AudioCoder *(*fhFinishAudio3)    ( char_t *fn, AudioCoder *c ) = NULL;
    void        (*fhPrepareToFinish) ( const char_t *filename, AudioCoder *coder ) = NULL;
    AudioCoder *fhaacpEncoder;

    CREATEAUDIO3TYPE   CreateAudio3 = NULL;
    GETAUDIOTYPES3TYPE GetAudioTypes3 = NULL;

    AudioCoder *(*finishAudio3)(char_t *fn, AudioCoder *c) = NULL;
    void( *PrepareToFinish )(const char_t *filename, AudioCoder *coder) = NULL;
    AudioCoder * aacpEncoder = NULL;
#endif

#ifdef HAVE_FAAC
    faacEncHandle aacEncoder = NULL;
#endif

    unsigned long samplesInput = 0, maxBytesOutput = 0;
    float    *faacFIFO = NULL;
    long      faacFIFOendpos = 0;
    char_t    gAACQuality[25] ={};
    char_t    gAACCutoff[25] ={};
    int       encoderNumber = 0;
    bool      forcedDisconnect = false;
    time_t    forcedDisconnectSecs = 0;
    int       autoconnect = 0;
    char_t    externalMetadata[255] ={};
    char_t    externalURL[255] ={};
    char_t    externalFile[255] ={};
    char_t    externalInterval[25] ={};
    char_t   *vorbisComments[30] ={};
    int       numVorbisComments = 0;
    char_t    outputControl[255] ={};
    char_t    metadataAppendString[255] ={};
    char_t    metadataRemoveStringBefore[255] ={};
    char_t    metadataRemoveStringAfter[255] ={};
    char_t    metadataWindowClass[255] ={};
    bool      metadataWindowClassInd = false;
#ifdef HAVE_FLAC
    FLAC__StreamEncoder  *flacEncoder = NULL;
    FLAC__StreamMetadata *flacMetadata = NULL;
    int       flacFailure = 0;
#endif
    char_t   *configVariables[255] ={};
    int       numConfigVariables = 0;
    pthread_mutex_t mutex ={};

    char_t    WindowsRecSubDevice[255] ={};
    char_t    WindowsRecDevice[255] ={};

    int       LAMEJointStereoFlag = 0;
    int       gForceDSPrecording = 0;
    int       gThreeHourBug = 0;
    int       gSkipCloseWarning = 0;
    int       gAsioRate = 0;
    //CBUFFER circularBuffer;
};

#define shuicastGlobals CEncoder  // TODO: replace everywhere and add functions as methods

int     do_encoding( CEncoder *g, short int *samples, int numsamples, Limiters * limiter = NULL );
int     updateSongTitle( CEncoder *g, int forceURL );
int     setCurrentSongTitleURL( CEncoder *g, char_t *song );
void    icecast2SendMetadata( CEncoder *g );
int     ogg_encode_dataout( CEncoder *g );
int     trimVariable( char_t *variable );
int     readConfigFile( CEncoder *g, int readOnly = 0 );
int     writeConfigFile( CEncoder *g );
void    ErrorMessage( char_t *title, char_t *fmt, ... );
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
void    setDumpData( CEncoder *g, int dump );
void    setConfigFileName( CEncoder *g, char_t *configFile );
char_t *getConfigFileName( CEncoder *g );
char_t *getServerDesc( CEncoder *g );
int     getReconnectFlag( CEncoder *g );
int     getReconnectSecs( CEncoder *g );
int     getIsConnected( CEncoder *g );
int     resetResampler( CEncoder *g );
void    setOggEncoderText( CEncoder *g, char_t *text );
char_t *getCurrentRecordingName( CEncoder *g );
void    setCurrentRecordingName( CEncoder *g, char_t *name );
void    setForceStop( CEncoder *g, int forceStop );
long    getLastXWindow( CEncoder *g );
long    getLastYWindow( CEncoder *g );
void    setLastXWindow( CEncoder *g, long x );
void    setLastYWindow( CEncoder *g, long y );
int     getFrontEndType( CEncoder *g );
void    setFrontEndType( CEncoder *g, int x );
int     getReconnectTrigger( CEncoder *g );
void    setReconnectTrigger( CEncoder *g, int x );
long    GetConfigVariableLong( CEncoder *g, char_t *appName, char_t *paramName, long defaultvalue, char_t *desc );
char_t *getLockedMetadata( CEncoder *g );
void    setLockedMetadata( CEncoder *g, char_t *buf );
int     getLockedMetadataFlag( CEncoder *g );
void    setLockedMetadataFlag( CEncoder *g, int flag );
void    setSaveDirectory( CEncoder *g, char_t *saveDir );
char_t *getSaveDirectory( CEncoder *g );
char_t *getgLogFile( CEncoder *g );
void    setgLogFile( CEncoder *g, char_t *logFile );
FILE   *getSaveFileP( CEncoder *g );
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
void    addDSPONLYsettings( CEncoder *g );
void    addStandaloneONLYsettings( CEncoder *g );
void    addUISettings( CEncoder *g );
void    addASIOSettings( CEncoder *g );
void    setDefaultLogFileName( char_t *filename );
void    setConfigDir( char_t *dirname );
void    LogMessage( CEncoder *g, int type, char_t *source, int line, char_t *fmt, ... );
char_t *getWindowsRecordingDevice( CEncoder *g );
void    setWindowsRecordingDevice( CEncoder *g, char_t *device );
char_t *getWindowsRecordingSubDevice( CEncoder *g );
void    setWindowsRecordingSubDevice( CEncoder *g, char_t *device );
int     triggerDisconnect( CEncoder *g );
int     getAppdata( bool checkonly, int locn, DWORD flags, LPCSTR subdir, LPCSTR configname, LPSTR strdestn );
bool    testLocal( LPCSTR dir, LPCSTR file );
