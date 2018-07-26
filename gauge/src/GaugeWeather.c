/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  W E A T H E R . C                                                                                      *
 *  ============================                                                                                      *
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
 *  \brief Routines to display the weather infomation.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <zlib.h>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "config.h"
#include "GaugeDisp.h"

extern FACE_SETTINGS *faceSettings[];
extern GAUGE_ENABLED gaugeEnabled[];
extern MENU_DESC gaugeMenuDesc[];
extern DIAL_CONFIG dialConfig;

enum 
{
	COL_NAME = 0,
	NUM_COLS
};

enum 
{
	CHNG_TEMP = 0,
	CHNG_HUMI,
	CHNG_PRES,
	CHNG_WIND
};

void saveCurrentWeather(void);

extern int weatherScales;

char *changeText[] = 
{
	__("Falling"),
	__("No change"),
	__("Rising")
};

struct 
{
	char tempUnit;
	int tempMin, tempMax;
	char *tempText;
}
tempUnits[] =
{
	{0, -10, 40, "\302\260C"},
	{1, 10, 110, "\302\260F"}
};

struct 
{
	char speedUnit;
	int speedMin, speedMax;
	char *speedText;
}
speedUnits[] =
{
	{0, 0, 100, "kph"},
	{1, 0, 25, "mps"},
	{2, 0, 50, "mph"},
	{3, 0, 50, "knots"}
};

struct 
{
	char pressureUnit;
	int pressureMin, pressureMax;
	char *pressureText;
}
pressureUnits[] =
{
	{0, 95, 105, "mb"},
	{1, 700, 800, "mmHg"},
	{2, 28, 31, "inHg"}
};

struct MemoryStruct 
{
	char *memory;
	size_t size;
};

char *weatherOBSURL = "https://weather-broker-cdn.api.bbci.co.uk/en/observation/rss/%s";
char *weatherTFCURL = "https://weather-broker-cdn.api.bbci.co.uk/en/forecast/rss/3day/%s";
//char *weatherOBSURL = "http://open.live.bbc.co.uk/weather/feeds/en/%s/observations.rss";
//char *weatherTFCURL = "http://open.live.bbc.co.uk/weather/feeds/en/%s/3dayforecast.rss";
int observations = 0;
extern char locationKey[];

char *daysOfWeek[7] = 
{
	__("Sunday"), __("Monday"), __("Tuesday"), __("Wednesday"), __("Thursday"), __("Friday"),
	__("Saturday")
};

#define FORECAST_NUM	3

typedef struct 
{
	int day;
	int evening;
	char date[21];
	int tempMaxC;
	int tempMinC;
	double windspeedKmph;
	double windspeedmph;
	char winddirPoint[21];
	char weatherDesc[81];
	int humidity;
	int visibility;
	char visView[21];
	char pollution[21];
	char sunrise[21];
	char sunset[21];
	int pressure;
	int uvRisk;

	double showTempMax;
	double showTempMin;
	double showWindSpeed;
	double showPressure;
} 
weatherForecast;

typedef struct 
{
	char updateTime[61];
	int tempC;
	char weatherDesc[81];
	char queryName[81];
	double windspeedmph;
	double windspeedKmph;
	int humidity;
	int visibility;
	char visView[21];
	char winddirPoint[21];
	int pressure;
	double apparent;
	double dewPoint;
	weatherForecast forecast[FORECAST_NUM];
	char *message;

	double showTemp;
	double showWindSpeed;
	double showPressure;
	double showApparent;
	double showDewPoint;

	char changed[4][2];
	time_t nextUpdate;
	short updateNum;
	int tUnits;
	int pUnits;
	int sUnits;
	int dUnits;
} 
weatherInfo;

weatherInfo myWeather;

/**********************************************************************************************************************
 *                                                                                                                    *
 *  U P D A T E  C H A N G E                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Update the change between two weather values.
 *  \param val Which value.
 *  \param newVal The new value just read.
 *  \param oldVal The old value shown.
 *  \result The new value.
 */
int updateChange(int val, int newVal, int oldVal)
{
	if (myWeather.updateNum != -1)
	{
		if (newVal > oldVal)
		{
			myWeather.changed[val][0] = 1;
			myWeather.changed[val][1] = 8;
		}
		else if (newVal < oldVal)
		{
			myWeather.changed[val][0] = -1;
			myWeather.changed[val][1] = 8;
		}
		else
		{
			if (myWeather.changed[val][1])
				myWeather.changed[val][1] = myWeather.changed[val][1] - 1;
			else
				myWeather.changed[val][0] = 0;
		}
	}
	return newVal;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C H A N G E  T E M P                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Convert between temp scales.
 *  \param temp Temp in C.
 *  \result Temp in other scale.
 */
double changeTemp(double temp)
{
	if (myWeather.tUnits == 1)
		return (((double)temp * 9) / 5) + 32;

	return (double)temp;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C H A N G E  S P E E D                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Convert between speed scales.
 *  \param speed Speed in kph.
 *  \result Speed in other scale.
 */
double changeSpeed(double speed)
{
	if (myWeather.sUnits == 1)	/* mps */
		return (speed * 1000) / 3600;
	if (myWeather.sUnits == 2)	/* mph */
		return speed * 0.621371;
	if (myWeather.sUnits == 3)	/* knots */
		return speed * 0.539957;
	return speed;				/* kph */
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C H A N G E  P R E S S U R E                                                                                      *
 *  ============================                                                                                      *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Convert between pressure scales.
 *  \param press Pressure in milli-bars.
 *  \result Pressure in other scale.
 */
double changePressure(int press)
{
	if (myWeather.pUnits == 1)
		return (double)press *0.75006375541921;
	if (myWeather.pUnits == 2)
		return (double)press *0.0295299830714;

	return (double)press;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  W E A T H E R  I N I T                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called once at the program start and find the location.
 *  \result None 0 all is OK.
 */
void readWeatherInit(void)
{
	if (gaugeEnabled[FACE_TYPE_WEATHER].enabled)
	{
		gaugeMenuDesc[MENU_GAUGE_WEATHER].disable = 0;
		myWeather.tempC = myWeather.apparent = tempUnits[myWeather.tUnits].tempMin;
		myWeather.windspeedKmph = speedUnits[myWeather.sUnits].speedMin;
		myWeather.pressure = pressureUnits[myWeather.pUnits].pressureMin;
		strcpy(myWeather.weatherDesc, _("Updating"));
		strcpy(myWeather.winddirPoint, _("Updating"));
		strcpy(myWeather.visView, _("Updating"));
		if (locationKey[0] == 0)
			strcpy(locationKey, "2647216");
		myWeather.nextUpdate = time(NULL) + 2;
		myWeather.updateNum = -1;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  W R I T E  M E M O R Y  C A L L B A C K                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called by curl to write memory in to a buffer.
 *  \param ptr Data to save.
 *  \param size Amount of data to save.
 *  \param nmemb number of elements.
 *  \param data Pointer to buffer.
 *  \result Amount we saved.
 */
static size_t writeMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t realsize = size * nmemb;
	if (realsize)
	{
		struct MemoryStruct *mem = (struct MemoryStruct *)data;
		/* char *text = (char *)ptr; */
		/* printf ("[s:%d,n:%d,%02X,%02X,%02X,%02X]\n>>>%s<<<\n", */
		/* size, nmemb, */
		/* text[0] & 0xFF, text[1] & 0xFF, text[2] & 0xFF, text[3] & 0xFF, */
		/* text); */
		mem->memory = realloc(mem->memory, mem->size + realsize + 1);
		if (mem->memory == NULL)
		{
			/*
             * out of memory!
             */
			printf("not enough memory (realloc returned NULL)\n");
			exit(EXIT_FAILURE);
		}
		memcpy(&(mem->memory[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
	}
	return realsize;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  D A Y  O F  W E E K                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Convert a 2013-02-13 into a day of week number.
 *  \param dateStr Sring to convert.
 *  \result Day of week Sundoy = 0.
 */
int getDayOfWeek(char *dateStr)
{
	int i = 0, val = 0, word = 0;
	struct tm theDate;

	memset(&theDate, 0, sizeof(struct tm));
	while (word < 3)
	{
		if (dateStr[i] >= '0' && dateStr[i] <= '9')
			val = (val * 10) + (dateStr[i] - '0');
		else
		{
			if (word == 0)
				theDate.tm_year = val - 1900;
			else if (word == 1)
				theDate.tm_mon = val - 1;
			else
			{
				theDate.tm_mday = val;
				mktime(&theDate);
				break;
			}
			val = 0;
			++word;
		}
		++i;
	}
	return theDate.tm_wday;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S P L I T  O U T  T I T L E                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Split out any info.
 *  \param value Value read for the obs title.
 *  \result None.
 */
void splitOutTitle(char *value)
{
	int i = 0, f = 0, c = 0;
	char buffer[81];

	buffer[c = 0] = 0;
	while (1)
	{
		if (value[i] == ':' && f < 2)
		{
			++f;
		}
		else if (f == 2 && (value[i] > ' ' || (value[i] == ' ' && c > 0)))
		{
			if (value[i] == ',')
			{
				strncpy(myWeather.weatherDesc, buffer, 80);
				break;
			}
			buffer[c] = value[i];
			buffer[++c] = 0;
		}
		if (value[i] == 0)
			break;
		++i;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S P L I T  O U T  F O R C A S T  T I T L E                                                                        *
 *  ==========================================                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Split out any info.
 *  \param value Value read for the forecast title.
 *  \param level Level of the title.
 *  \result None.
 */
void splitOutForcastTitle(char *value, int level)
{
	/*
     * Temperature: 21°C (70°F), Wind Direction: Southerly, Wind Speed:
     * 8mph, Humidity: 68%, Pressure: 1028mb, Falling, Visibility: Very Good
     */
	int i = 0, b = 0, c = 0, n;
	char buffers[2][81];

	buffers[0][0] = buffers[1][0] = 0;
	while (1)
	{
		if (value[i] == ':' && b == 0)
		{
			buffers[++b][c = 0] = 0;
		}
		else if (value[i] == ',' || value[i] == 0)
		{
			if (b == 1)
			{
				/* printf ("[o:%d][l:%d] %s = %s\n", observations, level, */
				/* &buffers[0][0], &buffers[1][0]); */
				for (n = 0; n < 7; ++n)
				{
					if (strcmp(daysOfWeek[n], &buffers[0][0]) == 0)
					{
						strncpy(myWeather.forecast[level - 1].date, &buffers[0][0], 20);
						strncpy(myWeather.forecast[level - 1].weatherDesc, &buffers[1][0], 80);
						break;
					}
				}
			}
			buffers[b = 0][c = 0] = 0;
		}
		else if (value[i] > ' ' || (value[i] == ' ' && c > 0))
		{
			if (c < 80)
			{
				buffers[b][c] = value[i];
				buffers[b][++c] = 0;
			}
		}
		if (value[i] == 0)
			break;
		++i;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S P L I T  O U T  D E S C R I P T I O N                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Split out any info.
 *  \param value Value of the description.
 *  \param level Level of the description.
 *  \result None.
 */
void splitOutDescription(char *value, int level)
{
	/*
     * Temperature: 21°C (70°F), Wind Direction: Southerly, Wind Speed:
     * 8mph, Humidity: 68%, Pressure: 1028mb, Falling, Visibility: Very Good
     */
	int i = 0, b = 0, c = 0;
	char buffers[2][81];

	buffers[0][0] = buffers[1][0] = 0;
	if (!observations)
	{
		myWeather.forecast[level - 1].evening = 1;
	}
	while (1)
	{
		if (value[i] == ':' && b == 0)
		{
			buffers[++b][c = 0] = 0;
		}
		else if (value[i] == ',' || value[i] == 0)
		{
			if (b == 1)
			{
				/* printf ("[o:%d][l:%d] %s = %s\n", observations, level, */
				/* &buffers[0][0], &buffers[1][0]); */
				if (strcmp(&buffers[0][0], "Temperature") == 0)
				{
					myWeather.tempC =
						updateChange(CHNG_TEMP, atoi(&buffers[1][0]), myWeather.tempC);
				}
				else if (strcmp(&buffers[0][0], "Maximum Temperature") == 0)
				{
					if (!observations)
					{
						myWeather.forecast[level - 1].tempMaxC = atoi(&buffers[1][0]);
						myWeather.forecast[level - 1].evening = 0;
					}
				}
				else if (strcmp(&buffers[0][0], "Minimum Temperature") == 0)
				{
					if (!observations)
					{
						myWeather.forecast[level - 1].tempMinC = atoi(&buffers[1][0]);
					}
				}
				else if (strcmp(&buffers[0][0], "Wind Speed") == 0)
				{
					int speed = atoi(&buffers[1][0]);
					if (observations)
					{
						if (strncmp (&buffers[1][0], "--", 2))
						{
							myWeather.windspeedmph = updateChange(CHNG_WIND, speed, myWeather.windspeedmph);
							myWeather.windspeedKmph = speed * 1.609344;
						}
						else
						{
							myWeather.windspeedmph = 
									updateChange(CHNG_WIND, myWeather.forecast[0].windspeedmph, myWeather.windspeedmph);
							myWeather.windspeedKmph = myWeather.forecast[0].windspeedKmph;
						}
					}
					else
					{
						myWeather.forecast[level - 1].windspeedmph = speed;
						myWeather.forecast[level - 1].windspeedKmph = speed * 1.609344;
					}
				}
				else if (strcmp(&buffers[0][0], "Pressure") == 0)
				{
					int press = atoi(&buffers[1][0]);
					if (observations)
					{
						if (strncmp (&buffers[1][0], "--", 2))
						{
							myWeather.pressure = updateChange(CHNG_PRES, press, myWeather.pressure);
						}
						else
						{
							myWeather.pressure = 
									updateChange(CHNG_PRES, myWeather.forecast[0].pressure, myWeather.pressure);
						}
					}
					else
					{
						myWeather.forecast[level - 1].pressure = press;
					}
				}
				else if (strcmp(&buffers[0][0], "Humidity") == 0)
				{
					int humid = atoi(&buffers[1][0]);
					if (observations)
					{
						if (strncmp (&buffers[1][0], "--", 2))
						{
							myWeather.humidity = updateChange(CHNG_HUMI, humid, myWeather.humidity);
						}
						else
						{
							myWeather.humidity = 
									updateChange(CHNG_HUMI, myWeather.forecast[0].humidity, myWeather.humidity);
						}
					}
					else
					{
						myWeather.forecast[level - 1].humidity = humid;
					}
				}
				else if (strcmp(&buffers[0][0], "Wind Direction") == 0)
				{
					if (observations)
						strncpy(myWeather.winddirPoint, &buffers[1][0], 20);
					else
						strncpy(myWeather.forecast[level - 1].winddirPoint, &buffers[1][0], 20);
				}
				else if (strcmp(&buffers[0][0], "Visibility") == 0)
				{
					if (observations)
						strncpy(myWeather.visView, &buffers[1][0], 20);
					else
						strncpy(myWeather.forecast[level - 1].visView, &buffers[1][0], 20);
				}
				else if (strcmp(&buffers[0][0], "Pollution") == 0)
				{
					if (!observations)
						strncpy(myWeather.forecast[level - 1].pollution, &buffers[1][0], 20);
				}
				else if (strcmp(&buffers[0][0], "UV Risk") == 0)
				{
					if (!observations)
						myWeather.forecast[level - 1].uvRisk = atoi(&buffers[1][0]);
				}
				else if (strcmp(&buffers[0][0], "Sunrise") == 0)
				{
					if (!observations)
						strncpy(myWeather.forecast[level - 1].sunrise, &buffers[1][0], 20);
				}
				else if (strcmp(&buffers[0][0], "Sunset") == 0)
				{
					if (!observations)
						strncpy(myWeather.forecast[level - 1].sunset, &buffers[1][0], 20);
				}
			}
			buffers[b = 0][c = 0] = 0;
		}
		else if (value[i] > ' ' || (value[i] == ' ' && c > 0))
		{
			if (c < 80)
			{
				buffers[b][c] = value[i];
				buffers[b][++c] = 0;
			}
		}
		if (value[i] == 0)
			break;
		++i;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  W E A T H E R  K E Y                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process each of the fields in the XML.
 *  \param readLevel 0 current, 1 today, 2 tomorrow.
 *  \param name Name of the field.
 *  \param value Value of the field.
 *  \result None.
 */
static void processWeatherKey(int readLevel, const char *name, char *value)
{
//  char *titleStr = "BBC Weather - Observations for  ";
	char *titleStr = "BBC Weather - Forecast for  ";

	if (readLevel > 0 && readLevel <= FORECAST_NUM && strcmp(name, "description") == 0)
	{
		if (value)
			splitOutDescription(value, readLevel);
	}
	else if (observations == 0 && readLevel == 0 && strcmp(name, "title") == 0)
	{
		if (strncmp(titleStr, value, strlen(titleStr)) == 0)
			strncpy(myWeather.queryName, &value[strlen(titleStr)], 80);
	}
	else if (observations == 1 && readLevel == 1 && strcmp(name, "title") == 0)
	{
		if (value)
			splitOutTitle(value);
	}
	else if (observations == 0 && readLevel <= FORECAST_NUM && strcmp(name, "title") == 0)
	{
		if (value)
			splitOutForcastTitle(value, readLevel);
	}
	else if (readLevel == 0 && strcmp(name, "pubDate") == 0)
	{
		if (value[0] != 0)
		{
			strncpy(myWeather.updateTime, value, 60);
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  E L E M E N T  N A M E S                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process each of the elements in the file.
 *  \param doc Document to read.
 *  \param aNode Current node.
 *  \param readLevel 0 current, 1 today, 2 tomorrow.
 *  \result None.
 */
static void processElementNames(xmlDoc * doc, xmlNode * aNode, int readLevel)
{
	xmlChar *key;
	xmlNode *curNode = NULL;

	for (curNode = aNode; curNode; curNode = curNode->next)
	{
		if (curNode->type == XML_ELEMENT_NODE)
		{
			if ((!xmlStrcmp(curNode->name, (const xmlChar *)"item")))
			{
				++readLevel;
			}
			else
			{
				key = xmlNodeListGetString(doc, curNode->xmlChildrenNode, 1);
				processWeatherKey(readLevel, (const char *)curNode->name, (char *)key);
				xmlFree(key);
			}
		}
		processElementNames(doc, curNode->children, readLevel);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G Z I P  I N F L A T E                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Inflate a compress buffer.
 *  \param compressedBytes Buffer to inflate.
 *  \param compressedSize Size of the input buffer.
 *  \param uncompressedBytes Save the inflated data here.
 *  \param uncompressedSize Size of the inflated beffer.
 *  \result Number of output bytes, 0 on error.
 */
int gzipInflate (Bytef *compressedBytes, int compressedSize, Bytef *uncompressedBytes, int uncompressedSize)
{
	bool done = false;

	z_stream strm;
	strm.next_in = compressedBytes;
	strm.avail_in = compressedSize;
	strm.next_out = uncompressedBytes;
	strm.avail_out = uncompressedSize;
	strm.total_out = 0;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;

	if (inflateInit2 (&strm, (16 + MAX_WBITS)) != Z_OK)
		return 0;

	int err = inflate(&strm, Z_SYNC_FLUSH);
	if (err == Z_STREAM_END)
		done = true;

	if (inflateEnd(&strm) != Z_OK)
		return 0;

	return (done ? strm.total_out : 0);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  B U F F E R                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process the down loaded buffer.
 *  \param buffer Buffer to process.
 *  \param size Size of the buffer.
 *  \result None.
 */
static void processBuffer(char *buffer, size_t size)
{
	xmlDoc *doc = NULL;
	xmlNode *rootElement = NULL;
	xmlChar *xmlBuffer = NULL;

/*  printf("Init: %0X:%0X %c%c\n", buffer[0], buffer[1],
           buffer[0] > ' ' && buffer[0] < 127 ? buffer[0] : '-',
           buffer[1] > ' ' && buffer[1] < 127 ? buffer[1] : '-');
*/
	if (size > 10 && buffer[0] == '\037' && buffer[1] == '\213')
	{
		int retn = 0;
		char *tempBuff = (char *)malloc(8001);
		if (tempBuff == NULL)
			return;

		if ((retn = gzipInflate ((Bytef *)buffer, size, (Bytef *)tempBuff, 8000)) == 0)
		{
			free(tempBuff);
			return;
		}
/*      printf("Unpk: %0X:%0X %c%c\n", (unsigned int)tempBuff[0], (unsigned int)tempBuff[1],
               tempBuff[0] > ' ' && tempBuff[0] < 127 ? tempBuff[0] : '-',
               tempBuff[1] > ' ' && tempBuff[1] < 127 ? tempBuff[1] : '-');
*/
		tempBuff[retn] = 0;
		xmlBuffer = xmlCharStrndup(tempBuff, retn);
		free(tempBuff);
	}
	else
	{
		xmlBuffer = xmlCharStrndup(buffer, size);
	}
	if (xmlBuffer != NULL)
	{
		doc = xmlParseDoc (xmlBuffer);

		if (doc != NULL)
		{
			rootElement = xmlDocGetRootElement (doc);
			processElementNames (doc, rootElement, 0);
			xmlFreeDoc (doc);
		}
		else
		{
			printf ("error: could not parse memory\n");
			printf ("BUFF[%lu] [[[%s]]]\n", size, buffer);
		}
		xmlFree (xmlBuffer);
	}
	xmlCleanupParser();
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F I X U P  S H O W  V A L U E S                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Convert the metric value to what the user selected.
 *  \result None.
 */
static void fixupShowValues()
{
	int i;

	if (myWeather.tempC <= 10 && myWeather.windspeedKmph > 5)
	{
		myWeather.apparent = 13.12 + (0.6215 * (double)myWeather.tempC) -
			(11.37 * pow(myWeather.windspeedKmph, 0.16)) +
			(0.3965 * (double)myWeather.tempC * pow(myWeather.windspeedKmph, 0.16));
	}
	else
	{
		myWeather.apparent = myWeather.tempC;
	}

	myWeather.dewPoint =
		(pow(((double)myWeather.humidity / 100), 0.125) * (112 + (double)myWeather.tempC * 0.9)) +
		(0.1 * (double)myWeather.tempC) - 112;

	myWeather.showTemp = changeTemp((double)myWeather.tempC);
	myWeather.showApparent = changeTemp(myWeather.apparent);
	myWeather.showDewPoint = changeTemp(myWeather.dewPoint);
	myWeather.showWindSpeed = changeSpeed(myWeather.windspeedKmph);
	myWeather.showPressure = changePressure(myWeather.pressure);
	for (i = 0; i < FORECAST_NUM; ++i)
	{
		myWeather.forecast[i].showTempMax = changeTemp(myWeather.forecast[i].tempMaxC);
		myWeather.forecast[i].showTempMin = changeTemp(myWeather.forecast[i].tempMinC);
		myWeather.forecast[i].showWindSpeed = changeSpeed(myWeather.forecast[i].windspeedKmph);
		myWeather.forecast[i].showPressure = changePressure(myWeather.forecast[i].pressure);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D O  U P D A T E  W E A T H E R  I N F O                                                                          *
 *  ========================================                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Send a request for the weather at the current location.
 *  \param weatherURL Called for each for the pages to read.
 *  \result None.
 */
void doUpdateWeatherInfo(char *weatherURL)
{
	CURL *curlHandle;
	char fullURL[512], *encodedLoc;
	struct MemoryStruct chunk;

	chunk.memory = malloc(1);
	chunk.size = 0;

	curl_global_init(CURL_GLOBAL_ALL);
	curlHandle = curl_easy_init();

	encodedLoc = curl_easy_escape(curlHandle, locationKey, 0);
	sprintf(fullURL, weatherURL, encodedLoc);
	curl_free(encodedLoc);

	curl_easy_setopt(curlHandle, CURLOPT_URL, fullURL);
#ifdef CURLOPT_TRANSFER_ENCODING
	curl_easy_setopt(curlHandle, CURLOPT_TRANSFER_ENCODING, 1);
#endif
#ifdef CURLOPT_ACCEPT_ENCODING
	curl_easy_setopt(curlHandle, CURLOPT_ACCEPT_ENCODING, "gzip");
#endif
	curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
	curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, (void *)&chunk);
	curl_easy_setopt(curlHandle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_perform(curlHandle);
	curl_easy_cleanup(curlHandle);

	if (chunk.size)
	{
		processBuffer(chunk.memory, chunk.size);
	}

	if (chunk.memory)
	{
		free(chunk.memory);
	}
	curl_global_cleanup();
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  U P D A T E  W E A T H E R  I N F O                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Update the weather information.
 *  \result None.
 */
void updateWeatherInfo()
{
	if (time(NULL) >= myWeather.nextUpdate)
	{
		myWeather.updateTime[0] = 0;

		observations = 0;
		doUpdateWeatherInfo(weatherTFCURL);
		observations = 1;
		doUpdateWeatherInfo(weatherOBSURL);

		if (myWeather.updateTime[0])
		{
			myWeather.nextUpdate = time(NULL) + (15 * 60);
			if (++myWeather.updateNum == 100)
				myWeather.updateNum = 0;

			fixupShowValues();
		}
		else
		{
			myWeather.nextUpdate = time(NULL) + 15;
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  W E A T H E R  G E T  M A X  M I N                                                                                *
 *  ==================================                                                                                *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get amx and min from the weather unit settings.
 *  \param faceSetting What face is this for.
 *  \result None.
 */
void weatherGetMaxMin(FACE_SETTINGS * faceSetting)
{
	myWeather.tUnits = weatherScales & 0xF;
	myWeather.sUnits = (weatherScales >> 4) & 0xF;
	myWeather.pUnits = (weatherScales >> 8) & 0xF;

	switch (faceSetting->faceSubType & 0x0007)
	{
	case 0:					/* Temp */
	case 4:					/* Today */
	case 5:					/* Tomorrow */
	case 6:					/* Two Days Time */
		faceSetting->faceScaleMin = tempUnits[myWeather.tUnits].tempMin;
		faceSetting->faceScaleMax = tempUnits[myWeather.tUnits].tempMax;
		faceSetting->faceFlags |= FACE_HOT_COLD;
		break;
	case 1:					/* Humidity */
		faceSetting->faceScaleMin = 0;
		faceSetting->faceScaleMax = 100;
		break;
	case 2:					/* Pressure */
		faceSetting->faceScaleMin = pressureUnits[myWeather.pUnits].pressureMin;
		faceSetting->faceScaleMax = pressureUnits[myWeather.pUnits].pressureMax;
		if (pressureUnits[myWeather.pUnits].pressureUnit == 2)
			faceSetting->faceFlags |= FACE_SHOW_POINT;
		else
			faceSetting->faceFlags &= ~FACE_SHOW_POINT;
		break;
	case 3:					/* Wind Speed */
		faceSetting->faceScaleMin = speedUnits[myWeather.sUnits].speedMin;
		faceSetting->faceScaleMax = speedUnits[myWeather.sUnits].speedMax;
		faceSetting->faceFlags |= FACE_HOT_COLD;
		break;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S H O W  E R R O R  M E S S A G E                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display an error message.
 *  \param message Message to display.
 *  \result None.
 */
void showErrorMessage(char *message)
{
	GtkWidget *dialog;
	dialog = gtk_message_dialog_new(GTK_WINDOW(dialConfig.mainWindow),
									GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, NULL);
	gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), message);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  W E A T H E R  V A L U E S                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Calculate the value to show on the face.
 *  \param face Which face to display on.
 *  \result None zero if the face should be re-shown.
 */
void readWeatherValues(int face)
{
	if (gaugeEnabled[FACE_TYPE_WEATHER].enabled)
	{
		FACE_SETTINGS *faceSetting = faceSettings[face];
		int subType = faceSetting->faceSubType & 0x0007, i;

		if (myWeather.message)
		{
			char *showMsg = myWeather.message;
			myWeather.message = NULL;
			showErrorMessage(showMsg);
			free(showMsg);
		}
		if (faceSetting->faceFlags & FACE_REDRAW)
		{
			;
		}
		else if (faceSetting->nextUpdate)
		{
			faceSetting->nextUpdate -= 1;
			return;
		}
		else if (!faceSetting->nextUpdate)
		{
			updateWeatherInfo();
			faceSetting->nextUpdate = 20;
			if (myWeather.updateNum == faceSetting->updateNum)
				return;
		}

		switch (subType)
		{
		case 0:					/* Temp */
			{
				char summary[81];
				strcpy (summary, myWeather.weatherDesc);
				if (strncasecmp (summary, "not available", 13) == 0)
				{
					strcpy (summary, myWeather.forecast[0].weatherDesc);
					strcat (summary, "*");
				}
				setFaceString(faceSetting, FACESTR_TOP, 16, summary);
				setFaceString(faceSetting, FACESTR_BOT, 0, "%0.0f%s", myWeather.showTemp,
							  tempUnits[myWeather.tUnits].tempText);
				if (myWeather.showTemp != myWeather.showApparent)
				{
					setFaceString(faceSetting, FACESTR_TIP, 0,
								_("<b>Location</b>: %s\n"
								"<b>Temperature</b>: %0.1f%s (%s)\n"
								"<b>Feels like</b>: %0.1f%s\n"
								"<b>Summary</b>: %s\n"
								"<b>Update</b>: %s"),
								myWeather.queryName,
								myWeather.showTemp, tempUnits[myWeather.tUnits].tempText,
								gettext(changeText[myWeather.changed[CHNG_TEMP][0] + 1]),
								myWeather.showApparent, tempUnits[myWeather.tUnits].tempText,
								summary,
								myWeather.updateTime);
					setFaceString(faceSetting, FACESTR_WIN, 0,
								_("Temp: %0.1f%s Feels: %0.1f%s - Gauge"),
								myWeather.showTemp, tempUnits[myWeather.tUnits].tempText,
								myWeather.showApparent, tempUnits[myWeather.tUnits].tempText);
				}
				else
				{
					setFaceString(faceSetting, FACESTR_TIP, 0,
								_("<b>Location</b>: %s\n"
								"<b>Temperature</b>: %0.1f%s (%s)\n"
								"<b>Summary</b>: %s\n"
								"<b>Update</b>: %s"),
								myWeather.queryName,
								myWeather.showTemp, tempUnits[myWeather.tUnits].tempText,
								gettext(changeText[myWeather.changed[CHNG_TEMP][0] + 1]),
								summary,
								myWeather.updateTime);
					setFaceString(faceSetting, FACESTR_WIN, 0, _("Temperature: %0.1%s - Gauge"),
								myWeather.showTemp, tempUnits[myWeather.tUnits].tempText);
				}
				while (myWeather.showTemp < faceSetting->faceScaleMin
					   || myWeather.showApparent < faceSetting->faceScaleMin)
				{
					faceSetting->faceScaleMin -= 5;
					faceSetting->faceScaleMax -= 5;
					maxMinReset(&faceSetting->savedMaxMin, 24, 3600);
				}
				while (myWeather.showTemp > faceSetting->faceScaleMax
					   || myWeather.showApparent > faceSetting->faceScaleMax)
				{
					faceSetting->faceScaleMin += 5;
					faceSetting->faceScaleMax += 5;
					maxMinReset(&faceSetting->savedMaxMin, 24, 3600);
				}
				faceSetting->firstValue = myWeather.showTemp;
				faceSetting->secondValue = myWeather.showApparent;
			}
			break;
		case 1:					/* Humidity */
			setFaceString(faceSetting, FACESTR_TOP, 0, _("%0.0f%s\nDew point"), myWeather.showDewPoint,
						  tempUnits[myWeather.tUnits].tempText);
			setFaceString(faceSetting, FACESTR_BOT, 0, _("%d%%"), myWeather.humidity);
			setFaceString(faceSetting, FACESTR_TIP, 0,
							_("<b>Location</b>: %s\n"
							"<b>Humidity</b>: %d%% (%s)\n"
							"<b>Dew point</b>: %0.1f%s\n"
							"<b>Update</b>: %s"),
							myWeather.queryName,
							myWeather.humidity, gettext(changeText[myWeather.changed[CHNG_HUMI][0] + 1]),
							myWeather.showDewPoint, tempUnits[myWeather.tUnits].tempText,
							myWeather.updateTime);
			setFaceString(faceSetting, FACESTR_WIN, 0, _("Humidity: %d%% - Gauge"), myWeather.humidity);
			faceSetting->firstValue = myWeather.humidity;
			faceSetting->secondValue = DONT_SHOW;
			break;
		case 2:					/* Pressure */
			{
				char vis[81];
				strcpy (vis, myWeather.visView);
				if (strncmp (vis, "--", 2) == 0)
				{
					strcpy (vis, myWeather.forecast[0].visView);
					strcat (vis, "*");
				}
				setFaceString(faceSetting, FACESTR_TOP, 0, wrapText(vis, 1));
				if (faceSetting->faceFlags & FACE_SHOW_POINT)
				{
					setFaceString(faceSetting, FACESTR_BOT, 0, _("%0.2f\n(%s)"), myWeather.showPressure,
								pressureUnits[myWeather.pUnits].pressureText);
					setFaceString(faceSetting, FACESTR_TIP, 0,
								_("<b>Location</b>: %s\n"
								"<b>Pressure</b>: %0.2f%s (%s)\n"
								"<b>Visibility</b>: %s\n"
								"<b>Update</b>: %s"),
								myWeather.queryName,
								myWeather.showPressure, pressureUnits[myWeather.pUnits].pressureText,
								gettext(changeText[myWeather.changed[CHNG_PRES][0] + 1]),
								vis, myWeather.updateTime);
					setFaceString(faceSetting, FACESTR_WIN, 0, _("Air Pressure: %0.2f%s - Gauge"),
								myWeather.showPressure, pressureUnits[myWeather.pUnits].pressureText);
				}
				else
				{
					setFaceString(faceSetting, FACESTR_BOT, 0, _("%0.0f\n(%s)"), myWeather.showPressure,
								  pressureUnits[myWeather.pUnits].pressureText);
					setFaceString(faceSetting, FACESTR_TIP, 0,
								_("<b>Location</b>: %s\n"
								"<b>Pressure</b>: %0.0f%s (%s)\n"
								"<b>Visibility</b>: %s\n"
								"<b>Update</b>: %s"),
								myWeather.queryName,
								myWeather.showPressure, pressureUnits[myWeather.pUnits].pressureText,
								gettext(changeText[myWeather.changed[CHNG_PRES][0] + 1]),
								vis, myWeather.updateTime);
					setFaceString(faceSetting, FACESTR_WIN, 0, _("Air Pressure: %0.0f%s - Gauge"),
								myWeather.showPressure, pressureUnits[myWeather.pUnits].pressureText);
				}
				faceSetting->firstValue = myWeather.showPressure;
				if (pressureUnits[myWeather.pUnits].pressureUnit == 0)
					faceSetting->firstValue /= 10;
				faceSetting->secondValue = DONT_SHOW;
			}
			break;
		case 3:					/* Wind Speed */
			{
				char direc[81];
				strcpy (direc, myWeather.winddirPoint);
				if (strncasecmp (direc, "direction not avail", 19) == 0)
				{
					strcpy (direc, myWeather.forecast[0].winddirPoint);
					strcat (direc, "*");
				}
				setFaceString(faceSetting, FACESTR_TOP, 16, direc);
				setFaceString(faceSetting, FACESTR_BOT, 0, _("%0.1f\n(%s)"), myWeather.showWindSpeed,
								speedUnits[myWeather.sUnits].speedText);
				setFaceString(faceSetting, FACESTR_TIP, 0,
								_("<b>Location</b>: %s\n"
								"<b>Wind speed</b>: %0.1f%s (%s)\n"
								"<b>Direction</b>: %s\n"
								"<b>Update</b>: %s"),
								myWeather.queryName,
								myWeather.showWindSpeed, speedUnits[myWeather.sUnits].speedText,
								gettext(changeText[myWeather.changed[CHNG_WIND][0] + 1]), direc,
								myWeather.updateTime);
				setFaceString(faceSetting, FACESTR_WIN, 0, _("Wind Speed: %0.1f%s - Gauge"),
								myWeather.showWindSpeed, speedUnits[myWeather.sUnits].speedText);
				faceSetting->firstValue = myWeather.showWindSpeed;
				faceSetting->secondValue = DONT_SHOW;
			}
			break;
		case 4:					/* Temp. forecast */
		case 5:
		case 6:
			i = (subType - 4);
			setFaceString(faceSetting, FACESTR_TOP, 16, "%s", myWeather.forecast[i].date);
			if (myWeather.forecast[i].evening)
			{
				setFaceString(faceSetting, FACESTR_BOT, 0,
							"%0.0f%s\n(%s)",
							myWeather.forecast[i].showTempMin, tempUnits[myWeather.tUnits].tempText,
							"Min");
				setFaceString(faceSetting, FACESTR_TIP, 0,
							_("<b>Location</b>: %s\n"
							"<b>Summary</b>: %s\n"
							"<b>Temp Min</b>: %0.1f%s\n"
							"<b>Pressure</b>: %0.2f%s\n"
							"<b>Wind</b>: %s, %0.1f%s\n"
							"<b>Pollution</b>: %s, <b>UV Risk</b>: %d\n"
							"<b>Sun Set</b>: %s\n"
							"<b>Update</b>: %s"),
							myWeather.queryName,
							myWeather.forecast[i].weatherDesc,
							myWeather.forecast[i].showTempMin, tempUnits[myWeather.tUnits].tempText,
							myWeather.forecast[i].showPressure, pressureUnits[myWeather.pUnits].pressureText,
							myWeather.forecast[i].winddirPoint,
							myWeather.forecast[i].showWindSpeed, speedUnits[myWeather.sUnits].speedText,
							myWeather.forecast[i].pollution, myWeather.forecast[i].uvRisk,
							myWeather.forecast[i].sunset,
							myWeather.updateTime);
				faceSetting->secondValue = DONT_SHOW;
			}
			else
			{
				setFaceString(faceSetting, FACESTR_BOT, 0, "%0.0f%s\n%0.0f%s",
							myWeather.forecast[i].showTempMin, tempUnits[myWeather.tUnits].tempText,
							myWeather.forecast[i].showTempMax, tempUnits[myWeather.tUnits].tempText);
				setFaceString(faceSetting, FACESTR_TIP, 0,
							_("<b>Location</b>: %s\n"
							"<b>Summary</b>: %s\n"
							"<b>Temp Min</b>: %0.1f%s, <b>Max</b>: %0.1f%s\n"
							"<b>Pressure</b>: %0.2f%s\n"
							"<b>Wind</b>: %s, %0.1f%s\n"
							"<b>Pollution</b>: %s, <b>UV Risk</b>: %d\n"
							"<b>Sun Rise</b>: %s, <b>Set</b>: %s\n"
							"<b>Update</b>: %s"),
							myWeather.queryName,
							myWeather.forecast[i].weatherDesc,
							myWeather.forecast[i].showTempMin, tempUnits[myWeather.tUnits].tempText,
							myWeather.forecast[i].showTempMax, tempUnits[myWeather.tUnits].tempText,
							myWeather.forecast[i].showPressure, pressureUnits[myWeather.pUnits].pressureText,
							myWeather.forecast[i].winddirPoint,
							myWeather.forecast[i].showWindSpeed, speedUnits[myWeather.sUnits].speedText,
							myWeather.forecast[i].pollution, myWeather.forecast[i].uvRisk,
							myWeather.forecast[i].sunrise, myWeather.forecast[i].sunset,
							myWeather.updateTime);
				faceSetting->secondValue = myWeather.forecast[i].showTempMax;
			}
			faceSetting->firstValue = myWeather.forecast[i].showTempMin;
			break;
		}
		if (myWeather.updateNum != -1)
		{
			faceSetting->faceFlags |= FACE_MAX_MIN;
			faceSetting->updateNum = myWeather.updateNum;
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  W E A T H E R  G A U G E  R E S E T                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Reset all the weather gauges.
 *  \result None.
 */
void weatherGaugeReset()
{
	int i, j, face = 0;

	fixupShowValues();
	for (j = 0; j < dialConfig.dialHeight; j++)
	{
		for (i = 0; i < dialConfig.dialWidth; i++)
		{
			if (faceSettings[face]->showFaceType == FACE_TYPE_WEATHER)
			{
				weatherGetMaxMin(faceSettings[face]);
				maxMinReset(&faceSettings[face]->savedMaxMin, 24, 3600);
				faceSettings[face]->nextUpdate = 0;
				faceSettings[face]->updateNum = -1;
			}
			++face;
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  W E A T H E R  C O M B O  C A L L B A C K                                                                         *
 *  =========================================                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Call back from a unit change.
 *  \param comboBox Which box changed.
 *  \param data Whick option it was.
 *  \result None.
 */
void weatherComboCallback(GtkWidget * comboBox, gpointer data)
{
	long option = (long)data;
	int sel = gtk_combo_box_get_active(GTK_COMBO_BOX(comboBox));

	switch (option)
	{
	case 0:
		myWeather.tUnits = sel;
		weatherScales &= 0xFFF0;
		weatherScales |= sel;
		break;
	case 1:
		myWeather.sUnits = sel;
		weatherScales &= 0xFF0F;
		weatherScales |= (sel << 4);
		break;
	case 2:
		myWeather.pUnits = sel;
		weatherScales &= 0xF0FF;
		weatherScales |= (sel << 8);
		break;
	}
	configSetIntValue ("Weather_scales", weatherScales);
	weatherGaugeReset();
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  W E A T H E R  S E T T I N G S                                                                                    *
 *  ==============================                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Show a dialog and get the weather settings.
 *  \param data Not used.
 *  \result None.
 */
void weatherSettings(guint data)
{
	GtkWidget *comboBox;
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *entryKey;
	const char *saveText;
	char textUpdate = 0;
#if GTK_MAJOR_VERSION == 2
	GtkWidget *hbox;
#else
	GtkWidget *contentArea;
	GtkWidget *grid;
#endif

#if GTK_MAJOR_VERSION == 2

	dialog = gtk_dialog_new_with_buttons ("Weather Settings", GTK_WINDOW(dialConfig.mainWindow),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);

	vbox = GTK_DIALOG(dialog)->vbox;
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 3);

	label = gtk_label_new(_("Weather from: http://www.bbc.co.uk/weather/"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 3);
	label = gtk_label_new("Location code:");
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	entryKey = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entryKey), 30);
	gtk_entry_set_text(GTK_ENTRY(entryKey), locationKey);
	gtk_box_pack_start(GTK_BOX(hbox), entryKey, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 3);
	label = gtk_label_new("Temperature units:");
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	comboBox = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(comboBox), "Centigrade");
	gtk_combo_box_append_text(GTK_COMBO_BOX(comboBox), "Fahrenheit");
	gtk_combo_box_set_active(GTK_COMBO_BOX(comboBox), myWeather.tUnits);
	g_signal_connect(comboBox, "changed", G_CALLBACK(weatherComboCallback), (gpointer) 0);
	gtk_box_pack_start(GTK_BOX(hbox), comboBox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 3);
	label = gtk_label_new("Pressure units:");
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	comboBox = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(comboBox), "Milli Bars");
	gtk_combo_box_append_text(GTK_COMBO_BOX(comboBox), "Millimeters Mercury (Hg)");
	gtk_combo_box_append_text(GTK_COMBO_BOX(comboBox), "Inches Mercury (Hg)");
	gtk_combo_box_set_active(GTK_COMBO_BOX(comboBox), myWeather.pUnits);
	g_signal_connect(comboBox, "changed", G_CALLBACK(weatherComboCallback), (gpointer) 2);
	gtk_box_pack_start(GTK_BOX(hbox), comboBox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 3);
	label = gtk_label_new("Speed units:");
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	comboBox = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(comboBox), "Kilometres Per Hour");
	gtk_combo_box_append_text(GTK_COMBO_BOX(comboBox), "Metres Per Second");
	gtk_combo_box_append_text(GTK_COMBO_BOX(comboBox), "Miles Per Hour");
	gtk_combo_box_append_text(GTK_COMBO_BOX(comboBox), "Knots (Nautical Miles Per Hour)");
	gtk_combo_box_set_active(GTK_COMBO_BOX(comboBox), myWeather.sUnits);
	g_signal_connect(comboBox, "changed", G_CALLBACK(weatherComboCallback), (gpointer) 1);
	gtk_box_pack_start(GTK_BOX(hbox), comboBox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

#else

	dialog = gtk_dialog_new_with_buttons("Weather Settings", GTK_WINDOW(dialConfig.mainWindow),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			_("_OK"), GTK_RESPONSE_ACCEPT, _("_Cancel"), GTK_RESPONSE_REJECT, NULL);

	contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 3);
	gtk_box_pack_start(GTK_BOX(contentArea), vbox, TRUE, TRUE, 0);
	grid = gtk_grid_new();

	label = gtk_label_new(_("Weather from: "));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 1, 1, 1);
	label = gtk_label_new(_("http://www.bbc.co.uk/weather/"));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 2, 1, 1, 1);

	label = gtk_label_new(_("Location code: "));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 2, 1, 1);
	entryKey = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entryKey), 30);
	gtk_entry_set_text(GTK_ENTRY(entryKey), locationKey);
	gtk_grid_attach(GTK_GRID(grid), entryKey, 2, 2, 1, 1);

	label = gtk_label_new("Temperature: ");
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 3, 1, 1);
	comboBox = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboBox), _("Centigrade"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboBox), _("Fahrenheit"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(comboBox), myWeather.tUnits);
	g_signal_connect(comboBox, "changed", G_CALLBACK(weatherComboCallback), (gpointer) 0);
	gtk_grid_attach(GTK_GRID(grid), comboBox, 2, 3, 1, 1);

	label = gtk_label_new("Pressure: ");
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 4, 1, 1);
	comboBox = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboBox), _("Milli Bars"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboBox), _("Millimeters Mercury (Hg)"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboBox), _("Inches Mercury (Hg)"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(comboBox), myWeather.pUnits);
	g_signal_connect(comboBox, "changed", G_CALLBACK(weatherComboCallback), (gpointer) 2);
	gtk_grid_attach(GTK_GRID(grid), comboBox, 2, 4, 1, 1);

	label = gtk_label_new("Wind speed: ");
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 5, 1, 1);
	comboBox = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboBox), _("Kilometres Per Hour"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboBox), _("Metres Per Second"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboBox), _("Miles Per Hour"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboBox),
								   _("Knots (Nautical Miles Per Hour)"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(comboBox), myWeather.sUnits);
	g_signal_connect(comboBox, "changed", G_CALLBACK(weatherComboCallback), (gpointer) 1);
	gtk_grid_attach(GTK_GRID(grid), comboBox, 2, 5, 1, 1);

	gtk_box_pack_start(GTK_BOX(vbox), grid, FALSE, FALSE, 0);

#endif

	gtk_widget_show_all(dialog);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		saveText = gtk_entry_get_text(GTK_ENTRY(entryKey));
		if (strcmp(saveText, locationKey) != 0)
		{
			strncpy(locationKey, saveText, 40);
			configSetValue ("location_key", locationKey);
			textUpdate = 1;
		}
		if (textUpdate)
		{
			weatherGaugeReset();
			myWeather.updateNum = -1;
			myWeather.nextUpdate = 0;
		}
	}
	gtk_widget_destroy(dialog);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S A V E  C U R R E N T  W E A T H E R                                                                             *
 *  =====================================                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Save the current weather.
 *  \result None.
 */
void saveCurrentWeather(void)
{
	static char saveDate[41] = "";

	if (strcmp(saveDate, myWeather.updateTime))
	{
		FILE *saveFile = fopen("/tmp/weather.csv", "a");

		if (saveFile != NULL)
		{
			fprintf(saveFile, "%s,%f,%f,%f,%f\n",
					myWeather.updateTime,
					myWeather.showTemp,
					myWeather.showWindSpeed, myWeather.showPressure, myWeather.showApparent);
			fclose(saveFile);
		}
		strcpy(saveDate, myWeather.updateTime);
	}
}
