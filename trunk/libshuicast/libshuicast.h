#pragma once
#pragma warning( disable : 4351 )  // default initialization of array members

#ifdef _WIN32
#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>
#endif

//#include <stdio.h>
//#include <time.h>
#include <pthread.h>

#ifndef _WIN32
#include <sys/ioctl.h>
#else
#include <mmsystem.h>
#endif

#ifdef _DMALLOC_
#include <dmalloc.h>
#endif

#include "libshuicast_cbuffer.h"
#include "libshuicast_limiters.h"
#include "libshuicast_socket.h"
#include "libshuicast_resample.h"
#ifdef HAVE_VORBIS
#include <vorbis/vorbisenc.h>
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
#include <FLAC/metadata.h>
#endif

#ifndef FALSE
#define FALSE false
#endif
#ifndef TRUE
#define TRUE true
#endif

typedef enum
{
    ENCODER_NONE,
    ENCODER_LAME,
    ENCODER_OGG,
    ENCODER_FLAC,
    ENCODER_AAC,
    ENCODER_AACP_HE,
    ENCODER_AACP_HE_HIGH,
    ENCODER_AACP_LC,
    ENCODER_FG_AACP_AUTO,
    ENCODER_FG_AACP_LC,
    ENCODER_FG_AACP_HE,
    ENCODER_FG_AACP_HEV2
}
EncoderType;

typedef enum
{
    SERVER_NONE,
    SERVER_SHOUTCAST,
    SERVER_ICECAST,
    SERVER_ICECAST2
}
ServerType;

typedef struct
{
    int     cbrflag;
    int     out_samplerate;
    int     quality;
#ifdef _WIN32
    int     mode;
#elif defined HAVE_LAME
    MPEG_mode   mode;
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

#if 0

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

#define FormatID 'fmt '   /* chunkID for Format Chunk. NOTE: There is a space at the end of this ID. */

typedef struct
{
    char_t  chunkID[4];
    long    chunkSize;
    short * waveformData;
}
DataChunk;

#endif

typedef struct
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
}
WavHeader;


class CEncoder
{
public:

    CEncoder( int encoderNumber );
    ~CEncoder();

    int     Load ();
    void    LoadConfig ();
    void    StoreConfig ();
    void    SetConfigDir ( char_t *dirname );
    int     ReadConfigFile ( const int readOnly = 0 );
    int     WriteConfigFile ();
    void    AddConfigVariable ( char_t *variable );
    void    SetConfigFileName ( char_t *configFile );
    void    GetConfigVariable     ( char_t *appName, char_t *paramName, char_t *defaultvalue, char_t *destValue, int destSize, char_t *desc );
    long    GetConfigVariableLong ( char_t *appName, char_t *paramName, long defaultvalue, char_t *desc );  // TODO: remove Long and use overloading
    void    PutConfigVariable     ( char_t *appName, char_t *paramName, char_t *destValue );
    void    PutConfigVariableLong ( char_t *appName, char_t *paramName, long value );
    void    DeleteConfigFile ();
    void    SetSaveDirectory ( char_t *saveDir );  // TODO: use
    char_t *GetSaveDirectory ();
    int     OpenArchiveFile ();
    void    CloseArchiveFile ();
    void    SetDefaultLogFileName ( char_t *filename );
    char_t *GetLogFile ();
    void    SetLogFile ( char_t *logFile );
    void    LogMessage ( int type, char_t *source, int line, char_t *fmt, ... );

    int     ConnectToServer ();
    int     DisconnectFromServer ();
    int     TriggerDisconnect ();
    int     SendToServer ( int sd, char_t *data, int length, int type );
    int     UpdateSongTitle ( int forceURL );
    void    GetCurrentSongTitle ( char_t *song, char_t *artist, char_t *full ) const;
    int     SetCurrentSongTitle ( char_t *song );
    char_t *GetLockedMetadata ();
    void    SetLockedMetadata ( char_t *buf );
    void    Icecast2SendMetadata ();
    void    AddVorbisComment ( char_t *comment );
    void    FreeVorbisComments ();
    int     OggEncodeDataout ();
    int     DoEncoding ( float *samples, int numsamples, Limiters *limiter = NULL );
    int     HandleOutput ( float *samples, int nsamples, int nchannels, int in_samplerate, int asioChannel = -1, int asioChannel2 = -1 );
    int     HandleOutputFast ( Limiters *limiter, int dataoffset = 0 );
    int     ConvertAudio ( float *in_samples, float *out_samples, int num_in_samples, int num_out_samples );
    int     InitResampler ( long inSampleRate, long inNCH );
    int     ResetResampler ();
    void    AddToFIFO ( float *samples, int numsamples );

    char_t *GetWindowsRecordingDevice ();
    void    SetWindowsRecordingDevice ( char_t *device );
    char_t *GetWindowsRecordingSubDevice ();
    void    SetWindowsRecordingSubDevice ( char_t *device );

    void    AddUISettings           ();
    void    AddBasicEncoderSettings ();
    void    AddMultiEncoderSettings ();
    void    AddDSPSettings          ();
    void    AddStandaloneSettings   ();
    void    AddASIOSettings         ();

    void    ReplaceString ( char_t *source, char_t *dest, char_t *from, char_t *to ) const;
    char_t* URLize ( char_t *input ) const;

    inline long GetCurrentSamplerate () const
    {
        return m_CurrentSamplerate;
    }

    inline int GetCurrentBitrate () const
    {
        return m_CurrentBitrate;
    }

    inline int GetCurrentChannels () const
    {
        return m_CurrentChannels;
    }

    inline double GetAttenuation () const
    {
        return m_Attenuation;
    }

    inline int IsConnected () const
    {
        return m_IsConnected;
    }

    inline long GetLastWindowPosX () const
    {
        return lastX;
    }

    inline long GetLastWindowPosY () const
    {
        return lastY;
    }

    inline void SetLastWindowPos ( const long x, const long y )
    {
        lastX = x;
        lastY = y;
    }

    inline void SetLimiterFlag ( const int flag )
    {
        m_Limiter = flag;
    }

    inline void SetLimiterValues ( const int db, const int pre, const int gain )
    {
        m_LimitdB  = db;
        m_LimitPre = pre;
        m_GaindB   = gain;
    }

    inline void SetStartMinimizedFlag ( const int flag )
    {
        m_StartMinimized = flag;
    }

    inline int GetStartMinimizedFlag () const
    {
        return m_StartMinimized;
    }

    inline void SetAutoConnect ( const int flag )
    {
        m_AutoConnect = flag;
    }

    inline int GetAutoConnect() const
    {
        return m_AutoConnect;
    }

    inline void SetForceStop ( const bool forceStop )
    {
        m_ForceStop = forceStop;
    }

    inline int SkipCloseWarning () const
    {
        return gSkipCloseWarning;
    }

    inline int GetLockedMetadataFlag () const  // TODO: rename GetXXXFlag to IsXXX
    {
        return gLockSongTitle;
    }

    inline void SetLockedMetadataFlag ( const int flag ) 
    {
        gLockSongTitle = flag;
    }

#if 0
    inline int GetLiveInSamplerate () const
    {
        return gLiveInSamplerate;
    }

    inline void SetLiveInSamplerate ( const int rate )
    {
        gLiveInSamplerate = rate;
    }
#endif

    inline void SetDestURLCallback ( void( *pCallback ) ( void*, void* ) )
    {
        destURLCallback = pCallback;
    }

    inline void SetSourceURLCallback ( void( *pCallback ) ( void*, void* ) )
    {
        sourceURLCallback = pCallback;
    }

    inline void SetServerStatusCallback ( void( *pCallback ) ( void*, void* ) )
    {
        serverStatusCallback = pCallback;
    }

    inline void SetGeneralStatusCallback ( void( *pCallback ) ( void*, void* ) )
    {
        generalStatusCallback = pCallback;
    }

    inline void SetWriteBytesCallback ( void( *pCallback ) ( void*, void* ) )
    {
        writeBytesCallback = pCallback;
    }

    inline void SetServerTypeCallback ( void( *pCallback ) ( void*, void* ) )
    {
        serverTypeCallback = pCallback;
    }

    inline void SetServerNameCallback ( void( *pCallback ) ( void*, void* ) )
    {
        serverNameCallback = pCallback;
    }

    inline void SetStreamTypeCallback ( void( *pCallback ) ( void*, void* ) )
    {
        streamTypeCallback = pCallback;
    }

    inline void SetBitrateCallback ( void( *pCallback ) ( void*, void* ) )
    {
        bitrateCallback = pCallback;
    }

    inline void SetVUCallback ( void( *pCallback ) ( double, double, double, double ) )
    {
        VUCallback = pCallback;
    }

#define DAY_SCHEDULE(dow) \
    private: \
        int       m_##dow##Enable  = 0; \
        int       m_##dow##OnTime  = 0; \
        int       m_##dow##OffTime = 0; \
    public: \
        inline int Get##dow##Enable () const \
        { \
	        return m_##dow##Enable; \
        } \
        inline int Get##dow##OnTime () const \
        { \
	        return m_##dow##OnTime; \
        } \
        inline int Get##dow##OffTime () const \
        { \
	        return m_##dow##OffTime; \
        } \
        inline void Get##dow##Enable ( const int val ) \
        { \
	        m_##dow##Enable = val; \
        } \
        inline void Get##dow##OnTime ( const int val ) \
        { \
	        m_##dow##OnTime = val; \
        } \
        inline void Get##dow##OffTime ( const int val ) \
        { \
	        m_##dow##OffTime = val; \
        }

    DAY_SCHEDULE( Monday    )
    DAY_SCHEDULE( Tuesday   )
    DAY_SCHEDULE( Wednesday )
    DAY_SCHEDULE( Thursday  )
    DAY_SCHEDULE( Friday    )
    DAY_SCHEDULE( Saturday  )
    DAY_SCHEDULE( Sunday    )

    EncoderType m_Type       = ENCODER_NONE;
    ServerType  m_ServerType = SERVER_NONE;

    long      m_CurrentSamplerate    = 0;  // TODO: add m_ for all, make private
    int       m_CurrentBitrate       = 0;
    int       m_CurrentBitrateMin    = 0;
    int       m_CurrentBitrateMax    = 0;
    int       m_CurrentChannels      = 0;
    char_t    m_AttenuationTable[30] = {};
    double    m_Attenuation          = 0;
    int       m_SCSocket             = 0;
    int       m_SCSocketCtrl         = 0;
    CMySocket m_DataChannel;
    CMySocket m_CtrlChannel;

    int       m_SCFlag               = 0;
    char_t    m_SourceURL[1024]      = {};
    char_t    m_Server[256]          = {};
    char_t    m_Port[10]             = {};
    char_t    m_Password[256]        = {};
    int       m_IsConnected          = 0;
    char_t    m_CurrentSong[1024]    = {};
    int       m_PubServ              = 0;
    char_t    m_ServIRC[20]          = {};
    char_t    m_ServICQ[20]          = {};
    char_t    m_ServAIM[20]          = {};
    char_t    m_ServURL[1024]        = {};
    char_t    m_ServDesc[1024]       = {};
    char_t    m_ServName[1024]       = {};
    char_t    m_ServGenre[100]       = {};
    char_t    m_Mountpoint[100]      = {};
private:
    int       m_AutoConnect          = 0;  // is used
    int       m_AutoReconnect        = 0;  // TODO
public:  // TODO
    int       m_ReconnectSec         = 0;
#ifndef _WIN32
#ifdef HAVE_LAME
    lame_global_flags *m_LameGlobalFlags = NULL;
#endif
#endif
    bool      m_CurrentlyEncoding    = false;
    char_t    m_OggQuality[25]       = {};
    int       m_LiveRecordingFlag    = 0;
    int       m_Limiter              = 0;
    int       m_LimitPre             = 0;
    int       m_LimitdB              = 0;
    int       m_GaindB               = 0;
private:
    int       m_StartMinimized       = 0;
public:  // TODO
    int       m_OggBitQualFlag       = 0;
private:
    char_t    m_OggBitQual[40]       = {};
public:  // TODO
    char_t    m_EncodeType[25]       = {};
    char_t    m_SaveDirectory[1024]  = {};
    char_t    m_LogFile[1024]        = {};
    int       m_LogLevel             = 0;
private:
    FILE     *m_LogFilePtr           = NULL;
public:  // TODO
    int       m_SaveDirectoryFlag    = 0;
    int       m_SaveAsWAV            = 0;
    FILE     *m_SaveFilePtr          = NULL;
    int       m_AsioSelectChannel    = 0;
    char_t    m_AsioChannel[255]     = {};
    int       m_EnableScheduler      = 0;

#ifdef HAVE_LAME
    LAMEOptions gLAMEOptions ={};
    int         gLAMEHighpassFlag = 0;
    int         gLAMELowpassFlag = 0;
#endif

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
    char_t    gSongTitle[1024] ={};  // TODO: must have same length as m_CurrentSong!
    char_t    gManualSongTitle[1024] ={};
    int       gLockSongTitle = 0;
    int gNumEncoders;  // TODO: make static s_NumEncoders

    res_state resampler ={};
    int       initializedResampler = 0;
    void( *sourceURLCallback )     (void *, void *) = NULL;
    void( *destURLCallback )       (void *, void *) = NULL;
    void( *serverStatusCallback )  (void *, void *) = NULL;
    void( *generalStatusCallback ) (void *, void *) = NULL;
    void( *writeBytesCallback )    (void *, void *) = NULL;
    void( *serverTypeCallback )    (void *, void *) = NULL;
    void( *serverNameCallback )    (void *, void *) = NULL;
    void( *streamTypeCallback )    (void *, void *) = NULL;
    void( *bitrateCallback )       (void *, void *) = NULL;
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

    //unsigned long result = 0;
    //short int     WAVsamplesbuffer1[1152*2] ={};
    //short int     WAVsamplesbuffer2[1152*2] ={};
    char_t        gAdvRecDevice[255] ={};
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
    bool      m_ForceStop = false;
    long      lastX = 0;
    long      lastY = 0;

#ifdef HAVE_VORBIS  // TODO: these things are candidates for child classes
    ogg_stream_state os ={};
    vorbis_dsp_state vd ={};
    vorbis_block     vb ={};
    vorbis_info      m_VorbisInfo ={};
#endif

#ifdef HAVE_AACP
    CREATEAUDIO3TYPE   fhCreateAudio3 = NULL;
    GETAUDIOTYPES3TYPE fhGetAudioTypes3 = NULL;

    AudioCoder *(*fhFinishAudio3)    (char_t *fn, AudioCoder *c) = NULL;
    void( *fhPrepareToFinish ) (const char_t *filename, AudioCoder *coder) = NULL;
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

    WavHeader wav_header;
};

int     getAppdata( bool checkonly, int locn, DWORD flags, LPCSTR subdir, LPCSTR configname, LPSTR strdestn );
bool    testLocal( LPCSTR dir, LPCSTR file );
