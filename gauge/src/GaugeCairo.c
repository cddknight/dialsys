/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  C A I R O . C                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File GaugeCairo.c part of Gauge is free software: you can redistribute it and/or modify it under the terms of     *
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
 *  \brief Display the gauge using cairo.
 */
#include "config.h"
#include "GaugeDisp.h"

extern int weHaveFocus;
extern int currentFace;
extern int toolTipFace;
extern HAND_STYLE handStyle[];
extern FACE_SETTINGS *faceSettings[];
extern DIAL_CONFIG dialConfig;

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E M O V E  E X T R A                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Remove extra zeros on the end of floating numbers.
 *  \param tempBuff String containing the number.
 *  \result Pointer to the number.
 */
char *removeExtra (char *tempBuff)
{
	int i = strlen (tempBuff);

	while (i)
	{
		--i;
		if (tempBuff[i] == '0')
			tempBuff[i] = 0;
		else if (tempBuff[i] == '.')
		{
			tempBuff[i] = 0;
			break;
		}
		else
			break;
	}
	return tempBuff;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D R A W  F A C E                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Draw the gauge face.
 *  \param cr Cairo handle.
 *  \param face Which face to draw.
 *  \param posX Corner position X.
 *  \param posY Corner position Y.
 *  \param circ Should background be a circle or a square.
 *  \result None.
 */
gboolean
drawFace (cairo_t *cr, int face, int posX, int posY, char circ)
{
	int i, maxVal, minVal, col;
	FACE_SETTINGS *faceSetting = faceSettings[face];

	/*------------------------------------------------------------------------------------------------*
     * Draw the face, it is made up of 3 overlapping circles                                          *
     *------------------------------------------------------------------------------------------------*/
	dialDrawStart (cr, posX, posY);

	col = (weHaveFocus && face == currentFace) ? FACE3_COLOUR : FACE4_COLOUR;
	if (circ)
	{
		dialCircleGradient (64, col, 1);
/*      dialDrawCircle (64, col, -1); */
	}
	else
	{
		dialSquareGradient (64, col, 1);
/*      dialDrawSquare (64, col, -1); */
	}

	dialCircleGradient (62, FACE2_COLOUR, 0);
	dialCircleGradient (58, FACE1_COLOUR, 1);

/*  dialDrawCircle (62, FACE3_COLOUR, -1); */
/*  dialDrawCircle (60, FACE4_COLOUR, -1); */

	/*------------------------------------------------------------------------------------------------*
     * Draw the hot and cold markers                                                                  *
     *------------------------------------------------------------------------------------------------*/
	if (faceSetting -> faceFlags & FACE_SHOWHOT)
	{
		col = (faceSetting -> faceFlags & FACE_HC_REVS) ? COLD__COLOUR : HOT___COLOUR;
		dialHotCold (54, col, 0);
	}
	if (faceSetting -> faceFlags & FACE_SHOWCOLD)
	{
		col = (faceSetting -> faceFlags & FACE_HC_REVS) ? HOT___COLOUR : COLD__COLOUR;
		dialHotCold (54, col, 1);
	}

	/*------------------------------------------------------------------------------------------------*
     * Add the text, ether the date or the timezone, plus an AM/PM indicator                          *
     *------------------------------------------------------------------------------------------------*/
	if (faceSetting -> text[FACESTR_TOP])
		dialDrawText (0, faceSetting -> text[FACESTR_TOP], TEXT__COLOUR);
	if (faceSetting -> text[FACESTR_BOT])
		dialDrawText (1, faceSetting -> text[FACESTR_BOT], TEXT__COLOUR);

	/*------------------------------------------------------------------------------------------------*
     * Draw the hour markers                                                                          *
     *------------------------------------------------------------------------------------------------*/
	for (i = 0; i <= 10 ; ++i)
	{
		char tempBuff[15];
		int markAngle = i * 90;
		float scale = ((faceSetting -> faceScaleMax - faceSetting -> faceScaleMin) * i) / 10;

		sprintf (tempBuff, "%0.3f", scale + faceSetting -> faceScaleMin);
		dialDrawMark (markAngle, 29, QMARK_COLOUR, QMARK_COLOUR, removeExtra (tempBuff));
		dialDrawMinute (29, 1, markAngle, HMARK_COLOUR);
	}

	/*------------------------------------------------------------------------------------------------*
     * Draw the hands                                                                                 *
     *------------------------------------------------------------------------------------------------*/
	maxVal = faceSetting -> savedMaxMin.shownMaxValue;
	minVal = faceSetting -> savedMaxMin.shownMinValue;
	if (minVal != -1)
		dialDrawHand (minVal, &handStyle[(faceSetting -> faceFlags & FACE_HC_REVS) ? HAND_MAX : HAND_MIN]);

	if (maxVal != -1)
		dialDrawHand (maxVal, &handStyle[(faceSetting -> faceFlags & FACE_HC_REVS) ? HAND_MIN : HAND_MAX]);

	if (faceSetting -> shownSecondValue != DONT_SHOW)
		dialDrawHand (faceSetting -> shownSecondValue, &handStyle[HAND_SECOND]);

	if (faceSetting -> shownFirstValue != DONT_SHOW)
		dialDrawHand (faceSetting -> shownFirstValue, &handStyle[HAND_FIRST]);

	dialDrawCircle (4, CFILL_COLOUR, CIRC__COLOUR);
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
 *  \brief Draw all the clock faces.
 *  \param cr Cairo handle.
 *  \result None.
 */
void clockExpose (cairo_t *cr)
{
	int i, j, face = 0;

	for (j = 0; j < dialConfig.dialHeight; j++)
	{
		for (i = 0; i < dialConfig.dialWidth; i++)
		{
			drawFace (cr, face, (i * dialConfig.dialSize), (j * dialConfig.dialSize), 0);
			if (face == currentFace)
			{
				if (faceSettings[face] -> text[FACESTR_WIN])
					gtk_window_set_title (GTK_WINDOW (dialConfig.mainWindow), faceSettings[face] -> text[FACESTR_WIN]);
			}
			if (face == toolTipFace)
			{
				if (faceSettings[face] -> text[FACESTR_TIP])
				{
					gtk_widget_set_tooltip_markup (GTK_WIDGET (dialConfig.mainWindow), faceSettings[face] -> text[FACESTR_TIP]);
				}
			}
			++face;
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

	surface = cairo_svg_surface_create (fileName, dialConfig.dialWidth * dialConfig.dialSize, dialConfig.dialHeight * dialConfig.dialSize);
	cr = cairo_create(surface);

	for (j = 0; j < dialConfig.dialHeight; j++)
	{
		for (i = 0; i < dialConfig.dialWidth; i++)
		{
			drawFace (cr, face++, (i * dialConfig.dialSize), (j * dialConfig.dialSize), 1);
		}
	}

	cairo_surface_destroy(surface);
	cairo_destroy(cr);
}

