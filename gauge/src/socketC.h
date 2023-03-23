/**********************************************************************************************************************
 *                                                                                                                    *
 *  S O C K E T  C . H                                                                                                *
 *  ==================                                                                                                *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File socketC.h part of Gauge is free software: you can redistribute it and/or modify it under the terms of the    *
 *  GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at  *
 *  your option) any later version.                                                                                   *
 *                                                                                                                    *
 *  Gauge is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied       *
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more     *
 *  details.                                                                                                          *
 *                                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program. If not, see:           *
 *  <http://www.gnu.org/licenses/>                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \file
 *  \brief Socket functions.
 */
#ifndef MY_SOCKET_H
#define MY_SOCKET_H

#define USE_IPV4	1
#define USE_IPV6	2
#define USE_ANY		3

int ServerSocketSetup (int port);
int ServerSocketFile (char *fileName);
int ServerSocketAccept (int socket, char *address);
int ConnectSocketFile (char *fileName);
int ConnectClientSocket (char *host, int port, int timeout, int useIPVer, char *address);
int SendSocket (int socket, char *buffer, int size);
int WaitRecvSocket (int socket, char *buffer, int size, int secs);
int RecvSocket (int socket, char *buffer, int size);
int WaitSocket (int socket, int secs);
int CloseSocket (int *socket);
int SocketValid (int socket);
void setNonBlocking(int socket, int set);
int GetAddressFromName (char *name, char *address, int useIPVer);

#endif

