/*
Oddsock - MySocket
Copyright (C) 2000  Oddsock

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

oddsock@oddsock.org
*/
// Socket.h: interface for the CMySocket class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#ifdef _WIN32
#include <windows.h>
//#include <ws2spi.h>
//#define int SOCKET
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#ifdef _DMALLOC_
#include <dmalloc.h>
#endif

#define SOCKET int
#define SOCKET_ERROR -1
#define closesocket close
#endif

#ifndef SOCKET
#define SOCKET int
#endif

class CMySocket  
{
public:
	CMySocket();
	virtual ~CMySocket();

	void initWinsockLib(void);
	void socketErrorExit(char *szError);
	SOCKET DoSocketListen(unsigned short portnum);
	SOCKET DoSocketAccept(SOCKET s);
	SOCKET DoSocketConnect(char *hostname, unsigned short portnum);
	void CheckSocketError(int iError, char *szMessage);
	void CleanAndExit(int iError);

	// struct sockaddr_in		connectedIP;

};
