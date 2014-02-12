/*----------------------------------------------------------------------------------------------------*
 *                                                                                                    *
 *  T z C L O C K C A I R O. C                                                                        *
 *  ==========================                                                                        *
 *                                                                                                    *
 *  TzClock developed by Chris Knight based on glock by Eric L. Smith.                                * 
 *                                                                                                    *
 *----------------------------------------------------------------------------------------------------*
 *                                                                                                    *
 *  TzClock is free software; you can redistribute it and/or modify it under the terms of the GNU     *
 *  General Public License version 2 as published by the Free Software Foundation.  Note that I       *
 *  am not granting permission to redistribute or modify TzClock under the terms of any later         *
 *  version of the General Public License.                                                            *
 *                                                                                                    *
 *  TzClock is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without      *
 *  even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *  GNU General Public License for more details.                                                      *
 *                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program (in     * 
 *  the file "COPYING"); if not, write to the Free Software Foundation, Inc.,                         *
 *  59 Temple Place - Suite 330, Boston, MA 02111, USA.                                               *
 *                                                                                                    *
 *----------------------------------------------------------------------------------------------------*/
/**
 *  @file
 *  @brief .
 *  @version $Id: TzClockCairo.c 1813 2013-12-20 11:39:09Z ukchkn $
 */
 
#include "config.h"
#include "TzClockDisp.h"
#include "TimeZone.h"

extern int faceSize;
extern int faceWidth;
extern int faceHeight;
extern int weHaveFocus;
extern int currentFace;
extern int timeSetting;
extern bool fastSetting;
extern bool showBounceSec;
extern int markerType;
extern time_t forceTime;
extern HAND_STYLE handStyle[];
extern FACE_SETTINGS *faceSettings[];
extern TZ_INFO *timeZones;
#if GTK_MAJOR_VERSION == 2
extern GtkWidget *drawingArea;
#endif

/*------------------------------------------------------------------------------------------------*
 * Change the font size as the window gets bigger.                                                *
 *------------------------------------------------------------------------------------------------*/
char *roman[25] = 
{
	"-", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII",
	"XIII", "XIV", "XV", "XVI", "XVII", "XVIII", "XIX", "XX", "XXI", "XXII", "XXIII",
	"XXIV"
};

/*----------------------------------------------------------------------------------------------------*
 *                                                                                                    *
 *  D R A W  F A C E                                                                                  *
 *  ================                                                                                  *
 *                                                                                                    *
 *----------------------------------------------------------------------------------------------------*/
/**
 *  @brief .
 *  @param cr .
 *  @param face .
 *  @param posX .
 *  @param posY .
 *  @param t .
 *  @result .
 */
gboolean 
drawFace (cairo_t *cr, int face, int posX, int posY, char circ)
{
	FACE_SETTINGS *faceSetting = faceSettings[face];
	int i, j, col, timeZone = faceSetting -> currentTZ;
	int centerX = posX + (faceSize >> 1), centerY = posY + (faceSize >> 1);
	int showSubSec, markerFlags = 0xFFFFFF;
	time_t t = faceSetting -> timeShown;
	char tempString[101];
	struct tm tm;

	/*------------------------------------------------------------------------------------------------*
	 * The alarm check has moved out of here                                                          *
	 *------------------------------------------------------------------------------------------------*/	
	getTheFaceTime (faceSetting, &t, &tm);
	
	/*------------------------------------------------------------------------------------------------*
	 * Draw the face, it is made up of 3 overlapping circles                                          *
	 *------------------------------------------------------------------------------------------------*/	
	dialDrawStart (cr, posX, posY);

	col = (weHaveFocus && face == currentFace) ? FACE1_COLOUR : FACE2_COLOUR;
	if (circ)
		dialDrawCircle (64, col, -1);
	else
		dialDrawSquare (64, col, -1);

	drawCircleGradient (62, FACE3_COLOUR, 0);
	drawCircleGradient (58,FACE4_COLOUR, 1);

//	dialDrawCircle (62, FACE3_COLOUR, -1);
//	dialDrawCircle (60, FACE4_COLOUR, -1);
	
	/*------------------------------------------------------------------------------------------------*
	 * Add the text, ether the date or the timezone, plus an AM/PM indicator                          *
	 *------------------------------------------------------------------------------------------------*/
	showSubSec = (faceSetting -> showSeconds && (faceSetting -> stopwatch || faceSetting -> subSecond)) ? 1 : 0;

	getStringValue (tempString, 100, faceSetting -> stopwatch ?
			(timeZone ? TXT_TOPSW_Z : TXT_TOPSW_L) : (timeZone ? TXT_TOP_Z : TXT_TOP_L), face, t);
	dialDrawText (0, tempString, TEXT__COLOUR);
	 
	if (!showSubSec)
	{
		getStringValue (tempString, 100, timeZone ? TXT_BOTTOM_Z : TXT_BOTTOM_L, face, t);	
		dialDrawText (1, tempString, TEXT__COLOUR);
	}

	/*------------------------------------------------------------------------------------------------*
	 * Calculate which markers to draw                                                                *
	 *------------------------------------------------------------------------------------------------*/
	if (markerType > 2)
	{
		if (showSubSec)
		{
			if (faceSetting -> show24Hour)
			{
				markerFlags &= ~(1 << 11);
				markerFlags &= ~(1 << 12);
				markerFlags &= ~(1 << 13);
			}
			else
				markerFlags &= ~(1 << 6);
		}
		if (faceSetting -> stopwatch)
		{
			if (faceSetting -> show24Hour)
			{
				markerFlags &= ~(1 << 5);
				markerFlags &= ~(1 << 6);
				markerFlags &= ~(1 << 7);
				markerFlags &= ~(1 << 17);
				markerFlags &= ~(1 << 18);
				markerFlags &= ~(1 << 19);
			}
			else
			{
				markerFlags &= ~(1 << 3);
				markerFlags &= ~(1 << 9);
			}
		}		
	}

	/*------------------------------------------------------------------------------------------------*
	 * Draw the hour markers                                                                          *
	 *------------------------------------------------------------------------------------------------*/
	j = faceSetting -> show24Hour ? 120 : 60;

	for (i = 0; i < j ; i++)
	{
		int m = faceSetting -> show24Hour ? i * 10 : i * 20;

		if (faceSize > 256)
		{
			if (!faceSetting -> show24Hour || !(i % 2))
				dialDrawMinute (30, 1, m, MMARK_COLOUR);
		}
		if (!(i % 5))
		{
			dialDrawMinute (29, 1, m, HMARK_COLOUR);
			if (markerFlags & (1 << (i / 5)))
			{
				char buff[11] = "";
				int hour = i == 0 ? (faceSetting -> show24Hour ? 24 : 12) : i / 5;
				if (markerType == 3)
					sprintf (buff, "%d", hour);
				if (markerType == 4)
					strcpy (buff, roman[hour]);
				dialDrawMark (m, 31, QFILL_COLOUR, QMARK_COLOUR, buff);
			}
		}
	}

	/*------------------------------------------------------------------------------------------------*
	 * Draw other clock faces                                                                         *
	 *------------------------------------------------------------------------------------------------*/
	if (showSubSec)
	{
		dialDrawCircleX (centerX, posY + ((3 * faceSize) >> 2), 21, FACE5_COLOUR, -1);
	}
	if (faceSetting -> stopwatch)
	{
		dialDrawCircleX (posX + (faceSize >> 2), centerY, 21, FACE5_COLOUR, -1);			
		dialDrawCircleX (posX + (3 * faceSize >> 2), centerY, 21, FACE5_COLOUR, -1);
	}	
	
	if (showSubSec || faceSetting -> stopwatch)
	{
		for (i = 0; i < 60 ; i++)
		{
			int m = i * 20;

			if (showSubSec)
			{
				if (!(i % 5))
					dialDrawMinuteX (centerX, posY + ((3 * faceSize) >> 2),
							(i % 15) ? 9 : 8, (i % 15) ? 1 : 2, m, WMARK_COLOUR);
			}
			if (faceSetting -> stopwatch)
			{
				if (!(i % 6))
					dialDrawMinuteX (posX + (faceSize >> 2), centerY,
							(i % 12) ? 9 : 8, (i % 12) ? 1 : 2, m, WMARK_COLOUR);
				if (!(i % 4))
					dialDrawMinuteX (posX + ((3 * faceSize) >> 2), centerY,
							(i % 20) ? 9 : 8, (i % 20) ? 1 : 2, m, WMARK_COLOUR);
			}
		}
	}
	
	/*------------------------------------------------------------------------------------------------*
	 * Draw the hands                                                                                 *
	 *------------------------------------------------------------------------------------------------*/
	if (faceSetting -> alarmInfo.showAlarm)
	{
		dialDrawHand (faceSetting -> handPosition[HAND_ALARM], &handStyle[HAND_ALARM]);
	}
	if (showSubSec)
	{
		dialDrawHandX (centerX, posY + ((3 * faceSize) >> 2), faceSetting -> handPosition[HAND_SECS], &handStyle[HAND_SUBS]);
		dialDrawCircleX (centerX, posY + ((3 * faceSize) >> 2), 2, SFILL_COLOUR, SEC___COLOUR);
	}
	if (faceSetting -> stopwatch)
	{
		dialDrawHandX (posX + (faceSize >> 2), centerY, faceSetting -> handPosition[HAND_STOPWT], &handStyle[HAND_STOPWT]);
		dialDrawHandX (posX + ((3 * faceSize) >> 2), centerY, faceSetting -> handPosition[HAND_STOPWM], &handStyle[HAND_STOPWM]);
		dialDrawCircleX (posX + (faceSize >> 2), centerY, 2, WFILL_COLOUR, WATCH_COLOUR);			
		dialDrawCircleX (posX + ((3 * faceSize) >> 2), centerY, 2, WFILL_COLOUR, WATCH_COLOUR);
	}

	dialDrawHand (faceSetting -> handPosition[HAND_HOUR], &handStyle[HAND_HOUR]);
	dialDrawHand (faceSetting -> handPosition[HAND_MINUTE], &handStyle[HAND_MINUTE]);

	if (faceSetting -> showSeconds || faceSetting -> stopwatch)
	{
		if (faceSetting -> stopwatch)
		{
			dialDrawHandX (centerX, centerY, faceSetting -> handPosition[HAND_STOPWS], &handStyle[HAND_SECS]);
			dialDrawCircleX (centerX, centerY, 4, WFILL_COLOUR, WATCH_COLOUR);
		}
		else if (faceSetting -> subSecond)
		{
			dialDrawCircleX (centerX, centerY, 4, MFILL_COLOUR, MIN___COLOUR);
		}
		else
		{
			dialDrawHand (faceSetting -> handPosition[HAND_SECS], &handStyle[HAND_SECS]);
			dialDrawCircle (4, SFILL_COLOUR, SEC___COLOUR);
		}
	}
	else
	{
		dialDrawCircle (4, MFILL_COLOUR, MIN___COLOUR);
	}
	dialDrawFinish ();
	return TRUE;
}

#if GTK_MAJOR_VERSION == 2

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C L O C K  E X P O S E                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief Call when the gauge needs to be drawn.
 *  @param cr The gauge has been exposed.
 *  @result None.
 */
void clockExpose (GtkWidget *widget)
{
	int i, j, face = 0;
	cairo_t *cr = gdk_cairo_create (drawingArea -> window);

	for (j = 0; j < faceHeight; j++)
	{
		for (i = 0; i < faceWidth; i++)
		{
			drawFace (cr, face++, (i * faceSize), (j * faceSize), 0);
		}
	}

	/*------------------------------------------------------------------------------------------------*
	 * Reset the color and stuff back to the default.                                                 *
	 *------------------------------------------------------------------------------------------------*/
	cairo_destroy (cr);
	cr = NULL;	
}

#else

void clockExpose (cairo_t *cr)
{
	int i, j, face = 0;

	for (j = 0; j < faceHeight; j++)
	{
		for (i = 0; i < faceWidth; i++)
		{
			drawFace (cr, face++, (i * faceSize), (j * faceSize), 0);
		}
	}
}

#endif

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I A L  S A V E                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief Save the display to a file.
 *  @param fileName Name of the file to save the SVG in.
 *  @result None.
 */
int dialSave(char *fileName) 
{
	cairo_surface_t *surface;
	int i, j, face = 0;
	cairo_t *cr;

	surface = cairo_svg_surface_create (fileName, faceWidth * faceSize, faceHeight * faceSize);
	cr = cairo_create(surface);

	for (j = 0; j < faceHeight; j++)
	{
		for (i = 0; i < faceWidth; i++)
		{
			drawFace (cr, face++, (i * faceSize), (j * faceSize), 1);
		}
	}

	cairo_surface_destroy(surface);
	cairo_destroy(cr);

	return 0;
}

