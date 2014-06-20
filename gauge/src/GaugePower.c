/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  T H E R M O . C                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 *  This is free software; you can redistribute it and/or modify it under the terms of the GNU General Public         *
 *  License version 2 as published by the Free Software Foundation.  Note that I am not granting permission to        *
 *  redistribute or modify this under the terms of any later version of the General Public License.                   *
 *                                                                                                                    *
 *  This is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the                *
 *  impliedwarranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   *
 *  more details.                                                                                                     *
 *                                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program (in the file            *
 *  "COPYING"); if not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111,   *
 *  USA.                                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @file
 *  @brief Routines to display the powermeter information.
 *  @version $Id: GaugeWeather.c 1531 2012-07-31 12:08:36Z ukchkn $
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>

#include "config.h"
#include "socket.h"
#include "GaugeDisp.h"

#ifdef GAUGE_HAS_POWER

extern FACE_SETTINGS *faceSettings[];
extern MENU_DESC gaugeMenuDesc[];
extern char powerServer[];
extern int powerPort;

double myPowerReading[5] = { 0, 0, 0, 0, 0 };

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  T H E R M O M E T E R  I N I T                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief Called once at the program start to find the devices.
 *  @result None 0 all is OK.
 */
void readPowerMeterInit (void)
{
	char addr[20];
	int clientSock = -1;
	
	if (GetAddressFromName (powerServer, addr))
	{
		clientSock = ConnectClientSocket (addr, powerPort);
	}
	if (SocketValid (clientSock))
	{
		gaugeMenuDesc[MENU_GAUGE_POWER].disable = 0;
		CloseSocket (&clientSock);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  T H E R M O M E T E R  I N F O                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief Read the current tempature from the powermeter.
 *  @result None.
 */
void readPowerMeterInfo ()
{
	char buffer[256] = "", addr[20];
	int clientSock = -1, bytesRead = 0;
	
	if (GetAddressFromName ("tinyone", addr))
	{
		clientSock = ConnectClientSocket (addr, 303030);
	}
	if (SocketValid (clientSock))
	{
		bytesRead = RecvSocket (clientSock, buffer, 255);
		CloseSocket (&clientSock);
	}
	if (bytesRead)
	{
		char string[41] = "";
		int i = 0, let = 0, field = 0;

		buffer[bytesRead] = 0;
		while (buffer[i])
		{
			if ((buffer[i] >= '0' && buffer[i] <= '9') || buffer[i] == '.' || buffer[i] == '-')
			{
				string[let] = buffer[i];
				string[++let] = 0;
			}
			else if (buffer[i] == ':')
			{
				field = atoi (string);
				string[let = 0] = 0;
			}
			else if (buffer[i] == ',')
			{
				if (field < 5)
				{
					myPowerReading[field] = atof (string);
				}
				string[let = 0] = 0;
			}
			++i;
		}		
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  T H E R M O M E T E R  V A L U E S                                                                       *
 *  ===========================================                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief Calculate the value to show on the face.
 *  @param face Which face to display on.
 *  @result None zero if the face should be re-shown.
 */
void readPowerMeterValues (int face)
{
	FACE_SETTINGS *faceSetting = faceSettings[face];

	if (faceSetting -> faceFlags & FACE_REDRAW)
	{
		;
	}
	else if (faceSetting -> nextUpdate)
	{
		faceSetting -> nextUpdate -= 1;
		return;
	}
	else if (!faceSetting -> nextUpdate)
	{
		readPowerMeterInfo ();
		faceSetting -> nextUpdate = 60;
	}

	setFaceString (faceSetting, FACESTR_TOP, 0, "Power Meter");
	setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Current</b>: %0.0fW\n<b>Max</b>: %0.0fW\n"
			"<b>Min</b>: %0.0fW\n<b>Hour</b>: %0.0fW\n<b>Day</b>: %0.0fW"), 
			myPowerReading[0], myPowerReading[1], myPowerReading[2], myPowerReading[3], myPowerReading[4]);
	setFaceString (faceSetting, FACESTR_WIN, 0, _("Current: %0.0fW, Hour: %0.0fW"),
			myPowerReading[0], myPowerReading[1]);
	setFaceString (faceSetting, FACESTR_BOT, 0, "%0.0fW", myPowerReading[0]);
	faceSetting -> firstValue = myPowerReading[0] / 1000;
	faceSetting -> secondValue = myPowerReading[1] / 1000;
}

#endif

