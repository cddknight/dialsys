/**********************************************************************************************************************
 *                                                                                                                    *
 *  S O C K E T . C                                                                                                   *
 *  ===============                                                                                                   *
 *                                                                                                                    *
 *  This is free software; you can redistribute it and/or modify it under the terms of the GNU General Public         *
 *  License version 2 as published by the Free Software Foundation.  Note that I am not granting permission to        *
 *  redistribute or modify this under the terms of any later version of the General Public License.                   *
 *                                                                                                                    *
 *  This is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied        *
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more     *
 *  details.                                                                                                          *
 *                                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program (in the file            *
 *  "COPYING"); if not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111,   *
 *  USA.                                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \file
 *  \brief Functions for sock access.
 */
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include "socket.h"

const int MAXCONNECTIONS = 5;

/******************************************************************************
 *                                                                            *
 *  S E R V E R  S O C K E T  S E T U P                                       *
 *  ===================================                                       *
 *                                                                            *
 ******************************************************************************/
/**
 *  @brief .
 *  @param port .
 *  @result .
 */
int ServerSocketSetup (int port)
{
	struct sockaddr_in mAddress;
	int on = 1, mSocket = socket (AF_INET, SOCK_STREAM, 0);

	if (!SocketValid (mSocket))
		return -1;

	if (setsockopt (mSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof (on)) == -1)
	{
		close (mSocket);
		return -1;
	}

	memset (&mAddress, 0, sizeof (mAddress));
	mAddress.sin_family = AF_INET;
	mAddress.sin_addr.s_addr = INADDR_ANY;
	mAddress.sin_port = htons (port);

	if (bind (mSocket, (struct sockaddr *) &mAddress, sizeof (mAddress)) == -1)
	{
		close (mSocket);
		return -1;
	}

	if (listen (mSocket, MAXCONNECTIONS) == -1)
	{
		close (mSocket);
		return -1;
	}
	return mSocket;
}

/******************************************************************************
 *                                                                            *
 *  S E R V E R  S O C K E T  A C C E P T                                     *
 *  =====================================                                     *
 *                                                                            *
 ******************************************************************************/
/**
 *  @brief .
 *  @param socket .
 *  @param address .
 *  @result .
 */
int ServerSocketAccept (int socket, char *address)
{
	struct sockaddr_in mAddress;
	struct timeval timeout;
	fd_set fdset;
	int addr_length = sizeof (mAddress), clientSocket = -1;

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	FD_ZERO (&fdset);
	FD_SET (socket, &fdset);
	if (select (FD_SETSIZE, &fdset, NULL, NULL, &timeout) < 1)
	{
		return -1;
	}
	if (!FD_ISSET(socket, &fdset))
	{
		return -1;
	}

	clientSocket = accept (socket, (struct sockaddr *) &mAddress, (socklen_t *) &addr_length);
	if (clientSocket != -1 && address)
	{
		sprintf (address, "%d.%d.%d.%d", 
				mAddress.sin_addr.s_addr & 0xFF,
				mAddress.sin_addr.s_addr >> 8  & 0xFF,
				mAddress.sin_addr.s_addr >> 16 & 0xFF,
				mAddress.sin_addr.s_addr >> 24 & 0xFF);
	}
	return clientSocket;
}

/******************************************************************************
 *                                                                            *
 *  C O N N E C T  C L I E N T  S O C K E T                                   *
 *  =======================================                                   *
 *                                                                            *
 ******************************************************************************/
/**
 *  @brief .
 *  @param host .
 *  @param port .
 *  @result .
 */
int ConnectClientSocket (char *host, int port)
{
	struct sockaddr_in mAddress;
	int on = 1, mSocket = socket (AF_INET, SOCK_STREAM, 0);

	if (!SocketValid (mSocket))
	{
		return -1;
	}

	if (setsockopt (mSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof (on)) == -1)
	{
		close (mSocket);
		return -1;
	}

	memset (&mAddress, 0, sizeof (mAddress));
	mAddress.sin_family = AF_INET;
	mAddress.sin_port = htons (port);

	if (inet_pton (AF_INET, host, &mAddress.sin_addr) == EAFNOSUPPORT) 
	{
		close (mSocket);
		return -1;
	}
	if (connect (mSocket, (struct sockaddr *) &mAddress, sizeof (mAddress)) != 0)
	{
		close (mSocket);
		return -1;
	}
	return mSocket;
}

/******************************************************************************
 *                                                                            *
 *  S E N D  S O C K E T                                                      *
 *  ====================                                                      *
 *                                                                            *
 ******************************************************************************/
/**
 *  @brief .
 *  @param socket .
 *  @param buffer .
 *  @param size .
 *  @result .
 */
int SendSocket (int socket, char *buffer, int size)
{
	return send (socket, buffer, size, MSG_NOSIGNAL);
}

/******************************************************************************
 *                                                                            *
 *  R E C V  S O C K E T                                                      *
 *  ====================                                                      *
 *                                                                            *
 ******************************************************************************/
/**
 *  @brief .
 *  @param socket .
 *  @param buffer .
 *  @param size .
 *  @result .
 */
int RecvSocket (int socket, char *buffer, int size)
{
	int status = 0, readSoFar = 0;
	struct timeval timeout;
	fd_set fdset;

	timeout.tv_sec = 2;
	timeout.tv_usec = 0;

	while (readSoFar < size)
	{
		FD_ZERO (&fdset);
		FD_SET (socket, &fdset);
		if (select (FD_SETSIZE, &fdset, NULL, NULL, &timeout) < 1)
		{
			return 0;
		}
		if (!FD_ISSET(socket, &fdset))
		{
			return 0;
		}
		status = recv (socket, &buffer[readSoFar], size - readSoFar, MSG_WAITALL);
		if (status < 1)
		{
			return readSoFar;
		}
		readSoFar += status;
	}
	return readSoFar;
}

/******************************************************************************
 *                                                                            *
 *  C L O S E  S O C K E T                                                    *
 *  ======================                                                    *
 *                                                                            *
 ******************************************************************************/
/**
 *  @brief .
 *  @param socket .
 *  @result .
 */
int CloseSocket (int *socket)
{
	if (SocketValid (*socket))
	{
		close (*socket);
		*socket = -1;
	}
	return -1;
}

/******************************************************************************
 *                                                                            *
 *  S O C K E T  V A L I D                                                    *
 *  ======================                                                    *
 *                                                                            *
 ******************************************************************************/
/**
 *  @brief .
 *  @param socket .
 *  @result .
 */
int SocketValid (int socket)
{
	return (socket != -1);
}

int GetAddressFromName (char *name, char *address)
{
	struct hostent *hostEntry = gethostbyname(name);

	if (hostEntry)
	{
		if (hostEntry -> h_addr_list[0])
		{
			sprintf (address, "%d.%d.%d.%d", 
					(int)hostEntry -> h_addr_list[0][0] & 0xFF, 
					(int)hostEntry -> h_addr_list[0][1] & 0xFF,
					(int)hostEntry -> h_addr_list[0][2] & 0xFF, 
					(int)hostEntry -> h_addr_list[0][3] & 0xFF);
			return 1;
		}
	}
	return 0;
}
