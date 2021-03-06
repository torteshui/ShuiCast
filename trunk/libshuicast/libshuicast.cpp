//=============================================================================
// ShuiCast v0.47
//-----------------------------------------------------------------------------
// Oddcast       Copyright (C) 2000-2010 Ed Zaleski (Oddsock)
// Edcast        Copyright (C) 2011-2012 Ed Zaleski (Oddsock)
// Edcast-Reborn Copyright (C) 2011-2014 RadioRio(?)
// AltaCast      Copyright (C) 2012-2016 DustyDrifter
// ShuiCast      Copyright (C) 2017-2018 TorteShui
//-----------------------------------------------------------------------------
// This file is part of ShuiCast.
// 
// ShuiCast is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 2 of the License, or (at your option) any later version.
// 
// ShuiCast is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Foobar. If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#include <fcntl.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include <sys/timeb.h>
//#include <time.h>
#include <stdarg.h>
#include <math.h>
#include <ShlObj.h>

#include "libshuicast.h"
#include "libshuicast_socket.h"

#if HAVE_VORBIS
#include <vorbis/vorbisenc.h>
#endif

#ifdef WIN32
//#include <bass.h>
#else
#if HAVE_LAME
#include <lame/lame.h>
#endif
#include <errno.h>
#endif
#if HAVE_FAAC
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

static char_t gAppName[] = "shuicast";

#ifdef WIN32
#define INT32	__int32
#else
#define INT32	int
#endif

#define MAX_ENCODERS 10  // TODO: also check when adding new ones!

#if HAVE_AACP
// Uninteresting stuff for the input plugin
static void SAAdd(void *data, int timestamp, int csa) {}
static void VSAAdd(void *data, int timestamp) {}
static void SAVSAInit(int maxlatency_in_ms, int srate) {}
static void SAVSADeInit() {}
static void SAAddPCMData(void *PCMData, int nch, int bps, int timestamp) {}
static int SAGetMode() { return 0; }
static int VSAGetMode(int *specNch, int *waveNch) { return 0; }
static void VSAAddPCMData(void *PCMData, int nch, int bps, int timestamp) {}
static void VSASetInfo(int nch, int srate) {}
static int dsp_isactive() { return 0; }
static int dsp_dosamples(short int *samples, int ns, int bps, int nch, int srate) { return ns; }
static void SetInfo(int bitrate0, int srate0, int stereo, int synched) {}
#endif

typedef struct
{
	char_t	Variable[256];
	char_t	Value[256];
	//char_t	Description[1024];
}
configFileValue;

typedef struct
{
	char_t	Variable[256];
	char_t	Description[1024];
}
configFileDesc;

static configFileValue	configFileValues[200];
static configFileDesc	configFileDescs[200];
static int				numConfigValues = 0;

char_t	defaultLogFileName[MAX_PATH] = "shuicast.log";  // TODO
char_t defaultConfigDir[MAX_PATH];  // TODO

//-----------------------------------------------------------------------------
// Constructor and destructor
//-----------------------------------------------------------------------------

CEncoder::CEncoder ( int encoderNumber ) :
    encoderNumber( encoderNumber ), m_ReconnectSec( 10 ), m_LogLevel( LM_ERROR ), m_JointStereo( 1 )
{
    pthread_mutex_init( &m_Mutex, NULL );
}

CEncoder::~CEncoder ()
{
#ifdef _WIN32
    pthread_mutex_destroy( &m_Mutex );
    if ( m_hDLL )
    {
        FreeLibrary( m_hDLL );
        m_hDLL = NULL;
    }
#if HAVE_FAAC
    if ( m_AACFIFO )
    {
        free( m_AACFIFO );
        m_AACFIFO = NULL;
    }
#endif
#endif
}

//-----------------------------------------------------------------------------
// Public methods - TODO: split
//-----------------------------------------------------------------------------

void CEncoder::AddVorbisComment ( char_t *comment )
{
    int commentLen = strlen( comment ) + 1;
    vorbisComments[numVorbisComments] = (char_t*)calloc( 1, commentLen );
    if ( vorbisComments[numVorbisComments] )
    {
        memset( vorbisComments[numVorbisComments], 0, commentLen );
        strcpy( vorbisComments[numVorbisComments], comment );
        numVorbisComments++;
    }
}

void CEncoder::FreeVorbisComments ()
{
    for ( int i = 0; i < numVorbisComments; ++i )
    {
        if ( vorbisComments[i] )
        {
            free( vorbisComments[i] );
            vorbisComments[i] = NULL;
        }
    }
    numVorbisComments = 0;
}

void CEncoder::AddConfigVariable(char_t *variable) 
{
    m_ConfigVars[m_NumConfigVars++] = _strdup( variable );
}

char_t* CEncoder::GetLockedMetadata ()
{
    return m_ManualSongTitle;
}

void CEncoder::SetLockedMetadata ( const char_t * const buf )
{
    strncpy( m_ManualSongTitle, buf, sizeof( m_ManualSongTitle ) - 1 );  // adds zeros up till count
    m_ManualSongTitle[sizeof( m_ManualSongTitle ) - 1] = '\0';
}

void CEncoder::SetSaveDirectory ( const char_t * const saveDir )
{
    strncpy( m_SaveDirectory, saveDir, sizeof( m_SaveDirectory ) - 1 );  // adds zeros up till count
    m_SaveDirectory[ sizeof( m_SaveDirectory ) - 1 ] = '\0';
}

char_t* CEncoder::GetSaveDirectory ()
{
    return m_SaveDirectory;
}

void CEncoder::SetLogFile( char_t *logFile )
{
    strcpy( m_LogFile, logFile );  // TODO: make them all safe
}

char_t* CEncoder::GetLogFile ()
{
    return m_LogFile;
}

int CEncoder::ResetResampler ()
{
    if ( m_ResamplerInitialized ) res_clear( &m_Resampler );
    m_ResamplerInitialized = false;
    return 1;
}

// Gratuitously ripped from util.c
static char_t			base64table[64] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' };
static signed char_t	base64decode[256] = { -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, 62, -2, -2, -2, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -2, -2, -2, -1, -2, -2, -2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -2, -2, -2, -2, -2, -2, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2 };

// This isn't efficient, but it doesn't need to be
static char_t *util_base64_encode( char_t *data )
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

#if 0
static char_t *util_base64_decode(unsigned char_t *input) 
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
#endif

static char* AsciiToUtf8 ( char *ascii )  // caller MUST free returned buffer when done
{
    int needlen = MultiByteToWideChar( CP_ACP, 0, ascii, strlen( ascii ) + 1, NULL, 0 );
    wchar_t * temp = (wchar_t *)malloc( needlen * sizeof( wchar_t ) );
    MultiByteToWideChar( CP_ACP, 0, ascii, strlen( ascii ) + 1, temp, needlen );
    needlen = WideCharToMultiByte( CP_UTF8, 0, temp, wcslen( temp ) + 1, NULL, 0, NULL, NULL );
    char *utf = (char *)malloc( needlen * sizeof( char ) );
    WideCharToMultiByte( CP_UTF8, 0, temp, wcslen( temp ) + 1, utf, needlen, NULL, NULL );
    free( temp );
    return utf;
}

#if 0
static char* UnicodeToUtf8 ( wchar_t *unicode )  // caller MUST free returned buffer when done
{
    int needlen = WideCharToMultiByte( CP_UTF8, 0, unicode, wcslen( unicode ) + 1, NULL, 0, NULL, NULL );
    char *utf = (char *)malloc( needlen * sizeof( char ) );
    WideCharToMultiByte( CP_UTF8, 0, unicode, wcslen( unicode ) + 1, utf, needlen, NULL, NULL );
    return utf;
}
#endif

#define HEADER_TYPE 1
#define CODEC_TYPE	2

void CEncoder::OpenArchiveFile ()
{
	char_t     outFilename[1024] = "";
	char_t     outputFile[1024] = "";
	time_t     aclock = time( NULL );
    struct tm *newtime = localtime( &aclock );

    wsprintf( outFilename, "%s_%s", m_ServDesc, asctime( newtime ) );
	memset(outputFile, '\000', sizeof(outputFile));
    ReplaceString( outFilename, outputFile, "\"", "'" );
	memset(outFilename, '\000', sizeof(outFilename));
    ReplaceString( outputFile, outFilename, "\\", "" );
	memset(outputFile, '\000', sizeof(outputFile));
    ReplaceString( outFilename, outputFile, "/", "" );
	memset(outFilename, '\000', sizeof(outFilename));
    ReplaceString( outputFile, outFilename, ":", "" );
	memset(outputFile, '\000', sizeof(outputFile));
    ReplaceString( outFilename, outputFile, "*", "" );
	memset(outFilename, '\000', sizeof(outFilename));
    ReplaceString( outputFile, outFilename, "?", "" );
	memset(outputFile, '\000', sizeof(outputFile));
    ReplaceString( outFilename, outputFile, "<", "" );
	memset(outFilename, '\000', sizeof(outFilename));
    ReplaceString( outputFile, outFilename, ">", "" );
	memset(outputFile, '\000', sizeof(outputFile));
    ReplaceString( outFilename, outputFile, "|", "" );
	memset(outFilename, '\000', sizeof(outFilename));
    ReplaceString( outputFile, outFilename, "\n", "" );
	memset(outputFile, '\000', sizeof(outputFile));
	strcpy(outputFile, outFilename);

    if ( m_SaveAsWAV ) strcat( outputFile, ".wav" );
    else if ( m_Type == ENCODER_OGG  ) strcat( outputFile, ".ogg"  );  // TODO: use switch
    else if ( m_Type == ENCODER_FLAC ) strcat( outputFile, ".flac" );
    else if ( m_Type == ENCODER_LAME ) strcat( outputFile, ".mp3"  );
    else if ( m_Type != ENCODER_NONE ) strcat( outputFile, ".aac"  );
    wsprintf( outFilename, "%s%s%s", m_SaveDirectory, FILE_SEPARATOR, outputFile );  // TODO: use StringCbPrintf everywhere

    m_SaveFilePtr = fopen( outFilename, "wb" );
    if ( m_SaveFilePtr )
    {
        if ( m_SaveAsWAV )
        {
            int nch = 2;
            int rate = 44100;

            memcpy( &m_WavHeader.main_chunk, "RIFF", 4 );
            m_WavHeader.length      = GUINT32_TO_LE( 0 );
            memcpy( &m_WavHeader.chunk_type, "WAVE", 4 );
            memcpy( &m_WavHeader.sub_chunk, "fmt ", 4 );
            m_WavHeader.sc_len      = GUINT32_TO_LE( 16 );
            m_WavHeader.format      = GUINT16_TO_LE( 1 );
            m_WavHeader.modus       = GUINT16_TO_LE( nch );
            m_WavHeader.sample_fq   = GUINT32_TO_LE( rate );
            m_WavHeader.bit_p_spl   = GUINT16_TO_LE( 16 );
            m_WavHeader.byte_p_sec  = GUINT32_TO_LE( rate * m_WavHeader.modus * (GUINT16_FROM_LE( m_WavHeader.bit_p_spl ) / 8) );
            m_WavHeader.byte_p_spl  = GUINT16_TO_LE( (GUINT16_FROM_LE( m_WavHeader.bit_p_spl ) / (8 / nch)) );
            memcpy( &m_WavHeader.data_chunk, "data", 4 );
            m_WavHeader.data_length = GUINT32_TO_LE( 0 );
            fwrite( &m_WavHeader, sizeof( WavHeader ), 1, m_SaveFilePtr );
        }
    }
    else
    {
        LogMessage( LOG_ERROR, "Cannot open %s", outputFile );
    }
}

void CEncoder::CloseArchiveFile ()
{
    if ( m_SaveFilePtr )
    {
        if ( m_SaveAsWAV )
        {
            m_WavHeader.length = GUINT32_TO_LE( m_ArchiveWritten + sizeof( WavHeader ) - 8 );
            m_WavHeader.data_length = GUINT32_TO_LE( m_ArchiveWritten );
            fseek( m_SaveFilePtr, 0, SEEK_SET );
            fwrite( &m_WavHeader, sizeof( WavHeader ), 1, m_SaveFilePtr );
            m_ArchiveWritten = 0;
        }
        fclose( m_SaveFilePtr );
        m_SaveFilePtr = NULL;
    }
}

// this needs to be virtualized to support NSV encapsualtion if I ever get around to supporting NSV
int CEncoder::SendToServer ( int sd, char_t *data, int length, int type )
{
	int ret = 0;
	int sendflags = 0;

    if ( m_SaveDirectoryFlag && !m_SaveFilePtr ) OpenArchiveFile();

#if !defined(WIN32) && !defined(__FreeBSD__)
	sendflags = MSG_NOSIGNAL;
#endif
    ret = send( sd, data, length, sendflags );
    if ( type == CODEC_TYPE )
    {
        if ( m_SaveDirectoryFlag && m_SaveFilePtr && !m_SaveAsWAV ) fwrite( data, length, 1, m_SaveFilePtr );
	}

	if ( ret > 0 )
	{
        if ( m_WriteBytesCallback && m_IsConnected ) m_WriteBytesCallback( (void *) this, (void *)ret );
	}

	return ret;
}


int CEncoder::ReadConfigFile ( const int readOnly )
{
    static bool firstRead = true;
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
        wsprintf( configFile, "%s", m_ConfigFileName );
	}
	else
	{
        if ( strlen( m_ConfigFileName ) == 0 ) wsprintf( configFile, "%s_%d.cfg", defaultConfigName, encoderNumber );
        else wsprintf( configFile, "%s_%d.cfg", m_ConfigFileName, encoderNumber );
	}

	filep = fopen(configFile, "r");
	if(filep == NULL) 
	{
		//g->LogMessage(LOG_ERROR, "Cannot open config file %s\n", configFile);
		//strcpy(g->m_ConfigFileName, defaultConfigName);
	}
	else
	{
		while(!feof(filep)) 
		{
			char_t	*p2;

			memset(buffer, '\000', sizeof(buffer));
			fgets(buffer, sizeof(buffer) - 1, filep);
			p2 = strchr(buffer, '\r');
			if(p2) *p2 = '\000';
			p2 = strchr(buffer, '\n');
			if(p2) *p2 = '\000';

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

		if(filep) fclose(filep);
	}

    LoadConfig();
	if(!readOnly) WriteConfigFile();

	return 1;
}

void CEncoder::DeleteConfigFile ()
{
    char_t	configFile[1024] = "";
    char_t	defaultConfigName[] = "shuicast";
    if ( strlen( m_ConfigFileName ) == 0 ) wsprintf( configFile, "%s_%d.cfg", defaultConfigName, encoderNumber );
    else wsprintf( configFile, "%s_%d.cfg", m_ConfigFileName, encoderNumber );
    _unlink( configFile );
}

void CEncoder::SetConfigFileName ( char_t *configFile )
{
    strcpy( m_ConfigFileName, configFile );
}

void CEncoder::SetDefaultLogFileName ( char_t *filename )
{
    strcpy( defaultLogFileName, filename );  // TODO
}

void CEncoder::SetConfigDir ( char_t *dirname )
{
    strcpy( defaultConfigDir, dirname );  // TODO
}

char_t *CEncoder::GetDescription ( const char_t *param ) const
{
    for ( int i = 0; i < 200; ++i )  // TODO
    {
        if ( !strcmp( configFileDescs[i].Variable, param ) )
        {
            return configFileDescs[i].Description;
        }
    }
    return NULL;
}

void CEncoder::WriteConfigFile ()
{
    char_t	configFile[1024] = "";
    char_t	defaultConfigName[] = "shuicast";
    StoreConfig();

    if ( strlen( m_ConfigFileName ) == 0 )
    {
        wsprintf( configFile, "%s_%d.cfg", defaultConfigName, encoderNumber );
    }
    else
    {
        wsprintf( configFile, "%s_%d.cfg", m_ConfigFileName, encoderNumber );
    }

    FILE *filep = fopen( configFile, "w" );
    if ( filep )
    {
        for ( int i = 0; i < numConfigValues; ++i )
        {
            int ok = 1;
            if ( m_ConfigVars )
            {
                ok = 0;
                for ( int j = 0; j < m_NumConfigVars; ++j )
                {
                    if ( !strcmp( m_ConfigVars[j], configFileValues[i].Variable ) )
                    {
                        ok = 1;
                        break;
                    }
                }
            }

            if ( ok )
            {
                char *desc = GetDescription( configFileValues[i].Variable );
                if ( desc ) fprintf( filep, "# %s\n", desc );
                fprintf( filep, "%s=%s\n", configFileValues[i].Variable, configFileValues[i].Value );
            }
        }

        fclose( filep );
    }
    else
    {
        LogMessage( LOG_ERROR, "Cannot open config file %s\n", configFile );
    }
}

#if 0
void printConfigFileValues() 
{
	for(int i = 0; i < numConfigValues; i++) 
	{
		LogMessage(LOG_DEBUG, "(%s) = (%s)\n", configFileValues[i].Variable, configFileValues[i].Value);
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

void CEncoder::GetConfigVariable ( char_t *appName, char_t *paramName, char_t *defaultvalue, char_t *destValue, int destSize, char_t *desc )
{
    if ( m_ConfigVars )
	{
		int ok = 0;
        for ( int j=0; j<m_NumConfigVars; j++ )
		{
            if ( !strcmp( m_ConfigVars[j], paramName ) )
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

long CEncoder::GetConfigVariable ( char_t *appName, char_t *paramName, long defaultvalue, char_t *desc )
{
    char_t	buf[1024] = "";
    char_t	defaultbuf[1024] = "";
    wsprintf( defaultbuf, "%d", defaultvalue );
    GetConfigVariable( appName, paramName, defaultbuf, buf, sizeof( buf ), desc );
    return atol( buf );
}

void CEncoder::PutConfigVariable ( char_t *appName, char_t *paramName, char_t *destValue )
{
    if ( m_ConfigVars )
	{
		int ok = 0;
        for ( int j=0; j<m_NumConfigVars; j++ )
		{
            if ( !strcmp( m_ConfigVars[j], paramName ) )
			{
				ok = 1;
				break;
			}
		}
		if (!ok) return;
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
}

void CEncoder::PutConfigVariable ( char_t *appName, char_t *paramName, long value )
{
    char_t	buf[1024] = "";
    wsprintf( buf, "%d", value );
    PutConfigVariable( appName, paramName, buf );
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

int CEncoder::SetCurrentSongTitle ( char_t *song )
{
    char_t *pCurrent = m_LockSongTitle ? m_ManualSongTitle : song;
    if ( strcmp( m_SongTitle, pCurrent ) )
    {
        strcpy( m_SongTitle, pCurrent );
        UpdateSongTitle( 0 );
        return 1;
    }
    return 0;
}

void CEncoder::GetCurrentSongTitle ( char_t *song, char_t *artist, char_t *full ) const
{
	char_t	songTitle[1024] = "";
    strcpy( songTitle, m_LockSongTitle ? m_ManualSongTitle : m_SongTitle );
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
            if ( m_SCFlag || (m_ServerType == SERVER_ICECAST) || (m_ServerType == SERVER_ICECAST2) || forceURL )
			{
                char_t	* URLSong = URLize( m_SongTitle );
                char_t	* URLPassword = URLize( m_Password );
                strcpy( m_CurrentSong, m_SongTitle );

                if ( m_ServerType == SERVER_ICECAST2 )
				{
					char_t	userAuth[4096] = "";
                    sprintf( userAuth, "source:%s", m_Password );
					char_t	*puserAuthbase64 = util_base64_encode(userAuth);
					if(puserAuthbase64) 
					{
						sprintf( contentString, "GET /admin/metadata?pass=%s&mode=updinfo&mount=%s&song=%s HTTP/1.0\r\nAuthorization: Basic %s\r\n%s",
                            URLPassword, m_Mountpoint, URLSong, puserAuthbase64, reqHeaders );
						free(puserAuthbase64);
					}
				}

                if ( m_ServerType == SERVER_ICECAST )
				{
					sprintf( contentString, "GET /admin.cgi?pass=%s&mode=updinfo&mount=%s&song=%s HTTP/1.0\r\n%s",
                        URLPassword, m_Mountpoint, URLSong, reqHeaders );
				}

                if ( m_SCFlag )
				{
                    if ( strchr( m_Password, ':' ) == NULL ) // use Basic Auth for non sc_trans 2 connections
					{
						char_t	userAuth[1024] = "";
                        sprintf( userAuth, "admin:%s", m_Password );
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

                m_SCSocketCtrl = m_CtrlChannel.DoConnect( m_Server, (unsigned short)atoi( m_Port ) );
                if ( m_SCSocketCtrl != -1 )
				{
                    send( m_SCSocketCtrl, contentString, strlen( contentString ), 0 );
					//int sent = SendToServer( m_SCSocketCtrl, contentString, strlen(contentString), HEADER_TYPE );
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
            m_Ice2songChange = true;
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
void CEncoder::Icecast2SendMetadata ()
{
#if HAVE_VORBIS
    pthread_mutex_lock( &m_Mutex );
    vorbis_analysis_wrote( &m_VorbisDSPState, 0 );
    OggEncodeDataout();
    Load();
    pthread_mutex_unlock( &m_Mutex );
#endif
}


#if HAVE_FLAC
extern "C"
{
	FLAC__StreamEncoderWriteStatus FLACWriteCallback ( const FLAC__StreamEncoder *flacEncoder, const FLAC__byte buffer[],
			unsigned bytes, unsigned samples, unsigned current_frame, void *client_data ) 
	{
        CEncoder *encoder = (CEncoder *)client_data;
        int sentbytes = encoder->SendToServer( encoder->GetSCSocket(), (char_t *)buffer, bytes, CODEC_TYPE );
        encoder->SetFLACFailure( (sentbytes < 0) );
		return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
	}

    void FLACMetadataCallback ( const FLAC__StreamEncoder *flacEncoder, const FLAC__StreamMetadata *metadata, void *client_data ) 
	{
        //CEncoder *encoder = (CEncoder *)client_data;
	}
}
#endif

/*
 =======================================================================================================================
 This function will connect to a server (Shoutcast/Icecast/Icecast2) and send the appropriate password info and check
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
int CEncoder::ConnectToServer ()
{
	char_t	buffer[1024] = "";
	char_t	contentString[1024] = "";
	char_t	brate[25] = "";
	char_t	ypbrate[25] = "";

	LogMessage( LOG_DEBUG, "Connecting encoder %d", encoderNumber );
    sprintf( brate, "%d", m_CurrentBitrate );
    if ( (m_Type == ENCODER_OGG) && !m_UseBitrate ) sprintf( ypbrate, "Quality %s", m_OggQuality );
    else strcpy( ypbrate, brate );

    m_SCFlag = false;
    SetServerStatus( "Connecting" );
    m_DataChannel.Init();

	/* If we are Icecast/Icecast2, then connect to specified port */
    if ( (m_ServerType == SERVER_ICECAST) || (m_ServerType == SERVER_ICECAST2) )
	{
        m_SCSocket = m_DataChannel.DoConnect( m_Server, (unsigned short)atoi( m_Port ) );
	}
	else
	{
		/* If we are Shoutcast, then the control socket (used for password) is port+1. */
        m_SCSocket = m_DataChannel.DoConnect( m_Server, (unsigned short)atoi( m_Port ) + 1 );
	}

	/* Check to see if we connected okay */
    if ( m_SCSocket == -1 )
	{
        SetServerStatus( "Unable to connect to socket" );
		return 0;
	}

	/* Yup, we did. */
    SetServerStatus( "Socket connected" );

	char_t	contentType[255] = "";
    switch ( m_Type )
	{
    case ENCODER_OGG:
    case ENCODER_FLAC:
        strcpy( contentType, "application/ogg" );
        break;
    case ENCODER_AAC:
        strcpy( contentType, "audio/aac" );
        break;
    case ENCODER_FG_AACP_AUTO:
    case ENCODER_FG_AACP_LC:
    case ENCODER_FG_AACP_HE:
    case ENCODER_FG_AACP_HEV2:
    case ENCODER_AACP_HE: // HE-AAC, AAC Plus
        strcpy( contentType, "audio/aacp" );
		break;
    case ENCODER_AACP_HE_HIGH: // HE-AAC High Bitrate
		strcpy(contentType, "audio/aach");
		break;
    case ENCODER_AACP_LC: // LC-AAC
		// strcpy(contentType, "audio/aacr");
		strcpy(contentType, "audio/aac");
		break;
	default:
		strcpy(contentType, "audio/mpeg");
        break;
	}

	// Here are all the variations of sending the password to a server..This if statement really is ugly...must fix.
    if ( m_ServerType == SERVER_ICECAST )
	{
		sprintf(contentString,
				"SOURCE %s %s\r\ncontent-type: %s\r\nx-audiocast-name: %s\r\nx-audiocast-url: %s\r\nx-audiocast-genre: %s\r\nx-audiocast-bitrate: %s\r\nx-audiocast-public: %d\r\nx-audiocast-description: %s\r\n\r\n",
                m_Password, m_Mountpoint, contentType, m_ServDesc, m_ServURL, m_ServGenre, brate, m_PubServ, m_ServDesc );
	}
    else if ( m_ServerType == SERVER_ICECAST2 )
	{
		char_t	audioInfo[1024] = "";
		sprintf(audioInfo, "ice-samplerate=%d;ice-bitrate=%s;ice-channels=%d", GetCurrentSamplerate(), ypbrate, GetCurrentChannels());
		char_t	userAuth[1024] = "";
        sprintf( userAuth, "source:%s", m_Password );
		char_t	*puserAuthbase64 = util_base64_encode(userAuth);
		if(puserAuthbase64)
		{
			sprintf(contentString,
					"SOURCE %s ICE/1.0\ncontent-type: %s\nAuthorization: Basic %s\nice-name: %s\nice-url: %s\nice-genre: %s\nice-bitrate: %s\nice-private: %d\nice-public: %d\nice-description: %s\nice-audio-info: %s\n\n",
                    m_Mountpoint, contentType, puserAuthbase64, m_ServName, m_ServURL, m_ServGenre, ypbrate, !m_PubServ, m_PubServ, m_ServDesc, audioInfo );
			free(puserAuthbase64);
		}
	}
	else  // Shoutcast
	{
        SendToServer( m_SCSocket, m_Password, strlen( m_Password ), HEADER_TYPE );
        SendToServer( m_SCSocket, "\r\n", strlen( "\r\n" ), HEADER_TYPE );

        recv( m_SCSocket, buffer, sizeof( buffer ), (int)0 );

		// if we get an OK, then we are not a Shoutcast server (could be live365 or other variant)..And OK2 means it's
		// Shoutcast and we can safely send in metadata via the admin.cgi interface.
		if(!strncmp(buffer, "OK", strlen("OK"))) 
		{
            m_SCFlag = !strncmp( buffer, "OK2", strlen( "OK2" ) );
            SetServerStatus( "Password OK" );
		}
		else
		{
            SetServerStatus( "Password Failed" );
            closesocket( m_SCSocket );
			return 0;
		}

		memset(contentString, '\000', sizeof(contentString));
        if ( strlen( m_ServICQ ) == 0 ) strcpy( m_ServICQ, "N/A" );
        if ( strlen( m_ServAIM ) == 0 ) strcpy( m_ServAIM, "N/A" );
        if ( strlen( m_ServIRC ) == 0 ) strcpy( m_ServIRC, "N/A" );
		sprintf(contentString,
				"content-type:%s\r\nicy-name:%s\r\nicy-genre:%s\r\nicy-url:%s\r\nicy-pub:%d\r\nicy-irc:%s\r\nicy-icq:%s\r\nicy-aim:%s\r\nicy-br:%s\r\n\r\n",
                contentType, m_ServName, m_ServGenre, m_ServURL, m_PubServ, m_ServIRC, m_ServICQ, m_ServAIM, brate );
	}

    SendToServer( m_SCSocket, contentString, strlen( contentString ), HEADER_TYPE );

    if ( m_ServerType == SERVER_ICECAST )
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
                m_SCFlag = !strncmp( buffer, "OK2", strlen( "OK2" ) );
                SetServerStatus( "Password OK" );
			}
			else
			{
                SetServerStatus( "Password failed" );
                closesocket( m_SCSocket );
				return 0;
			}
		}
	}

	/* We are connected */
    //time_t     aclock = time( NULL );
    //struct tm *newtime = localtime( &aclock );
    int ret = Load();
    m_ForcedDisconnect = false;
	if(ret)
	{
        m_IsConnected = 1;
        SetServerStatus( "Success" );
	}
	else
	{
		DisconnectFromServer();
#ifdef _WIN32
        if ( m_Type == ENCODER_LAME ) SetServerStatus( "Error with lame_enc.dll" );
        else if ( m_Type == ENCODER_AAC ) SetServerStatus( "Cannot find libfaac.dll" );
		else SetServerStatus( "Encoder init failed" );
#else
        SetServerStatus( "Encoder init failed" );
#endif
		return 0;
	}

    SetServerStatus( "Connected" );
    SetCurrentSongTitle( m_SongTitle );
	UpdateSongTitle( 0 );
	return 1;
}

int CEncoder::DisconnectFromServer ()
{
    m_IsConnected = 0;
    SetServerStatus( "Disconnecting" );
    int retry = 10;
    while ( m_CurrentlyEncoding && retry-- )
    {
#ifdef _WIN32
        Sleep( 1000 );
#else
        sleep( 1 );
#endif
    }
    if ( retry == 0 )
    {
        SetServerStatus( "Disconnecting - encoder did not stop" );
    }

    // Close all open sockets
    closesocket( m_SCSocket );
    closesocket( m_SCSocketCtrl );

    // Reset the Status to Disconnected, and reenable the config button
    m_SCSocket     = 0;
    m_SCSocketCtrl = 0;

#if HAVE_VORBIS
    if ( m_Type == ENCODER_OGG )
    {
        ogg_stream_clear( &m_OggStreamState );
        vorbis_block_clear( &m_VorbisBlock );
        vorbis_dsp_clear( &m_VorbisDSPState );
        vorbis_info_clear( &m_VorbisInfo );
        memset( &m_VorbisInfo, '\000', sizeof( m_VorbisInfo ) );
    }
#endif

#if HAVE_LAME
#ifndef _WIN32
    if ( m_LAMEGlobalFlags )
    {
        lame_close( m_LAMEGlobalFlags );
        m_LAMEGlobalFlags = NULL;
    }
#endif
#endif

    SetServerStatus( "Disconnected" );
    CloseArchiveFile();
    return 1;
}

// These are some ogg routines that are used for Icecast2
int CEncoder::OggEncodeDataout ()
{
    int			sentbytes = 0;
#if HAVE_VORBIS
	ogg_packet	op;
	ogg_page	og;
	int			result;

    if ( m_InHeader )
	{
        result = ogg_stream_flush( &m_OggStreamState, &og );
        m_InHeader = false;
	}

    while ( vorbis_analysis_blockout( &m_VorbisDSPState, &m_VorbisBlock ) == 1 )
	{
        vorbis_analysis( &m_VorbisBlock, NULL );
        vorbis_bitrate_addblock( &m_VorbisBlock );
		int packetsdone = 0;

        while ( vorbis_bitrate_flushpacket( &m_VorbisDSPState, &op ) )
		{
			/* Add packet to bitstream */
            ogg_stream_packetin( &m_OggStreamState, &op );
			packetsdone++;

			/* If we've gone over a page boundary, we can do actual output, so do so (for however many pages are available) */
			int eos = 0;
			while(!eos) 
			{
                int result = ogg_stream_pageout( &m_OggStreamState, &og );
				if(!result) break;
                sentbytes = SendToServer( m_SCSocket, (char_t *)og.header, og.header_len, CODEC_TYPE );
				if(sentbytes < 0) 
				{
					return sentbytes;
				}
                sentbytes += SendToServer( m_SCSocket, (char_t *)og.body, og.body_len, CODEC_TYPE );
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
#endif
    return sentbytes;
}

#if HAVE_LAME
#ifndef _WIN32
void LAMEErrorHandler ( const char_t *format, va_list ap )
{
}
#endif
#endif

int CEncoder::InitResampler ( long inSampleRate, long inNCH )
{
    if ( !m_ResamplerInitialized )
    {
        long in_samplerate  = inSampleRate;
        long out_samplerate = GetCurrentSamplerate();
        long out_nch        = 2;
        if ( res_init( &m_Resampler, out_nch, out_samplerate, in_samplerate, RES_END ) )
        {
            LogMessage( LOG_ERROR, "Error initializing resampler" );
            return 0;
        }
        m_ResamplerInitialized = true;
    }
    return 1;
}

int CEncoder::ConvertAudio ( float *in_samples, float *out_samples, int num_in_samples, int num_out_samples )
{
    int max_num_samples = res_push_max_input( &m_Resampler, num_out_samples );
    int ret_samples     = res_push_interleaved( &m_Resampler, (SAMPLE*)out_samples, (const SAMPLE*)in_samples, max_num_samples );
    return ret_samples;
}

void CEncoder::LoadDLL ( const char_t *name, const char_t *msg )
{
    if ( m_hDLL ) ::FreeLibrary( m_hDLL );
    LogMessage( LOG_DEBUG, "Loading %s", name );
    m_hDLL = ::LoadLibrary( name );
    if ( m_hDLL == NULL )
    {
        char_t name2[32] = "plugins\\";  // long enough to hold this string and DLL name
        strcat( name2, name );
        LogMessage( LOG_DEBUG, "Loading %s", name2 );
        m_hDLL = ::LoadLibrary( name2 );
    }
    if ( m_hDLL == NULL )
    {
        char_t msg2[32] = "Unable to load ";  // long enough to hold this string and DLL name
        strcat( msg2, name );
        if ( msg ) LogMessage( LOG_ERROR, msg );
        else LogMessage( LOG_ERROR, msg2 );
        SetServerStatus( msg2 );
    }
}

int CEncoder::Load()
{
	ResetResampler();

    if ( m_Type == ENCODER_LAME )
	{
#if HAVE_LAME
#ifdef _WIN32
		BE_ERR		err = 0;
		BE_VERSION	Version = { 0, };
		BE_CONFIG	beConfig = { 0, };

        LoadDLL( "lame_enc.dll",
				"Unable to load lame_enc.dll\n\
You have selected encoding with LAME, but apparently the plugin cannot find LAME installed. \
Due to legal issues, ShuiCast cannot distribute LAME directly, and so you'll have to download it separately. \
You will need to put the LAME DLL (lame_enc.dll) \
into the same directory as the application in order to get it working-> \
To download it, check out http://www.rarewares.org/mp3-lame-bundle.php");
        if ( m_hDLL == NULL ) return 0;

		/* Get Interface functions from the DLL */
        m_beInitStream     = (BEINITSTREAM)GetProcAddress( m_hDLL, TEXT_BEINITSTREAM );
        m_beEncodeChunk    = (BEENCODECHUNK)GetProcAddress( m_hDLL, TEXT_BEENCODECHUNK );
        m_beDeinitStream   = (BEDEINITSTREAM)GetProcAddress( m_hDLL, TEXT_BEDEINITSTREAM );
        m_beCloseStream    = (BECLOSESTREAM)GetProcAddress( m_hDLL, TEXT_BECLOSESTREAM );
        m_beVersion        = (BEVERSION)GetProcAddress( m_hDLL, TEXT_BEVERSION );
        m_beWriteVBRHeader = (BEWRITEVBRHEADER)GetProcAddress( m_hDLL, TEXT_BEWRITEVBRHEADER );

        if ( !m_beInitStream || !m_beEncodeChunk || !m_beDeinitStream || !m_beCloseStream || !m_beVersion || !m_beWriteVBRHeader )
		{
			LogMessage( LOG_ERROR, "Unable to get LAME interfaces - This DLL (lame_enc.dll) doesn't appear to be LAME?!?!?" );
			return 0;
		}

		/* Get the version number */
        m_beVersion( &Version );
		if(Version.byMajorVersion < 3)
		{
            LogMessage( LOG_ERROR, "This version of ShuiCast expects at least version 3.91 of the LAME DLL, the DLL found is at %u.%02u, please consider upgrading",
                Version.byDLLMajorVersion, Version.byDLLMinorVersion );
		}
		else
		{
			if(Version.byMinorVersion < 91)
			{
                LogMessage( LOG_ERROR, "This version of ShuiCast expects at least version 3.91 of the LAME DLL, the DLL found is at %u.%02u, please consider upgrading",
                    Version.byDLLMajorVersion, Version.byDLLMinorVersion );
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
            beConfig.format.LHV1.nMode = m_JointStereo ? BE_MP3_MODE_JSTEREO : BE_MP3_MODE_STEREO;
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
        beConfig.format.LHV1.bStrictIso = m_LAMEOptions.strict_ISO;
		beConfig.format.LHV1.bCRC = FALSE;							//
        beConfig.format.LHV1.bCopyright = m_LAMEOptions.copywrite;
        beConfig.format.LHV1.bOriginal = m_LAMEOptions.original;
		beConfig.format.LHV1.bPrivate = FALSE;						//
        beConfig.format.LHV1.bNoRes = m_LAMEOptions.disable_reservoir;
        beConfig.format.LHV1.nQuality = (WORD)(m_LAMEOptions.quality | ((~m_LAMEOptions.quality) << 8));
        beConfig.format.LHV1.dwBitrate = m_CurrentBitrate;		// BIT RATE

        if ( (m_LAMEOptions.cbrflag) || !strcmp( m_LAMEOptions.VBR_mode, "vbr_none" ) || m_LAMEPreset == LQP_CBR )
		{
			beConfig.format.LHV1.nVbrMethod = VBR_METHOD_NONE;
			beConfig.format.LHV1.bEnableVBR = FALSE;
			beConfig.format.LHV1.nVBRQuality = 0;
		}
        else if ( !strcmp( m_LAMEOptions.VBR_mode, "vbr_abr" ) || m_LAMEPreset == LQP_ABR )
		{
			beConfig.format.LHV1.nVbrMethod = VBR_METHOD_ABR;
			beConfig.format.LHV1.bEnableVBR = TRUE;
            beConfig.format.LHV1.dwVbrAbr_bps = m_CurrentBitrate * 1000;
            beConfig.format.LHV1.dwMaxBitrate = m_CurrentBitrateMax;
            beConfig.format.LHV1.nVBRQuality = m_LAMEOptions.quality;
		}
		else
		{
			beConfig.format.LHV1.bEnableVBR = TRUE;
            beConfig.format.LHV1.dwMaxBitrate = m_CurrentBitrateMax;
            beConfig.format.LHV1.nVBRQuality = m_LAMEOptions.quality;

                 if ( !strcmp( m_LAMEOptions.VBR_mode, "vbr_rh"   ) ) beConfig.format.LHV1.nVbrMethod = VBR_METHOD_OLD;
            else if ( !strcmp( m_LAMEOptions.VBR_mode, "vbr_new"  ) ) beConfig.format.LHV1.nVbrMethod = VBR_METHOD_NEW;
            else if ( !strcmp( m_LAMEOptions.VBR_mode, "vbr_mtrh" ) ) beConfig.format.LHV1.nVbrMethod = VBR_METHOD_MTRH;
            else if ( !strcmp( m_LAMEOptions.VBR_mode, "vbr_abr"  ) ) beConfig.format.LHV1.nVbrMethod = VBR_METHOD_ABR;
            else                                                      beConfig.format.LHV1.nVbrMethod = VBR_METHOD_DEFAULT;
		}

		//if(m_LAMEPreset != LQP_NOPRESET) 
		//{
        beConfig.format.LHV1.nPreset = m_LAMEPreset;
		//}

        err = m_beInitStream( &beConfig, &m_LAMESamples, &m_LAMEMP3Buffer, &m_hbeStream );

		if(err != BE_ERR_SUCCESSFUL)
		{
            LogMessage( LOG_ERROR, "Error opening encoding stream (%lu)", err );
			return 0;
		}

#else
		m_LAMEGlobalFlags = lame_init();
        lame_set_errorf(m_LAMEGlobalFlags, LAMEErrorHandler);
        lame_set_debugf(m_LAMEGlobalFlags, LAMEErrorHandler);
        lame_set_msgf(m_LAMEGlobalFlags, LAMEErrorHandler);
        lame_set_brate(m_LAMEGlobalFlags, m_CurrentBitrate);
        lame_set_quality(m_LAMEGlobalFlags, m_LAMEOptions.quality);
        lame_set_num_channels(m_LAMEGlobalFlags, 2);

        if(m_CurrentChannels == 1)
		{
            lame_set_mode(m_LAMEGlobalFlags, MONO);
			//lame_set_num_channels(m_LAMEGlobalFlags, 1);
		}
		else
		{
            lame_set_mode(m_LAMEGlobalFlags, STEREO);
		}

		// Make the input sample rate the same as output..i.e. don't make lame do
		// any resampling->..cause we are handling it ourselves...
        lame_set_in_samplerate(m_LAMEGlobalFlags, m_CurrentSamplerate);
        lame_set_out_samplerate(m_LAMEGlobalFlags, m_CurrentSamplerate);
        lame_set_copyright(m_LAMEGlobalFlags, m_LAMEOptions.copywrite);
        lame_set_strict_ISO(m_LAMEGlobalFlags, m_LAMEOptions.strict_ISO);
        lame_set_disable_reservoir(m_LAMEGlobalFlags, m_LAMEOptions.disable_reservoir);

        if(!m_LAMEOptions.cbrflag)
		{
            if(!strcmp(m_LAMEOptions.VBR_mode, "vbr_rh"))
			{
                lame_set_VBR(m_LAMEGlobalFlags, vbr_rh);
			}
            else if(!strcmp(m_LAMEOptions.VBR_mode, "vbr_mtrh"))
			{
                lame_set_VBR(m_LAMEGlobalFlags, vbr_mtrh);
			}
            else if(!strcmp(m_LAMEOptions.VBR_mode, "vbr_abr"))
			{
                lame_set_VBR(m_LAMEGlobalFlags, vbr_abr);
			}

            lame_set_VBR_mean_bitrate_kbps(m_LAMEGlobalFlags, m_CurrentBitrate);
            lame_set_VBR_min_bitrate_kbps(m_LAMEGlobalFlags, m_CurrentBitrateMin);
            lame_set_VBR_max_bitrate_kbps(m_LAMEGlobalFlags, m_CurrentBitrateMax);
		}

        if(strlen(m_LAMEBasicPreset) > 0)
		{
            if(!strcmp(m_LAMEBasicPreset, "r3mix"))
			{
				//presets_set_r3mix(m_LAMEGlobalFlags, m_LAMEBasicPreset, stdout);
			}
			else
			{
				//presets_set_basic(m_LAMEGlobalFlags, m_LAMEBasicPreset, stdout);
			}
		}

        if(strlen(m_LAMEAltPreset) > 0)
		{
            int altbitrate = atoi(m_LAMEAltPreset);
			//dm_presets(m_LAMEGlobalFlags, 0, altbitrate, m_LAMEAltPreset, "shuicast");
		}

		/* do internal inits... */
        lame_set_lowpassfreq(m_LAMEGlobalFlags, m_LAMEOptions.lowpassfreq);
        lame_set_highpassfreq(m_LAMEGlobalFlags, m_LAMEOptions.highpassfreq);

        int lame_ret = lame_init_params(m_LAMEGlobalFlags);
		if(lame_ret != 0)
		{
			printf("Error initializing LAME");
		}
#endif
#else
        SetServerStatus( "Not compiled with LAME support" );
		LogMessage( LOG_ERROR, "Not compiled with LAME support" );
		return 0;
#endif
	}

    if ( m_Type == ENCODER_AAC )
	{
#if HAVE_FAAC
		faacEncConfigurationPtr m_pConfig;

#ifdef WIN32
        LoadDLL( "libfaac.dll" );
        if ( m_hDLL == NULL ) return 0;
        //FreeLibrary( m_hDLL );
#endif
        if ( m_AACEncoder )
		{
            faacEncClose( m_AACEncoder );
            m_AACEncoder = NULL;
		}

        m_AACEncoder = faacEncOpen( m_CurrentSamplerate, m_CurrentChannels, &m_AACSamplesInput, &m_AACMaxBytesOutput );

        if ( m_AACFIFO ) free( m_AACFIFO );
        m_AACFIFO = (float *)malloc( m_AACSamplesInput * sizeof( float ) * 16 );
        m_AACFIFOEndPos = 0;
        m_pConfig = faacEncGetCurrentConfiguration( m_AACEncoder );
		m_pConfig->mpegVersion = MPEG2;
        m_pConfig->quantqual = atoi( m_AACQuality );

        int cutoff = atoi( m_AACCutoff );
		if(cutoff > 0) m_pConfig->bandWidth = cutoff;

		//m_pConfig->bitRate = (m_CurrentBitrate * 1000) / m_CurrentChannels;
		m_pConfig->allowMidside = 1;
		m_pConfig->useLfe = 0;
		m_pConfig->useTns = 1;
		m_pConfig->aacObjectType = LOW;
		m_pConfig->outputFormat = 1;
		m_pConfig->inputFormat = FAAC_INPUT_FLOAT;

		/* set new config */
        faacEncSetConfiguration( m_AACEncoder, m_pConfig );
#else
        SetServerStatus( "Not compiled with AAC support" );
		LogMessage( LOG_ERROR, "Not compiled with AAC support" );
		return 0;
#endif
	}

    if ( (m_Type == ENCODER_FG_AACP_AUTO) || (m_Type == ENCODER_FG_AACP_LC) || (m_Type == ENCODER_FG_AACP_HE) || (m_Type == ENCODER_FG_AACP_HEV2) )
	{
#if HAVE_FHGAACP
        LoadDLL( "enc_fhgaac.dll" );
        if ( m_hDLL == NULL ) return 0;
        m_CreateAudio3 = (CREATEAUDIO3TYPE)GetProcAddress( m_hDLL, "CreateAudio3" );
        if ( !m_CreateAudio3 )
		{
			LogMessage( LOG_ERROR, "Invalid enc_fhgaac.dll" );
            SetServerStatus( "Invalid enc_fhgaac.dll" );
			return 0;
		}
        m_GetAudioTypes3 = (GETAUDIOTYPES3TYPE)GetProcAddress( m_hDLL, "GetAudioTypes3"  );
        *(void **)&m_FinishAudio3    = (void *)GetProcAddress( m_hDLL, "FinishAudio3"    );
        *(void **)&m_PrepareToFinish = (void *)GetProcAddress( m_hDLL, "PrepareToFinish" );
        if ( m_AACPEncoder )
		{
            delete m_AACPEncoder;
            m_AACPEncoder = NULL;
		}
		unsigned int	outt = mmioFOURCC('A', 'D', 'T', 'S');//1346584897;
		char_t			conf_file[MAX_PATH] = "";	/* Default ini file */
		char_t			sectionName[255] = "audio_adtsaac";
		wsprintf(conf_file, "%s\\shuicast_fhaacp_%d.ini", defaultConfigDir, encoderNumber);
		/*
		switch(m_Type)
		{
			case ENCODER_FG_AACP_AUTO:
				outt = mmioFOURCC('A', 'A', 'C', 'P');
				break;
			case ENCODER_FG_AACP_LC:
				outt = mmioFOURCC('A', 'A', 'C', 'r');
				break;
			case ENCODER_FG_AACP_HE:
				outt = mmioFOURCC('A', 'A', 'C', 'P');
				break;
			case ENCODER_FG_AACP_HEV2:
				outt = mmioFOURCC('A', 'A', 'C', 'P');
				break;
		}
		*/
		/*
		xyzzy - this should be different if using enc_fhgaac.dll
		AACPFlag will be 10(auto) 11(LC) 12(HE-AAC) 13(HE-AACv2) config file is far simpler, i.e.:
			[audio_adtsaac] | [audio_fhgaac]
			profile=AACPFlag-10
			bitrate=sampleRate/1000!!
			surround=0
			shoutcast=1
			//preset=?
			//mode=?
		*/
		char_t tmp[2048];
        wsprintf(tmp, "%d", (int)(m_Type - ENCODER_FG_AACP_AUTO));
		WritePrivateProfileString(sectionName, "profile", tmp, conf_file);
        wsprintf(tmp, "%d", m_CurrentBitrate);
		WritePrivateProfileString(sectionName, "bitrate", tmp, conf_file);
		WritePrivateProfileString(sectionName, "surround", "0", conf_file);
		WritePrivateProfileString(sectionName, "shoutcast", "1", conf_file);
		//WritePrivateProfileString(sectionName, "preset", "0", conf_file);
        m_AACPEncoder = m_CreateAudio3( (int)m_CurrentChannels, (int)m_CurrentSamplerate, 16, mmioFOURCC( 'P', 'C', 'M', ' ' ), &outt, conf_file );
        if ( !m_AACPEncoder )
		{
            SetServerStatus( "Invalid FHGAAC+ settings" );
			LogMessage( LOG_ERROR, "Invalid FHGAAC+ settings" );
			return 0;
		}
#endif
	}
    if ( (m_Type == ENCODER_AACP_HE) || (m_Type == ENCODER_AACP_HE_HIGH) || (m_Type == ENCODER_AACP_LC) )
	{
#if HAVE_AACP

#ifdef _WIN32
        LoadDLL( "enc_aacplus.dll" );
        if ( m_hDLL == NULL ) return 0;
        m_CreateAudio3 = (CREATEAUDIO3TYPE)GetProcAddress( m_hDLL, "CreateAudio3" );
        if ( !m_CreateAudio3 )
		{
			LogMessage( LOG_ERROR, "Invalid DLL (enc_aacplus.dll)" );
            SetServerStatus( "Invalid enc_aacplus.dll" );
			return 0;
		}

        m_GetAudioTypes3 = (GETAUDIOTYPES3TYPE)GetProcAddress( m_hDLL, "GetAudioTypes3"  );
        *(void **)&m_FinishAudio3    = (void *)GetProcAddress( m_hDLL, "FinishAudio3"    );
        *(void **)&m_PrepareToFinish = (void *)GetProcAddress( m_hDLL, "PrepareToFinish" );
		//FreeLibrary(m_hDLL);
#endif
        if ( m_AACPEncoder )
		{
            delete m_AACPEncoder;
            m_AACPEncoder = NULL;
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

		wsprintf(conf_file, "%s\\shuicast_aacp_%d.ini", defaultConfigDir, encoderNumber);
		sprintf(bitrateValue, "%d", bitrateLong);
        switch ( m_Type )
        {
        case ENCODER_AACP_HE:
            strcpy( channelMode, "2" );
            outt = mmioFOURCC( 'A', 'A', 'C', 'P' );
            //strcpy(aacpV2Enable, "0");
            strcpy( sectionName, "audio_aacplus" );
            if ( bitrateLong > 64000 )
            {
                strcpy( channelMode, "2" );
            }
            if ( m_CurrentChannels == 2 )
            {
                if ( m_JointStereo && bitrateLong >=16000 && bitrateLong <= 56000 )
                {
                    strcpy( channelMode, "4" );
                    //strcpy(aacpV2Enable, "1");
                }
                if ( bitrateLong >=12000 && bitrateLong < 16000 )
                {
                    strcpy( channelMode, "4" );
                    //strcpy(aacpV2Enable, "1");
                }
            }
            if ( m_CurrentChannels == 1 || bitrateLong < 12000 )
            {
                strcpy( channelMode, "1" );
            }
            break;
        case ENCODER_AACP_HE_HIGH:
            outt = mmioFOURCC( 'A', 'A', 'C', 'H' );
            //strcpy(aacpV2Enable, "0");
            strcpy( sectionName, "audio_aacplushigh" );
            break;
        case ENCODER_AACP_LC:
            outt = mmioFOURCC( 'A', 'A', 'C', 'r' );
            //strcpy(aacpV2Enable, "0");
            strcpy( sectionName, "audio_aac" );
            break;
        }
        /*
		if(bitrateLong > 128000)
		{
			outt = mmioFOURCC('A', 'A', 'C', 'H');
			strcpy(aacpV2Enable, "1");
			strcpy(sectionName, "audio_aacplushigh");
		}
		if(bitrateLong >= 56000 || (m_Type != ENCODER_AACP_HE))
		{
			if(m_CurrentChannels == 2) strcpy(channelMode, "2");
			else strcpy(channelMode, "1");
		}
		else if(bitrateLong >= 16000)
		{
			if(m_CurrentChannels == 2) strcpy(channelMode, "4");
			else strcpy(channelMode, "1");
		}
		else strcpy(channelMode, "1");
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

        m_AACPEncoder = m_CreateAudio3( (int)m_CurrentChannels, (int)m_CurrentSamplerate, 16, mmioFOURCC( 'P', 'C', 'M', ' ' ), &outt, conf_file );
        if ( !m_AACPEncoder )
		{
            SetServerStatus( "Invalid AAC+ settings" );
			LogMessage( LOG_ERROR, "Invalid AAC+ settings" );
			return 0;
		}

#else
        SetServerStatus( "Not compiled with AAC Plus support" );
		LogMessage( LOG_ERROR, "Not compiled with AAC Plus support" );
		return 0;
#endif
	}

    if ( m_Type == ENCODER_OGG )
	{
#if HAVE_VORBIS
		/* Ogg Vorbis Initialization */
        ogg_stream_clear( &m_OggStreamState );
        vorbis_block_clear( &m_VorbisBlock );
        vorbis_dsp_clear( &m_VorbisDSPState );
        vorbis_info_clear( &m_VorbisInfo );
        vorbis_info_init( &m_VorbisInfo );

		int encode_ret = 0;

        if ( !m_UseBitrate )
		{
            encode_ret = vorbis_encode_setup_vbr( &m_VorbisInfo, m_CurrentChannels, m_CurrentSamplerate, ((float)atof( m_OggQuality ) * (float) .1) );
		}
		else
		{
			int maxbit = -1;
			int minbit = -1;
            if ( m_CurrentBitrateMax > 0 ) maxbit = m_CurrentBitrateMax;
            if ( m_CurrentBitrateMin > 0 ) minbit = m_CurrentBitrateMin;
            encode_ret = vorbis_encode_setup_managed( &m_VorbisInfo, m_CurrentChannels, m_CurrentSamplerate, m_CurrentBitrate * 1000, m_CurrentBitrate * 1000, m_CurrentBitrate * 1000 );
		}
        if ( encode_ret ) vorbis_info_clear( &m_VorbisInfo );

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

        int ret = vorbis_encode_setup_init( &m_VorbisInfo );

		// Now, set up the analysis engine, stream encoder, and other preparation before the encoding begins
        ret = vorbis_analysis_init( &m_VorbisDSPState, &m_VorbisInfo );
        ret = vorbis_block_init( &m_VorbisDSPState, &m_VorbisBlock );

		srand((unsigned int)time(NULL));
        ret = ogg_stream_init( &m_OggStreamState, rand() );

		// Now, build the three header packets and send through to the stream output stage (but defer actual file output until the main encode loop)
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

		if( !GetLockedMetadataFlag() )
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
        if ( strlen( m_SourceDesc ) > 0 )
		{
            sprintf( Streamed, "TRANSCODEDFROM=%s", m_SourceDesc );
			vorbis_comment_add(&vc, Streamed);
		}

		/* Build the packets */
		memset(&header_main, '\000', sizeof(header_main));
		memset(&header_comments, '\000', sizeof(header_comments));
		memset(&header_codebooks, '\000', sizeof(header_codebooks));

        vorbis_analysis_headerout( &m_VorbisDSPState, &vc, &header_main, &header_comments, &header_codebooks );

        ogg_stream_packetin( &m_OggStreamState, &header_main );
        ogg_stream_packetin( &m_OggStreamState, &header_comments );
        ogg_stream_packetin( &m_OggStreamState, &header_codebooks );

        m_InHeader = true;

		ogg_page	og;
		int			eos = 0;
		int			sentbytes = 0;

		while(!eos) 
		{
            int result = ogg_stream_flush( &m_OggStreamState, &og );
			if(result == 0) break;
            sentbytes += SendToServer( m_SCSocket, (char *)og.header, og.header_len, CODEC_TYPE );
            sentbytes += SendToServer( m_SCSocket, (char *)og.body, og.body_len, CODEC_TYPE );
		}

		vorbis_comment_clear(&vc);
		FreeVorbisComments();

#else
        SetServerStatus( "Not compiled with Ogg Vorbis support" );
		LogMessage( LOG_ERROR, "Not compiled with Ogg Vorbis support" );
		return 0;
#endif
	}

    if ( m_Type == ENCODER_FLAC )
	{
#if HAVE_FLAC
        char FullTitle[1024] = {};  // zeroes everything, not just first byte as "" would
        char SongTitle[1024] = {};
        char Artist[1024]    = {};
        //char Streamed[1024]  = {};

        if ( m_FLACEncoder )
		{
            FLAC__stream_encoder_finish( m_FLACEncoder );
            FLAC__stream_encoder_delete( m_FLACEncoder );
            FLAC__metadata_object_delete( m_FLACMetadata );
		}

        m_FLACEncoder  = FLAC__stream_encoder_new();
        m_FLACMetadata = FLAC__metadata_object_new( FLAC__METADATA_TYPE_VORBIS_COMMENT );

        FLAC__stream_encoder_set_streamable_subset( m_FLACEncoder, false );
        //FLAC__stream_encoder_set_client_data( m_FLACEncoder, (void*)this );
        FLAC__stream_encoder_set_channels( m_FLACEncoder, m_CurrentChannels );
        //FLAC__stream_encoder_set_write_callback( m_FLACEncoder,(FLAC__StreamEncoderWriteCallback) FLACWriteCallback, (FLAC__StreamEncoderWriteCallback) FLACWriteCallback );
        //FLAC__stream_encoder_set_metadata_callback( m_FLACEncoder, (FLAC__StreamEncoderMetadataCallback) FLACMetadataCallback );
        srand( (unsigned int)time( NULL ) );

		if ( !GetLockedMetadataFlag() )
		{
			FLAC__StreamMetadata_VorbisComment_Entry entry;
			FLAC__StreamMetadata_VorbisComment_Entry entry3;
			FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ENCODEDBY", "shuicast");
            FLAC__metadata_object_vorbiscomment_append_comment( m_FLACMetadata, entry, true );
            if ( strlen( m_SourceDesc ) > 0 )
			{
				FLAC__StreamMetadata_VorbisComment_Entry entry2;
                FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair( &entry2, "TRANSCODEDFROM", m_SourceDesc );
                FLAC__metadata_object_vorbiscomment_append_comment( m_FLACMetadata, entry2, true );
			}
            GetCurrentSongTitle( SongTitle, Artist, FullTitle );
			FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry3, "TITLE", FullTitle);
            FLAC__metadata_object_vorbiscomment_append_comment( m_FLACMetadata, entry3, true );

		}
        FLAC__stream_encoder_set_ogg_serial_number( m_FLACEncoder, rand() );

        FLAC__StreamEncoderInitStatus ret = FLAC__stream_encoder_init_ogg_stream( m_FLACEncoder, NULL, (FLAC__StreamEncoderWriteCallback)FLACWriteCallback, NULL, NULL, (FLAC__StreamEncoderMetadataCallback)FLACMetadataCallback, (void*)this );
		if(ret == FLAC__STREAM_ENCODER_INIT_STATUS_OK) 
		{
            SetServerStatus( "FLAC initialized" );
		}
		else
		{
            SetServerStatus( "Error initializing FLAC" );
			LogMessage( LOG_ERROR, "Error initializing FLAC" );
			return 0;
		}
#endif
	}

	return 1;
}

#if HAVE_FAAC

void CEncoder::AddToFIFO ( float *samples, int numsamples )
{
    int currentFIFO = m_AACFIFOEndPos;
    for ( int i = 0; i < numsamples; i++ )
    {
        m_AACFIFO[currentFIFO] = samples[i];
        currentFIFO++;
    }
    m_AACFIFOEndPos = currentFIFO;
}

#endif

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
	unsigned char	mp3buffer[LAME_MAXMP3BUFFER];
	int				imp3;
	short int		*int_samples = NULL;
	int				ret = 0;
	int				sentbytes = 0;

    if ( m_IsConnected )
	{
        m_CurrentlyEncoding = true;

		int		samplecounter = 0;
        if ( m_VUMeterCallback )
		{
			if(limiter)
			{
                m_VUMeterCallback( limiter->PeakL, limiter->PeakR, limiter->RmsL, limiter->RmsR );
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
                    m_VUMeterCallback( leftMax, rightMax, leftMax, rightMax );
				}
			}
		}

#if HAVE_VORBIS
        if ( m_Type == ENCODER_OGG )
		{
			// If a song change was detected, close the stream and resend new ;
			// vorbis headers (with new comments) - all done by Icecast2SendMetadata();
            if ( m_Ice2songChange )
			{
				LogMessage( LOG_DEBUG, "Song change processing..." );
                m_Ice2songChange = false;
				Icecast2SendMetadata();
			}

			LogMessage( LOG_DEBUG, "vorbis_analysis_buffer..." );

            float	**buffer = vorbis_analysis_buffer( &m_VorbisDSPState, numsamples );
			samplecounter = 0;
            float * src = samples;
            if ( m_CurrentChannels == 1 )
            {
                while ( samplecounter < numsamples )
                {
                    buffer[0][samplecounter] = *(src++);
                    samplecounter++;
                }
            }
            else
            {
                while ( samplecounter < numsamples )
                {
                    buffer[0][samplecounter] = *(src++);
                    buffer[1][samplecounter] = *(src++);
                    samplecounter++;
                }
            }
            LogMessage( LOG_DEBUG, "vorbis_analysis_wrote..." );

            ret = vorbis_analysis_wrote( &m_VorbisDSPState, numsamples );

            pthread_mutex_lock( &m_Mutex );
			LogMessage( LOG_DEBUG, "OggEncodeDataout..." );
			/* Stream out what we just prepared for Vorbis... */
            sentbytes = OggEncodeDataout();
			LogMessage( LOG_DEBUG, "done OggEncodeDataout..." );
            pthread_mutex_unlock( &m_Mutex );
		}
#endif

#if HAVE_FAAC
        if ( m_Type == ENCODER_AAC )
		{
			float	*buffer = (float *) malloc(numsamples * 2 * sizeof(float));
            FloatScale( buffer, samples, numsamples * 2, m_CurrentChannels );
            AddToFIFO( buffer, numsamples * m_CurrentChannels );

            while ( m_AACFIFOEndPos > (long)m_AACSamplesInput )
			{
                float	*buffer2 = (float *)malloc( m_AACSamplesInput * 2 * sizeof( float ) );
                ExtractFromFIFO( buffer2, m_AACFIFO, m_AACSamplesInput );
				int counter = 0;

                for ( int i = m_AACSamplesInput; i < m_AACFIFOEndPos; i++ )
				{
                    m_AACFIFO[counter] = m_AACFIFO[i];
					counter++;
				}

                m_AACFIFOEndPos = counter;
                unsigned char *aacbuffer = (unsigned char *)malloc( m_AACMaxBytesOutput );
                imp3 = faacEncEncode( m_AACEncoder, (int32_t *)buffer2, m_AACSamplesInput, aacbuffer, m_AACMaxBytesOutput );
				if(imp3) sentbytes = SendToServer( m_SCSocket, (char *)aacbuffer, imp3, CODEC_TYPE );

				if(buffer2) free(buffer2);
				if(aacbuffer) free(aacbuffer);
			}

			if(buffer) free(buffer);
		}
#endif

#if HAVE_FHGAACP
        if ( (m_Type == ENCODER_FG_AACP_AUTO) || (m_Type == ENCODER_FG_AACP_LC) || (m_Type == ENCODER_FG_AACP_HE) || (m_Type == ENCODER_FG_AACP_HEV2) )
		{
			static char outbuffer[32768];
            int cnt = numsamples * m_CurrentChannels;
            int len = cnt * sizeof(short);
			int_samples = (short *) malloc(len);
            float * src = samples;
            short * dst = int_samples;

            if(m_CurrentChannels == 1) 
			{
                int samplecount = 0;
                for(int i = 0; i < numsamples * 2; i += 2)
				{
					int_samples[samplecount] = (short int) (samples[i] * 32767.0);
					samplecount++;
				}
			}
			else
			{
                for(int i = 0; i < cnt; ++i)
                {
                    *(dst++) = (short int) (*(src++) * 32767.0);
                }
            }

			char	*bufcounter = (char *) int_samples;

			for(;;)
			{
				int in_used = 0;
				if(len <= 0) break;
                int enclen = m_AACPEncoder->Encode( in_used, bufcounter, len, &in_used, outbuffer, sizeof( outbuffer ) );

				if(enclen > 0) sentbytes = SendToServer( m_SCSocket, (char *) outbuffer, enclen, CODEC_TYPE );  // can be part of NSV stream
				else break;

				if(in_used > 0) 
				{
					bufcounter += in_used;
					len -= in_used;
				}
			}

			if(int_samples) free(int_samples);
		}
#endif

#if HAVE_AACP
        if ( (m_Type == ENCODER_AACP_HE) || (m_Type == ENCODER_AACP_HE_HIGH) || (m_Type == ENCODER_AACP_LC) )
		{
			static char outbuffer[32768];
            int cnt = numsamples * m_CurrentChannels;
            int len = cnt * sizeof( short );
			int_samples = (short *) malloc(len);
            float * src = samples;
            short * dst = int_samples;

            if ( m_CurrentChannels == 1 )
			{
                int samplecount = 0;
                for ( int i = 0; i < numsamples * 2; i += 2 )
				{
					int_samples[samplecount] = (short int) (samples[i] * 32767.0);
					samplecount++;
				}
			}
			else
			{
                for ( int i = 0; i < cnt; ++i )
                {
                    *(dst++) = (short int)(*(src++) * 32767.0);
                }
            }

			char	*bufcounter = (char *) int_samples;

			for(;;)
			{
				int in_used = 0;
				if(len <= 0) break;
                int enclen = m_AACPEncoder->Encode( in_used, bufcounter, len, &in_used, outbuffer, sizeof( outbuffer ) );
				if(enclen > 0) 
				{
					// can be part of NSV stream
                    sentbytes = SendToServer(  m_SCSocket, (char *)outbuffer, enclen, CODEC_TYPE );
				}
				else break;
				if(in_used > 0) 
				{
					bufcounter += in_used;
					len -= in_used;
				}
			}

			if(int_samples) free(int_samples);
		}
#endif

#if HAVE_LAME
        if ( m_Type == ENCODER_LAME )
		{
			/* Lame encoding is simple, we are passing it interleaved samples */
            int cnt = numsamples * 2;  // TODO: not *m_CurrentChannels (edcast-reborn uses this)?
            int len = cnt * sizeof( short );
            int_samples = (short int *)malloc( len );
            int samplecount = 0;
            float * src = samples;
            short * dst = int_samples;
            if ( m_CurrentChannels == 1 )  // TODO: use FloatScale for all?
            {
                samplecount = numsamples;
                for ( int i = 0; i < numsamples * 2; i += 2 )
                {
                    int_samples[samplecount] = (short)(samples[i] * 32767.0);
                }
            }
            else
            {
                samplecount = cnt;  // TODO: edcast-reborn uses numsamples - wrong? should be *2?
                for ( int i = 0; i < cnt; ++i )
                {
                    *(dst++) = (short)(*(src++) * 32767.0);
                }
            }

#ifdef WIN32
			unsigned long dwWrite = 0;
            /*int err =*/ m_beEncodeChunk( m_hbeStream, samplecount, (short *)int_samples, (PBYTE)mp3buffer, &dwWrite );

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

            imp3 = lame_encode_buffer_float(m_LAMEGlobalFlags, (float *) samples_left, (float *) samples_right, numsamples, mp3buffer, sizeof(mp3buffer));
			if(samples_left) free(samples_left);
			if(samples_right) free(samples_right);
#endif
			if(int_samples) free(int_samples);

			if(imp3 == -1) 
			{
				LogMessage( LOG_ERROR, "mp3 buffer is not big enough!" );
                m_CurrentlyEncoding = false;
				return -1;
			}

			/* Send out the encoded buffer */
			// can be part of NSV stream
            sentbytes = SendToServer( m_SCSocket, (char *)mp3buffer, imp3, CODEC_TYPE );
		}
#endif

#if HAVE_FLAC
        if ( m_Type == ENCODER_FLAC )
		{
			INT32		*int32_samples;

#if 0  // from edcast-reborn's do_encoding_faster - TODO: see LAME
            int cnt = numsamples * m_CurrentChannels;
            int32_samples = (INT32 *) malloc(cnt * sizeof(INT32));
            float * src = samples;
            INT32 * dst = int32_samples;

            for(int i = 0; i < cnt; ++i) 
            {
                *(dst++) = (INT32) (*(src++) * 32767.0);
            }
#else
			int32_samples = (INT32 *) malloc(numsamples * 2 * sizeof(INT32));
			int samplecount = 0;
            if ( m_CurrentChannels == 1 )
			{
				for ( int i = 0; i < numsamples * 2; i += 2 )
				{
					int32_samples[samplecount] = (INT32) (samples[i] * 32767.0);
					samplecount++;
				}
			}
			else
			{
				for ( int i = 0; i < numsamples * 2; ++i )
				{
					int32_samples[i] = (INT32) (samples[i] * 32767.0);
					samplecount++;
				}
			}
#endif
            FLAC__stream_encoder_process_interleaved( m_FLACEncoder, int32_samples, numsamples );

			if(int32_samples) free(int32_samples);
            sentbytes = m_FLACFailure ? 0 : 1;
		}
#endif
		// Generic error checking, if there are any socket problems, the trigger a disconnection handling->..
		if(sentbytes < 0) 
		{
            m_CurrentlyEncoding = false;
			int rret = TriggerDisconnect();
			if (rret == 0) return 0;
		}
	}

    m_CurrentlyEncoding = false;
	return 1;
}

int CEncoder::TriggerDisconnect ()
{
    char_t buf[2046] = "";
    DisconnectFromServer();
    if ( m_ForceStop )
    {
        m_ForceStop = false;
        return 0;
    }
    wsprintf( buf, "Disconnected from server" );
    m_ForcedDisconnect = true;
    m_ForcedDisconnectSecs = time( NULL );
    SetServerStatus( buf );
    return 1;
}

void CEncoder::LoadConfig ()
{
	char	buf[255] = "";
	char	desc[1024] = "";

#ifdef XMMS_PLUGIN
	wsprintf(desc, "This is the named pipe used to communicate with the XMMS effect plugin. Make sure it matches the settings in that plugin");
    GetConfigVariable( gAppName, "SourceURL", "/tmp/shuicastFIFO", m_SourceURL, sizeof( m_SourceURL ), desc );
#else
	wsprintf(desc, "The source URL for the broadcast. It must be in the form http://server:port/mountpoint.  For those servers without a mountpoint (Shoutcast) use http://server:port.");
    GetConfigVariable( gAppName, "SourceURL", "http://localhost/", m_SourceURL, sizeof( m_SourceURL ), desc );
#endif
    if ( m_SourceURLCallback )
	{
        m_SourceURLCallback( this, (char*)m_SourceURL );
	}

	wsprintf(desc, "Destination server details (to where you are encoding).  Valid server types : Shoutcast, Icecast, Icecast2");
    GetConfigVariable( gAppName, "ServerType", "Icecast2", m_ServerTypeName, sizeof( m_ServerTypeName ), desc );
//	wsprintf(desc, "The server to which the stream is sent. It can be a hostname  or IP (example: www.stream.com, 192.168.1.100)");
    GetConfigVariable( gAppName, "Server", "localhost", m_Server, sizeof( m_Server ), NULL );
//	wsprintf(desc, "The port to which the stream is sent. Must be a number (example: 8000)");
    GetConfigVariable( gAppName, "Port", "8000", m_Port, sizeof( m_Port ), NULL );
//	wsprintf(desc, "This is the encoder password for the destination server (example: hackme)");
    GetConfigVariable( gAppName, "ServerPassword", "changemenow", m_Password, sizeof( m_Password ), NULL );
//	wsprintf(desc,"Used for Icecast/Icecast2 servers, The mountpoint must end in .ogg for Vorbis streams and have NO extention for MP3 streams.  If you are sending to a Shoutcast server, this MUST be blank. (example: /mp3, /myvorbis.ogg)");
    GetConfigVariable( gAppName, "ServerMountpoint", "/stream.ogg", m_Mountpoint, sizeof( m_Mountpoint ), NULL );
//	wsprintf(desc,"This setting tells the destination server to list on any available YP listings. Not all servers support this (Shoutcast does, Icecast2 doesn't) (example: 1 for YES, 0 for NO)");
	wsprintf(desc,"YP (Stream Directory) Settings");
    m_PubServ = GetConfigVariable( gAppName, "ServerPublic", 1, desc );
//	wsprintf(desc, "This is used in the YP listing, I think only Shoutcast supports this (example: #mystream)");
    GetConfigVariable( gAppName, "ServerIRC", "", m_ServIRC, sizeof( m_ServIRC ), NULL );
//	wsprintf(desc, "This is used in the YP listing, I think only Shoutcast supports this (example: myAIMaccount)");
    GetConfigVariable( gAppName, "ServerAIM", "", m_ServAIM, sizeof( m_ServAIM ), NULL );
//	wsprintf(desc, "This is used in the YP listing, I think only Shoutcast supports this (example: 332123132)");
    GetConfigVariable( gAppName, "ServerICQ", "", m_ServICQ, sizeof( m_ServICQ ), NULL );
//	wsprintf(desc, "The URL that is associated with your stream. (example: http://www.mystream.com)");
    GetConfigVariable( gAppName, "ServerStreamURL", "http://www.shoutcast.com", m_ServURL, sizeof( m_ServURL ), NULL );
//	wsprintf(desc, "The Stream Name");
    GetConfigVariable( gAppName, "ServerName", "This is my server name", m_ServName, sizeof( m_ServName ), NULL );
//	wsprintf(desc, "A short description of the stream (example: Stream House on Fire!)");
    GetConfigVariable( gAppName, "ServerDescription", "This is my server description", m_ServDesc, sizeof( m_ServDesc ), NULL );
//	wsprintf(desc, "Genre of music, can be anything you want... (example: Rock)");
    GetConfigVariable( gAppName, "ServerGenre", "Rock", m_ServGenre, sizeof( m_ServGenre ), NULL );
	wsprintf(desc, "Reconnect if disconnected from the destination server: 1 for YES, 0 for NO");
    m_AutoReconnect = GetConfigVariable( gAppName, "AutomaticReconnect", 1, desc );
	wsprintf(desc, "How long to wait (in seconds) between reconnect attempts, e.g.: 10");
    m_ReconnectSec = GetConfigVariable( gAppName, "AutomaticReconnectSecs", 10, desc );
    m_AutoConnect = GetConfigVariable( gAppName, "AutoConnect", 0, NULL );
	wsprintf(desc,"Set to 1 to start minimized");
    m_StartMinimized = GetConfigVariable( gAppName, "StartMinimized", 0, desc );
	wsprintf(desc,"Set to 1 to enable limiter");
    m_Limiter = GetConfigVariable( gAppName, "Limiter", 0, desc );
	wsprintf(desc,"Limiter dB, Gain dB and pre-emphasis");
    m_LimitdB = GetConfigVariable( gAppName, "LimitDB", 0, desc );
    m_GaindB = GetConfigVariable( gAppName, "LimitGainDB", 0, NULL );
    m_LimitPre = GetConfigVariable( gAppName, "LimitPRE", 0, NULL );
	wsprintf(desc, "Output codec selection. Valid selections: MP3, OggVorbis, Ogg FLAC, AAC, HE-AAC, HE-AAC High, LC-AAC, FHGAAC-AUTO, FHGAAC-LC, FHGAAC-HE, FHGAAC-HEv2");
    GetConfigVariable( gAppName, "Encode", "OggVorbis", m_EncodeType, sizeof( m_EncodeType ), desc );

    if ( !strncmp( m_EncodeType, "MP3", 3 ) ) m_Type = ENCODER_LAME;
    else if ( !strcmp( m_EncodeType, "FHGAAC-AUTO" ) ) m_Type = ENCODER_FG_AACP_AUTO;
    else if ( !strcmp( m_EncodeType, "FHGAAC-LC"   ) ) m_Type = ENCODER_FG_AACP_LC;
    else if ( !strcmp( m_EncodeType, "FHGAAC-HE"   ) ) m_Type = ENCODER_FG_AACP_HE;
    else if ( !strcmp( m_EncodeType, "FHGAAC-HEv2" ) ) m_Type = ENCODER_FG_AACP_HEV2;
    else if ( !strcmp( m_EncodeType, "HE-AAC"      ) ) m_Type = ENCODER_AACP_HE;
    else if ( !strcmp( m_EncodeType, "HE-AAC High" ) ) m_Type = ENCODER_AACP_HE_HIGH;
    else if ( !strcmp( m_EncodeType, "LC-AAC"      ) ) m_Type = ENCODER_AACP_LC;
    else if ( !strcmp( m_EncodeType, "AAC"         ) ) m_Type = ENCODER_AAC;
    else if ( !strcmp( m_EncodeType, "OggVorbis"   ) ) m_Type = ENCODER_OGG;
    else if ( !strcmp( m_EncodeType, "Ogg FLAC"    ) ) m_Type = ENCODER_FLAC;
    else m_Type = ENCODER_NONE;

    if ( m_StreamTypeCallback )
	{
        if ( m_Type == ENCODER_OGG ) m_StreamTypeCallback( this, (void *) "OggVorbis" );  // TODO: use switch
        if ( m_Type == ENCODER_LAME ) m_StreamTypeCallback( this, (void *) "MP3" );
        if ( m_Type == ENCODER_AAC ) m_StreamTypeCallback( this, (void *) "AAC" );
        if ( (m_Type == ENCODER_AACP_HE) || (m_Type == ENCODER_AACP_HE_HIGH) || (m_Type == ENCODER_AACP_LC) ) m_StreamTypeCallback( this, (void *) "AAC+" );
        if ( (m_Type == ENCODER_FG_AACP_AUTO) || (m_Type == ENCODER_FG_AACP_LC) || (m_Type == ENCODER_FG_AACP_HE) || (m_Type == ENCODER_FG_AACP_HEV2) ) m_StreamTypeCallback( this, (void *) "FHGAAC" );
        if ( m_Type == ENCODER_FLAC ) m_StreamTypeCallback( this, (void *) "OggFLAC" );
	}

    if ( m_DestURLCallback )
	{
        wsprintf( buf, "http://%s:%s%s", m_Server, m_Port, m_Mountpoint );
        m_DestURLCallback( this, (char *)buf );
	}

	wsprintf(desc, "Bitrate. This is the mean bitrate if using VBR.");
    m_CurrentBitrate = GetConfigVariable( gAppName, "BitrateNominal", 128, desc );
	wsprintf(desc,"Minimum and maximum bitrates. Used only for Bitrate Management (not recommended) or LAME VBR (example: 64, 128)");
    m_CurrentBitrateMin = GetConfigVariable( gAppName, "BitrateMin", 128, desc );
    m_CurrentBitrateMax = GetConfigVariable( gAppName, "BitrateMax", 128, NULL );
	wsprintf(desc, "Number of channels. Valid values are (1, 2) - 1 means Mono, 2 means Stereo");
    m_CurrentChannels = GetConfigVariable( gAppName, "NumberChannels", 2, desc );

	wsprintf(desc, "Per encoder Attenuation");
    GetConfigVariable( gAppName, "Attenuation", "0.0", m_AttenuationTable, sizeof( m_AttenuationTable ), desc );
    double atten = -fabs( atof( m_AttenuationTable ) );
    m_Attenuation = pow( 10.0, atten/20.0 );

    wsprintf(desc, "Sample rate for the stream. Valid values depend on whether using Lame or Vorbis. Vorbis supports odd samplerates such as 32kHz and 48kHz, but Lame appears not to. Feel free to experiment (example: 44100, 22050, 11025)");
    m_CurrentSamplerate = GetConfigVariable( gAppName, "Samplerate", 44100, desc );
//	wsprintf(desc, "Vorbis Quality Level. Valid values are between -1 (lowest quality) and 10 (highest).  The lower the quality the lower the output bitrate. (example: -1, 3)");
	wsprintf(desc, "Ogg Vorbis specific settings.  Note: Valid settings for BitrateQuality flag are (Quality, Bitrate Management)");
    GetConfigVariable( gAppName, "OggQuality", "0", m_OggQuality, sizeof( m_OggQuality ), desc );
//	wsprintf(desc,"This flag specifies if you want Vorbis Quality or Bitrate Management.  Quality is always recommended. Valid values are (Bitrate, Quality). (example: Quality, Bitrate Management)");
    GetConfigVariable( gAppName, "OggBitrateQualityFlag", "Quality", m_OggBitQual, sizeof( m_OggBitQual ), NULL );
    m_UseBitrate = false;
    if ( !strncmp( m_OggBitQual, "Q", 1 ) ) m_UseBitrate = false;  // Quality
    if ( !strncmp( m_OggBitQual, "B", 1 ) ) m_UseBitrate = true;   // Bitrate

    char	tempString[255] = "";
	memset(tempString, '\000', sizeof(tempString));
    ReplaceString( m_Server, tempString, " ", "" );
    strcpy( m_Server, tempString );
	memset(tempString, '\000', sizeof(tempString));
    ReplaceString( m_Port, tempString, " ", "" );
    strcpy( m_Port, tempString );

//	wsprintf(desc,"LAME specific settings.  Note: Setting the low/highpass freq to 0 will disable them.");
	wsprintf(desc,"This LAME flag indicates that CBR encoding is desired. If this flag is set then LAME with use CBR, if not set then it will use VBR (and you must then specify a VBR mode). Valid values are (1 for SET, 0 for NOT SET) (example: 1)");
    m_LAMEOptions.cbrflag = GetConfigVariable( gAppName, "LameCBRFlag", 1, desc );
	wsprintf(desc,"A number between 0 and 9 which indicates the desired quality level of the stream.  0 = highest to 9 = lowest");
    m_LAMEOptions.quality = GetConfigVariable( gAppName, "LameQuality", 0, desc );

	wsprintf(desc, "Copywrite flag-> Not used for much. Valid values (1 for YES, 0 for NO)");
    m_LAMEOptions.copywrite = GetConfigVariable( gAppName, "LameCopywrite", 0, desc );
	wsprintf(desc, "Original flag-> Not used for much. Valid values (1 for YES, 0 for NO)");
    m_LAMEOptions.original = GetConfigVariable( gAppName, "LameOriginal", 0, desc );
	wsprintf(desc, "Strict ISO flag-> Not used for much. Valid values (1 for YES, 0 for NO)");
    m_LAMEOptions.strict_ISO = GetConfigVariable( gAppName, "LameStrictISO", 0, desc );
	wsprintf(desc, "Disable Reservior flag-> Not used for much. Valid values (1 for YES, 0 for NO)");
    m_LAMEOptions.disable_reservoir = GetConfigVariable( gAppName, "LameDisableReservior", 1, desc );
	wsprintf(desc, "This specifies the type of VBR encoding LAME will perform if VBR encoding is set (CBRFlag is NOT SET). See the LAME documention for more on what these mean. Valid values are (vbr_rh, vbr_mt, vbr_mtrh, vbr_abr, vbr_cbr)");
    GetConfigVariable( gAppName, "LameVBRMode", "vbr_cbr", m_LAMEOptions.VBR_mode, sizeof( m_LAMEOptions.VBR_mode ), desc );

	wsprintf(desc, "Use LAMEs lowpass filter. If you set this to 0, then no filtering is done - not implemented");
    m_LAMEOptions.lowpassfreq = GetConfigVariable( gAppName, "LameLowpassfreq", 0, desc );
	wsprintf(desc, "Use LAMEs highpass filter. If you set this to 0, then no filtering is done - not implemented");
    m_LAMEOptions.highpassfreq = GetConfigVariable( gAppName, "LameHighpassfreq", 0, desc );

    if ( m_LAMEOptions.lowpassfreq > 0 ) m_LAMELowpassFlag = 1;
    if ( m_LAMEOptions.highpassfreq > 0 ) m_LAMEHighpassFlag = 1;

    wsprintf(desc, "LAME Preset");
	int defaultPreset = 0;
#ifdef _WIN32
	defaultPreset = LQP_NOPRESET;
#endif
	wsprintf(desc, "LAME Preset - Interesting ones are: -1 = None, 0 = Normal Quality, 1 = Low Quality, 2 = High Quality ... 5 = Very High Quality, 11 = ABR, 12 = CBR");
    m_LAMEPreset = GetConfigVariable( gAppName, "LAMEPreset", defaultPreset, desc );

	wsprintf(desc, "AAC Quality Level. Valid values are between 10 (lowest quality) and 500 (highest).");
    GetConfigVariable( gAppName, "AACQuality", "100", m_AACQuality, sizeof( m_AACQuality ), desc );
	wsprintf(desc, "AAC Cutoff Frequency.");
    GetConfigVariable( gAppName, "AACCutoff", "", m_AACCutoff, sizeof( m_AACCutoff ), desc );

         if ( !strcmp( m_ServerTypeName, "KasterBlaster" ) ) m_ServerType = SERVER_SHOUTCAST;
    else if ( !strcmp( m_ServerTypeName, "Shoutcast"     ) ) m_ServerType = SERVER_SHOUTCAST;
    else if ( !strcmp( m_ServerTypeName, "Icecast"       ) ) m_ServerType = SERVER_ICECAST;
    else if ( !strcmp( m_ServerTypeName, "Icecast2"      ) ) m_ServerType = SERVER_ICECAST2;
    else m_ServerType = SERVER_NONE;

    if ( m_ServerTypeCallback )
	{
        m_ServerTypeCallback( this, (void *)m_ServerTypeName );
	}

    if ( m_ServerNameCallback )
	{
		char	*pdata = NULL;
        int		pdatalen = strlen( m_ServDesc ) + strlen( m_ServName ) + strlen( " () " ) + 1;

		pdata = (char *) calloc(1, pdatalen);
        wsprintf( pdata, "%s (%s)", m_ServName, m_ServDesc );
        m_ServerNameCallback( this, (void *)pdata );
		free(pdata);
	}

	wsprintf(desc, "If recording from linein, what device to use (not needed for win32) (example: /dev/dsp)");
	GetConfigVariable( gAppName, "AdvRecDevice", "/dev/dsp", buf, sizeof(buf), desc);
    strcpy( m_AdvRecDevice, buf );
	wsprintf(desc, "If recording from linein, what sample rate to open the device with. (example: 44100, 48000)");
	GetConfigVariable( gAppName, "LiveInSamplerate", "44100", buf, sizeof(buf), desc);
    m_LiveInSamplerate = atoi( buf );
	wsprintf(desc, "Used for any window positions (X value)");
    m_LastX = GetConfigVariable( gAppName, "lastX", 0, desc );
	wsprintf(desc, "Used for any window positions (Y value)");
    m_LastY = GetConfigVariable( gAppName, "lastY", 0, desc );
	wsprintf(desc, "Used for plugins that show the VU meter");
    m_ShowVUMeter = GetConfigVariable( gAppName, "showVU", 0, desc );

    wsprintf(desc, "Flag which indicates we are recording from line in");
	int lineInDefault = 0;
#ifdef SHUICASTSTANDALONE  // TODO: get rid of this here in lib
	lineInDefault = 1;
#endif
    m_LiveRecordingFlag = GetConfigVariable( gAppName, "LineInFlag", lineInDefault, desc );

	wsprintf(desc, "Locked Metadata");
    GetConfigVariable( gAppName, "LockMetadata", "", m_ManualSongTitle, sizeof( m_ManualSongTitle ), desc );
	wsprintf(desc, "Flag which indicates if we are using locked metadata");
    m_LockSongTitle = GetConfigVariable( gAppName, "LockMetadataFlag", 0, desc );
	wsprintf(desc, "Save directory for archive streams");
    GetConfigVariable(  gAppName, "SaveDirectory", "", m_SaveDirectory, sizeof( m_SaveDirectory ), desc );
	wsprintf(desc, "Flag which indicates if we are saving archives");
    m_SaveDirectoryFlag = GetConfigVariable( gAppName, "SaveDirectoryFlag", 0, desc );
	wsprintf(desc, "Log Level 1 = LOG_ERROR, 2 = LOG_ERROR+LOG_INFO, 3 = LOG_ERROR+LOG_INFO+LOG_DEBUG");
    m_LogLevel = GetConfigVariable( gAppName, "LogLevel", 2, desc );
	wsprintf(desc, "Log File");
    GetConfigVariable( gAppName, "LogFile", defaultLogFileName, m_LogFile, sizeof( m_LogFile ), desc );
    SetLogFile( m_LogFile );
	wsprintf(desc, "Save Archives in WAV format");
    m_SaveAsWAV = GetConfigVariable( gAppName, "SaveAsWAV", 0, desc );
	wsprintf(desc, "ASIO channel selection 0 1 or 2 only");
    m_AsioSelectChannel = GetConfigVariable( gAppName, "AsioSelectChannel", 0, desc );
	wsprintf(desc, "ASIO channel");
    GetConfigVariable( gAppName, "AsioChannel", "", m_AsioChannel, sizeof( m_AsioChannel ), desc );
	wsprintf(desc, "Encoder Scheduler");
    m_EnableScheduler = GetConfigVariable( gAppName, "EnableEncoderScheduler", 0, desc );

#define DOW_GETCONFIG( dow ) \
    wsprintf( desc, #dow##" Schedule" ); \
    m_##dow##Enable  = GetConfigVariable( gAppName, #dow##"Enable",   1, desc ); \
    m_##dow##OnTime  = GetConfigVariable( gAppName, #dow##"OnTime",   0, NULL ); \
    m_##dow##OffTime = GetConfigVariable( gAppName, #dow##"OffTime", 24, NULL ); 

	DOW_GETCONFIG( Monday    );
	DOW_GETCONFIG( Tuesday   );
	DOW_GETCONFIG( Wednesday );
	DOW_GETCONFIG( Thursday  );
	DOW_GETCONFIG( Friday    );
	DOW_GETCONFIG( Saturday  );
	DOW_GETCONFIG( Sunday    );

	wsprintf(desc, "Append this string to all metadata");
	GetConfigVariable( gAppName, "MetadataAppend", "", metadataAppendString, sizeof(metadataAppendString),  desc);
	wsprintf(desc, "Remove this string (and everything after) from the window title of the window class the metadata is coming from");
    GetConfigVariable( gAppName, "MetadataRemoveAfter", "", metadataRemoveStringAfter, sizeof( metadataRemoveStringAfter ), desc );
	wsprintf(desc,"Remove this string (and everything before) from the window title of the window class the metadata is coming from");
    GetConfigVariable( gAppName, "MetadataRemoveBefore", "", metadataRemoveStringBefore, sizeof( metadataRemoveStringBefore ), desc );
	wsprintf(desc, "Window classname to grab metadata from (uses window title)");
    GetConfigVariable( gAppName, "MetadataWindowClass", "", metadataWindowClass, sizeof( metadataWindowClass ), desc );
	wsprintf(desc, "Indicator which tells ShuiCast to grab metadata from a defined window class");
    metadataWindowClassInd = GetConfigVariable( gAppName, "MetadataWindowClassInd", 0, NULL ) != 0;
	wsprintf(desc, "LAME Joint Stereo Flag");
    m_JointStereo = GetConfigVariable( gAppName, "LAMEJointStereo", 1, desc );
	wsprintf(desc, "Set to 1, this encoder will record from DSP regardless of live record state");
    m_ForceDSPRecording = GetConfigVariable( gAppName, "ForceDSPrecording", 0, desc );
	wsprintf(desc, "Set ThreeHourBug=1 if your stream distorts after 3 hours 22 minutes and 56 seconds");
    m_ThreeHourBug = GetConfigVariable( gAppName, "ThreeHourBug", 0, desc );
	wsprintf(desc, "Set SkipCloseWarning=1 to remove the windows close warning");
    m_SkipCloseWarning = GetConfigVariable( gAppName, "SkipCloseWarning", 0, desc );
	wsprintf(desc, "Set ASIO rate to 44100 or 48000");
    m_AsioRate = GetConfigVariable( gAppName, "AsioRate", 48000, desc );

	/* Set some derived values */
	char	localBitrate[255] = "";
	char	mode[50] = "";

    if ( m_CurrentChannels == 1 ) strcpy(mode, "Mono");
    else if ( m_CurrentChannels == 2 ) strcpy(mode, "Stereo");

    if ( m_BitrateCallback )
    {
        if ( m_Type == ENCODER_OGG )
    	{
            if ( !m_UseBitrate )  /* Quality */
			{
                wsprintf( localBitrate, "Vorbis: Quality %s/%s/%d", m_OggQuality, mode, m_CurrentSamplerate );
			}
			else 
			{
                wsprintf( localBitrate, "Vorbis: %dkbps/%s/%d", m_CurrentBitrate, mode, m_CurrentSamplerate );
			}
		}

        if ( m_Type == ENCODER_LAME )
	    {
            if ( m_LAMEOptions.cbrflag ) wsprintf( localBitrate, "MP3: %dkbps/%dHz/%s", m_CurrentBitrate, m_CurrentSamplerate, mode );
			else wsprintf( localBitrate, "MP3: (%d/%d/%d)/%s/%d", m_CurrentBitrateMin, m_CurrentBitrate, m_CurrentBitrateMax, mode, m_CurrentSamplerate );
		}

        if ( m_Type == ENCODER_AAC )
	    {
            wsprintf( localBitrate, "AAC: Quality %s/%dHz/%s", m_AACQuality, m_CurrentSamplerate, mode );
		}

        if ( (m_Type == ENCODER_FG_AACP_AUTO) || (m_Type == ENCODER_FG_AACP_LC) || (m_Type == ENCODER_FG_AACP_HE) || (m_Type == ENCODER_FG_AACP_HEV2) )
	    {
			char enc[20];
            switch ( m_Type )
			{
            case ENCODER_FG_AACP_AUTO: strcpy( enc, "AACP-AUTO(fh)" ); break;
            case ENCODER_FG_AACP_LC:   strcpy( enc, "LC-AAC(fh)"    ); break;
            case ENCODER_FG_AACP_HE:   strcpy( enc, "HE-AAC(fh)"    ); break;
            case ENCODER_FG_AACP_HEV2: strcpy( enc, "HE-AACv2(fh)"  ); break;
			}
            wsprintf( localBitrate, "%s: %dkbps/%dHz", enc, m_CurrentBitrate, m_CurrentSamplerate );
		}

        if ( (m_Type == ENCODER_AACP_HE) || (m_Type == ENCODER_AACP_HE_HIGH) || (m_Type == ENCODER_AACP_LC) )
	    {
            long	bitrateLong = m_CurrentBitrate * 1000;
			char enc[20];

            switch ( m_Type )
			{
            case ENCODER_AACP_HE:
				strcpy(enc, "HE-AAC(ct)");
				if(bitrateLong > 64000)
				{
					strcpy(mode, "Stereo");
                    if ( m_CurrentChannels == 1 ) strcat(mode, "*");
				}
				else if(bitrateLong > 56000) 
				{
                    if ( m_CurrentChannels == 2 ) strcpy(mode, "Stereo");
					else strcpy(mode, "Mono");
				}
				else if(bitrateLong >= 16000) 
				{
                    if ( m_CurrentChannels == 2 )
					{
                        if ( m_JointStereo && bitrateLong <= 56000 ) strcpy( mode, "PS" );
						else strcpy(mode, "Stereo");
					}
					else strcpy(mode, "Mono");
				}
				else if(bitrateLong >= 12000) 
				{
                    if ( m_CurrentChannels == 2 )
					{
						strcpy(mode, "PS");
                        if ( !m_JointStereo ) strcat( mode, "*" );
					}
					else strcpy(mode, "Mono");
				}
				else 
				{
					strcpy(mode, "Mono");
                    if ( m_CurrentChannels != 1 ) strcat(mode, "*");
				}
				break;
            case ENCODER_AACP_HE_HIGH:
				strcpy(enc, "HE-AAC High(ct)");
				strcpy(mode, "Stereo");
				break;
            case ENCODER_AACP_LC:
				strcpy(enc, "LC-AAC(ct)");
				strcpy(mode, "Stereo");
				break;
			}
            wsprintf( localBitrate, "%s: %dkbps/%dHz/%s", enc, m_CurrentBitrate, m_CurrentSamplerate, mode );
		}

        if ( m_Type == ENCODER_FLAC )
	    {
            wsprintf( localBitrate, "FLAC: %dHz/%s", m_CurrentSamplerate, mode );
		}
        m_BitrateCallback( this, (void *)localBitrate );
    }

    SetServerStatus( "Disconnected" );

	wsprintf(desc, "Number of encoders to use");
    m_NumEncoders = GetConfigVariable( gAppName, "NumEncoders", 0, desc );  // TODO: only for gMain
	wsprintf(desc, "Enable external metadata calls (DISABLED, URL, FILE)");
    GetConfigVariable( gAppName, "ExternalMetadata", "DISABLED", externalMetadata, sizeof( externalMetadata ), desc );
	wsprintf(desc, "URL to retrieve for external metadata");
    GetConfigVariable( gAppName, "ExternalURL", "", externalURL, sizeof( externalURL ), desc );
	wsprintf(desc, "File to retrieve for external metadata");
    GetConfigVariable( gAppName, "ExternalFile", "", externalFile, sizeof( externalFile ), desc );
	wsprintf(desc, "Interval for retrieving external metadata");
    GetConfigVariable( gAppName, "ExternalInterval", "60", externalInterval, sizeof( externalInterval ), desc );
	wsprintf(desc, "Advanced setting");
    GetConfigVariable( gAppName, "OutputControl", "", outputControl, sizeof( outputControl ), desc );
	wsprintf(desc, "Windows Recording Device");
    GetConfigVariable( gAppName, "WindowsRecDevice", "", buf, sizeof( buf ), desc );
    strcpy( m_RecDevice, buf );
	wsprintf(desc, "Windows Recording Sub Device");
    GetConfigVariable( gAppName, "WindowsRecSubDevice", "", buf, sizeof( buf ), desc );
    strcpy( m_RecSubDevice, buf );
	wsprintf(desc, "LAME Joint Stereo Flag");
    m_JointStereo = GetConfigVariable( gAppName, "LAMEJointStereo", 1, desc );
}

void CEncoder::StoreConfig ()
{
	strcpy(gAppName, "shuicast");

	char	tempString[1024] = "";

	memset(tempString, '\000', sizeof(tempString));
    ReplaceString( m_Server, tempString, " ", "" );
    strcpy( m_Server, tempString );

	memset(tempString, '\000', sizeof(tempString));
    ReplaceString( m_Port, tempString, " ", "" );
    strcpy( m_Port, tempString );

    PutConfigVariable( gAppName, "SourceURL", m_SourceURL );
    PutConfigVariable( gAppName, "ServerType", m_ServerTypeName );
    PutConfigVariable( gAppName, "Server", m_Server );
    PutConfigVariable( gAppName, "Port", m_Port );
    PutConfigVariable( gAppName, "ServerMountpoint", m_Mountpoint );
    PutConfigVariable( gAppName, "ServerPassword", m_Password );
    PutConfigVariable( gAppName, "ServerPublic", m_PubServ );
    PutConfigVariable( gAppName, "ServerIRC", m_ServIRC );
    PutConfigVariable( gAppName, "ServerAIM", m_ServAIM );
    PutConfigVariable( gAppName, "ServerICQ", m_ServICQ );
    PutConfigVariable( gAppName, "ServerStreamURL", m_ServURL );
    PutConfigVariable( gAppName, "ServerDescription", m_ServDesc );
    PutConfigVariable( gAppName, "ServerName", m_ServName );
    PutConfigVariable( gAppName, "ServerGenre", m_ServGenre );
    PutConfigVariable( gAppName, "AutomaticReconnect", m_AutoReconnect );
    PutConfigVariable( gAppName, "AutomaticReconnectSecs", m_ReconnectSec );
    PutConfigVariable( gAppName, "AutoConnect", m_AutoConnect );
    PutConfigVariable( gAppName, "StartMinimized", m_StartMinimized );
    PutConfigVariable( gAppName, "Limiter", m_Limiter );
    PutConfigVariable( gAppName, "LimitDB", m_LimitdB );
    PutConfigVariable( gAppName, "LimitGainDB", m_GaindB );
    PutConfigVariable( gAppName, "LimitPRE", m_LimitPre );
    PutConfigVariable( gAppName, "Encode", m_EncodeType );

    PutConfigVariable( gAppName, "BitrateNominal", m_CurrentBitrate );
    PutConfigVariable( gAppName, "BitrateMin", m_CurrentBitrateMin );
    PutConfigVariable( gAppName, "BitrateMax", m_CurrentBitrateMax );
    PutConfigVariable( gAppName, "NumberChannels", m_CurrentChannels );
    PutConfigVariable( gAppName, "Attenuation", m_AttenuationTable );
    PutConfigVariable( gAppName, "Samplerate", m_CurrentSamplerate );
    PutConfigVariable( gAppName, "OggQuality", m_OggQuality );
    if ( m_UseBitrate ) strcpy( m_OggBitQual, "Bitrate" );
    else strcpy( m_OggBitQual, "Quality" );
    PutConfigVariable( gAppName, "OggBitrateQualityFlag", m_OggBitQual );
    PutConfigVariable( gAppName, "LameCBRFlag", m_LAMEOptions.cbrflag );
    PutConfigVariable( gAppName, "LameQuality", m_LAMEOptions.quality );
    PutConfigVariable( gAppName, "LameCopywrite", m_LAMEOptions.copywrite );
    PutConfigVariable( gAppName, "LameOriginal", m_LAMEOptions.original );
    PutConfigVariable( gAppName, "LameStrictISO", m_LAMEOptions.strict_ISO );
    PutConfigVariable( gAppName, "LameDisableReservior", m_LAMEOptions.disable_reservoir );
    PutConfigVariable( gAppName, "LameVBRMode", m_LAMEOptions.VBR_mode );
    PutConfigVariable( gAppName, "LameLowpassfreq", m_LAMEOptions.lowpassfreq );
    PutConfigVariable( gAppName, "LameHighpassfreq", m_LAMEOptions.highpassfreq );
    PutConfigVariable( gAppName, "LAMEPreset", m_LAMEPreset );
    PutConfigVariable( gAppName, "AACQuality", m_AACQuality );
    PutConfigVariable( gAppName, "AACCutoff", m_AACCutoff );
    PutConfigVariable( gAppName, "AdvRecDevice", m_AdvRecDevice );
    PutConfigVariable( gAppName, "LiveInSamplerate", m_LiveInSamplerate );
    PutConfigVariable( gAppName, "LineInFlag", m_LiveRecordingFlag );
    PutConfigVariable( gAppName, "lastX", m_LastX );
    PutConfigVariable( gAppName, "lastY", m_LastY );
    PutConfigVariable( gAppName, "showVU", m_ShowVUMeter );
    PutConfigVariable( gAppName, "LockMetadata", m_ManualSongTitle );
    PutConfigVariable( gAppName, "LockMetadataFlag", m_LockSongTitle );
    PutConfigVariable( gAppName, "SaveDirectory", m_SaveDirectory );
    PutConfigVariable( gAppName, "SaveDirectoryFlag", m_SaveDirectoryFlag );
    PutConfigVariable( gAppName, "SaveAsWAV", m_SaveAsWAV );
    PutConfigVariable( gAppName, "LogFile", m_LogFile );
    PutConfigVariable( gAppName, "LogLevel", m_LogLevel );
    PutConfigVariable( gAppName, "AsioSelectChannel", m_AsioSelectChannel );
    PutConfigVariable( gAppName, "AsioChannel", m_AsioChannel );
    PutConfigVariable( gAppName, "EnableEncoderScheduler", m_EnableScheduler );

#define PUTDOWVARS( dow ) \
    PutConfigVariable( gAppName, #dow##"Enable",  m_##dow##Enable  ); \
    PutConfigVariable( gAppName, #dow##"OnTime",  m_##dow##OnTime  ); \
    PutConfigVariable( gAppName, #dow##"OffTime", m_##dow##OffTime );

    PUTDOWVARS( Monday    );
    PUTDOWVARS( Tuesday   );
    PUTDOWVARS( Wednesday );
    PUTDOWVARS( Thursday  );
    PUTDOWVARS( Friday    );
    PUTDOWVARS( Saturday  );
    PUTDOWVARS( Sunday    );

    PutConfigVariable( gAppName, "NumEncoders", m_NumEncoders );
	PutConfigVariable( gAppName, "ExternalMetadata", externalMetadata);
	PutConfigVariable( gAppName, "ExternalURL", externalURL);
	PutConfigVariable( gAppName, "ExternalFile", externalFile);
	PutConfigVariable( gAppName, "ExternalInterval", externalInterval);
    PutConfigVariable( gAppName, "OutputControl", outputControl );
	PutConfigVariable( gAppName, "MetadataAppend", metadataAppendString);
	PutConfigVariable( gAppName, "MetadataRemoveBefore", metadataRemoveStringBefore);
	PutConfigVariable( gAppName, "MetadataRemoveAfter", metadataRemoveStringAfter);
	PutConfigVariable( gAppName, "MetadataWindowClass", metadataWindowClass);
    PutConfigVariable( gAppName, "MetadataWindowClassInd", metadataWindowClassInd );
    PutConfigVariable( gAppName, "WindowsRecDevice", m_RecDevice );
    PutConfigVariable( gAppName, "WindowsRecSubDevice", m_RecSubDevice );
    PutConfigVariable( gAppName, "LAMEJointStereo", m_JointStereo );
    PutConfigVariable( gAppName, "ForceDSPrecording", m_ForceDSPRecording );
    PutConfigVariable( gAppName, "ThreeHourBug", m_ThreeHourBug );
    PutConfigVariable( gAppName, "SkipCloseWarning", m_SkipCloseWarning );
    PutConfigVariable( gAppName, "AsioRate", m_AsioRate );
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
        if ( m_SaveFilePtr )
		{
            if ( m_SaveAsWAV )
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

                    fwrite( int_samples, sizeofData, 1, m_SaveFilePtr );
                    m_ArchiveWritten += sizeofData;
					free(int_samples);

					/*
					 * int sizeofData = nsamples*nchannels*sizeof(float);
					 * fwrite(samples, sizeofData, 1, m_SaveFilePtr);
					 * m_ArchiveWritten += sizeofData;
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
                    fwrite( int_samples, sizeofData, 1, m_SaveFilePtr );
                    m_ArchiveWritten += sizeofData;
					free(int_samples);
				}
			}
		}
		if(current_insamplerate != in_samplerate)
		{
			ResetResampler();
			current_insamplerate = in_samplerate;
		}

		if(current_nchannels != nchannels) 
		{
			ResetResampler();
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
            InitResampler( in_samplerate, nchannels );
			samples_resampled = (float *) malloc(sizeof(float) * buf_samples * nchannels);
			memset(samples_resampled, '\000', sizeof(float) * buf_samples * nchannels);
			LogMessage( LOG_DEBUG, "calling ConvertAudio" );
			long	out_samples = ConvertAudio( samplePtr, samples_resampled, nsamples, buf_samples );
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

			//if(samples_resampled_int) free(samples_resampled_int);
			if(samples_resampled) free(samples_resampled);
		}
		else 
		{
			LogMessage( LOG_DEBUG, "DoEncoding start" );
            ret = DoEncoding( (float *)samples_rechannel, nsamples, NULL );
			LogMessage( LOG_DEBUG, "DoEncoding end (%d)", ret );
		}

		if(samples_rechannel) free(samples_rechannel);
		if(working_samples) free(working_samples);
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
		if(out_nch == 1) samples = limiter->outputMono + dataoffset * limiter->outputSize * 2;
		else samples = limiter->outputStereo + dataoffset * limiter->outputSize * 2;

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

        if ( m_SaveFilePtr )
		{
            if ( m_SaveAsWAV )
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
                    fwrite( int_samples, sizeofData, 1, m_SaveFilePtr );
                    m_ArchiveWritten += sizeofData;
					free(int_samples);
					//int sizeofData = nsamples*nchannels*sizeof(float);
					//fwrite(samples, sizeofData, 1, m_SaveFilePtr);
					//m_ArchiveWritten += sizeofData;
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
						//if(nchannels > 1)  // always the case because nchannels is const 2
							int_samples[k++] = (short int) (samples[i+rightChan] * 32767.f);
					}
                    fwrite( int_samples, sizeofData, 1, m_SaveFilePtr );
                    m_ArchiveWritten += sizeofData;
					free(int_samples);
				}
			}
		}

		if(current_insamplerate != in_samplerate)
		{
			ResetResampler();
			current_insamplerate = in_samplerate;
		}

		if(current_nchannels != nchannels) 
		{
			ResetResampler();
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
            InitResampler( in_samplerate, nchannels );
			samples_resampled = (float *) malloc(sizeof(float) * buf_samples * nchannels);
			LogMessage( LOG_DEBUG, "calling ConvertAudio" );
			long	out_samples = ConvertAudio( samplePtr, samples_resampled, nsamples, buf_samples );
			LogMessage( LOG_DEBUG, "ready to do encoding" );

			if(out_samples > 0) 
			{
				/* Here is the call to actually do the encoding->... */
				LogMessage( LOG_DEBUG, "DoEncoding (resampled) start" );
                ret = DoEncoding( (float *)(samples_resampled), out_samples, limiter );
				LogMessage( LOG_DEBUG, "DoEncoding end (%d)", ret );
			}

			if(samples_resampled) free(samples_resampled);
            //if(samples_rechannel) free(samples_rechannel);
		}
		else 
		{
			LogMessage( LOG_DEBUG, "DoEncoding start" );
            ret = DoEncoding( samples, nsamples, limiter );
			LogMessage( LOG_DEBUG, "DoEncoding end (%d)", ret );
		}

		if(working_samples) free(working_samples);
		LogMessage( LOG_DEBUG, "%d Calling handle output - Ret = %d", encoderNumber, ret );
	}

	return ret;
}

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
    AddConfigVariable( "WindowsRecDevice" );
    AddConfigVariable( "WindowsRecSubDevice" );
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
    AddConfigVariable( "StartMinimized" );
    AddConfigVariable( "ThreeHourBug" );
}

void CEncoder::AddASIOSettings ()
{
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

void CEncoder::LogMessage ( int type, const char_t *source, int line, const char_t *fmt, ...)
{
	va_list parms;
	char	errortype[25] = "";
	int	addNewline = 1;
	struct tm *tp;
	time_t t;
	char    timeStamp[255];
	char	sourceLine[1024] = "";
	char *p1 = NULL;

#ifdef WIN32
	p1 = strrchr((char_t*)source, '\\');
#else
    p1 = strrchr((char_t*)source, '/');
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

    if ( type <= m_LogLevel )
	{
		va_start(parms, fmt);
		char_t	logfile[1024] = "";

        if ( !m_LogFilePtr )
		{
            wsprintf( logfile, "%s.log", m_LogFile );
            m_LogFilePtr = fopen( logfile, "a" );
		}
		
        if ( !m_LogFilePtr )
		{
			fprintf(stdout, "Cannot open logfile: %s(%s:%d): ", logfile, sourceLine, line);
			vfprintf(stdout, fmt, parms);
			va_end(parms);
			if (addNewline) fprintf(stdout, "\n");
		}
		else
		{
            fprintf( m_LogFilePtr, "%s %s(%s:%d): ", timeStamp, errortype, sourceLine, line );
            vfprintf( m_LogFilePtr, fmt, parms );
			va_end(parms);
            if ( addNewline ) fprintf( m_LogFilePtr, "\n" );
            fflush( m_LogFilePtr );
		}
	}
}

char_t *CEncoder::GetWindowsRecordingDevice ()
{
    return m_RecDevice;
}

void CEncoder::SetWindowsRecordingDevice ( char_t *device )
{
    strcpy( m_RecDevice, device );
}

char_t *CEncoder::GetWindowsRecordingSubDevice ()
{
    return m_RecSubDevice;
}

void CEncoder::SetWindowsRecordingSubDevice ( char_t *device )
{
    strcpy( m_RecSubDevice, device );
}

/*
	return - 
		0 : exists, config exists
		1 : exists, no config
		2 : locn exists, subfolder does not
		3 : locn does not exist
		4 : other error
*/
int CEncoder::GetAppData ( bool checkonly, int locn, DWORD flags, LPCSTR subdir, LPCSTR configname, LPSTR strdestn ) const
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
        if ( CheckLocalDir( strdestn, configname ) )
		{
			retVal = 0;
		}
		// check if configname exists, set retVal = 0 if true
	}
	return retVal;
}

bool CEncoder::CheckLocalDir ( LPCSTR dir, LPCSTR file ) const
{
	char tmpfile[MAX_PATH] = "";
	FILE *filep;
	if(file == NULL)
	{
		wsprintf(tmpfile, "%s\\shuicast.tmp", dir);
		filep = fopen(tmpfile, "w");
	}
	else
	{
		wsprintf(tmpfile, "%s\\%s", dir, file);
		filep = fopen(tmpfile, "r");
	}
	if (filep) 
	{
		fclose(filep);
		if(file == NULL) _unlink(tmpfile);
        return true;
    }
    return false;
}

bool _getDirName(LPCSTR inDir, LPSTR dst, int lvl=1) // base
{
	// inDir = ...\winamp\Plugins
	char * dir = _strdup(inDir);
	bool retval = false;
	// remove trailing slash

	if(dir[strlen(dir)-1] == '\\') dir[strlen(dir)-1] = '\0';
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

void CEncoder::LoadConfigs ( char *currentDir, char *subdir, char * logPrefix, char *currentConfigDir, bool inWinamp ) const  // TODO: not used yet, see MainWindow.cpp
{
	char tmpfile[MAX_PATH] = "";
	char tmp2file[MAX_PATH] = "";
	char cfgfile[MAX_PATH];
	wsprintf(cfgfile, "%s_0.cfg", logPrefix);

    bool canUseCurrent = CheckLocalDir( currentDir, NULL );
	bool hasCurrentData = false;
	bool hasAppData = false;
	bool canUseAppData = false;
	bool hasProgramData = false;
	bool canUseProgramData = false;
	if(canUseCurrent)
	{
        hasCurrentData = CheckLocalDir( currentDir, cfgfile );
	}
	if(!hasCurrentData)
	{
		int iHasAppData = -1;
		if(inWinamp)
		{
			int iHasWinampDir = -1;
			int iHasPluginDir = -1;
			char wasubdir[MAX_PATH] = "";
			char wa_instance[MAX_PATH] = "";
		
			_getDirName(currentDir, wa_instance, 1); //...../{winamp name}/ - we want {winamp name}
			if(!strcmp(wa_instance, "plugins"))
			{
				_getDirName(currentDir, wa_instance, 2); //...../{winamp name}/plugins - we want {winamp name}
			}

			wsprintf(wasubdir, "%s", wa_instance);
            iHasWinampDir = GetAppData( false, CSIDL_APPDATA, SHGFP_TYPE_CURRENT, wasubdir, cfgfile, tmpfile );
			if(iHasWinampDir < 2)
			{
				wsprintf(wasubdir, "%s\\Plugins", wa_instance);
                iHasPluginDir = GetAppData( false, CSIDL_APPDATA, SHGFP_TYPE_CURRENT, wasubdir, cfgfile, tmpfile );
				if(iHasPluginDir < 2)
				{
					wsprintf(wasubdir, "%s\\Plugins\\%s", wa_instance, subdir);
                    iHasAppData = GetAppData( true, CSIDL_APPDATA, SHGFP_TYPE_CURRENT, wasubdir, cfgfile, tmpfile );
				}
			}
		}
		else
		{
			/*
			int iHasInstanceDir = -1;
			int iHasPluginDir = -1;
			char instance_subdir[MAX_PATH] = "";
			char _instance[MAX_PATH] = "";
		
			_getDirName(currentDir, _instance, 1); //...../{winamp name}/ - we want {winamp name}
			if(!strcmp(_instance, "plugins"))
			{
				_getDirName(currentDir, _instance, 2); //...../{winamp name}/plugins - we want {winamp name}
			}

			wsprintf(instance_subdir, "%s", _instance);
			iHasInstanceDir = GetAppData(false, CSIDL_LOCAL_APPDATA, SHGFP_TYPE_CURRENT, instance_subdir, cfgfile, tmpfile);
			if(iHasInstanceDir < 2)
			{
				wsprintf(instance_subdir, "%s\\Plugins", _instance);
				iHasPluginDir = GetAppData(false, CSIDL_LOCAL_APPDATA, SHGFP_TYPE_CURRENT, instance_subdir, cfgfile, tmpfile);
				if(iHasPluginDir < 2)
				{
					wsprintf(instance_subdir, "%s\\Plugins\\%s", _instance, subdir);
					iHasAppData = GetAppData(true, CSIDL_LOCAL_APPDATA, SHGFP_TYPE_CURRENT, instance_subdir, cfgfile, tmpfile);
				}
			}
			*/
            iHasAppData = GetAppData( true, CSIDL_LOCAL_APPDATA, SHGFP_TYPE_CURRENT, subdir, cfgfile, tmpfile );
		}

		hasAppData = (iHasAppData == 0);
		canUseAppData = (iHasAppData < 2);
		if(!hasAppData)
		{
            int iHasProgramData = GetAppData( true, CSIDL_COMMON_APPDATA, SHGFP_TYPE_CURRENT, subdir, cfgfile, tmp2file );
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
