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
 *  @brief Routines to display the thermometer information.
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

#ifdef GAUGE_HAS_THERMO

extern FACE_SETTINGS *faceSettings[];
extern MENU_DESC gaugeMenuDesc[];
extern char thermoServer[];
extern int thermoPort;

double myThermoReading[5] = { 0, 0, 0, 0, 0 };

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
void readThermometerInit (void)
{
	char addr[20];
	int clientSock = -1;
	
	if (GetAddressFromName (thermoServer, addr))
	{
		clientSock = ConnectClientSocket (addr, thermoPort);
	}
	if (SocketValid (clientSock))
	{
		gaugeMenuDesc[MENU_GAUGE_THERMO].disable = 0;
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
 *  @brief Read the current tempature from the thermometer.
 *  @result None.
 */
void readThermometerInfo ()
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
					myThermoReading[field] = atof (string);
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
void readThermometerValues (int face)
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
		readThermometerInfo ();
		faceSetting -> nextUpdate = 60;
	}

	setFaceString (faceSetting, FACESTR_TOP, 0, "Thermometer");
	setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Outside</b>: %0.1f\302\260C\n<b>Inside</b>: %0.1f\302\260C\n"
			"<b>Pressure</b>: %0.0fmb\n<b>Brightness</b>: %0.0flux\n<b>Humidity</b>: %0.0f%%"), 
			myThermoReading[0], myThermoReading[1], myThermoReading[2], myThermoReading[3], myThermoReading[4]);
	setFaceString (faceSetting, FACESTR_WIN, 0, _("Outside: %0.1f%s, Inside: %0.1f%s"),
			myThermoReading[0], "\302\260C", myThermoReading[1], "\302\260C");
	setFaceString (faceSetting, FACESTR_BOT, 0, "%0.1f%s", myThermoReading[0], "\302\260C");
	faceSetting -> firstValue = myThermoReading[0];
	faceSetting -> secondValue = myThermoReading[1];
}

#endif

