/*$T MainWindow.cpp GC 1.140 10/23/05 09:48:26 */

/*
https://github.com/Guik/edcast-reborn
https://github.com/DustyDrifter/AltaCast
https://web.archive.org/web/20101126080704/http://svn.oddsock.org:80/public/trunk/
*/

#include "stdafx.h"
#include "shuicast.h"
#include "MainWindow.h"
#include <process.h>
#include <bass.h>
#include <math.h>
#include <afxinet.h>
#include "About.h"
#include "SystemTray.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char				THIS_FILE[] = __FILE__;
#endif

CMainWindow   *pWindow;
Limiters      *limiter = NULL;
CEncoder      *gEncoders[MAX_ENCODERS];
CEncoder       gMain( 0 );
unsigned int   shuicastThread = 0;
int            m_BASSOpen = 0;
HRECORD        inRecHandle;
HDC            specdc = 0;
HBITMAP        specbmp = 0;
BYTE          *specbuf;
DWORD          timer = 0;
char           currentConfigDir[MAX_PATH] = "";

extern char    logPrefix[255];
extern int     shuicast_init ( CEncoder *encoder );

static UINT BASED_CODE	indicators[] = { ID_STATUSPANE };

// mutex - next 4 only needed if 3hr bug persists!! bass_2.4.x.dll should fix it
bool audio_mutex_inited = false;
pthread_mutex_t audio_mutex;
DWORD totalLength = 0;
bool needRestart = false;

extern "C"
{
    unsigned int __stdcall startThread ( void *obj )
    {
        CMainWindow *pWindow = (CMainWindow *)obj;
        pWindow->startshuicast( -1 );
        //Begin patch for multiple cpu
        HANDLE hProc = GetCurrentProcess();//Gets the current process handle
        DWORD procMask;
        DWORD sysMask;
        HANDLE hDup;
        DuplicateHandle( hProc, hProc, hProc, &hDup, 0, FALSE, DUPLICATE_SAME_ACCESS );
        GetProcessAffinityMask( hDup, &procMask, &sysMask );//Gets the current process affinity mask
        DWORD newMask = 2;//new Mask, uses only the first CPU
        BOOL res = SetProcessAffinityMask( hDup, (DWORD_PTR)newMask );//Set the affinity mask for the process
        //end patch multiple cpu 
        //_endthread();
        return(1);
    }

    unsigned int __stdcall startSpecificThread ( void *obj )
    {
        int		enc = (int)obj;
        // CMainWindow *pWindow = (CMainWindow *)obj;
        int		ret = pWindow->startshuicast( enc );
        gEncoders[enc]->m_ForcedDisconnectSecs = time( NULL );
        //Begin patch multiple cpu
        HANDLE hProc = GetCurrentProcess();//Gets the current process handle
        DWORD procMask;
        DWORD sysMask;
        HANDLE hDup;
        DuplicateHandle( hProc, hProc, hProc, &hDup, 0, FALSE, DUPLICATE_SAME_ACCESS );
        GetProcessAffinityMask( hDup, &procMask, &sysMask );//Gets the current process affinity mask
        DWORD newMask = 2;//new Mask, uses only the first CPU
        BOOL res = SetProcessAffinityMask( hDup, (DWORD_PTR)newMask );//Set the affinity mask for the process
        //End patch multiple cpu 
        //_endthread();
        return(1);
    }
}

VOID CALLBACK ReconnectTimer ( HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime )
{
    time_t	currentTime;
    currentTime = time( NULL );
    for ( int i = 0; i < gMain.GetNumEncoders(); i++ )
    {
        if ( gEncoders[i]->m_ForcedDisconnect )
        {
            int timeout = gEncoders[i]->m_ReconnectSec;  // TODO: use m_AutoReconnect, too
            time_t timediff = currentTime - gEncoders[i]->m_ForcedDisconnectSecs;
            if ( timediff > timeout )
            {
                gEncoders[i]->m_ForcedDisconnect = false;
                _beginthreadex( NULL, 0, startSpecificThread, (void *)i, 0, &shuicastThread );
                //Begin patch multiple cpu
                HANDLE hProc = GetCurrentProcess();//Gets the current process handle
                DWORD procMask;
                DWORD sysMask;
                HANDLE hDup;
                DuplicateHandle( hProc, hProc, hProc, &hDup, 0, FALSE, DUPLICATE_SAME_ACCESS );
                GetProcessAffinityMask( hDup, &procMask, &sysMask );//Gets the current process affinity mask
                DWORD newMask = 2;//new Mask, uses only the first CPU
                BOOL res = SetProcessAffinityMask( hDup, (DWORD_PTR)newMask );//Set the affinity mask for the process
                //End patch multiple cpu 
            }
            else
            {
                char buf[255] = "";
                wsprintf( buf, "Connecting in %d seconds", timeout - timediff );
                pWindow->outputStatusCallback( i + 1, buf, FILE_LINE );
            }
        }
    }
}

VOID CALLBACK MetadataTimer ( HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime )
{
    if ( !strcmp( gMain.externalMetadata, "FILE" ) )
    {
        FILE *filep = fopen( gMain.externalFile, "r" );
        if ( !filep )
        {
            char	buf[1024] = "";
            wsprintf( buf, "Cannot open metadata file (%s)", gMain.externalFile );
            pWindow->generalStatusCallback( buf, FILE_LINE );
        }
        else
        {
            char	buffer[1024];
            memset( buffer, '\000', sizeof( buffer ) );
            fgets( buffer, sizeof( buffer ) - 1, filep );
            char	*p1;
            p1 = strstr( buffer, "\r\n" );
            if ( p1 ) *p1 = '\000';
            p1 = strstr( buffer, "\n" );
            if ( p1 ) *p1 = '\000';
            fclose( filep );
            setMetadata( buffer );
        }
    }

    if ( !strcmp( gMain.externalMetadata, "URL" ) )
    {
        char szCause[255];
        TRY
        {
            CInternetSession	session( "shuicast" );
            CStdioFile			*file = NULL;
            file = session.OpenURL( gMain.externalURL, 1, INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE );
            if ( file == NULL )
            {
                char	buf[1024] = "";
                wsprintf( buf, "Cannot open metadata URL (%s)", gMain.externalURL );
                pWindow->generalStatusCallback( buf, FILE_LINE );
            }
            else
            {
                CString metadata;
                file->ReadString( metadata );
                setMetadata( (char *)LPCSTR( metadata ) );
                file->Close();
                delete file;
            }

            session.Close();
            //delete session;
        }
        CATCH_ALL ( error )
        {
            error->GetErrorMessage( szCause, 254, NULL );
            pWindow->generalStatusCallback( szCause, FILE_LINE );
        }
        END_CATCH_ALL;
    }
}

#define UNICODE
VOID CALLBACK MetadataCheckTimer ( HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime )
{
    static char lastWindowTitleString[4096] = "";
#ifndef _DEBUG
    pWindow->KillTimer( 5 );
#endif
    if ( gMain.metadataWindowClassInd )
    {
        if ( strlen( gMain.metadataWindowClass ) > 0 )
        {
            HWND winHandle = FindWindow( gMain.metadataWindowClass, NULL );
            if ( winHandle )
            {
                char buf[1024] = "";
                GetWindowText( winHandle, buf, sizeof( buf ) - 1 );
                if ( strcmp( buf, lastWindowTitleString ) )
                {
                    setMetadata( buf );
                    strcpy( lastWindowTitleString, buf );
                }
            }
        }
    }
    pWindow->SetTimer( 5, 1000, (TIMERPROC)MetadataCheckTimer );
    if ( needRestart ) pWindow->DoStartRecording( true );
}
#undef UNICODE

VOID CALLBACK AutoConnectTimer ( HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime )
{
    pWindow->DoConnect();
    pWindow->generalStatusCallback( "AutoConnect", FILE_LINE );
#ifndef _DEBUG
    pWindow->KillTimer( pWindow->autoconnectTimerId );
#endif
}

void freeComment ()
{
    for ( int i = 0; i < gMain.GetNumEncoders(); i++ )
    {
        gEncoders[i]->FreeVorbisComments();
    }
}

void addComment ( char *comment )
{
    for ( int i = 0; i < gMain.GetNumEncoders(); i++ )
    {
        if ( gEncoders[i]->IsEncoderType( ENCODER_OGG ) )
        {
            if ( gEncoders[i]->IsConnected() ) gEncoders[i]->AddVorbisComment( comment );
        }
    }
}

int handleAllOutput ( float *samples, int nsamples, int nchannels, int in_samplerate )
{
    if ( nchannels > 2 )
    {
        if ( limiter == NULL )
        {
            limiter = (Limiters *) new limitMultiMono( nchannels );
        }
        if ( pWindow->m_Limiter )
        {
            limiter->multiLimit( samples, nsamples, in_samplerate, NULL, NULL, NULL );//0.55, -3.0);
        }
        else
        {
            limiter->multiLimit( samples, nsamples, in_samplerate, NULL, NULL, NULL );//0.0, 20.0);
        }
    }
    else
    {
        if ( limiter != NULL )
        {
            //if(nchannels != limiter->channels)  // TODO: BassWindow
            if ( (nchannels == 1 && limiter->sourceIsStereo) || (nchannels == 2 && !limiter->sourceIsStereo) )
            {
                delete limiter;
                limiter = NULL;
            }
        }

        if ( limiter == NULL )
        {
            if ( nchannels == 1 )
            {
                limiter = (Limiters *) new limitMonoToStereoMono();
            }
            else if ( nchannels == 2 )
            {
                limiter = (Limiters *) new limitStereoToStereoMono();
            }
            else
            {
                limiter = (Limiters *) new limitMultiMono( nchannels );
            }
        }

        if ( pWindow->m_Limiter )
        {
            limiter->limit( samples, nsamples, in_samplerate, pWindow->m_limitpre, pWindow->m_limitdb, pWindow->m_gaindb );
        }
        else
        {
            limiter->limit( samples, nsamples, in_samplerate, 0.0, 20.0, 0.0 );
        }
    }

#if 1  // edcast-reborn

    if ( gMain.GetVUMeterType() == 2 )
    {
        static int showPeakL = -60;
        static int showPeakR = -60;
        static int holdPeakL = 10;
        static int holdPeakR = 10;

        if ( holdPeakL > 0 ) holdPeakL--;
        if ( holdPeakR > 0 ) holdPeakR--;

        showPeakL = showPeakL - (holdPeakL ? 0 : 2);
        showPeakR = showPeakR - (holdPeakR ? 0 : 2);
        if ( showPeakL < (int)limiter->PeakL )
        {
            showPeakL = (int)limiter->PeakL;
            holdPeakL = 10;
        }
        if ( showPeakR < (int)limiter->PeakR )
        {
            showPeakR = (int)limiter->PeakR;
            holdPeakR = 10;
        }
        pWindow->UpdatePeak( (int)limiter->PeakL + 60, (int)limiter->PeakR + 60, showPeakL + 60, showPeakR + 60 );
    }
    else pWindow->UpdatePeak( (int)limiter->RmsL + 60, (int)limiter->RmsR + 60, (int)limiter->PeakL + 60, (int)limiter->PeakR + 60 );

#else  // altacast

    long	ileftMax = 0;
    long	irightMax = 0;
    long	leftMax = 0;
    long	rightMax = 0;
    double	sumLeft = 0.0;
    double	sumRight = 0.0;
    if ( nchannels == 2 )
    {
        for ( int i = 0; i < nsamples * 2; i = i + 2 )
        {
            ileftMax = abs( (int)((float)samples[i] * 32767.f) );
            irightMax = abs( (int)((float)samples[i + 1] * 32767.f) );
            sumLeft = sumLeft + (ileftMax * ileftMax);
            sumRight = sumRight + (irightMax * irightMax);
            if ( ileftMax > leftMax ) leftMax = ileftMax;
            if ( irightMax > rightMax ) rightMax = irightMax;
        }
    }
    else
    {
        for ( int i = 0; i < nsamples; i = i + 1 )
        {
            ileftMax = abs( (int)((float)samples[i] * 32767.f) );
            irightMax = ileftMax;
            sumLeft = sumLeft + (ileftMax * ileftMax);
            sumRight = sumRight + (irightMax * irightMax);
            if ( ileftMax > leftMax ) leftMax = ileftMax;
            if ( irightMax > rightMax ) rightMax = irightMax;
        }
    }

    double RMSLeft = sqrt( sumLeft );
    double RMSRight = sqrt( sumRight );
    double newPeakL = (double)20 * log10( (double)leftMax / 32768.0 );
    double newPeakR = (double)20 * log10( (double)rightMax / 32768.0 );
    double newRmsL = (double)20 * log10( (double)RMSLeft/32768.0 );
    double newRmsR = (double)20 * log10( (double)RMSRight/32768.0 );
    if ( gMain.GetVUMeterType() == 2 ) UpdatePeak( (int)newPeakL + 60, (int)newPeakR + 60, 0, 0 );
    else UpdatePeak( (int)newRmsL + 60, (int)newRmsR + 60, (int)newPeakL + 60, (int)newPeakR + 60 );

#endif

    for ( int i = 0; i < gMain.GetNumEncoders(); i++ )
    {
        if ( gEncoders[i] )
        {
#ifdef MULTIASIO
#ifdef MONOASIO
            gEncoders[i]->HandleOutputFast( limiter, getChannelFromName( gEncoders[i]->m_AsioChannel, 0 ) );
#else
            gEncoders[i]->HandleOutput( samples, nsamples, nchannels, in_samplerate, getChannelFromName( gEncoders[i]->m_AsioChannel, 0 ), getChannelFromName( gEncoders[i]->gAsioChannel, 0 ) + 1 );
#endif
#else
            //if(!pWindow->m_LiveRecRunning || gEncoders[i]->m_ForceDSPRecording) // flagged for always DSP
            //if(!gEncoders[i]->m_ForceDSPRecording) // check if gEncoders[i] flagged for AlwaysDSP
            if ( pWindow->m_Limiter ) gEncoders[i]->HandleOutputFast( limiter );
            else gEncoders[i]->HandleOutput( samples, nsamples, nchannels, in_samplerate );
#endif
        }
    }

    {
#ifdef DSPPROCESS  // TODO: when exactly is this needed?
        float * src = samples;
        short int *dst = short_samples;

        for ( int i=0; i<nsamples; i++ )
        {
            float sample = *(src++);
            if ( sample < -1.0 ) sample = -1.0;
            else if ( sample > 1.0 ) sample = 1.0;
            *(dst++) = (short int)(sample * 32767.0);
            if ( nchannels > 1 )
            {
                sample = *(src++);
                if ( sample < -1.0 ) sample = -1.0;
                else if ( sample > 1.0 ) sample = 1.0;
                *(dst++) = (short int)(sample * 32767.0);
            }
            else
            {
                src++;
            }
            for ( int j = 2; j < nchannels; j++ )
            {
                dst++;
            }
        }
#else
        //long sizedata = (nchannels * nsamples) * sizeof(float);
        //CopyMemory(samples, limiter->outputStereo, sizedata);
#endif
    }

    return 1;
}

void CMainWindow::UpdatePeak ( int rmsL, int rmsR, int peakL, int peakR )
{
    flexmeters.GetMeterInfoObject( 0 )->value = rmsL;
    flexmeters.GetMeterInfoObject( 1 )->value = rmsR;
    flexmeters.GetMeterInfoObject( 0 )->peak = peakL;
    flexmeters.GetMeterInfoObject( 1 )->peak = peakR;
}

BOOL LiveRecordingCheck ()
{
    return pWindow->m_LiveRecRunning;
}

bool HaveEncoderAlwaysDSP ()
{
    for ( int i = 0; i < gMain.GetNumEncoders(); i++ )
    {
        if ( gEncoders[i]->m_ForceDSPRecording ) // flagged for always DSP
        {
            return true;
        }
    }
    return false;
}

int getLastX ()
{
    return gMain.GetLastWindowPosX();
}

int getLastY ()
{
    return gMain.GetLastWindowPosY();
}

void writeMainConfig ()
{
    gMain.WriteConfigFile();
}

int initializeshuicast ()
{
    char    currentlogFile[1024] = "";
    wsprintf( currentlogFile, "%s\\%s", currentConfigDir, logPrefix );

    gMain.SetDefaultLogFileName( currentlogFile );
    gMain.SetLogFile( currentlogFile );
    gMain.SetConfigFileName( currentlogFile );

    gMain.AddUISettings();
#ifdef SHUICASTSTANDALONE
    gMain.AddStandaloneSettings();
#endif
    return shuicast_init( &gMain );
}

void setMetadataFromMediaPlayer ( char *metadata )
{
    if ( !gMain.metadataWindowClassInd )
    {
        if ( !strcmp( gMain.externalMetadata, "DISABLED" ) )
        {
            setMetadata( metadata );
        }
    }
}

void setMetadata ( char *metadata )
{
    char  modifiedSong[4096] = "";
    char  modifiedSongBuffer[4096] = "";
    char *pData;

    strcpy( modifiedSong, metadata );
    if ( strlen( gMain.metadataRemoveStringAfter ) > 0 ) {
        char *p1 = strstr( modifiedSong, gMain.metadataRemoveStringAfter );
        if ( p1 ) *p1 = '\000';
    }

    if ( strlen( gMain.metadataRemoveStringBefore ) > 0 ) {
        char	*p1 = strstr( modifiedSong, gMain.metadataRemoveStringBefore );
        if ( p1 )
        {
            memset( modifiedSongBuffer, '\000', sizeof( modifiedSongBuffer ) );
            strcpy( modifiedSongBuffer, p1 + strlen( gMain.metadataRemoveStringBefore ) );
            strcpy( modifiedSong, modifiedSongBuffer );
        }
    }

    if ( strlen( gMain.metadataAppendString ) > 0 )
    {
        strcat( modifiedSong, gMain.metadataAppendString );
    }

    pData = modifiedSong;
    pWindow->m_Metadata = modifiedSong;
    pWindow->inputMetadataCallback( 0, (void *)pData, FILE_LINE );

    for ( int i = 0; i < gMain.GetNumEncoders(); i++ )
    {
        if ( gMain.GetLockedMetadataFlag() )
        {
            if ( gEncoders[i]->SetCurrentSongTitle( (char *)gMain.GetLockedMetadata() ) )
            {
                pWindow->inputMetadataCallback( i, (void *)gMain.GetLockedMetadata(), FILE_LINE );
            }
        }
        else
        {
            if ( gEncoders[i]->SetCurrentSongTitle( (char *)pData ) )
            {
                pWindow->inputMetadataCallback( i, (void *)pData, FILE_LINE );
            }
        }
    }
}

bool getDirName ( LPCSTR inDir, LPSTR dst, int lvl=1 )
{
    // inDir = ...\winamp\Plugins
    char * dir = _strdup( inDir );
    bool retval = false;
    // remove trailing slash

    if ( dir[strlen( dir )-1] == '\\' )
    {
        dir[strlen( dir )-1] = '\0';
    }

    for ( char *p1 = dir + strlen( dir ) - 1; p1 >= dir; p1-- )
    {
        if ( *p1 == '\\' )
        {
            if ( --lvl > 0 )
            {
                *p1 = '\0';
            }
            else
            {
                strcpy( dst, p1 );
                retval = true;
                break;
            }
        }
    }
    free( dir );
    return retval;
}

#if USE_NEW_CONFIG
void LoadConfigs ( char *currentDir, char *subdir, bool dsp )
#else
void LoadConfigs ( char *currentDir, char *logFile )
#endif
{
    char	configFile[1024] = "";
    char	currentlogFile[1024] = "";

#if USE_NEW_CONFIG
    char tmpfile[MAX_PATH] = "";
    char tmp2file[MAX_PATH] = "";

    char cfgfile[MAX_PATH];
    wsprintf( cfgfile, "%s_0.cfg", logPrefix );

    bool canUseCurrent = gMain.CheckLocalDir( currentDir, NULL );
    bool hasCurrentData = false;
    bool hasAppData = false;
    bool canUseAppData = false;
    bool hasProgramData = false;
    bool canUseProgramData = false;
    if ( canUseCurrent )
    {
        hasCurrentData = gMain.CheckLocalDir( currentDir, cfgfile );
    }
    if ( !hasCurrentData )
    {
        int iHasAppData = -1;
        if ( dsp )
        {
            int iHasWinampDir = -1;
            int iHasPluginDir = -1;
            //int iHasEdcastDir = -1;
            //int iHasEdcastCfg = -1;
            char wasubdir[MAX_PATH] = "";
            char wa_instance[MAX_PATH] = "";

            getDirName( currentDir, wa_instance, 2 ); //...../{winamp name}/plugins - we want {winamp name}
            wsprintf( wasubdir, "%s", wa_instance );
            iHasWinampDir = gMain.GetAppData( false, CSIDL_APPDATA, SHGFP_TYPE_CURRENT, wasubdir, cfgfile, tmpfile );
            if ( iHasWinampDir < 2 )
            {
                wsprintf( wasubdir, "%s\\Plugins", wa_instance );
                iHasPluginDir = GetAppData( false, CSIDL_APPDATA, SHGFP_TYPE_CURRENT, wasubdir, cfgfile, tmpfile );
                if ( iHasPluginDir < 2 )
                {
                    wsprintf( wasubdir, "%s\\Plugins\\%s", wa_instance, subdir );
                    iHasAppData = GetAppData( true, CSIDL_APPDATA, SHGFP_TYPE_CURRENT, wasubdir, cfgfile, tmpfile );
                }
            }
        }
        else
        {
            // TODO: look for shuicast instance name - our path will be like C:\Program Files\{this instance name}
            iHasAppData = GetAppData( true, CSIDL_LOCAL_APPDATA, SHGFP_TYPE_CURRENT, subdir, cfgfile, tmpfile );
        }
        hasAppData = (iHasAppData == 0);
        canUseAppData = (iHasAppData < 2);
        if ( !hasAppData )
        {
            int iHasProgramData = GetAppData( true, CSIDL_COMMON_APPDATA, SHGFP_TYPE_CURRENT, subdir, cfgfile, tmp2file );
            hasProgramData = (iHasProgramData == 0);
            canUseProgramData = (iHasProgramData < 2);
        }
    }
    if ( hasAppData && hasCurrentData )
    {
        // tmpfile already has the right value
    }
    else if ( hasAppData )
    {
        // tmpfile already has the right value
    }
    else if ( hasCurrentData )
    {
        strcpy( tmpfile, currentDir );
    }
    else if ( canUseAppData )
    {
        // tmpfile alread has the right value
    }
    else if ( canUseProgramData )
    {
        strcpy( tmpfile, tmp2file );
    }
    else if ( canUseCurrent )
    {
        strcpy( tmpfile, currentDir );
    }
    else
    {
        // fail!!
    }
    strcpy( currentConfigDir, tmpfile );
#else
    strcpy( currentConfigDir, currentDir );
#endif

    wsprintf( configFile, "%s\\%s", currentConfigDir, logPrefix );
    wsprintf( currentlogFile, "%s\\%s", currentConfigDir, logPrefix );

    gMain.SetDefaultLogFileName( currentlogFile );
    gMain.SetLogFile( currentlogFile );
    gMain.SetConfigDir( currentConfigDir );
    gMain.SetConfigFileName( configFile );
    gMain.AddUISettings();
#ifdef SHUICASTSTANDALONE
    gMain.AddStandaloneSettings();
#endif
    gMain.ReadConfigFile();
}

// We usually handle 1 or 2 channels of data here. However, when ALL is selected as a channel, we are in
// multi source mode. Each encoder specifies a source channel(s) (1 or 2) to encode
#if ( BASSVERSION == 0x203 )
BOOL CALLBACK BASSwaveInputProc ( HRECORD handle, const void *buffer, DWORD length, DWORD user )
#else  // BASSVERSION == 0x204
BOOL CALLBACK BASSwaveInputProc ( HRECORD handle, const void *buffer, DWORD length, void *user )
#endif
{
    int n;
    char *name;
    static char currentDevice[1024] = "";
    char *selectedDevice = gMain.GetWindowsRecordingSubDevice();

    if ( gMain.m_ThreeHourBug )  // remove whole block when 3hr bug gone
    {
        pthread_mutex_lock( &audio_mutex );
        totalLength += length;
        if ( totalLength > 0xFF000000UL ) needRestart = true;
    }

    if ( pWindow->m_LiveRecRunning )
    {
        char *firstEnabledDevice = NULL;
        BOOL foundDevice = FALSE;
        BOOL foundSelectedDevice = FALSE;
        for ( n = 0; name = (char *)BASS_RecordGetInputName( n ); n++ )
        {
#if ( BASSVERSION == 0x203 )
            int s = BASS_RecordGetInput( n );
#else  // BASSVERSION == 0x204
            float currentVolume;
            int s = BASS_RecordGetInput( n, &currentVolume );
#endif
            if ( !(s & BASS_INPUT_OFF) )
            {
                if ( !firstEnabledDevice ) firstEnabledDevice = name;
                if ( !strcmp( currentDevice, name ) ) foundDevice = TRUE;
                if ( !strcmp( selectedDevice, name ) ) foundSelectedDevice = TRUE;
            }
        }
        if ( foundSelectedDevice )
        {
            strcpy( currentDevice, selectedDevice );
            if ( !foundDevice )
            {
                char	msg[255] = "";
                wsprintf( msg, "Recording from %s", currentDevice );
                pWindow->generalStatusCallback( (void *)msg, FILE_LINE );
            }
        }
        else if ( !foundDevice && firstEnabledDevice )
        {
            strcpy( currentDevice, firstEnabledDevice );
            char	msg[255] = "";
            wsprintf( msg, "Recording from %s (first enabled device)", currentDevice );
            pWindow->generalStatusCallback( (void *)msg, FILE_LINE );
        }

        unsigned int c_size = length;	/* in bytes. */
        int          nch    = 2;
        int          srate  = 44100;
        float       *samples;

#if USE_BASS_FLOAT
        int numsamples = c_size / sizeof( float );
        samples = (float *)buffer;
        handleAllOutput( samples, numsamples / nch, nch, srate );
#else
        short *z = (short *)buffer;	/* signed short for pcm data. */
        int numsamples = c_size / sizeof( short );
        samples = (float *)malloc( sizeof( float ) * numsamples * 2 );
        memset( samples, '\000', sizeof( float ) * numsamples * 2 );

        long avgLeft = 0;
        long avgRight = 0;
        int  flip = 0;

        for ( int i = 0; i < numsamples; i++ )
        {
            signed int	sample;
            sample = z[i];
            samples[i] = sample / 32767.f;

            /* clipping */
            if ( samples[i] >  1.0 ) samples[i] =  1.0;
            if ( samples[i] < -1.0 ) samples[i] = -1.0;
        }

        handleAllOutput( samples, numsamples / nch, nch, srate );  // TODO: mix into DSP samples
        free( samples );
#endif

        if ( gMain.m_ThreeHourBug ) pthread_mutex_unlock( &audio_mutex ); // remove when 3hr bug gone
        return 1;
    }

    if ( gMain.m_ThreeHourBug ) pthread_mutex_unlock( &audio_mutex );
    return 0;
}

/*
=======================================================================================================================
#define SPECWIDTH	76						// display width
#define SPECHEIGHT	30						// height (changing requires palette adjustments too)
void CALLBACK UpdateSpectrum( UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2 ) {
    HDC dc;
int x,y;
float fft[512];
// get the FFT data BASS_ChannelGetData(i nRecHandle,fft,BASS_DATA_FFT1024);
int b0=0;
memset(specbuf,0,SPECWIDTH*SPECHEIGHT);
#define BANDS 15 for (x=0;
x<BANDS;
x++) { float sum=0;
int sc,b1=pow(2,x*10.0/(BANDS-1));
if (b1>511) b1=511;
if (b1<=b0) b1=b0+1;
// make sure it uses at least 1 FFT bin sc=10+b1-b0;
for (;
b0<b1;
b0++) sum+=fft[1+b0];
y=(sqrt(sum/log10(sc))*1.7*SPECHEIGHT)-4;
// scale it if (y>SPECHEIGHT) y=SPECHEIGHT;
// cap it while (--y>=0) memset(specbuf+y*SPECWIDTH+x*(SPECWIDTH/BANDS),y+1,0.9*(SPECWIDTH/BANDS));
// draw bar } // update the display dc=GetDC(pWindow->m_hWnd);
//int bmpPos = (int)(pWindow->gWindowHeight / 2.475);
BitBlt(dc,315,70,SPECWIDTH,SPECHEIGHT,specdc,0,0,SRCCOPY);
ReleaseDC(pWindow->m_hWnd,dc);
}
=======================================================================================================================
*/

void CMainWindow::DoStartRecording ( bool restart ) // only needed if 3hr bug persists - BASS_RecordStart can be placed in StartRecording function
{
    if ( gMain.m_ThreeHourBug )
    {
        if ( !audio_mutex_inited )
        {
            audio_mutex_inited = true;
            pthread_mutex_init( &audio_mutex, NULL );
        }
        if ( restart )
        {
            pthread_mutex_lock( &audio_mutex );
            int dev = BASS_RecordGetDevice();
            BASS_RecordFree();
            BASS_RecordInit( dev );
            pthread_mutex_unlock( &audio_mutex );
        }
        needRestart = false;
        totalLength = 0UL;
    }
#if ( BASSVERSION == 0x203 )
    inRecHandle = BASS_RecordStart( 44100, 2, MAKELONG( 0, 25 ), BASSwaveInputProc, NULL );
#else  // BASSVERSION == 0x204
#if USE_BASS_FLOAT
    inRecHandle = BASS_RecordStart( 44100, 2, BASS_SAMPLE_FLOAT, &BASSwaveInputProc, NULL );
#else
    inRecHandle = BASS_RecordStart( 44100, 2, 0, &BASSwaveInputProc, NULL );
#endif
#endif
}

void CMainWindow::StopRecording ()
{
    BASS_ChannelStop( inRecHandle );
    BASS_RecordFree();
    m_BASSOpen = 0;
    m_LiveRecRunning = FALSE;
}

int CMainWindow::StartRecording ()
{
    int ret = BASS_RecordInit( m_CurrentInputCard );
    m_BASSOpen = 1;

    if ( !ret )
    {
        DWORD errorCode = BASS_ErrorGetCode();
        switch ( errorCode ) {
        case BASS_ERROR_ALREADY:
            pWindow->generalStatusCallback( (char *) "Recording device already opened!", FILE_LINE );
            return 0;
        case BASS_ERROR_DEVICE:
            pWindow->generalStatusCallback( (char *) "Recording device invalid!", FILE_LINE );
            return 0;
        case BASS_ERROR_DRIVER:
            pWindow->generalStatusCallback( (char *) "Recording device driver unavailable!", FILE_LINE );
            return 0;
        default:
            pWindow->generalStatusCallback( (char *) "There was an error opening the preferred Digital Audio In device!", FILE_LINE );
            return 0;
        }
    }

    DoStartRecording( false );
    int n = 0;
    char *name;

#if ( BASSVERSION == 0x204 )
    BASS_DEVICEINFO bInfo;
    BASS_RecordGetDeviceInfo( m_CurrentInputCard, &bInfo );
#endif

    for ( n = 0; name = (char *)BASS_RecordGetInputName( n ); n++ )
    {
#if ( BASSVERSION == 0x203 )
        int s = BASS_RecordGetInput( n );
#else  // BASSVERSION == 0x204
        float vol = 0.0;
        int s = BASS_RecordGetInput( n, &vol );
#endif
        if ( !(s & BASS_INPUT_OFF) )
        {
            char	msg[255] = "";
#if ( BASSVERSION == 0x203 )
            wsprintf( msg, "Recording from %s", name );
#else  // BASSVERSION == 0x204
            wsprintf( msg, "Recording from %s/%s", bInfo.name, name );
#endif
            pWindow->generalStatusCallback( (void *)msg, FILE_LINE );
        }
    }

    pWindow->m_LiveRecRunning = TRUE;
    return 1;
}

/* CMainWindow dialog */
const char	*kpcTrayNotificationMsg_ = "shuicast";

CMainWindow::CMainWindow( CWnd *pParent /* NULL */ ) :
    CDialog( CMainWindow::IDD, pParent ), bMinimized_( false ), pTrayIcon_( 0 ), nTrayNotificationMsg_( RegisterWindowMessage( kpcTrayNotificationMsg_ ) )
{
    //{{AFX_DATA_INIT(CMainWindow)
    m_Bitrate           = _T( "" );
    m_Destination       = _T( "" );
    m_Bandwidth         = _T( "" );
    m_Metadata          = _T( "" );
    m_ServerDescription = _T( "" );
    m_LiveRec           = FALSE;
    m_LiveRecRunning    = FALSE;
    m_RecDevices        = _T( "" );
    m_RecCards          = _T( "" );
    m_RecVolume         = 0;
    m_AutoConnect       = FALSE;
    m_startMinimized    = FALSE;
    m_Limiter           = FALSE;
    m_LiveMix           = FALSE;
    m_StaticStatus      = _T( "" );
    //}}AFX_DATA_INIT
    hIcon_ = AfxGetApp()->LoadIcon( IDR_MAINFRAME );
    memset( gEncoders, 0, sizeof( gEncoders ) );  // zeroes the pointers
    m_BASSOpen = 0;  // TODO: is no member yet
    pWindow = this;
    memset( m_currentDir, '\000', sizeof( m_currentDir ) );
    strcpy( m_currentDir, "." );
    gMain.LogMessage( LOG_DEBUG, "CMainWindow complete" );
}

CMainWindow::~CMainWindow ()
{
    for ( int i = 0; i < MAX_ENCODERS; i++ )
    {
        if ( gEncoders[i] ) delete gEncoders[i];
        gEncoders[i] = NULL;
    }
}

void CMainWindow::InitializeWindow ()
{
    configDialog = new CConfig();
    configDialog->Create( (UINT)IDD_CONFIG, this );
    configDialog->parentDialog = this;

    editMetadata = new CEditMetadata();
    editMetadata->Create( (UINT)IDD_EDIT_METADATA, this );
    editMetadata->parentDialog = this;

    aboutBox = new CAbout();
    aboutBox->Create( (UINT)IDD_ABOUT, this );
    gMain.LogMessage( LOG_DEBUG, "CMainWindow::InitializeWindow complete" );
}

LRESULT CMainWindow::startMinimized ( WPARAM wParam, LPARAM lParam )
{
    if ( gMain.GetStartMinimizedFlag() )
    {
        bMinimized_ = true;
        SetupTrayIcon();
        SetupTaskBarButton();
    }
    return 0L;
}

void CMainWindow::DoDataExchange ( CDataExchange *pDX )
{
    CDialog::DoDataExchange( pDX );
    //{{AFX_DATA_MAP(CMainWindow)
    DDX_Control( pDX, IDC_RECCARDS, m_RecCardsCtrl );
    DDX_Control( pDX, IDC_ASIORATE, m_AsioRateCtrl );
    DDX_Control( pDX, IDC_VUONOFF, m_OnOff );
    DDX_Control( pDX, IDC_METER_PEAK, m_MeterPeak );
    DDX_Control( pDX, IDC_METER_RMS, m_MeterRMS );
    DDX_Control( pDX, IDC_AUTOCONNECT, m_AutoConnectCtrl );
    DDX_Control( pDX, IDC_MINIM, m_startMinimizedCtrl );
    DDX_Control( pDX, IDC_LIMITER, m_LimiterCtrl );
    DDX_Control( pDX, IDC_LIVEMIX, m_LiveMixCtrl );
    DDX_Control( pDX, IDC_RECVOLUME, m_RecVolumeCtrl );
    DDX_Control( pDX, IDC_RECDEVICES, m_RecDevicesCtrl );
    DDX_Control( pDX, IDC_CONNECT, m_ConnectCtrl );
    DDX_Control( pDX, IDC_LIVEREC, m_LiveRecCtrl );
    DDX_Control( pDX, IDC_ENCODERS, m_Encoders );
    DDX_Text( pDX, IDC_METADATA, m_Metadata );
    DDX_Check( pDX, IDC_LIVEREC, m_LiveRec );
    DDX_CBString( pDX, IDC_RECDEVICES, m_RecDevices );
    DDX_CBString( pDX, IDC_RECCARDS, m_RecCards );
    DDX_CBString( pDX, IDC_ASIORATE, m_AsioRate );
    DDX_Slider( pDX, IDC_RECVOLUME, m_RecVolume );
    DDX_Check( pDX, IDC_AUTOCONNECT, m_AutoConnect );
    DDX_Check( pDX, IDC_LIMITER, m_Limiter );
    DDX_Check( pDX, IDC_LIVEMIX, m_LiveMix );
    DDX_Check( pDX, IDC_MINIM, m_startMinimized );
    DDX_Text( pDX, IDC_STATIC_STATUS, m_StaticStatus );
    DDX_Control( pDX, IDC_STATIC_STATUS, m_StaticStatusCtrl );
    DDX_Control( pDX, IDC_LIMITDB, m_limitdbCtrl );
    DDX_Slider( pDX, IDC_LIMITDB, m_limitdb );
    DDX_Text( pDX, IDC_LIMITDBTEXT, m_staticLimitdb );
    DDX_Control( pDX, IDC_GAINDB, m_gaindbCtrl );
    DDX_Slider( pDX, IDC_GAINDB, m_gaindb );
    DDX_Text( pDX, IDC_GAINDBTEXT, m_staticGaindb );
    DDX_Control( pDX, IDC_LIMITPREEMPH, m_limitpreCtrl );
    DDX_Slider( pDX, IDC_LIMITPREEMPH, m_limitpre );
    DDX_Text( pDX, IDC_LIMITPREEMPHTEXT, m_staticLimitpre );
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP ( CMainWindow, CDialog )
    //{{AFX_MSG_MAP(CMainWindow)
    ON_BN_CLICKED( IDC_CONNECT, OnConnect )
    ON_BN_CLICKED( IDC_ADD_ENCODER, OnAddEncoder )
    ON_NOTIFY( NM_DBLCLK, IDC_ENCODERS, OnDblclkEncoders )
    ON_NOTIFY( NM_RCLICK, IDC_ENCODERS, OnRclickEncoders )
    ON_COMMAND( ID_POPUP_CONFIGURE, OnPopupConfigure )
    ON_COMMAND( ID_POPUP_CONNECT, OnPopupConnect )
    ON_BN_CLICKED( IDC_LIVEREC, OnLiveRec )
    ON_BN_CLICKED( IDC_LIMITER, OnLimiter )
    ON_BN_CLICKED( IDC_LIVEMIX, OnLiveMix )
    ON_BN_CLICKED( IDC_MINIM, OnStartMinimized )
    ON_COMMAND( ID_POPUP_DELETE, OnPopupDelete )
    ON_CBN_SELCHANGE( IDC_RECDEVICES, OnSelchangeRecdevices )
    ON_WM_HSCROLL()
    ON_BN_CLICKED( IDC_MANUALEDIT_METADATA, OnManualeditMetadata )
    ON_WM_CLOSE()
    ON_WM_DESTROY()
    ON_COMMAND( ID_ABOUT_ABOUT, OnAboutAbout )
    ON_COMMAND( ID_ABOUT_HELP, OnAboutHelp )
    ON_WM_PAINT()
    ON_WM_TIMER()
    ON_BN_CLICKED( IDC_METER, OnMeter )
    ON_NOTIFY( LVN_KEYDOWN, IDC_ENCODERS, OnKeydownEncoders )
    ON_BN_CLICKED( IDC_BUTTON1, OnButton1 )
    ON_NOTIFY( NM_SETFOCUS, IDC_ENCODERS, OnSetfocusEncoders )
    ON_WM_QUERYDRAGICON()
    ON_CBN_SELCHANGE( IDC_RECCARDS, OnSelchangeReccards )
    ON_CBN_SELCHANGE( IDC_ASIORATE, OnSelchangeAsioRate )
    //}}AFX_MSG_MAP
    ON_WM_SYSCOMMAND()
    ON_COMMAND( IDI_RESTORE, OnSTRestore )
    ON_MESSAGE( WM_MY_MESSAGE, startMinimized )
    ON_MESSAGE( WM_SHOWWINDOW, gotShowWindow )
END_MESSAGE_MAP()

void CMainWindow::generalStatusCallback ( void *pValue, char *source, int line )
{
    gMain.LogMessage( LM_INFO, source, line, "%s", (char *)pValue );
    SetDlgItemText( IDC_STATIC_STATUS, (char *)pValue );
    m_StaticStatus = (char*)pValue;
}

void CMainWindow::inputMetadataCallback ( int enc, void *pValue, char *source, int line )
{
    SetDlgItemText( IDC_METADATA, (char *)pValue );
    if ( enc == 0 )
    {
        gMain.LogMessage( LM_INFO, source, line, "%s", (char *)pValue );
    }
    else
    {
        gEncoders[enc-1]->LogMessage( LM_INFO, source, line, "%s", (char *)pValue );
    }
}

void CMainWindow::outputStatusCallback ( int enc, void *pValue, char *source, int line, bool bSendToLog )
{
    if ( enc != 0 )
    {
        if ( bSendToLog )
        {
            gEncoders[enc-1]->LogMessage( LM_INFO, source, line, "%s", (char *)pValue );
        }
        else
        {
            gEncoders[enc-1]->LogMessage( LM_DEBUG, source, line, "%s", (char *)pValue );
        }

        int numItems = m_Encoders.GetItemCount();
        if ( enc - 1 >= numItems )
        {
            m_Encoders.InsertItem( enc - 1, (char *) "" );
        }

        m_Encoders.SetItemText( enc - 1, 1, (char *)pValue );
    }
}

void CMainWindow::outputMountCallback ( int enc, void *pValue )
{
    if ( enc != 0 )
    {
        int numItems = m_Encoders.GetItemCount();
        if ( enc - 1 >= numItems )
        {
            m_Encoders.InsertItem( enc - 1, (char *) "" );
        }
        m_Encoders.SetItemText( enc - 1, 2, (char *)pValue );
    }
}

void CMainWindow::outputChannelCallback ( int enc, void *pValue )
{
    if ( enc != 0 )
    {
        int numItems = m_Encoders.GetItemCount();
        if ( enc - 1 >= numItems )
        {
            m_Encoders.InsertItem( enc - 1, (char *) "" );
        }
        m_Encoders.SetItemText( enc - 1, 3, (char *)pValue );
    }
}

void CMainWindow::writeBytesCallback ( int enc, void *pValue )  // pValue is a long
{
    static long bytesWrittenInterval[MAX_ENCODERS];  // TODO: into CEncoder?
    static long totalBytesWritten[MAX_ENCODERS];
    static int	initted = 0;
    char		kBPSstr[255] = "";

    if ( !initted )
    {
        initted = 1;
        memset( &bytesWrittenInterval, '\000', sizeof( bytesWrittenInterval ) );
        memset( &totalBytesWritten, '\000', sizeof( totalBytesWritten ) );
    }

    if ( enc != 0 )
    {
        int		enc_index = enc - 1;
        long	bytesWritten = (long)pValue;

        if ( bytesWritten == -1 )
        {
            strcpy( kBPSstr, "" );
            outputStatusCallback( enc, kBPSstr, FILE_LINE );
            gEncoders[enc_index]->m_StartTime = 0;  // TODO: Get/Set/Reset methods
            return;
        }

        if ( gEncoders[enc_index]->m_StartTime == 0 )
        {
            gEncoders[enc_index]->m_StartTime = time( NULL );
            bytesWrittenInterval[enc_index] = 0;
        }

        bytesWrittenInterval[enc_index] += bytesWritten;
        totalBytesWritten[enc_index] += bytesWritten;
        gEncoders[enc_index]->m_EndTime = time( NULL );

        if ( (gEncoders[enc_index]->m_EndTime - gEncoders[enc_index]->m_StartTime) > 4 )
        {
            time_t bytespersec = bytesWrittenInterval[enc_index] / (gEncoders[enc_index]->m_EndTime - gEncoders[enc_index]->m_StartTime);
            long   kBPS = (long)((bytespersec * 8) / 1000);
            if ( strlen( gEncoders[enc_index]->m_Mountpoint ) > 0 ) wsprintf( kBPSstr, "%ld Kbps (%s)", kBPS, gEncoders[enc_index]->m_Mountpoint );
            else wsprintf( kBPSstr, "%ld Kbps", kBPS );
            outputStatusCallback( enc, kBPSstr, FILE_LINE, false );
            gEncoders[enc_index]->m_StartTime = 0;
        }
    }
}

void CMainWindow::outputServerNameCallback ( int enc, void *pValue )
{
    //SetDlgItemText(IDC_SERVER_DESC, (char *)pValue);
}

void CMainWindow::outputBitrateCallback ( int enc, void *pValue )
{
    if ( enc != 0 )
    {
        int numItems = m_Encoders.GetItemCount();
        if ( enc - 1 >= numItems )
        {
            m_Encoders.InsertItem( enc - 1, (char *)pValue );
        }
        else
        {
            m_Encoders.SetItemText( enc - 1, 0, (char *)pValue );
        }
    }
}

void CMainWindow::outputStreamURLCallback ( int enc, void *pValue )
{
    //SetDlgItemText(IDC_DESTINATION_LOCATION, (char *)pValue);
}

void CMainWindow::stopshuicast ()
{
    for ( int i = 0; i < gMain.GetNumEncoders(); i++ )
    {
        gEncoders[i]->SetForceStop( true );
        gEncoders[i]->DisconnectFromServer();
        gEncoders[i]->m_ForcedDisconnect = false;
    }
}

int CMainWindow::startshuicast ( int which )
{
    if ( which == -1 )
    {
        for ( int i = 0; i < gMain.GetNumEncoders(); i++ )
        {
            if ( !gEncoders[i]->IsConnected() )
            {
                gEncoders[i]->SetForceStop( false );
                if ( !gEncoders[i]->ConnectToServer() )
                {
                    gEncoders[i]->m_ForcedDisconnect = true;
                    continue;
                }
            }
        }
    }
    else if ( !gEncoders[which]->IsConnected() )
    {
        gEncoders[which]->SetForceStop( false );
        int ret = gEncoders[which]->ConnectToServer();
        if ( ret == 0 )
        {
            gEncoders[which]->m_ForcedDisconnect = true;
        }
    }

    return 1;
}

void CMainWindow::DoConnect ()
{
    OnConnect();
    generalStatusCallback( "AutoConnect", FILE_LINE );
    KillTimer( autoconnectTimerId );
}

void CMainWindow::OnConnect ()
{
    static bool connected = false;
    if ( !connected )
    {
        _beginthreadex( NULL, 0, startThread, (void *) this, 0, &shuicastThread );
        //Begin patch multiple cpu
        HANDLE hProc = GetCurrentProcess();//Gets the current process handle
        DWORD procMask;
        DWORD sysMask;
        HANDLE hDup;
        DuplicateHandle( hProc, hProc, hProc, &hDup, 0, FALSE, DUPLICATE_SAME_ACCESS );
        GetProcessAffinityMask( hDup, &procMask, &sysMask );//Gets the current process affinity mask
        DWORD newMask = 2;//new Mask, uses only the first CPU
        BOOL res = SetProcessAffinityMask( hDup, (DWORD_PTR)newMask );//Set the affinity mask for the process
        //End patch multiple cpu 

        connected = true;
        m_ConnectCtrl.SetWindowText( "Disconnect" );
#ifndef _DEBUG
        KillTimer( 2 );
#endif
        reconnectTimerId = SetTimer( 2, 1000, (TIMERPROC)ReconnectTimer );
    }
    else
    {
        stopshuicast();
        connected = false;
        m_ConnectCtrl.SetWindowText( "Connect" );
#ifndef _DEBUG
        KillTimer( 2 );
#endif
    }
}

void CMainWindow::OnAddEncoder ()
{
    int orig_index = gMain.GetNumEncoders();
    gEncoders[orig_index] = new CEncoder( orig_index + 1 );
    char currentlogFile[1024] = "";  // TODO: put this all in constructor
    wsprintf( currentlogFile, "%s\\%s_%d", currentConfigDir, logPrefix, gEncoders[orig_index]->encoderNumber );
    gMain.SetDefaultLogFileName( currentlogFile );
    gEncoders[orig_index]->SetLogFile( currentlogFile );
    gEncoders[orig_index]->SetConfigFileName( gMain.m_ConfigFileName );
    gMain.IncNumEncoders();
    gEncoders[orig_index]->AddBasicEncoderSettings();
#ifndef SHUICASTSTANDALONE
    gEncoders[orig_index]->AddDSPSettings();
#endif
    shuicast_init( gEncoders[orig_index] );
}

BOOL CMainWindow::OnInitDialog ()
{
    CDialog::OnInitDialog();
    UpdateData( TRUE );
    //SetWindowText("shuicast");
    RECT	rect;
    rect.left = 340;
    rect.top = 190;
    m_Encoders.InsertColumn( 0, "Encoder Settings" );
    m_Encoders.InsertColumn( 1, "Transfer Rate" );
    m_Encoders.InsertColumn( 2, "Mount" );
    m_Encoders.SetColumnWidth( 0, 190 );
    m_Encoders.SetColumnWidth( 1, 110 );
    m_Encoders.SetColumnWidth( 2, 96 );
    m_Encoders.SetExtendedStyle( LVS_EX_FULLROWSELECT );
    m_Encoders.SendMessage( LB_SETTABSTOPS, 0, NULL );
    liveRecOn.LoadBitmap( IDB_LIVE_ON );
    liveRecOff.LoadBitmap( IDB_LIVE_OFF );

#ifdef SHUICASTSTANDALONE
    gMain.m_LiveRecordingFlag = 1;
#endif
    m_LiveRec = gMain.m_LiveRecordingFlag;
    if ( m_LiveRec ) m_LiveRecCtrl.SetBitmap( HBITMAP( liveRecOn ) );
    else m_LiveRecCtrl.SetBitmap( HBITMAP( liveRecOff ) );

    for ( int i = 0; i < gMain.GetNumEncoders(); i++ )
    {
        if ( !gEncoders[i] ) gEncoders[i] = new CEncoder( i + 1 );
        char currentlogFile[1024] = "";  // TODO: put this all in constructor
        wsprintf( currentlogFile, "%s\\%s_%d", currentConfigDir, logPrefix, gEncoders[i]->encoderNumber );
        gMain.SetDefaultLogFileName( currentlogFile );
        gEncoders[i]->SetLogFile( currentlogFile );
        gEncoders[i]->SetConfigFileName( gMain.m_ConfigFileName );
        gEncoders[i]->AddBasicEncoderSettings();
#ifndef SHUICASTSTANDALONE
        gEncoders[i]->AddDSPSettings();
#endif
        shuicast_init( gEncoders[i] );
    }

    int		count = 0;	/* the device counter */
    char	*pDesc = (char *)1;

    gMain.LogMessage( LOG_INFO, "Finding recording device" );
    BASS_RecordInit( 0 );
    m_BASSOpen = 1;

    int		n;
    char	*name;
    int		currentInput = 0;

#if ( BASSVERSION == 0x203 )
    for ( int n = 0; name = (char *)BASS_RecordGetDeviceDescription( n ); n++ )
    {
        m_RecCardsCtrl.AddString( name );
        //if ( n == BASS_RecordGetDevice() )
        if ( !strcmp( gMain.GetWindowsRecordingDevice(), "" ) )
        {
            m_RecCards = name;
            m_CurrentInputCard = n;
        }
        else
        {
            if ( !strcmp( gMain.GetWindowsRecordingDevice(), name ) )
            {
                m_RecCards = name;
                m_CurrentInputCard = n;
            }
        }
    }
#else  // BASSVERSION == 0x204
    BASS_DEVICEINFO info;
    for ( int n = 0; BASS_RecordGetDeviceInfo( n, &info ); n++ )
    {
        if ( info.flags&BASS_DEVICE_ENABLED )
        {
            m_RecCardsCtrl.AddString( info.name );
            if ( !strcmp( gMain.GetWindowsRecordingDevice(), "" ) )
            {
                gMain.SetWindowsRecordingDevice( (char *)info.name );
                gMain.LogMessage( LOG_DEBUG, "NO DEVICE CONFIGURED USING : %s", info.name );
                m_RecCards = info.name;
                m_CurrentInputCard = n;
            }
            else
            {
                if ( !strcmp( gMain.GetWindowsRecordingDevice(), info.name ) )
                {
                    gMain.LogMessage( LOG_DEBUG, "FOUND CONFIGURED DEVICE : %s", info.name );
                    m_RecCards = info.name;
                    m_CurrentInputCard = n;
                }
            }
        }
    }
#endif

    BASS_RecordFree();
    BASS_RecordInit( m_CurrentInputCard );
    //BASS_RecordSetDevice(m_CurrentInputCard);
    for ( n = 0; name = (char *)BASS_RecordGetInputName( n ); n++ )
    {
#if ( BASSVERSION == 0x203 )
        int s = BASS_RecordGetInput( n );
#else  // BASSVERSION == 0x204
        float vol = 0.0;
        int s = BASS_RecordGetInput( n, &vol );
#endif
        m_RecDevicesCtrl.AddString( name );
        gMain.LogMessage( LOG_DEBUG, "ADDING INPUT NAME : %s", name );
        if ( !(s & BASS_INPUT_OFF) )
        {
            if ( !strcmp( gMain.GetWindowsRecordingSubDevice(), "" ) )
            {
                m_RecDevices = name;
                gMain.LogMessage( LOG_DEBUG, "NO INPUT SOURCE CONFIGURE - USING : %s", name );
#if ( BASSVERSION == 0x203 )
                m_RecVolume = LOWORD( s );
#else  // BASSVERSION == 0x204
                m_RecVolume = (int)(vol*100);
#endif
                m_CurrentInput = n;
                gMain.SetWindowsRecordingSubDevice( (char *)name );
            }
            else if ( !strcmp( gMain.GetWindowsRecordingSubDevice(), name ) )
            {
                m_RecDevices = name;
                gMain.LogMessage( LOG_DEBUG, "CURRENT INPUT SOURCE : %s", name );
#if ( BASSVERSION == 0x203 )
                m_RecVolume = LOWORD( s );
#else  // BASSVERSION == 0x204
                m_RecVolume = (int)(vol*100);
#endif
                m_CurrentInput = n;
            }
        }
    }

    BASS_RecordFree();
    m_BASSOpen = 0;

    if ( gMain.GetLockedMetadataFlag() )
    {
        m_Metadata = gMain.GetLockedMetadata();
    }

    m_AutoConnect = gMain.GetAutoConnect();
    m_startMinimized = gMain.GetStartMinimizedFlag();
    //m_LiveMix = gMain.m_LiveMix;  // TODO
    m_Limiter = gMain.m_Limiter;
    m_limitdbCtrl.SetRange( -15, 0, TRUE );
    m_gaindbCtrl.SetRange( 0, 20, TRUE );
    m_limitpreCtrl.SetRange( 0, 75, TRUE );
    m_limitdb = gMain.m_LimitdB;
    m_staticLimitdb.Format( "%d", gMain.m_LimitdB );
    m_gaindb = gMain.m_GaindB;
    m_staticGaindb.Format( "%d", m_gaindb );
    m_limitpre = gMain.m_LimitPre;
    m_staticLimitpre.Format( "%d", m_limitpre );
    UpdateData( FALSE );
#ifdef SHUICASTSTANDALONE
    m_LiveRecCtrl.SetBitmap( HBITMAP( liveRecOn ) );
    m_RecDevicesCtrl.EnableWindow( TRUE );
    m_RecCardsCtrl.EnableWindow( TRUE );
    m_RecVolumeCtrl.EnableWindow( TRUE );
    StartRecording();
#endif
    m_AsioRateCtrl.ShowWindow( SW_HIDE );
    UpdateData( FALSE );
    OnLiveRec();
    reconnectTimerId = SetTimer( 2, 1000, (TIMERPROC)ReconnectTimer );  // TODO: use m_AutoReconnect?
    if ( m_AutoConnect )
    {
        char buf[255];
        wsprintf( buf, "AutoConnecting in 5 seconds" );
        generalStatusCallback( buf, FILE_LINE );
        autoconnectTimerId = SetTimer( 3, 5000, (TIMERPROC)AutoConnectTimer );
    }

    int metadataInterval = atoi( gMain.externalInterval );
    metadataInterval = metadataInterval * 1000;
    metadataTimerId = SetTimer( 4, metadataInterval, (TIMERPROC)MetadataTimer );

    SetTimer( 5, 1000, (TIMERPROC)MetadataCheckTimer );

    FlexMeters_InitStruct	*fmis = flexmeters.Initialize_Step1();	/* returns a pointer to a struct, */

    /* with default values filled in. */
    fmis->max_x             = 512;  // buffer size - must be at least as big as the meter window
    fmis->max_y             = 512;  // buffer size - must be at least as big as the meter window
    fmis->hWndFrame         = GetDlgItem( IDC_METER )->m_hWnd;  // the window to grab coordinates from
    fmis->border.left       = 3;    // borders
    fmis->border.right      = 3;
    fmis->border.top        = 3;
    fmis->border.bottom     = 3;
    fmis->meter_count       = 2;    // number of meters
    fmis->horizontal_meters = 1;    // 0 = vertical
    flexmeters.Initialize_Step2( fmis );  // news meter info objects - after this, you must set them up

    for ( int a = 0; a < fmis->meter_count; a++ )
    {
        CFlexMeters_MeterInfo	*pMeterInfo = flexmeters.GetMeterInfoObject( a );
        /* nice gradient */
        pMeterInfo->extra_spacing      = 3;
        pMeterInfo->colour[0].colour   = 0x00FF00;
        pMeterInfo->colour[0].at_value = 0;
        pMeterInfo->colour[1].colour   = 0xFFFF00;
        pMeterInfo->colour[1].at_value = 55;
        pMeterInfo->colour[2].colour   = 0xFF0000;
        pMeterInfo->colour[2].at_value = 58;
        pMeterInfo->colours_used       = 3;
        pMeterInfo->value              = 0;
        pMeterInfo->peak               = 0;
        pMeterInfo->meter_width        = 60;  // BassWindow: 61
        //if(a & 1) pMeterInfo->direction=eMeterDirection_Backwards;
    }

    flexmeters.Initialize_Step3();			/* finalize init. */

    if ( gMain.GetVUMeterType() )
    {
        m_VUStatus = VU_ON;
        m_OnOff.ShowWindow( SW_HIDE );
        if ( gMain.GetVUMeterType() == 1 )
        {
            m_MeterPeak.ShowWindow( SW_HIDE );
            m_MeterRMS.ShowWindow( SW_SHOW );
        }
        else
        {
            m_MeterRMS.ShowWindow( SW_HIDE );
            m_MeterPeak.ShowWindow( SW_SHOW );
        }

    }
    else
    {
        m_VUStatus = VU_OFF;
        m_MeterPeak.ShowWindow( SW_HIDE );
        m_MeterRMS.ShowWindow( SW_HIDE );
        m_OnOff.ShowWindow( SW_SHOW );
    }
    UpdateData( FALSE );
    SetTimer( 73, 50, 0 );					/* set up timer. 20ms=50hz - probably good. */
    //#if SHUICASTSTANDALONE
    //	visible = !gMain.GetStartMinimizedFlag();
    //#else
    //	visible = TRUE;
    //#endif
    return TRUE;							/* return TRUE unless you set the focus to a control */
    /* EXCEPTION: OCX Property Pages should return FALSE */
}

void CMainWindow::OnDblclkEncoders ( NMHDR *pNMHDR, LRESULT *pResult )
{
    OnPopupConfigure();
    *pResult = 0;
}

void CMainWindow::OnRclickEncoders ( NMHDR *pNMHDR, LRESULT *pResult )
{
    int iItem = m_Encoders.GetNextItem( -1, LVNI_SELECTED );
    if ( iItem >= 0 )
    {
        CMenu menu;
        VERIFY( menu.LoadMenu( IDR_CONTEXT ) );

        /* Pop up sub menu 0 */
        CMenu *popup = menu.GetSubMenu( 0 );
        if ( popup )
        {
            if ( gEncoders[iItem]->IsConnected() )
            {
                popup->ModifyMenu( ID_POPUP_CONNECT, MF_BYCOMMAND, ID_POPUP_CONNECT, "Disconnect" );
            }
            else if ( gEncoders[iItem]->m_ForcedDisconnect )
            {
                popup->ModifyMenu( ID_POPUP_CONNECT, MF_BYCOMMAND, ID_POPUP_CONNECT, "Stop AutoConnect" );
            }
            else
            {
                popup->ModifyMenu( ID_POPUP_CONNECT, MF_BYCOMMAND, ID_POPUP_CONNECT, "Connect" );
            }

            POINT pt;
            GetCursorPos( &pt );
            popup->TrackPopupMenu( TPM_LEFTALIGN, pt.x, pt.y, this );
        }
    }

    *pResult = 0;
}

void CMainWindow::OnPopupConfigure ()
{
    int iItem = m_Encoders.GetNextItem( -1, LVNI_SELECTED );
    if ( iItem >= 0 )
    {
        configDialog->GlobalsToDialog( gEncoders[iItem] );
        configDialog->ShowWindow( SW_SHOW );
    }
}

void CMainWindow::OnPopupConnect ()
{
    int iItem = m_Encoders.GetNextItem( -1, LVNI_SELECTED );
    if ( iItem >= 0 )
    {
        if ( !gEncoders[iItem]->IsConnected() )
        {
            if ( gEncoders[iItem]->m_ForcedDisconnect )
            {
                gEncoders[iItem]->m_ForcedDisconnect = false;
                outputStatusCallback( iItem + 1, "AutoConnect stopped.", FILE_LINE );
            }
            else
            {
                m_SpecificEncoder = iItem;
                _beginthreadex( NULL, 0, startSpecificThread, (void *)iItem, 0, &shuicastThread );
                //Begin patch multiple cpu
                HANDLE hProc = GetCurrentProcess();//Gets the current process handle
                DWORD procMask;
                DWORD sysMask;
                HANDLE hDup;
                DuplicateHandle( hProc, hProc, hProc, &hDup, 0, FALSE, DUPLICATE_SAME_ACCESS );
                GetProcessAffinityMask( hDup, &procMask, &sysMask );//Gets the current process affinity mask
                DWORD newMask = 2;//new Mask, uses only the first CPU
                BOOL res = SetProcessAffinityMask( hDup, (DWORD_PTR)newMask );//Set the affinity mask for the process
                //End patch multiple cpu 
            }
        }
        else
        {
            gEncoders[iItem]->DisconnectFromServer();
            gEncoders[iItem]->SetForceStop( true );
            gEncoders[iItem]->m_ForcedDisconnect = false;
        }
    }
}

void CMainWindow::OnLimiter ()
{
    UpdateData( TRUE );
}

void CMainWindow::OnStartMinimized ()
{
    UpdateData( TRUE );
}

void CMainWindow::OnLiveMix()
{
    UpdateData( TRUE );
}

void CMainWindow::OnLiveRec()
{
#ifndef SHUICASTSTANDALONE
    UpdateData( TRUE );
    if ( m_LiveRec )
    {
        m_LiveRecCtrl.SetBitmap( HBITMAP( liveRecOn ) );
        m_RecDevicesCtrl.EnableWindow( TRUE );
        m_RecCardsCtrl.EnableWindow( TRUE );
        m_RecVolumeCtrl.EnableWindow( TRUE );
        StartRecording();
    }
    else
    {
        m_LiveRecCtrl.SetBitmap( HBITMAP( liveRecOff ) );
        m_RecCardsCtrl.EnableWindow( FALSE );
        m_RecDevicesCtrl.EnableWindow( FALSE );
        m_RecVolumeCtrl.EnableWindow( FALSE );
        generalStatusCallback( (void *) "Recording from DSP", FILE_LINE );
        StopRecording();
    }
#endif
    /*#ifdef MULTIASIO
    StopRecording();
    PaAsio_ShowControlPanel(m_CurrentInputCard, m_hWnd);
    StartRecording();
    #endif*/
}

void CMainWindow::ProcessConfigDone ( int enc, CConfig *pConfig )
{
    if ( enc > 0 )
    {
        pConfig->DialogToGlobals( gEncoders[enc - 1] );
        gEncoders[enc - 1]->WriteConfigFile();
        shuicast_init( gEncoders[enc - 1] );
    }

    SetFocus();
    m_Encoders.SetFocus();
}

void CMainWindow::ProcessEditMetadataDone ( CEditMetadata *pConfig )
{
    pConfig->UpdateData( TRUE );

    bool ok = true;
    if ( pConfig->m_ExternalFlag == 0 )
    {
        strcpy( gMain.externalMetadata, "URL" );
        ok = false;
    }
    else if ( pConfig->m_ExternalFlag == 1 )
    {
        strcpy( gMain.externalMetadata, "FILE" );
        ok = false;
    }
    else if ( pConfig->m_ExternalFlag == 2 )
    {
        strcpy( gMain.externalMetadata, "DISABLED" );
    }

    strcpy( gMain.externalInterval, LPCSTR( pConfig->m_MetaInterval ) );
    strcpy( gMain.externalFile, LPCSTR( pConfig->m_MetaFile ) );
    strcpy( gMain.externalURL, LPCSTR( pConfig->m_MetaURL ) );
    strcpy( gMain.metadataAppendString, LPCSTR( pConfig->m_AppendString ) );
    strcpy( gMain.metadataRemoveStringBefore, LPCSTR( pConfig->m_RemoveStringBefore ) );
    strcpy( gMain.metadataRemoveStringAfter, LPCSTR( pConfig->m_RemoveStringAfter ) );
    strcpy( gMain.metadataWindowClass, LPCSTR( pConfig->m_WindowClass ) );

    if ( ok )
    {
        gMain.SetLockedMetadata( (char *)LPCSTR( pConfig->m_Metadata ) );
        gMain.SetLockedMetadataFlag( editMetadata->m_LockMetadata );
        if ( strlen( (char *)LPCSTR( pConfig->m_Metadata ) ) > 0 )
        {
            setMetadata( (char *)LPCSTR( pConfig->m_Metadata ) );
        }
    }

    gMain.metadataWindowClassInd = pConfig->m_WindowTitleGrab ? true : false;
#ifndef _DEBUG
    KillTimer( metadataTimerId );
#endif
    int metadataInterval = atoi( gMain.externalInterval );
    metadataInterval = metadataInterval * 1000;
    metadataTimerId = SetTimer( 4, metadataInterval, (TIMERPROC)MetadataTimer );

    SetFocus();
}

void CMainWindow::OnPopupDelete ()
{
    for ( int i = 0; i < gMain.GetNumEncoders(); i++ )
    {
        if ( gEncoders[i]->IsConnected() )
        {
            MessageBox( "You need to disconnect all the encoders before deleting one from the list", "Message", MB_OK );
            return;
        }
    }

    int iItem = m_Encoders.GetNextItem( -1, LVNI_SELECTED );
    if ( iItem >= 0 )
    {
        int ret = MessageBox( "Delete this encoder ?", "Message", MB_YESNO );
        if ( ret == IDYES )
        {
            if ( gEncoders[iItem] )
            {
                gEncoders[iItem]->DeleteConfigFile();
                delete gEncoders[iItem];
                gEncoders[iItem] = NULL;
            }

            m_Encoders.DeleteAllItems();
            for ( int i = iItem; i < gMain.GetNumEncoders(); i++ )
            {
                if ( gEncoders[i + 1] )
                {
                    gEncoders[i] = gEncoders[i + 1];
                    gEncoders[i + 1] = 0;
                    gEncoders[i]->DeleteConfigFile();
                    gEncoders[i]->encoderNumber--;  // TODO
                    gEncoders[i]->WriteConfigFile();
                }
            }

            gMain.DecNumEncoders();
            for ( int i = 0; i < gMain.GetNumEncoders(); i++ )
            {
                shuicast_init( gEncoders[i] );
            }
        }
    }
}

void CMainWindow::CleanUp ()
{
    timeKillEvent( timer );
    Sleep( 100 );
    if ( specbmp ) DeleteObject( specbmp );
    specbmp = NULL;
    if ( specdc ) DeleteDC( specdc );
    specdc = NULL;
    if ( m_LiveRecRunning ) StopRecording();
}

void CMainWindow::OnHScroll ( UINT nSBCode, UINT nPos, CScrollBar *pScrollBar )
{
    bool	opened = false;

    if ( pScrollBar->m_hWnd == m_limitdbCtrl.m_hWnd )
    {
        UpdateData( TRUE );
        m_staticLimitdb.Format( "%d", m_limitdb );
        UpdateData( FALSE );
    }
    else if ( pScrollBar->m_hWnd == m_gaindbCtrl.m_hWnd )
    {
        UpdateData( TRUE );
        m_staticGaindb.Format( "%d", m_gaindb );
        UpdateData( FALSE );
    }
    else if ( pScrollBar->m_hWnd == m_limitpreCtrl.m_hWnd )
    {
        UpdateData( TRUE );
        m_staticLimitpre.Format( "%d", m_limitpre );
        UpdateData( FALSE );
    }
    else if ( pScrollBar->m_hWnd == m_RecVolumeCtrl.m_hWnd )
    {
        UpdateData( TRUE );
        if ( !m_BASSOpen )
        {
            int ret = BASS_RecordInit( m_CurrentInputCard );
            m_BASSOpen = 1;
            opened = true;
        }

#if ( BASSVERSION == 0x203 )
        BASS_RecordSetInput( m_CurrentInput, BASS_INPUT_LEVEL | m_RecVolume );
#else  // BASSVERSION == 0x204
        BASS_RecordSetInput( m_CurrentInput, BASS_INPUT_ON, (float)((float)m_RecVolume/100) );
#endif
        if ( opened )
        {
            m_BASSOpen = 0;
            BASS_RecordFree();
        }
    }
    CDialog::OnHScroll( nSBCode, nPos, pScrollBar );
}

void CMainWindow::OnManualeditMetadata ()
{
    editMetadata->m_LockMetadata = gMain.GetLockedMetadataFlag();
    editMetadata->m_Metadata = gMain.GetLockedMetadata();

    if ( !strcmp( "DISABLED", gMain.externalMetadata ) )
    {
        editMetadata->m_ExternalFlag = 2;
    }
    else if ( !strcmp( "FILE", gMain.externalMetadata ) )
    {
        editMetadata->m_ExternalFlag = 1;
    }
    else if ( !strcmp( "URL", gMain.externalMetadata ) )
    {
        editMetadata->m_ExternalFlag = 0;
    }

    editMetadata->m_MetaFile = gMain.externalFile;
    editMetadata->m_MetaURL = gMain.externalURL;
    editMetadata->m_MetaInterval = gMain.externalInterval;
    editMetadata->m_AppendString = gMain.metadataAppendString;
    editMetadata->m_RemoveStringAfter = gMain.metadataRemoveStringAfter;
    editMetadata->m_RemoveStringBefore = gMain.metadataRemoveStringBefore;
    editMetadata->m_WindowClass = gMain.metadataWindowClass;
    editMetadata->m_WindowTitleGrab = gMain.metadataWindowClassInd;

    editMetadata->UpdateRadio();
    editMetadata->UpdateData( FALSE );
    editMetadata->ShowWindow( SW_SHOW );
}

void CMainWindow::OnClose ()
{
#ifndef SHUICASTSTANDALONE
    bMinimized_ = true;
    SetupTrayIcon();
    SetupTaskBarButton();
#else
    int ret = IDOK;
    if ( !gMain.SkipCloseWarning() )
    {
        ret = MessageBox( "WARNING: Exiting ShuiCast\r\nSelect OK to exit!", "Exit Selected", MB_OKCANCEL | MB_ICONWARNING );
    }
    if ( ret == IDOK )
    {
        OnDestroy();
        EndModalLoop( 1 );
        exit( 1 );
    }
#endif
}

void CMainWindow::OnDestroy ()
{
    RECT pRect;
    bMinimized_ = false;
    SetupTrayIcon();

    GetWindowRect( &pRect );
    UpdateData( TRUE );
    gMain.SetLastWindowPos( pRect.left, pRect.top );
    gMain.m_LiveRecordingFlag = m_LiveRec;  // TODO: setter
    gMain.SetAutoConnect( m_AutoConnect );
    gMain.SetStartMinimizedFlag( m_startMinimized );
    gMain.SetLimiterFlag( m_Limiter );
    gMain.SetLimiterValues( m_limitdb, m_limitpre, m_gaindb );
    if ( m_LiveRecRunning ) StopRecording();
    stopshuicast();
    CleanUp();

    if ( configDialog )
    {
        configDialog->DestroyWindow();
        delete configDialog;
        configDialog = NULL;
    }
    if ( editMetadata )
    {
        editMetadata->DestroyWindow();
        delete editMetadata;
        editMetadata = NULL;
    }
    if ( aboutBox )
    {
        aboutBox->DestroyWindow();
        delete aboutBox;
        aboutBox = NULL;
    }

    writeMainConfig();
    //DestroyWindow();
    CDialog::OnDestroy();
}

void CMainWindow::OnAboutAbout ()
{
    aboutBox->m_Version = _T( "Built on: "__DATE__ " "__TIME__ );
    aboutBox->m_Configpath.Format( _T( "Config: %s" ), currentConfigDir );
    aboutBox->UpdateData( FALSE );
    aboutBox->ShowWindow( SW_SHOW );
}

void CMainWindow::OnAboutHelp ()
{
    char loc[2046] = "";
    wsprintf( loc, "%s\\%s", m_currentDir, "shuicast.chm" );
    HINSTANCE ret = ShellExecute( NULL, "open", loc, NULL, NULL, SW_SHOWNORMAL );
}

void CMainWindow::OnPaint ()
{
    CPaintDC dc( this );  // device context for painting
}

void CMainWindow::OnTimer ( UINT nIDEvent )
{
    if ( nIDEvent == 73 )
    {
        int			a = 0;
        static int	oldL = 0;
        static int	oldR = 0;
        static int	oldPeakL = 0;
        static int	oldPeakR = 0;
        static int	oldCounter = 0;

        HWND	hWnd = GetDlgItem( IDC_METER )->m_hWnd;
        HDC		hDC = ::GetDC( hWnd );	/* get the DC for the window. */

        if ( (m_VUStatus == VU_ON) || (m_VUStatus == VU_SWITCHOFF) )
        {
            if ( (flexmeters.GetMeterInfoObject( 0 )->value == -1) && (flexmeters.GetMeterInfoObject( 1 )->value == -1) )
            {
                if ( oldCounter > 10 )
                {
                    flexmeters.GetMeterInfoObject( 0 )->value = 0;
                    flexmeters.GetMeterInfoObject( 1 )->value = 0;
                    flexmeters.GetMeterInfoObject( 0 )->peak = 0;
                    flexmeters.GetMeterInfoObject( 1 )->peak = 0;
                    oldCounter = 0;
                }
                else
                {
                    flexmeters.GetMeterInfoObject( 0 )->value = oldL;
                    flexmeters.GetMeterInfoObject( 1 )->value = oldR;
                    flexmeters.GetMeterInfoObject( 0 )->peak = oldPeakL;
                    flexmeters.GetMeterInfoObject( 1 )->peak = oldPeakR;
                    oldCounter++;
                }
            }
            else
            {
                oldCounter = 0;
            }

            if ( m_VUStatus == VU_SWITCHOFF )
            {
                flexmeters.GetMeterInfoObject( 0 )->value = 0;
                flexmeters.GetMeterInfoObject( 1 )->value = 0;
                flexmeters.GetMeterInfoObject( 0 )->peak = 0;
                flexmeters.GetMeterInfoObject( 1 )->peak = 0;
                m_VUStatus = VU_OFF;
            }

            flexmeters.RenderMeters( hDC );	/* render */
            oldL = flexmeters.GetMeterInfoObject( 0 )->value;
            oldR = flexmeters.GetMeterInfoObject( 1 )->value;
            oldPeakL = flexmeters.GetMeterInfoObject( 0 )->peak;
            oldPeakR = flexmeters.GetMeterInfoObject( 1 )->peak;

            flexmeters.GetMeterInfoObject( 0 )->value = -1;
            flexmeters.GetMeterInfoObject( 1 )->value = -1;
            flexmeters.GetMeterInfoObject( 0 )->peak = -1;
            flexmeters.GetMeterInfoObject( 1 )->peak = -1;
        }
        else
        {
            flexmeters.GetMeterInfoObject( 0 )->value = 0;
            flexmeters.GetMeterInfoObject( 1 )->value = 0;
            flexmeters.GetMeterInfoObject( 0 )->peak = 0;
            flexmeters.GetMeterInfoObject( 1 )->peak = 0;
            flexmeters.RenderMeters( hDC );	/* render */
        }
        ::ReleaseDC( hWnd, hDC );			/* release the DC */
    }

    CDialog::OnTimer( nIDEvent );
}

void CMainWindow::OnMeter ()
{
    if ( m_VUStatus == VU_ON )
    {
        if ( gMain.GetVUMeterType() == 1 )
        {
            gMain.SetVUMeterType( 2 );
            m_MeterPeak.ShowWindow( SW_SHOW );
            m_OnOff.ShowWindow( SW_HIDE );
            m_MeterRMS.ShowWindow( SW_HIDE );
        }
        else
        {
            m_VUStatus = VU_SWITCHOFF;
            gMain.SetVUMeterType( 0 );
            m_OnOff.ShowWindow( SW_SHOW );
            m_MeterPeak.ShowWindow( SW_HIDE );
            m_MeterRMS.ShowWindow( SW_HIDE );
        }
    }
    else
    {
        m_VUStatus = VU_ON;
        gMain.SetVUMeterType( 1 );
        m_MeterRMS.ShowWindow( SW_SHOW );
        m_OnOff.ShowWindow( SW_HIDE );
        m_MeterPeak.ShowWindow( SW_HIDE );
    }
}

void CMainWindow::OnSTExit ()
{
    OnCancel();
}

void CMainWindow::OnSTRestore ()
{
    ShowWindow( SW_RESTORE );
    bMinimized_ = false;
    SetupTrayIcon();
    SetupTaskBarButton();
}

void CMainWindow::SetupTrayIcon ()
{
    if ( bMinimized_ && (pTrayIcon_ == 0) )
    {
        pTrayIcon_ = new CSystemTray;
        pTrayIcon_->Create( 0, nTrayNotificationMsg_, "ShuiCast Restore", hIcon_, IDR_SYSTRAY );
        pTrayIcon_->SetMenuDefaultItem( IDI_RESTORE, false );
        pTrayIcon_->SetNotifier( this );
    }
    else
    {
        delete pTrayIcon_;
        pTrayIcon_ = 0;
    }
}

/*
=======================================================================================================================
SetupTaskBarButton Show or hide the taskbar button for this app, depending on whether
we're minimized right now or not.
=======================================================================================================================
*/
void CMainWindow::SetupTaskBarButton ()
{
    ShowWindow( bMinimized_ ? SW_HIDE : SW_SHOW );
}

void CMainWindow::OnSelchangeRecdevices ()
{
    char selectedDevice[1024] = "";
    bool opened = false;
    char msg[2046] = "";
    int  index = m_RecDevicesCtrl.GetCurSel();
    memset( selectedDevice, '\000', sizeof( selectedDevice ) );
    m_RecDevicesCtrl.GetLBText( index, selectedDevice );
    int  dNum = m_RecDevicesCtrl.GetItemData( index );
    m_RecDevices = selectedDevice;
    char *name;

    if ( !m_BASSOpen )
    {
        int ret = BASS_RecordInit( m_CurrentInputCard );
        m_BASSOpen = 1;
        opened = true;
    }

    for ( int n = 0; name = (char *)BASS_RecordGetInputName( n ); n++ )
    {
#if ( BASSVERSION == 0x203 )
        int		s = BASS_RecordGetInput( n );
#else  // BASSVERSION == 0x204
        float vol = 0.0;
        int		s = BASS_RecordGetInput( n, &vol );
#endif
        CString description = name;

        if ( m_RecDevices == description ) {
#if ( BASSVERSION == 0x203 )
            BASS_RecordSetInput( n, BASS_INPUT_ON );
#else  // BASSVERSION == 0x204
            BASS_RecordSetInput( n, BASS_INPUT_ON, -1 );
#endif
            m_CurrentInput = n;
#if ( BASSVERSION == 0x203 )
            m_RecVolume = LOWORD( s );
#else  // BASSVERSION == 0x204
            m_RecVolume = (int)(vol*100);
#endif
            wsprintf( msg, "Recording from %s/%s", m_RecCards, name );
            pWindow->generalStatusCallback( (void *)msg, FILE_LINE );
            gMain.SetWindowsRecordingSubDevice( (char *)name );
        }
    }

    if ( opened )
    {
        m_BASSOpen = 0;
        BASS_RecordFree();
    }

    UpdateData( FALSE );
}

void CMainWindow::OnSelchangeReccards ()
{
    char selectedCard[1024] = "";
    bool opened = false;
    int  index  = m_RecCardsCtrl.GetCurSel();
    int  dNum   = m_RecCardsCtrl.GetItemData( index );
    memset( selectedCard, '\000', sizeof( selectedCard ) );
    m_RecCardsCtrl.GetLBText( index, selectedCard );
    m_RecCards = selectedCard;
    gMain.SetWindowsRecordingDevice( selectedCard );
    char *name;
    BASS_DEVICEINFO info;

#if ( BASSVERSION == 0x203 )
    for ( int n = 0; name = (char *)BASS_RecordGetDeviceDescription( n ); n++ )
    {
        if ( !strcmp( selectedCard, name ) )
        {
            BASS_RecordSetDevice( n );
            m_CurrentInputCard = n;
        }
    }
#else  // BASSVERSION == 0x204
    for ( int n = 0; BASS_RecordGetDeviceInfo( n, &info ); n++ )
    {
        if ( info.flags & BASS_DEVICE_ENABLED )
        {
            if ( !strcmp( selectedCard, info.name ) )
            {
                BASS_RecordSetDevice( n );
                m_CurrentInputCard = n;
            }
        }
    }
#endif

    if ( m_BASSOpen )
    {
        m_BASSOpen = 0;
        BASS_RecordFree();
    }

    if ( !m_BASSOpen )
    {
        int ret = BASS_RecordInit( m_CurrentInputCard );
        m_BASSOpen = 1;
        opened = true;
    }

    m_RecDevicesCtrl.ResetContent();
    char	selectedDevice[1024] = "";
    memset( selectedDevice, '\000', sizeof( selectedDevice ) );
    int dindex = m_RecDevicesCtrl.GetCurSel();
    m_RecDevicesCtrl.GetLBText( dindex, selectedDevice );
    BOOL bFound = FALSE;

    for ( int n = 0; name = (char *)BASS_RecordGetInputName( n ); n++ )
    {
#if ( BASSVERSION == 0x203 )
        int s = BASS_RecordGetInput( n );
#else  // BASSVERSION == 0x204
        float vol = 0.0;
        int s = BASS_RecordGetInput( n, &vol );
#endif
        m_RecDevicesCtrl.AddString( name );
        if ( !(s & BASS_INPUT_OFF) )
        {
            if ( !strcmp( selectedDevice, "" ) )
            {
                m_RecDevices = name;
#if ( BASSVERSION == 0x203 )
                m_RecVolume = LOWORD( s );
#else  // BASSVERSION == 0x204
                m_RecVolume = (int)(vol*100);
#endif
                m_CurrentInput = n;
                bFound = TRUE;
                gMain.SetWindowsRecordingSubDevice( (char *)name );
            }
            else if ( !strcmp( selectedDevice, name ) )
            {
                m_RecDevices = name;
#if ( BASSVERSION == 0x203 )
                m_RecVolume = LOWORD( s );
#else  // BASSVERSION == 0x204
                m_RecVolume = (int)(vol*100);
#endif
                m_CurrentInput = n;
                bFound = TRUE;
            }
        }
        if ( !bFound )
        {
            m_RecDevicesCtrl.SetCurSel( dindex = 0 );
            memset( selectedDevice, '\000', sizeof( selectedDevice ) );
            m_RecDevicesCtrl.GetLBText( dindex, selectedDevice );
            m_RecDevices = selectedDevice;
            gMain.SetWindowsRecordingSubDevice( selectedDevice );
        }
    }

    if ( m_BASSOpen )
    {
        m_BASSOpen = 0;
        BASS_RecordFree();
    }

    StartRecording();
    UpdateData( FALSE );
}

void CMainWindow::OnSysCommand ( UINT nID, LPARAM lParam )
{
    DoSysCommand( nID, lParam );
}

void CMainWindow::DoSysCommand ( UINT nID, LPARAM lParam )
{
    // Decide if minimize state changed
    bool bOldMin = bMinimized_;
    if ( nID == SC_MINIMIZE )
    {
#ifndef SHUICASTSTANDALONE
        bMinimized_ = false;
#else
        bMinimized_ = true;
        SetupTrayIcon();
        ShowWindow( SW_HIDE );
        return;
#endif
    }
    else if ( nID == SC_RESTORE )
    {
        bMinimized_ = false;
        if ( bOldMin != bMinimized_ )
        {
            // Minimize state changed. Create the systray icon and do custom taskbar button handling.
            SetupTrayIcon();
            SetupTaskBarButton();
        }
    }

    CDialog::OnSysCommand( nID, lParam );
}

void CMainWindow::OnKeydownEncoders ( NMHDR *pNMHDR, LRESULT *pResult )
{
    LV_KEYDOWN	*pLVKeyDow = (LV_KEYDOWN *)pNMHDR;

    if ( pLVKeyDow->wVKey == 32 )
    {
        OnPopupConfigure();
    }
    else if ( pLVKeyDow->wVKey == 46 )
    {
        OnPopupDelete();
    }
    else if ( pLVKeyDow->wVKey == 93 )
    {
        int iItem = m_Encoders.GetNextItem( -1, LVNI_SELECTED );

        CMenu menu;
        VERIFY( menu.LoadMenu( IDR_CONTEXT ) );

        /* Pop up sub menu 0 */
        CMenu *popup = menu.GetSubMenu( 0 );
        if ( popup )
        {
            if ( gEncoders[iItem]->IsConnected() )
            {
                popup->ModifyMenu( ID_POPUP_CONNECT, MF_BYCOMMAND, ID_POPUP_CONNECT, "Disconnect" );
            }
            else if ( gEncoders[iItem]->m_ForcedDisconnect )
            {
                popup->ModifyMenu( ID_POPUP_CONNECT, MF_BYCOMMAND, ID_POPUP_CONNECT, "Stop AutoConnect" );
            }
            else
            {
                popup->ModifyMenu( ID_POPUP_CONNECT, MF_BYCOMMAND, ID_POPUP_CONNECT, "Connect" );
            }

            RECT	pt;
            WINDOWPLACEMENT wp;
            WINDOWPLACEMENT wp2;
            GetWindowPlacement( &wp );
            m_Encoders.GetWindowPlacement( &wp2 );

            //GetCursorPos(&pt);
            pt.bottom = wp.rcNormalPosition.bottom + wp2.rcNormalPosition.bottom;
            pt.left   = wp.rcNormalPosition.left   + wp2.rcNormalPosition.left;
            pt.right  = wp.rcNormalPosition.right  + wp2.rcNormalPosition.right;
            pt.top    = wp.rcNormalPosition.top    + wp2.rcNormalPosition.top;

            popup->TrackPopupMenu( TPM_LEFTALIGN, pt.left, pt.top, m_Encoders.GetActiveWindow() );
        }
    }

    *pResult = 0;
}

void CMainWindow::OnButton1 ()
{
    OnPopupConfigure();
}

void CMainWindow::OnCancel ()
{
}

void CMainWindow::OnSetfocusEncoders ( NMHDR *pNMHDR, LRESULT *pResult )
{
    int mark = m_Encoders.GetSelectionMark();
    if ( mark == -1 )
    {
        if ( m_Encoders.GetItemCount() > 0 )
        {
            m_Encoders.SetItemState( 0, LVIS_SELECTED, LVIS_SELECTED | LVIS_FOCUSED );
            m_Encoders.EnsureVisible( 0, FALSE );
        }
    }
    else
    {
        m_Encoders.SetCheck( m_Encoders.GetSelectionMark(), true );
    }
    *pResult = 0;
}

LRESULT CMainWindow::gotShowWindow ( WPARAM wParam, LPARAM lParam )
{
    if ( wParam )
    {
        if ( !visible )
        {
            visible = TRUE;
            PostMessage( WM_MY_MESSAGE );
        }
    }
    return DefWindowProc( WM_SHOWWINDOW, wParam, lParam );
}

void CMainWindow::OnSelchangeAsioRate ()
{
}
