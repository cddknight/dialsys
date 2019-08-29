/**********************************************************************************************************************
 *                                                                                                                    *
 *  S O C K E T  C . C                                                                                                *
 *  ==================                                                                                                *
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
 *  \brief Socket connections.
 */
#include <netinet/in.h>
#include <netdb.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include "socketC.h"

const int MAXCONNECTIONS = 5;

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S E R V E R  S O C K E T  S E T U P                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Setup a server socket listenning on a port.
 *  \param port Port to listen on.
 *  \result The socket handle of the server, or -1 if server failed.
 */
int ServerSocketSetup (int port)
{
	struct sockaddr_in6 mAddress;
	int on = 1, mSocket = socket (AF_INET6, SOCK_STREAM, 0);

	if (!SocketValid (mSocket))
	{
		return -1;
	}
	if (setsockopt (mSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof (on)) == -1)
	{
		close (mSocket);
		return -1;
	}
	memset (&mAddress, 0, sizeof(mAddress));
	mAddress.sin6_family = AF_INET6;
	mAddress.sin6_addr	 = in6addr_any;
	mAddress.sin6_port	 = htons(port);

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

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S E R V E R  S O C K E T  F I L E                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Setup a server listenning on a unix socket.
 *  \param fileName Name for the unix socket.
 *  \result The socket handle of the server, or -1 if server failed.
 */
int ServerSocketFile (char *fileName)
{
	int len, mSocket = socket (AF_UNIX, SOCK_STREAM, 0);
	struct sockaddr_un mAddress;

	if (!SocketValid (mSocket))
	{
		return -1;
	}
	mAddress.sun_family = AF_UNIX;
	strcpy (mAddress.sun_path, fileName);
	unlink (mAddress.sun_path);
	len = strlen(mAddress.sun_path) + sizeof(mAddress.sun_family);

	if (bind (mSocket, (struct sockaddr *)&mAddress, len) == -1)
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

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S E R V E R  S O C K E T  A C C E P T                                                                             *
 *  =====================================                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Accept a new connection on a listening port.
 *  \param socket Listening socket.
 *  \param address Save remote address here.
 *  \result Handle of new socket.
 */
int ServerSocketAccept (int socket, char *address)
{
	struct timeval timeout;
	fd_set fdset;
	int clientSocket = -1;

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

	clientSocket = accept (socket, NULL, NULL);
	if (clientSocket != -1)
	{
		struct sockaddr_in6 clientaddr;
		socklen_t addrlen=sizeof(clientaddr);
		char str[INET6_ADDRSTRLEN];

		getpeername (clientSocket, (struct sockaddr *)&clientaddr, &addrlen);
		if (inet_ntop (AF_INET6, &clientaddr.sin6_addr, str, sizeof(str)))
		{
			strcpy (address, str);
		}
	}
	return clientSocket;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O N N E C T  S O C K E T  F I L E                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Connect to a unix socket (file).
 *  \param fileName Socket file to connecto to.
 *  \result Handle of socket or -1 if failed.
 */
int ConnectSocketFile (char *fileName)
{
	struct sockaddr_un mAddress;
	int len, mSocket = socket(AF_UNIX, SOCK_STREAM, 0);

	if (!SocketValid (mSocket))
	{
		return -1;
	}
	mAddress.sun_family = AF_UNIX;
	strcpy(mAddress.sun_path, fileName);
	len = strlen(mAddress.sun_path) + sizeof(mAddress.sun_family);

	if (connect (mSocket, (struct sockaddr *)&mAddress, len) == -1)
	{
		close (mSocket);
		return -1;
	}
	return mSocket;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  T I M E D  C O N N E C T                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Make a timed connection attempt.
 *  \param socket Handle for the socket.
 *  \param secs Time to wait for.
 *  \param addr Address to connecto to.
 *  \param addrSize Size of the address.
 *  \result 0 if connected OK.
 */
int TimedConnect (int socket, int secs, struct sockaddr *addr, int addrSize)
{
	int conRetn;
	fd_set connfd;
	struct timeval timeout;
	char dummyBuff[10];

	setNonBlocking (socket, 1);
	conRetn = connect (socket, addr, addrSize);

	if (conRetn != 0 && errno == EINPROGRESS)
	{
		int selRetn;
		timeout.tv_sec = secs;
		timeout.tv_usec = 0;

		FD_ZERO(&connfd);
		FD_SET (socket, &connfd);

		selRetn = select (FD_SETSIZE, NULL, &connfd, NULL, &timeout);
		if (selRetn > 0)
		{
			int recRetn;
			recRetn = recv (socket, dummyBuff, 0, MSG_DONTWAIT);
			if (recRetn == 0 || (recRetn == -1 && errno == EAGAIN))
			{
				setNonBlocking (socket, 0);
				return 0;
			}
		}
		conRetn = -1;
	}
	return conRetn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O N N E C T  C L I E N T  S O C K E T                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Connect to a remote socket.
 *  \param host Host address to connecto to.
 *  \param port Host port to connect to.
 *  \param timeout Seconds to wait for the connect.
 *  \param retnAddr Optional (can be NULL) pointer to return used address.
 *  \result Handle of socket or -1 if failed.
 */
int ConnectClientSocket (char *host, int port, int timeout, int useIPVer, char *retnAddr)
{
	struct addrinfo *result;
	struct addrinfo *res;
	struct addrinfo addrInfoHint;
	int error, connected = 0;
	int mSocket = -1;

	/* only get all stream addresses */
	memset (&addrInfoHint, 0, sizeof (addrInfoHint));
	addrInfoHint.ai_flags = AI_ALL | AI_CANONNAME | AI_ADDRCONFIG;
	addrInfoHint.ai_socktype = SOCK_STREAM;

	/* resolve the domain name into a list of addresses */
	if ((error = getaddrinfo (host, NULL, &addrInfoHint, &result)) == 0)
	{
		/* loop over all returned results and do inverse lookup */
		for (res = result; res != NULL && !connected; res = res->ai_next)
		{
			switch (res->ai_family)
			{
			case AF_INET:
				if (useIPVer & USE_IPV4)
				{
					if ((mSocket = socket (AF_INET, SOCK_STREAM, 0)) != -1)
					{
						struct sockaddr_in *address4 = (struct sockaddr_in *)res -> ai_addr;
						if (retnAddr != NULL)
						{
							inet_ntop (AF_INET, &(address4->sin_addr), retnAddr, INET_ADDRSTRLEN);
						}
						address4 -> sin_port = htons (port);

						if (TimedConnect (mSocket, timeout, (struct sockaddr *)address4, sizeof (struct sockaddr_in)) == 0)
						{
							connected = 1;
						}
					}
				}
				break;

			case AF_INET6:
				if (useIPVer & USE_IPV6)
				{
					if ((mSocket = socket (AF_INET6, SOCK_STREAM, 0)) != -1)
					{
						struct sockaddr_in6 *address6 = (struct sockaddr_in6 *)res -> ai_addr;
						if (retnAddr != NULL)
						{
							inet_ntop(AF_INET6, &(address6->sin6_addr), retnAddr, INET6_ADDRSTRLEN);
						}
						address6 -> sin6_port = htons (port);

						if (TimedConnect (mSocket, timeout, (struct sockaddr *)address6, sizeof (struct sockaddr_in6)) == 0)
						{
							connected = 1;
						}
					}
				}
				break;
			}
			if (!connected && mSocket != -1)
			{
				close (mSocket);
				mSocket = -1;
			}
		}
		freeaddrinfo (result);
	}
	return mSocket;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S E T  N O N  B L O C K I N G                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Set the non-blocking flag on/off on a socket.
 *  \param socket Socket to change.
 *  \param set Set or clear non-blocking flag.
 *  \result None.
 */
void setNonBlocking (int socket, int set)
{
	if (socket != -1)
	{
		int opts = fcntl (socket, F_GETFL);

		if (opts >= 0)
		{
			if (set)
			{
				opts = (opts | O_NONBLOCK);
			}
			else
			{
				opts = (opts & ~O_NONBLOCK);
			}
			fcntl(socket, F_SETFL, opts);
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S E N D  S O C K E T                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Send data to a socket.
 *  \param socket Which socket to send to.
 *  \param buffer Buffer to send.
 *  \param size Size of data to send.
 *  \result Bytes sent.
 */
int SendSocket (int socket, char *buffer, int size)
{
	return send (socket, buffer, size, MSG_NOSIGNAL);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  W A I T  R E C V  S O C K E T                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Wait before reading data from the socket.
 *  \param socket Socket to wait and receive on.
 *  \param buffer Buffer to read to.
 *  \param size Size to read.
 *  \param secs Seconds to wait.
 *  \result Size of data read.
 */
int WaitRecvSocket (int socket, char *buffer, int size, int secs)
{
	int retn = WaitSocket (socket, secs);

	if (retn == 1)
	{
		retn = RecvSocket (socket, buffer, size);
	}
	return retn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  W A I T  S O C K E T                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Wait on recv data to be available.
 *  \param socket Socket to wait on.
 *  \param secs Seconds to wait.
 *  \result Return from select.
 */
int WaitSocket (int socket, int secs)
{
	fd_set readfds;
	struct timeval timeout;

	timeout.tv_sec = secs;
	timeout.tv_usec = 0;

	FD_ZERO(&readfds);
	FD_SET (socket, &readfds);

	return select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E C V  S O C K E T                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Receive data from the socket.
 *  \param socket Which socket to receive from.
 *  \param buffer Buffer to save to.
 *  \param size Max size of the receive buffer.
 *  \result Bytes received.
 */
int RecvSocket (int socket, char *buffer, int size)
{
	int retn = recv (socket, buffer, size, MSG_DONTWAIT);
	return (retn > 0 ? retn : 0);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C L O S E  S O C K E T                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Clock a socket.
 *  \param socket Socket to close.
 *  \result None.
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

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S O C K E T  V A L I D                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Check socket handle is not -1.
 *  \param socket Socket handle to check.
 *  \result True if handle is not -1.
 */
int SocketValid (int socket)
{
	return (socket < 0 ? 0 : 1);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  A D D R E S S  F R O M  N A M E                                                                            *
 *  ======================================                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Convert and addess to an IP address with a lookup.
 *  \param name Name to look up.
 *  \param address Out out the address here.
 *  \result 1 if address resolved.
 */
int GetAddressFromName (char *name, char *address, int useIPVer)
{
	int retn = 0;
	struct addrinfo *result;
	struct addrinfo *res;
	struct addrinfo addrInfoHint;

	/* only get all stream addresses */
	memset (&addrInfoHint, 0, sizeof (addrInfoHint));
	addrInfoHint.ai_flags = AI_ALL | AI_CANONNAME | AI_ADDRCONFIG;
	addrInfoHint.ai_socktype = SOCK_STREAM;

	/* resolve the domain name into a list of addresses */
	if (getaddrinfo(name, NULL, &addrInfoHint, &result) == 0)
	{
		/* loop over all returned results and do inverse lookup */
		for (res = result; res != NULL && retn == 0; res = res->ai_next)
		{
			switch (res->ai_family)
			{
			case AF_INET:
				if (useIPVer & USE_IPV4)
				{
					struct sockaddr_in *address4 = (struct sockaddr_in *)res -> ai_addr;
					if (inet_ntop (AF_INET, &(address4->sin_addr), address, INET_ADDRSTRLEN) != NULL)
					{
						retn = 1;
					}
				}
				break;

			case AF_INET6:
				if (useIPVer & USE_IPV6)
				{
					struct sockaddr_in6 *address6 = (struct sockaddr_in6 *)res -> ai_addr;
					if (inet_ntop(AF_INET6, &(address6->sin6_addr), address, INET6_ADDRSTRLEN) != NULL)
					{
						retn = 1;
					}
				}
				break;
			}
		}
		freeaddrinfo (result);
	}
	return retn;
}

