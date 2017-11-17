/**********************************************************************************************************************
 *                                                                                                                    *
 *  P A R S E  Z O N E . C                                                                                            *
 *  ======================                                                                                            *
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
 *  \brief Function to parse the zone file.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>

#include "TzClockDisp.h"
#include "ParseZone.h"

typedef struct _areaInfo
{
	char *areaName;
	void *cityList;
	void *subAreaList;
}
AREAINFO;

int areaCount;
int subAreaCount;
int cityCount;

extern MENU_DESC *timeZoneMenu;
extern TZ_INFO *timeZones;
extern int nTimeZones;

static char timeNames[FIRST_CITY + 3][31] = { "Local Time", "Greenwich Mean Time", "-" };
static char *areaSwap[] = 
{
	"Arctic", "Arctic Ocean",
	"Atlantic", "Atlantic Ocean",
	"Indian", "Indian Ocean",
	"Pacific", "Pacific Ocean",
	NULL, NULL
};

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  A R E A  A N D  C I T Y                                                                                    *
 *  ==============================                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Split the area into parts.
 *  \param inBuffer Input full name.
 *  \param area Output area.
 *  \param subArea Output sub-area.
 *  \param city Output city.
 *  \result None.
 */
int getAreaAndCity (char *inBuffer, char *area, char *subArea, char *city)
{
	int i = 0, j = 0, word = 0;
	
	
	area[0] = subArea[0] = city[0] = 0;
	
	while (inBuffer[i] && word < 4)
	{
		if (inBuffer[i] <= ' ')
		{
			j = 0;
			word ++;
		}
		else
		{
			if (word == 2)
			{
				if (inBuffer[i] == '/')
				{
					j = 0;
					word ++;
				}
				else
				{
					area [j++] = inBuffer[i];
					area [j] = 0;
				}
			}
			else if (word == 3)
			{
				if (inBuffer[i] == '/')
				{
					strcpy (subArea, city);
					city[j = 0] = 0;
				}
				else
				{
					city [j++] = inBuffer[i];
					city [j] = 0;
				}
			}
		}
		i++;
	}
	return (area[0] != 0 && city[0] != 0);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F I N D  A R E A  I N F O                                                                                         *
 *  =========================                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Search for an area.
 *  \param areaInfoList List to search in.
 *  \param area Area to look for.
 *  \result None.
 */
AREAINFO *findAreaInfo (void *areaInfoList, char *area)
{
	int i = 0;
	AREAINFO *retnAreaInfo;
	
	while ((retnAreaInfo = (AREAINFO *)queueRead (areaInfoList, i)) != NULL)
	{
		if (strcmp (retnAreaInfo -> areaName, area) == 0)
			break;
			
		i++;
	}
	return retnAreaInfo;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  T I D Y  N A M E                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Tidy up a name.
 *  \param name Name to tidy up.
 *  \result None.
 */
char *tidyName (char *name)
{
	int i = 0;
	char *retnName;
	
	if ((retnName = (char *)malloc (strlen(name) + 1)) != NULL)
	{
		while (name[i])
		{
			if (name[i] == '_' || (name[i] >= 0 && name[i] < ' '))
				retnName[i] = ' ';
			else
				retnName[i] = name[i];
						
			retnName[++i] = 0;
		}
	}
	return retnName;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O M P A R E  A R E A                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Compare two areas.
 *  \param item1 First area.
 *  \param item2 Other area.
 *  \result 1, 0 or -1 depending on the order.
 */
int compareArea (void *item1, void *item2)
{
	AREAINFO *areaInfo1 = (AREAINFO *)item1;
	AREAINFO *areaInfo2 = (AREAINFO *)item2;
	
	return (strcmp (areaInfo1 -> areaName, areaInfo2 -> areaName));
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O M P A R E  C I T Y                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Compare two cities.
 *  \param item1 First city.
 *  \param item2 Other city.
 *  \result 1, 0 or -1 depending on the order.
 */
int compareCity (void *item1, void *item2)
{
	return (strcmp ((char *)item1, (char *)item2));
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  A D D  A R E A  A N D  C I T Y                                                                                    *
 *  ==============================                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Add a city to the list.
 *  \param areaInfoList List to add to.
 *  \param area Area to add.
 *  \param subArea Sub-area to add.
 *  \param city City to add.
 *  \result None.
 */
int addAreaAndCity (void *areaInfoList, char *area, char *subArea, char *city)
{
	AREAINFO *areaInfo = NULL, *subAreaInfo = NULL;
	char *tempArea, *tempSubArea, *tempCity;
	
	if ((areaInfo = findAreaInfo (areaInfoList, area)) == NULL)
	{
		if ((areaInfo = (AREAINFO *)malloc (sizeof (AREAINFO))) == NULL)
		{
			return 0;
		}
		if ((tempArea = (char *)malloc (strlen (area) + 1)) == NULL)
		{
			free (areaInfo);
			return 0;
		}
		strcpy (tempArea, area);
		areaInfo -> areaName = tempArea;
		areaInfo -> cityList = queueCreate();
		areaInfo -> subAreaList = queueCreate();
		areaCount ++;
		
		queuePutSort (areaInfoList, areaInfo, compareArea);
	}
	
	if (subArea[0])
	{
		if ((subAreaInfo = findAreaInfo (areaInfo -> subAreaList, subArea)) == NULL)
		{
			if ((subAreaInfo = (AREAINFO *)malloc (sizeof (AREAINFO))) == NULL)
			{
				return 0;
			}
			if ((tempSubArea = (char *)malloc (strlen (subArea) + 1)) == NULL)
			{
				free (subAreaInfo);
				return 0;
			}
			strcpy (tempSubArea, subArea);
			subAreaInfo -> areaName = tempSubArea;
			subAreaInfo -> cityList = queueCreate();
			subAreaInfo -> subAreaList = NULL;
			subAreaCount ++;
		
			queuePutSort (areaInfo -> subAreaList, subAreaInfo, compareArea);
		}
	}
	
	if ((tempCity = (char *)malloc (strlen (city) + 1)) == NULL)
	{
		return 0;
	}
	strcpy (tempCity, city);
	cityCount ++;
	
	if (subAreaInfo)
	{
		queuePutSort (subAreaInfo -> cityList, tempCity, compareCity);
	}
	else
	{
		queuePutSort (areaInfo -> cityList, tempCity, compareCity);
	}
	return 1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  B U I L D  A R E A  I N F O                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Build the area info.
 *  \param areaInfoList List to use.
 *  \result None.
 */
int buildAreaInfo (void *areaInfoList)
{
	AREAINFO *areaInfo, *subAreaInfo;
	int ac = 0, zoneSize, menuSize;
	char tempBuff[80], *cityName;
	MENU_DESC *menuDesc, *menuSubDesc = NULL, *menuCityDesc = NULL;
	TZ_INFO *timeZone;

	zoneSize = areaCount + subAreaCount + cityCount + 40;
	if ((timeZones = (TZ_INFO *)malloc (sizeof (TZ_INFO) * zoneSize)) == NULL)
	{
		return 0;
	}
	memset (timeZones, 0, sizeof (TZ_INFO) * zoneSize);
	timeZone = timeZones;
	
	if ((timeZoneMenu = (MENU_DESC *)malloc (sizeof (MENU_DESC) * (areaCount + 4))) == NULL)
	{
		free (timeZoneMenu);
		return 0;
	}
	memset (timeZoneMenu, 0, sizeof (MENU_DESC) * (areaCount + 4));
	menuDesc = timeZoneMenu;

	/*------------------------------------------------------------------------------------------------*
	 * Localtime menu                                                                                 *
	 *------------------------------------------------------------------------------------------------*/
	strcpy (tempBuff, "Local/Time");
	if ((timeZone -> envName = (char *)malloc (strlen (tempBuff) + 1)) != NULL)
	{
		menuDesc -> menuName = timeNames[ac];
		menuDesc -> funcCallBack = setTimeZoneCallback;
		menuDesc -> param = 0;
		++menuDesc;

		menuDesc -> menuName = timeNames[ac + 1];
		menuDesc -> subMenuDesc = menuSubDesc = (MENU_DESC *)malloc (sizeof (MENU_DESC) * FIRST_CITY);
		memset (menuSubDesc, 0, sizeof (MENU_DESC) * FIRST_CITY);
		menuDesc -> param = 0;
		++menuDesc;

		menuDesc -> menuName = timeNames[ac + 2];
		++menuDesc;
		
		strcpy (timeZone -> envName, tempBuff);
		timeZone -> value = ac++;
		++timeZone;
	}

	/*------------------------------------------------------------------------------------------------*
	 * Other GMT options                                                                              *
	 *------------------------------------------------------------------------------------------------*/
	for (;ac < FIRST_CITY; ac++)
	{
		if (ac == GMT_ZERO)
		{
			strcpy (tempBuff, "GMT/GMT");
			strcpy (timeNames[ac + 2], "GMT");
		}
		else
		{
			sprintf (tempBuff, "GMT/GMT %c %d", ac < GMT_ZERO ? '-' : '+', abs(ac - GMT_ZERO));
			sprintf (timeNames[ac + 2], "GMT %c %d", ac < GMT_ZERO ? '-' : '+', abs(ac - GMT_ZERO));
		}
		if ((timeZone -> envName = (char *)malloc (strlen (tempBuff) + 1)) != NULL)
		{
			menuSubDesc -> menuName = timeNames[ac + 2];
			menuSubDesc -> funcCallBack = setTimeZoneCallback;
			menuSubDesc -> param = ac;
			strcpy (timeZone -> envName, tempBuff);
			timeZone -> value = ac;
			++menuSubDesc;
			++timeZone;
		}
	}

	/*------------------------------------------------------------------------------------------------*
	 *                                                                                                *
	 *------------------------------------------------------------------------------------------------*/
	while ((areaInfo = (AREAINFO *)queueGet (areaInfoList)) != NULL)
	{
		int swapped = 0, i;
		menuSize = queueGetItemCount(areaInfo -> subAreaList) + queueGetItemCount(areaInfo -> cityList) + 2;
				
		for (i = 0; areaSwap[i]; i += 2)
		{
			if (strcmp (areaInfo -> areaName, areaSwap[i]) == 0)
			{
				menuDesc -> menuName = tidyName (areaSwap[i + 1]);
				swapped = 1;
				break;
			}
		}
		if (!swapped)
		{
			menuDesc -> menuName = tidyName (areaInfo -> areaName);
		}
		
		menuDesc -> subMenuDesc = menuSubDesc = (MENU_DESC *)malloc (menuSize * sizeof (MENU_DESC));
		memset (menuSubDesc, 0, menuSize * sizeof (MENU_DESC));
		menuDesc -> param = 0;
		++menuDesc;
		
		while ((subAreaInfo = (AREAINFO *)queueGet (areaInfo -> subAreaList)) != NULL)
		{
			menuSize = queueGetItemCount(subAreaInfo -> cityList) + 2;
			menuSubDesc -> menuName = tidyName (subAreaInfo -> areaName);
			menuSubDesc -> subMenuDesc = menuCityDesc = (MENU_DESC *)malloc (menuSize * sizeof (MENU_DESC));
			memset (menuCityDesc, 0, menuSize * sizeof (MENU_DESC));
			menuSubDesc -> param = 0;
			++menuSubDesc;
			
			while ((cityName = (char *)queueGet (subAreaInfo -> cityList)) != NULL)
			{
				sprintf (tempBuff, "%s/%s/%s", areaInfo -> areaName, subAreaInfo -> areaName, cityName);
				if ((timeZone -> envName = malloc (strlen (tempBuff) + 1)) != NULL)
				{
					menuCityDesc -> menuName = tidyName (cityName);
					menuCityDesc -> funcCallBack = setTimeZoneCallback;
					menuCityDesc -> param = ac;
					strcpy (timeZone -> envName, tempBuff);
					timeZone -> value = ac++;
					++menuCityDesc;
					++timeZone;
				}
				free (cityName);
			}
			free (subAreaInfo -> areaName);
			free (subAreaInfo);
		}
		
		while ((cityName = (char *)queueGet (areaInfo -> cityList)) != NULL)
		{
			sprintf (tempBuff, "%s/%s", areaInfo -> areaName, cityName);
			if ((timeZone -> envName = malloc (strlen (tempBuff) + 1)) != NULL)
			{
				menuSubDesc -> menuName = tidyName (cityName);
				menuSubDesc -> funcCallBack = setTimeZoneCallback;
				menuSubDesc -> param = ac;	
				strcpy (timeZone -> envName, tempBuff);
				timeZone -> value = ac++;
				++menuSubDesc;
				++timeZone;
			}
			free (cityName);
		}
		free (areaInfo -> areaName);
		free (areaInfo);
	}
	nTimeZones = (timeZone - timeZones);
	return nTimeZones;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P A R S E  Z O N E                                                                                                *
 *  ==================                                                                                                *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Parse the zone file.
 *  \result None.
 */
int parseZone (void)
{
	FILE *inFile;
	int retn = 0;
	char inBuffer[161], area[41], subArea[41], city[41];
	void *areaInfoList;
	
	areaInfoList = queueCreate ();
	if ((inFile = fopen ("/usr/share/zoneinfo/zone.tab", "r")) == NULL)
	{
		inFile = fopen ("/usr/share/lib/zoneinfo/tab/zone_sun.tab", "r");
	}
	if (inFile != NULL)
	{
		while (fgets (inBuffer, 160, inFile))
		{
			if (inBuffer[0] == '#' || inBuffer[0] <= ' ')
				continue;
			
			if (getAreaAndCity (inBuffer, area, subArea, city))
			{
				addAreaAndCity (areaInfoList, area, subArea, city);
			}
		}
		fclose (inFile);
	}
	retn = buildAreaInfo (areaInfoList);
	queueDelete (areaInfoList);	
	return retn;
}

