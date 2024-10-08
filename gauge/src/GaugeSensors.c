/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  S E N S O R S . C                                                                                      *
 *  ============================                                                                                      *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File GaugeSensors.c part of Gauge is free software: you can redistribute it and/or modify it under the terms of   *
 *  the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or  *
 *  (at your option) any later version.                                                                               *
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
 *  \brief Calculate the sensor value for the gauge.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "GaugeDisp.h"
#include "config.h"
#ifdef HAVE_SENSORS_SENSORS_H
#include <sensors/sensors.h>
#else
#define SENSORS_API_VERSION 0
#endif

extern FACE_SETTINGS *faceSettings[];
extern GAUGE_ENABLED gaugeEnabled[];
extern MENU_DESC gaugeMenuDesc[];
extern MENU_DESC sTempMenuDesc[];
extern MENU_DESC sFanMenuDesc[];
extern MENU_DESC sInputMenuDesc[];
extern MENU_DESC sensorMenuDesc[];
extern int sysUpdateID;

static char *sysThermalFile = "/sys/class/thermal/thermal_zone0/temp";
static int initSensorsOK = 0;

#if SENSORS_API_VERSION >= 1024

int faceTypes[3] =
{
	SENSORS_SUBFEATURE_TEMP_INPUT,
	SENSORS_SUBFEATURE_FAN_INPUT,
	SENSORS_SUBFEATURE_IN_INPUT
};

/**********************************************************************************************************************
 *                                                                                                                    *
 *  A P P E N D  E X T R A                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Append extra info in words upto max size.
 *  \param inStr Extra info to add.
 *  \param outStr Where to add it to.
 *  \param maxLen Max final string length.
 *  \result Pointer to out string.
 */
char *appendExtra (char *inStr, char *outStr, int maxLen)
{
	char word[81];
	int i = 0, j = 0, w = 0;
	
	outStr[0] = 0;
	while (inStr[i] != 0 && i < maxLen)
	{	
		if (inStr[i] <= ' ')
		{
			if (j > 0 && (strlen (word) + strlen (outStr) + 1) < maxLen)
			{
				strcat (outStr, w == 0 ? "\n" : " ");
				strcat (outStr, word);
				j = 0;
				++w;
			}
		}
		else
		{
			word[j] = inStr[i]; 
			word[++j] = 0;
			if (j >= 80) j = 79;
		}
		++i;
	}
	if (inStr[i] == 0 && j > 0 && (strlen (word) + strlen (outStr) + 1) < maxLen)
	{
		strcat (outStr, w == 0 ? "\n" : " ");
		strcat (outStr, word);
		j = 0;
	}
	return outStr;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F I N D  S E N S O R S                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Look for sensors and enable the menus if found.
 *  \result None.
 */
int findSensors ()
{
	int nr = 0, tempCount = 0, fanCount = 0, inputCount = 0;
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
				if (subfeature -> type == SENSORS_SUBFEATURE_TEMP_INPUT && tempCount < 15)
				{
					if (gaugeEnabled[FACE_TYPE_SENSOR_TEMP].enabled)
					{
						sTempMenuDesc[tempCount].disable = 0;
						++tempCount;
					}
				}
				else if (subfeature -> type == SENSORS_SUBFEATURE_FAN_INPUT && fanCount < 8)
				{
					if (gaugeEnabled[FACE_TYPE_SENSOR_FAN].enabled)
					{
						sFanMenuDesc[fanCount].disable = 0;
						++fanCount;
					}
				}
				else if (subfeature -> type == SENSORS_SUBFEATURE_IN_INPUT && inputCount < 15)
				{
					if (gaugeEnabled[FACE_TYPE_SENSOR_INPUT].enabled)
					{
						sInputMenuDesc[inputCount].disable = 0;
						++inputCount;
					}
				}
				/* else
				{
					printf ("Unknown feature -> type = %d, subfeature -> type = (%d:%d)\n", feature -> type, 
							subfeature -> type >> 8, subfeature -> type & 0xFF);
				} */
				subfeature = sensors_get_all_subfeatures (chipset, feature, &nr2);
			}
			feature = sensors_get_features (chipset, &nr1);
		}
		chipset = sensors_get_detected_chips (NULL, &nr);
	}
	if (tempCount)
	{
		sensorMenuDesc[MENU_SENSOR_TEMP].disable = 0;
	}
	if (fanCount)
	{
		sensorMenuDesc[MENU_SENSOR_FAN].disable = 0;
	}
	if (inputCount)
	{
		sensorMenuDesc[MENU_SENSOR_INPUT].disable = 0;
	}
	if (tempCount || fanCount || inputCount)
	{
		gaugeMenuDesc[MENU_GAUGE_SENSOR].disable = 0;
	}
	return (tempCount + fanCount + inputCount);
}
#endif

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  S E N S O R  I N I T                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called at program start to init sensor reading.
 *  \result None.
 */
void readSensorInit (void)
{
	if (gaugeEnabled[FACE_TYPE_SENSOR_TEMP].enabled || 
			gaugeEnabled[FACE_TYPE_SENSOR_FAN].enabled ||
			gaugeEnabled[FACE_TYPE_SENSOR_INPUT].enabled)
	{
		FILE *inputFile = NULL;

#if SENSORS_API_VERSION >= 1024
		if ((inputFile = fopen ("/etc/sensors3.conf", "r")) != NULL)
		{
			if (sensors_init(inputFile) == 0)
			{
				if (findSensors() > 0)
				{
					initSensorsOK |= 1;
				}
			}
			fclose (inputFile);
		}
#endif
		if ((inputFile = fopen (sysThermalFile, "r")) != NULL)
		{
			sTempMenuDesc[15].disable = 0;
			sensorMenuDesc[MENU_SENSOR_TEMP].disable = 0;
			gaugeMenuDesc[MENU_GAUGE_SENSOR].disable = 0;
			initSensorsOK |= 2;
			fclose (inputFile);
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  S E N S O R  V A L U E S                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read the sensor values for the the face.
 *  \param face Which face the reading is for.
 *  \result Non zero then display.
 */
void readSensorValues (int face)
{
	if (gaugeEnabled[FACE_TYPE_SENSOR_TEMP].enabled || 
			gaugeEnabled[FACE_TYPE_SENSOR_FAN].enabled ||
			gaugeEnabled[FACE_TYPE_SENSOR_INPUT].enabled)
	{
		FACE_SETTINGS *faceSetting = faceSettings[face];

#if SENSORS_API_VERSION >= 1024
		int nr = 0, count = 0;
		int type = faceSetting -> showFaceType;
		int number = faceSetting -> faceSubType;
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
		if (initSensorsOK & 1)
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
									char *label, sensorName[41];

									switch (type)
									{
									case FACE_TYPE_SENSOR_TEMP:
										if (gaugeEnabled[FACE_TYPE_SENSOR_TEMP].enabled)
										{
											sprintf (sensorName, _("Temp %d"), number + 1);
											if ((label = sensors_get_label (chipset, feature)) != NULL)
											{
												appendExtra (label, &sensorName[strlen (sensorName)], 12);
												free (label);
											}	
											setFaceString (faceSetting, FACESTR_TOP, 0, sensorName);
											setFaceString (faceSetting, FACESTR_WIN, 0, _("Sensor Temp %d - Gauge"), number + 1);
											setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Sensor Temp %d</b>: %0.0f\302\260C\n"
														"<b>Chipset Name</b>: %s"), number + 1, value, chipset -> prefix);
											setFaceString (faceSetting, FACESTR_BOT, 0, _("%0.0f\302\260C"), value);
											faceSetting -> firstValue = value;
											while (faceSetting -> firstValue > faceSetting -> faceScaleMax)
											{
												faceSetting -> faceScaleMax += 25;
												maxMinReset (&faceSetting -> savedMaxMin, 10, 2);
											}
										}
										break;

									case FACE_TYPE_SENSOR_FAN:
										if (gaugeEnabled[FACE_TYPE_SENSOR_FAN].enabled)
										{
											sprintf (sensorName, _("Fan %d"), number + 1);
											if ((label = sensors_get_label (chipset, feature)) != NULL)
											{
												appendExtra (label, &sensorName[strlen (sensorName)], 12);
												free (label);
											}
											setFaceString (faceSetting, FACESTR_TOP, 0, _("Fan %d"), number + 1);
											setFaceString (faceSetting, FACESTR_WIN, 0, _("Sensor Fan %d - Gauge"), number + 1);
											setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Sensor Fan %d</b>: %0.0f rpm\n"
														"<b>Chipset Name</b>: %s"), number + 1, value, chipset -> prefix);
											setFaceString (faceSetting, FACESTR_BOT, 0, _("%0.0f\n(rpm)"), value);
											faceSetting -> firstValue = value / 100;
											while (faceSetting -> firstValue > faceSetting -> faceScaleMax)
											{
												faceSetting -> faceScaleMax += 25;
												maxMinReset (&faceSetting -> savedMaxMin, 10, 2);
											}
										}
										break;

									case FACE_TYPE_SENSOR_INPUT:
										if (gaugeEnabled[FACE_TYPE_SENSOR_INPUT].enabled)
										{
											sprintf (sensorName, _("Input %d"), number + 1);
											if ((label = sensors_get_label (chipset, feature)) != NULL)
											{
												appendExtra (label, &sensorName[strlen (sensorName)], 12);
												free (label);
											}
											setFaceString (faceSetting, FACESTR_TOP, 0, _("Input %d"), number + 1);
											setFaceString (faceSetting, FACESTR_WIN, 0, _("Sensor Input %d - Gauge"), number + 1);
											if (value < 1)
											{
												setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Sensor Input %d</b>: %0.0f mV\n"
														"<b>Chipset Name</b>: %s"), number + 1, value * 1000, chipset -> prefix);
												setFaceString (faceSetting, FACESTR_BOT, 0, _("%0.0f mV"), value * 1000);
											}
											else
											{
												setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Sensor Input %d</b>: %0.2f V\n"
														"<b>Chipset Name</b>: %s"), number + 1, value, chipset -> prefix);
												setFaceString (faceSetting, FACESTR_BOT, 0, _("%0.2f V"), value);
											}
											faceSetting -> firstValue = value;
											while (faceSetting -> firstValue > faceSetting -> faceScaleMax)
											{
												if (faceSetting -> faceScaleMax == 1)
												{
													faceSetting -> faceScaleMax = 6;
												}
												else
												{
													faceSetting -> faceScaleMax += 6;
												}
												maxMinReset (&faceSetting -> savedMaxMin, 10, 2);
											}
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
		if (initSensorsOK & 2)
		{
			if (faceSetting -> faceSubType == 15)
			{
				FILE *thermFile = fopen (sysThermalFile, "r");
				if (thermFile != NULL)
				{
					int readTemp;
					if (fscanf(thermFile, "%d", &readTemp) == 1)
					{
						faceSetting -> firstValue = (float)readTemp / 1000;
						setFaceString (faceSetting, FACESTR_TOP, 0, _("System\nTemp"));
						setFaceString (faceSetting, FACESTR_WIN, 0, _("System Temp %0.1f - Gauge"), 
								faceSetting -> firstValue);
						setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>System Temp</b>: %0.0f\302\260C"),
								faceSetting -> firstValue);
						setFaceString (faceSetting, FACESTR_BOT, 0, _("%0.0f\302\260C"), faceSetting -> firstValue);
						while (faceSetting -> firstValue > faceSetting -> faceScaleMax)
						{
							faceSetting -> faceScaleMax += 25;
							maxMinReset (&faceSetting -> savedMaxMin, 10, 2);
						}
					}
					fclose (thermFile);
				}
			}
		}
	}
}

