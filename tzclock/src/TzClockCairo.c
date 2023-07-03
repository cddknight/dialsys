/**********************************************************************************************************************
 *                                                                                                                    *
 *  T Z  C L O C K  C A I R O . C                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File TzClockCairo.c part of TzClock is free software: you can redistribute it and/or modify it under the terms    *
 *  of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License,  *
 *  or (at your option) any later version.                                                                            *
 *                                                                                                                    *
 *  TzClock is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied     *
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more     *
 *  details.                                                                                                          *
 *                                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program. If not, see:           *
 *  <http://www.gnu.org/licenses/>                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \file
 *  \brief Clock drawing functions.
 */
#include "config.h"
#include "TzClockDisp.h"
#include "TimeZone.h"

extern CLOCK_INST clockInst;
extern time_t forceTime;
extern HAND_STYLE handStyle[];
extern TZ_INFO *timeZones;

/*------------------------------------------------------------------------------------------------*
 * Change the font size as the window gets bigger.                                                *
 *------------------------------------------------------------------------------------------------*/
char *roman[25] =
{
	"-", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII",
	"XIII", "XIV", "XV", "XVI", "XVII", "XVIII", "XIX", "XX", "XXI", "XXII", "XXIII",
	"XXIV"
};

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D R A W  F A C E                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Draw the clock face.
 *  \param cr Cairo handle.
 *  \param face Which face.
 *  \param posX Face location X.
 *  \param posY Face location Y.
 *  \param circ Diameter.
 *  \result none.
 */
gboolean
drawFace (cairo_t *cr, int face, int posX, int posY, char circ)
{
	FACE_SETTINGS *faceSetting = clockInst.faceSettings[face];
	int i, j, col, timeZone = faceSetting -> currentTZ;
	int centerX = posX + (clockInst.dialConfig.dialSize >> 1), centerY = posY + (clockInst.dialConfig.dialSize >> 1);
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

	col = (clockInst.weHaveFocus && face == clockInst.currentFace) ? FACE3_COLOUR : FACE4_COLOUR;
	if (circ)
	{
		dialCircleGradient (64, col, 1);
	}
	else
	{
		dialSquareGradient (64, col, 1);
	}
	dialCircleGradient (62, FACE2_COLOUR, 0);
	dialCircleGradient (58, FACE1_COLOUR, 1);

	/*------------------------------------------------------------------------------------------------*
     * Add the text, ether the date or the timezone, plus an AM/PM indicator                          *
     *------------------------------------------------------------------------------------------------*/
	showSubSec = (faceSetting -> showTime && faceSetting -> showSeconds && (faceSetting -> stopwatch || faceSetting -> timer ||
			faceSetting -> subSecond)) ? 1 : 0;

	getStringValue (tempString, 100, faceSetting -> stopwatch || faceSetting -> timer ?
			(timeZone ? TXT_TOPSW_Z : TXT_TOPSW_L) : (timeZone ? TXT_TOP_Z : TXT_TOP_L), face, t);
	dialDrawText (0, tempString, TEXT__COLOUR);

	if (!showSubSec)
	{
		getStringValue (tempString, 100, faceSetting -> stopwatch || faceSetting -> timer ?
			(timeZone ? TXT_BOTTOMSW_Z : TXT_BOTTOMSW_L) : timeZone ? TXT_BOTTOM_Z : TXT_BOTTOM_L, face, t);
		dialDrawText (1, tempString, TEXT__COLOUR);
	}

	/*------------------------------------------------------------------------------------------------*
     * Calculate which markers to draw                                                                *
     *------------------------------------------------------------------------------------------------*/
	if (clockInst.dialConfig.markerType > 2)
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
		if (faceSetting -> stopwatch || faceSetting -> timer)
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

		if (clockInst.dialConfig.dialSize > 256)
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
				int hour = (i == 0 ? (faceSetting -> show24Hour ? 24 : 12) : i / 5);
				if (clockInst.dialConfig.markerType == 3)
					sprintf (buff, "%d", hour);
				if (clockInst.dialConfig.markerType == 4)
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
		dialCircleGradientX (centerX, posY + ((3 * clockInst.dialConfig.dialSize) >> 2), 21, FACE2_COLOUR, 1);
		dialDrawCircleX (centerX, posY + ((3 * clockInst.dialConfig.dialSize) >> 2), 19, FACE5_COLOUR, -1);
	}
	if (faceSetting -> stopwatch || faceSetting -> timer)
	{
		dialCircleGradientX (posX + (clockInst.dialConfig.dialSize >> 2), centerY, 21, FACE2_COLOUR, 1);
		dialDrawCircleX (posX + (clockInst.dialConfig.dialSize >> 2), centerY, 19, FACE5_COLOUR, -1);
		dialCircleGradientX (posX + (3 * clockInst.dialConfig.dialSize >> 2), centerY, 21, FACE2_COLOUR, 1);
		dialDrawCircleX (posX + (3 * clockInst.dialConfig.dialSize >> 2), centerY, 19, FACE5_COLOUR, -1);
	}

	if (showSubSec || faceSetting -> stopwatch || faceSetting -> timer)
	{
		for (i = 0; i < 60 ; i++)
		{
			int m = i * 20;

			if (showSubSec)
			{
				if (!(i % 5))
					dialDrawMinuteX (centerX, posY + ((3 * clockInst.dialConfig.dialSize) >> 2),
							(i % 15) ? 9 : 8, (i % 15) ? 1 : 2, m, WMARK_COLOUR);
			}
			if (faceSetting -> stopwatch)
			{
				if (!(i % 6))
					dialDrawMinuteX (posX + (clockInst.dialConfig.dialSize >> 2), centerY,
							(i % 12) ? 9 : 8, (i % 12) ? 1 : 2, m, WMARK_COLOUR);
				if (!(i % 2))
					dialDrawMinuteX (posX + ((3 * clockInst.dialConfig.dialSize) >> 2), centerY,
							(i % 10) ? 9 : 8, (i % 10) ? 1 : 2, m, WMARK_COLOUR);
			}
			else if (faceSetting -> timer)
			{
				if (!(i % 5))
					dialDrawMinuteX (posX + (clockInst.dialConfig.dialSize >> 2), centerY,
							(i % 10) ? 9 : 8, (i % 10) ? 1 : 2, m, WMARK_COLOUR);
				if (!(i % 2))
					dialDrawMinuteX (posX + ((3 * clockInst.dialConfig.dialSize) >> 2), centerY,
							(i % 10) ? 9 : 8, (i % 10) ? 1 : 2, m, WMARK_COLOUR);
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
		dialDrawHandX (centerX, posY + ((3 * clockInst.dialConfig.dialSize) >> 2), faceSetting -> handPosition[HAND_SECS], &handStyle[HAND_SUBS]);
		dialDrawCircleX (centerX, posY + ((3 * clockInst.dialConfig.dialSize) >> 2), 2, SFILL_COLOUR, SEC___COLOUR);
	}
	if (faceSetting -> stopwatch || faceSetting -> timer)
	{
		dialDrawHandX (posX + (clockInst.dialConfig.dialSize >> 2), centerY, faceSetting -> handPosition[HAND_STOPWT], &handStyle[HAND_STOPWT]);
		dialDrawHandX (posX + ((3 * clockInst.dialConfig.dialSize) >> 2), centerY, faceSetting -> handPosition[HAND_STOPWM], &handStyle[HAND_STOPWM]);
		dialDrawCircleX (posX + (clockInst.dialConfig.dialSize >> 2), centerY, 2, WFILL_COLOUR, WATCH_COLOUR);
		dialDrawCircleX (posX + ((3 * clockInst.dialConfig.dialSize) >> 2), centerY, 2, WFILL_COLOUR, WATCH_COLOUR);
	}
	if (faceSetting -> showTime)
	{
		dialDrawHand (faceSetting -> handPosition[HAND_HOUR], &handStyle[HAND_HOUR]);
		dialDrawHand (faceSetting -> handPosition[HAND_MINUTE], &handStyle[HAND_MINUTE]);
	}
	if ((faceSetting -> showSeconds && faceSetting -> showTime) || faceSetting -> stopwatch || faceSetting -> timer)
	{
		if (faceSetting -> stopwatch || faceSetting -> timer)
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

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C L O C K  E X P O S E                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Call when the gauge needs to be drawn.
 *  \param cr Cairo handle.
 *  \result None.
 */
void clockExpose (cairo_t *cr)
{
	int i, j, face = 0;

	for (j = 0; j < clockInst.dialConfig.dialHeight; j++)
	{
		for (i = 0; i < clockInst.dialConfig.dialWidth; i++)
		{
			drawFace (cr, face++, (i * clockInst.dialConfig.dialSize), (j * clockInst.dialConfig.dialSize), 0);
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I A L  S A V E                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Save the display to a file.
 *  \param fileName Name of the file to save the SVG in.
 *  \result None.
 */
void dialSave(char *fileName)
{
	cairo_surface_t *surface;
	int i, j, face = 0;
	cairo_t *cr;

	surface = cairo_svg_surface_create (fileName, clockInst.dialConfig.dialWidth * clockInst.dialConfig.dialSize, clockInst.dialConfig.dialHeight * clockInst.dialConfig.dialSize);
	cr = cairo_create(surface);

	for (j = 0; j < clockInst.dialConfig.dialHeight; j++)
	{
		for (i = 0; i < clockInst.dialConfig.dialWidth; i++)
		{
			drawFace (cr, face++, (i * clockInst.dialConfig.dialSize), (j * clockInst.dialConfig.dialSize), 1);
		}
	}

	cairo_surface_destroy(surface);
	cairo_destroy(cr);
}

