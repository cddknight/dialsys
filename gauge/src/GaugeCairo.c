/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  C A I R O . C                                                                                          *
 *  ========================                                                                                          *
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
 *  @brief Display the gauge using cairo.
 *  @version $Id: GaugeCairo.c 1813 2013-12-20 11:39:09Z ukchkn $
 */
#include "config.h"
#include "GaugeDisp.h"

extern GtkWidget *mainWindow;
extern int faceSize;
extern int faceWidth;
extern int faceHeight;
extern int weHaveFocus;
extern int currentFace;
extern int toolTipFace;
extern HAND_STYLE handStyle[];
extern FACE_SETTINGS *faceSettings[];
#if GTK_MAJOR_VERSION == 2
extern GtkWidget *drawingArea;
#endif

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
//		dialDrawCircle (64, col, -1);
	}
	else
	{
		dialSquareGradient (64, col, 1);
//		dialDrawSquare (64, col, -1);
	}

	dialCircleGradient (62, FACE2_COLOUR, 0);
	dialCircleGradient (58, FACE1_COLOUR, 1);

//	dialDrawCircle (62, FACE3_COLOUR, -1);
//	dialDrawCircle (60, FACE4_COLOUR, -1);

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
		dialDrawMark (markAngle, 29, QFILL_COLOUR, QMARK_COLOUR, removeExtra (tempBuff));
		dialDrawMinute (29, 1, markAngle, QMARK_COLOUR);
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

#if GTK_MAJOR_VERSION == 2

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C L O C K  E X P O S E                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Call when the gauge needs to be drawn.
 *  \param widget .
 *  \result None.
 */
void clockExpose (GtkWidget *widget)
{
	int i, j, face = 0;
	cairo_t *cr = gdk_cairo_create (drawingArea -> window);

	for (j = 0; j < faceHeight; j++)
	{
		for (i = 0; i < faceWidth; i++)
		{
			drawFace (cr, face, (i * faceSize), (j * faceSize), 0);
			if (face == currentFace)
			{
				if (faceSettings[face] -> text[FACESTR_WIN])
					gtk_window_set_title (GTK_WINDOW (mainWindow), faceSettings[face] -> text[FACESTR_WIN]);
			}
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION > 11)
			if (face == toolTipFace)
			{
				if (faceSettings[face] -> text[FACESTR_TIP])
				{
					gtk_widget_set_tooltip_markup (GTK_WIDGET (mainWindow), faceSettings[face] -> text[FACESTR_TIP]);
				}
			}
#endif
			++face;			
		}
	}

	/*------------------------------------------------------------------------------------------------*
	 * Reset the color and stuff back to the default.                                                 *
	 *------------------------------------------------------------------------------------------------*/
	cairo_destroy (cr);
	cr = NULL;	
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C L O C K  E X P O S E                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief .
 *  \param cr .
 *  \result .
 */
#else

void clockExpose (cairo_t *cr)
{
	int i, j, face = 0;

	for (j = 0; j < faceHeight; j++)
	{
		for (i = 0; i < faceWidth; i++)
		{
			drawFace (cr, face, (i * faceSize), (j * faceSize), 0);
			if (face == currentFace)
			{
				if (faceSettings[face] -> text[FACESTR_WIN])
					gtk_window_set_title (GTK_WINDOW (mainWindow), faceSettings[face] -> text[FACESTR_WIN]);
			}
			if (face == toolTipFace)
			{
				if (faceSettings[face] -> text[FACESTR_TIP])
				{
					gtk_widget_set_tooltip_markup (GTK_WIDGET (mainWindow), faceSettings[face] -> text[FACESTR_TIP]);
				}
			}
			++face;			
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
 *  \brief Save the display to a file.
 *  \param fileName Name of the file to save the SVG in.
 *  \result None.
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

