/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  S E N S O R S . C                                                                                      *
 *  ============================                                                                                      *
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
 *  @brief Calculate the sensor value for the gauge.
 *  @version $Id: GaugeSensors.c 1856 2014-01-14 15:10:08Z ukchkn $
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "GaugeDisp.h"

#ifdef GAUGE_HAS_SENSOR

#include <sensors/sensors.h>

extern FACE_SETTINGS *faceSettings[];
extern MENU_DESC gaugeMenuDesc[];
extern MENU_DESC sTempMenuDesc[];
extern MENU_DESC sFanMenuDesc[];
extern MENU_DESC sensorMenuDesc[];
extern int sysUpdateID;

#if SENSORS_API_VERSION >= 1024

int faceTypes[2] = 
{
	SENSORS_SUBFEATURE_TEMP_INPUT,
	SENSORS_SUBFEATURE_FAN_INPUT
};

#endif

#if SENSORS_API_VERSION >= 1024

static int initSensorsOK = 0;

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F I N D  S E N S O R S                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief Look for sensors and enable the menus if found.
 *  @result None.
 */
void findSensors ()
{
	int nr = 0, tempCount = 0, fanCount = 0;
	const sensors_chip_name *chipset;
	const sensors_feature *feature;
	const sensors_subfeature *subfeature;

	chipset = sensors_get_detected_chips (NULL, &nr);
	while (chipset)
	{
		int nr1 = 0;

		feature = sensors_get_features (chipset, &nr1);
		while (feature)
		{
			int nr2 = 0;

			subfeature = sensors_get_all_subfeatures (chipset, feature, &nr2);
			while (subfeature)
			{
				if (subfeature -> type == SENSORS_SUBFEATURE_TEMP_INPUT && tempCount < 8)
				{
					sTempMenuDesc[tempCount].disable = 0;
					++tempCount;
				}
				else if (subfeature -> type == SENSORS_SUBFEATURE_FAN_INPUT && fanCount < 8)
				{
					sFanMenuDesc[fanCount].disable = 0;
					++fanCount;
				}
				subfeature = sensors_get_all_subfeatures (chipset, feature, &nr2);
			}
			feature = sensors_get_features (chipset, &nr1);
		}
		chipset = sensors_get_detected_chips (NULL, &nr);
	}
	if (tempCount)
		sensorMenuDesc[MENU_SENSOR_TEMP].disable = 0;
	if (fanCount)
		sensorMenuDesc[MENU_SENSOR_FAN].disable = 0;
	if (tempCount || fanCount)
		gaugeMenuDesc[MENU_GAUGE_SENSOR].disable = 0;
}

#endif

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  S E N S O R  I N I T                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief Called at program start to init sensor reading.
 *  @result None.
 */
void readSensorInit (void)
{
#if SENSORS_API_VERSION >= 1024
	FILE *inputFile;

	if ((inputFile = fopen ("/etc/sensors3.conf", "r")) != NULL)
	{
		if (sensors_init(inputFile) == 0)
		{
			initSensorsOK = 1;
			findSensors();
		}
		fclose (inputFile);
	}
#endif
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  S E N S O R  V A L U E S                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief Read the sensor values for the the face.
 *  @param face Which face the reading is for.
 *  @result Non zero then display.
 */
void readSensorValues (int face)
{
	int nr = 0, count = 0;
	FACE_SETTINGS *faceSetting = faceSettings[face];
	int type = faceSetting -> showFaceType;
	int number = faceSetting -> faceSubType;

#if SENSORS_API_VERSION >= 1024

	const sensors_chip_name *chipset;
	const sensors_feature *feature;
	const sensors_subfeature *subfeature;

#endif

	if (faceSetting -> faceFlags & FACE_REDRAW)
	{
		;
	}
	else if (sysUpdateID % 10 != 0)
	{
		return;
	}

#if SENSORS_API_VERSION >= 1024

	if (initSensorsOK)
	{
		chipset = sensors_get_detected_chips (NULL, &nr);
		while (chipset)
		{
			int nr1 = 0;

			feature = sensors_get_features (chipset, &nr1);
			while (feature)
			{
				int nr2 = 0;

				subfeature = sensors_get_all_subfeatures (chipset, feature, &nr2);
				while (subfeature)
				{
					if (subfeature -> type == faceTypes[type - FACE_TYPE_SENSOR_TEMP])
					{
						if (count++ == number)
						{
							double value;
	
							if (sensors_get_value (chipset, nr2 - 1, &value) == 0)
							{
								switch (type)
								{
								case FACE_TYPE_SENSOR_TEMP:
									setFaceString (faceSetting, FACESTR_TOP, 0, _("Temp %d"), number + 1);
									setFaceString (faceSetting, FACESTR_WIN, 0, _("Sensor Temp %d - Gauge"), number + 1);
									setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Sensor Temp %d</b>: %0.0f\302\260C\n"
												"<b>Chipset Name</b>: %s"),	number + 1, value, chipset -> prefix);
									setFaceString (faceSetting, FACESTR_BOT, 0, _("%0.0f\302\260C"), value);
									faceSetting -> firstValue = value;
									while (faceSetting -> firstValue > faceSetting -> faceScaleMax)
									{
										faceSetting -> faceScaleMax += 25;
										maxMinReset (&faceSetting -> savedMaxMin, 10, 2);
									}									
									break;

								case FACE_TYPE_SENSOR_FAN:
									setFaceString (faceSetting, FACESTR_TOP, 0, _("Fan %d"), number + 1);
									setFaceString (faceSetting, FACESTR_WIN, 0, _("Sensor Fan %d - Gauge"), number + 1);
									setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Sensor Fan %d</b>: %0.0f rpm\n"
												"<b>Chipset Name</b>: %s"),	number + 1, value, chipset -> prefix);
									setFaceString (faceSetting, FACESTR_BOT, 0, _("%0.0f\n(rpm)"), value);
									faceSetting -> firstValue = value / 100;
									while (faceSetting -> firstValue > faceSetting -> faceScaleMax)
									{
										faceSetting -> faceScaleMax += 25;
										maxMinReset (&faceSetting -> savedMaxMin, 10, 2);
									}									
									break;
								}
								return;
							}
						}
					}
					subfeature = sensors_get_all_subfeatures (chipset, feature, &nr2);
				}
				feature = sensors_get_features (chipset, &nr1);
			}
			chipset = sensors_get_detected_chips (NULL, &nr);
		}
	}
	
#endif
}

#endif
