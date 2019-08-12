/**********************************************************************************************************************
 *                                                                                                                    *
 *  S O C K E T  C . H                                                                                                *
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
 *  \brief .
 */
#ifndef MY_SOCKET_H
#define MY_SOCKET_H

int ServerSocketSetup (int port);
int ServerSocketFile (char *fileName);
int ServerSocketAccept (int socket, char *address);
int ConnectSocketFile (char *fileName);
int ConnectClientSocket (char *host, int port, int timeout, char *address);
int SendSocket (int socket, char *buffer, int size);
int RecvSocket (int socket, char *buffer, int size);
int WaitSocket (int socket, int secs);
int CloseSocket (int *socket);
int SocketValid (int socket);
void setNonBlocking(int socket, int set);
int GetAddressFromName (char *name, char *address);

#endif

