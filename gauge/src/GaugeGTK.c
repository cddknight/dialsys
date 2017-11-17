/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  G T K . C                                                                                              *
 *  ====================                                                                                              *
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
 *  \brief Display using GTK.
 */
#include "config.h"
#include "GaugeDisp.h"

extern GtkWidget *mainWindow;
extern GtkWidget *drawingArea;
extern GdkColor faceColours[];
extern char fontName[];
extern int faceSize;
extern int faceWidth;
extern int faceHeight;
extern int weHaveFocus;
extern int currentFace;
extern int timeSetting;
extern int fastSetting;
extern int showBounceSec;
extern int markerType;
extern int markerStep;
extern time_t forceTime;
extern HAND_STYLE handStyle[];
extern FACE_SETTINGS *faceSettings[];

static PangoLayout *layout;
static PangoFontDescription *fontDesc;
static GdkBitmap *windowShapeBitmap;

void drawText (GtkWidget * widget, GdkGC *gc, int posX, int posY, char *string1, int colour, int scale);
int getFontSize (char *fontName);

/******************************************************************************************************
 * Change the font size as the window gets bigger.                                                    *
 ******************************************************************************************************/
static int fontSizes[16] = 
{
	 6, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64,
};

/**********************************************************************************************************************
 *                                                                                                                    *
 *  M A K E  W I N D O W  M A S K                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Make a mask for the window.
 *  \result None.
 */
void makeWindowMask ()
{
	int fullHeight, fullWidth, i, j;
	GdkGC *gc;
	
	fullHeight = faceHeight * faceSize;
	fullWidth = faceWidth * faceSize;
	
	if (windowShapeBitmap)
	{
		g_object_unref (windowShapeBitmap);
		windowShapeBitmap = NULL;
	}

	windowShapeBitmap = (GdkBitmap *) gdk_pixmap_new (NULL, fullWidth, fullHeight, 1);

	gc = gdk_gc_new (windowShapeBitmap);
	gdk_gc_set_foreground (gc, &faceColours[BLACK_COLOUR]);
	gdk_gc_set_background (gc, &faceColours[WHITE_COLOUR]);

	gdk_draw_rectangle (windowShapeBitmap, gc, TRUE, 0, 0, fullWidth, fullHeight);

	gdk_gc_set_foreground (gc, &faceColours[WHITE_COLOUR]);
	gdk_gc_set_background (gc, &faceColours[BLACK_COLOUR]);

	for (i = 0; i < faceWidth; i++)
	{
		for (j = 0; j < faceHeight; j++)
			gdk_draw_arc (windowShapeBitmap, gc, TRUE, (i * faceSize) - 1, (j * faceSize) - 1, 
					faceSize, faceSize, 0, 360 * 64);
	}
	gtk_widget_shape_combine_mask (mainWindow, windowShapeBitmap, 0, 0);
	
	g_object_unref (gc);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D R A W  M I N U T E                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Draw a minute marker.
 *  \param widget What to draw on.
 *  \param gc GDK context.
 *  \param posX Position X.
 *  \param posY Position Y.
 *  \param size Size.
 *  \param len Length.
 *  \param angle Angle.
 *  \param colour Colour.
 *  \result None.
 */
void drawMinute (GtkWidget * widget, GdkGC *gc, int posX, int posY, int size, int len, int angle,
		int colour)
{
	gdk_gc_set_foreground (gc, &faceColours[colour]);
	
	if (len < size)
	{
		gdk_gc_set_line_attributes (gc, 1, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER);
		gdk_draw_line (widget->window, gc,
				posX + xSinCos ((faceSize * size) / 64, angle, 0),
				posY - xSinCos ((faceSize * size) / 64, angle, 1),
				posX + xSinCos ((faceSize * (size + len)) / 64, angle, 0),
				posY - xSinCos ((faceSize * (size + len)) / 64, angle, 1));
	}
	else
	{
		gdk_gc_set_line_attributes (gc, (faceSize/256) + 1, GDK_LINE_SOLID, GDK_CAP_ROUND, 
				GDK_JOIN_MITER);
		gdk_draw_line (widget->window, gc, 
				posX + xSinCos ((faceSize * size) / 64, angle + 180, 0),
				posY - xSinCos ((faceSize * size) / 64, angle + 180, 1),
				posX + xSinCos ((faceSize * (size + len)) / 64, angle, 0),
				posY - xSinCos ((faceSize * (size + len)) / 64, angle, 1));
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D R A W  C I R C L E                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Drame a circle.
 *  \param widget What to draw on.
 *  \param gc GDK context.
 *  \param posX Position X.
 *  \param posY Position Y.
 *  \param size Size.
 *  \param colFill Fill colour.
 *  \param colOut Outline colour.
 *  \result None.
 */
void drawCircle (GtkWidget * widget, GdkGC *gc, int posX, int posY, int size, int colFill, int colOut)
{
	if (colFill != -1)
	{
		gdk_gc_set_foreground (gc, &faceColours[colFill]);
		gdk_draw_arc (widget->window, gc, TRUE,
				posX - ((faceSize * size) / 128), 
				posY - ((faceSize * size) / 128), 
				(faceSize * size) / 64, 
				(faceSize * size) / 64, 
				0, 64 * 360);
	}
	if (colOut != -1)
	{
		gdk_gc_set_foreground (gc, &faceColours[colOut]);
		gdk_draw_arc (widget->window, gc, FALSE,
				posX - ((faceSize * size) / 128), 
				posY - ((faceSize * size) / 128), 
				(faceSize * size) / 64, 
				(faceSize * size) / 64, 
				0, 64 * 360);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D R A W  H A N D                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Draw a hand.
 *  \param widget What to draw on.
 *  \param gc GDK context.
 *  \param posX Position X.
 *  \param posY Position Y.
 *  \param angle Angle.
 *  \param handStyle The hand style.
 *  \result None.
 */
void drawHand (GtkWidget *widget, GdkGC *gc, int posX, int posY, int angle, HAND_STYLE *handStyle)
{
	GdkPoint points[10];
	int numPoints, fill = 0;
	int size = handStyle -> length, style = handStyle -> style, tail = handStyle -> tail;
	int colFill = handStyle -> fill, colOut = handStyle -> line;

	if (angle < 0) angle = 0;
	if (angle > 600) angle = 600;

	switch (style)
	{
	case 0:
		// Original double triangle
		points[0].x = posX + xSinCos ((faceSize * tail) >> 6, angle + 400, 0);
		points[0].y = posY - xSinCos ((faceSize * tail) >> 6, angle + 400, 1);
		points[1].x = posX + xSinCos (faceSize >> 6, angle + 200, 0);
		points[1].y = posY - xSinCos (faceSize >> 6, angle + 200, 1);
		points[2].x = posX + xSinCos ((faceSize * size) >> 6, angle, 0);
		points[2].y = posY - xSinCos ((faceSize * size) >> 6, angle, 1);
		points[3].x = posX + xSinCos (faceSize >> 6, angle + 600, 0);
		points[3].y = posY - xSinCos (faceSize >> 6, angle + 600, 1); 
		numPoints = 4;
		fill = 1;
		break;

	case 1:
		// Single triangle
		points[0].x = posX + xSinCos ((faceSize * tail) >> 6, angle + 400, 0) 
					+ xSinCos (faceSize >> 6, angle + 200, 0);
		points[0].y = posY - xSinCos ((faceSize * tail) >> 6, angle + 400, 1)
					- xSinCos (faceSize >> 6, angle + 200, 1);
		points[1].x = posX + xSinCos ((faceSize * tail) >> 6, angle + 400, 0) 
					+ xSinCos (faceSize >> 6, angle + 600, 0);
		points[1].y = posY - xSinCos ((faceSize * tail) >> 6, angle + 400, 1)
					- xSinCos (faceSize >> 6, angle + 600, 1);
		points[2].x = posX + xSinCos ((faceSize * size) >> 6, angle, 0);
		points[2].y = posY - xSinCos ((faceSize * size) >> 6, angle, 1);
		numPoints = 3;
		fill = 1;
		break;

	case 2:
		// Rectangle
		points[0].x = posX + xSinCos ((faceSize * tail) >> 6, angle + 400, 0) + xSinCos (faceSize >> 6, angle + 200, 0);
		points[0].y = posY - xSinCos ((faceSize * tail) >> 6, angle + 400, 1) - xSinCos (faceSize >> 6, angle + 200, 1);
		points[1].x = posX + xSinCos ((faceSize * tail) >> 6, angle + 400, 0) + xSinCos (faceSize >> 6, angle + 600, 0);
		points[1].y = posY - xSinCos ((faceSize * tail) >> 6, angle + 400, 1) - xSinCos (faceSize >> 6, angle + 600, 1);
		points[2].x = posX + xSinCos ((faceSize * size) >> 6, angle, 0) + xSinCos (faceSize >> 6, angle + 600, 0);
		points[2].y = posY - xSinCos ((faceSize * size) >> 6, angle, 1) - xSinCos (faceSize >> 6, angle + 600, 1);
		points[3].x = posX + xSinCos ((faceSize * size) >> 6, angle, 0) + xSinCos (faceSize >> 6, angle + 200, 0);
		points[4].y = posY - xSinCos ((faceSize * size) >> 6, angle, 1) - xSinCos (faceSize >> 6, angle + 200, 1);
		numPoints = 4;
		fill = 1;
		break;

	case 3:
		//Rectangle with pointer
		points[0].x = posX + xSinCos ((faceSize * tail) >> 6, angle + 400, 0) + xSinCos (faceSize >> 6, angle + 200, 0);
		points[0].y = posY - xSinCos ((faceSize * tail) >> 6, angle + 400, 1) - xSinCos (faceSize >> 6, angle + 200, 1);
		points[1].x = posX + xSinCos ((faceSize * tail) >> 6, angle + 400, 0) + xSinCos (faceSize >> 6, angle + 600, 0);
		points[1].y = posY - xSinCos ((faceSize * tail) >> 6, angle + 400, 1) - xSinCos (faceSize >> 6, angle + 600, 1);
		points[2].x = posX + xSinCos ((faceSize * (size * 15)) >> 10, angle, 0) + xSinCos (faceSize >> 6, angle + 600, 0);
		points[2].y = posY - xSinCos ((faceSize * (size * 15)) >> 10, angle, 1) - xSinCos (faceSize >> 6, angle + 600, 1);
		points[3].x = posX + xSinCos ((faceSize * size) >> 6, angle, 0);
		points[3].y = posY - xSinCos ((faceSize * size) >> 6, angle, 1);
		points[4].x = posX + xSinCos ((faceSize * (size * 15)) >> 10, angle, 0) + xSinCos (faceSize >> 6, angle + 200, 0);
		points[4].y = posY - xSinCos ((faceSize * (size * 15)) >> 10, angle, 1) - xSinCos (faceSize >> 6, angle + 200, 1);
		numPoints = 5;
		fill = 1;
		break;

	case 4:
		//Rectangle with arrow
		points[0].x = posX + xSinCos ((faceSize * tail) >> 6, angle + 400, 0) + xSinCos (faceSize >> 6, angle + 200, 0);
		points[0].y = posY - xSinCos ((faceSize * tail) >> 6, angle + 400, 1)	- xSinCos (faceSize >> 6, angle + 200, 1);
		points[1].x = posX + xSinCos ((faceSize * tail) >> 6, angle + 400, 0)	+ xSinCos (faceSize >> 6, angle + 600, 0);
		points[1].y = posY - xSinCos ((faceSize * tail) >> 6, angle + 400, 1)	- xSinCos (faceSize >> 6, angle + 600, 1);
		points[2].x = posX + xSinCos ((faceSize * (size * 12)) >> 10, angle, 0) + xSinCos (faceSize >> 6, angle + 600, 0);
		points[2].y = posY - xSinCos ((faceSize * (size * 12)) >> 10, angle, 1) - xSinCos (faceSize >> 6, angle + 600, 1);
		points[3].x = posX + xSinCos ((faceSize * (size * 12)) >> 10, angle, 0) + xSinCos (faceSize / 30, angle + 600, 0);
		points[3].y = posY - xSinCos ((faceSize * (size * 12)) >> 10, angle, 1) - xSinCos (faceSize / 30, angle + 600, 1);
		points[4].x = posX + xSinCos ((faceSize * size) >> 6, angle, 0);
		points[4].y = posY - xSinCos ((faceSize * size) >> 6, angle, 1);
		points[5].x = posX + xSinCos ((faceSize * (size * 12)) >> 10, angle, 0) + xSinCos (faceSize / 30, angle + 200, 0);
		points[5].y = posY - xSinCos ((faceSize * (size * 12)) >> 10, angle, 1) - xSinCos (faceSize / 30, angle + 200, 1);
		points[6].x = posX + xSinCos ((faceSize * (size * 12)) >> 10, angle, 0) + xSinCos (faceSize >> 6, angle + 200, 0);
		points[6].y = posY - xSinCos ((faceSize * (size * 12)) >> 10, angle, 1) - xSinCos (faceSize >> 6, angle + 200, 1);
		numPoints = 7;
		fill = 1;
		break;

	case 9:
	default:
		// Simple line
		points[0].x = posX + xSinCos ((faceSize * tail) >> 6, angle + 400, 0);
		points[0].y = posY - xSinCos ((faceSize * tail) >> 6, angle + 400, 1);
		points[1].x = posX + xSinCos ((faceSize * size) >> 6, angle, 0);
		points[1].y = posY - xSinCos ((faceSize * size) >> 6, angle, 1);
		numPoints = 2;
		break;
	}
	if (!handStyle -> fillIn)
	{
		fill = 0;
	}
	if (fill)
	{
		gdk_gc_set_foreground (gc, &faceColours[colFill]);
		gdk_draw_polygon (widget->window, gc, TRUE, points, numPoints);
	}
	gdk_gc_set_foreground (gc, &faceColours[colOut]);
	gdk_draw_polygon (widget->window, gc, FALSE, points, numPoints);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D R A W  M A R K                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Draw a morker.
 *  \param widget What to draw on.
 *  \param gc GDK context.
 *  \param posX Position X.
 *  \param posY Position Y.
 *  \param angle Angle.
 *  \param size Size.
 *  \param colFill Fill colour.
 *  \param colOut Outline colour.
 *  \param hour Poistion.
 *  \param faceMin Min value .
 *  \param faceMax Max Value.
 *  \result None.
 */
void drawMark (GtkWidget *widget, GdkGC *gc, int posX, int posY, int angle, int size, 
		int colFill, int colOut, int hour, int faceMin, int faceMax)
{
	if (markerStep == 0) markerStep = 1;

	if (!(hour % markerStep))
	{
		switch (markerType)
		{
		case 0:
			/* No markers */
			break;
	
		case 1:
			/* Triangle markers */
			{
				GdkPoint points[3];
	
				points[0].x = posX + xSinCos ((faceSize * size) / 64, angle + 2, 0);
				points[0].y = posY - xSinCos ((faceSize * size) / 64, angle + 2, 1);
				points[1].x = posX + xSinCos ((faceSize * (size - 3)) / 64, angle, 0);
				points[1].y = posY - xSinCos ((faceSize * (size - 3)) / 64, angle, 1);
				points[2].x = posX + xSinCos ((faceSize * size) / 64, angle - 2, 0);
				points[2].y = posY - xSinCos ((faceSize * size) / 64, angle - 2, 1);

				gdk_gc_set_foreground (gc, &faceColours[colFill]);
				gdk_draw_polygon (widget->window, gc, TRUE, points, 3);
				gdk_gc_set_foreground (gc, &faceColours[colOut]);
				gdk_draw_polygon (widget->window, gc, FALSE, points, 3);
			}
			break;

		case 2:
			/* Circle markers */
			posX += xSinCos ((faceSize * (size - 1)) / 64, angle, 0);
			posY -= xSinCos ((faceSize * (size - 1)) / 64, angle, 1);

			drawCircle (widget, gc, posX, posY, 2, colFill, colOut);
			break;

		case 3:
			/* Latin number markers */
			{
				char tempBuff[15];
				int scale = ((faceMax - faceMin) * hour) / 100;

				sprintf (tempBuff, "%d", faceMin + scale);
				posX += xSinCos ((faceSize * (size - 5)) / 64, angle, 0);
				posY -= xSinCos ((faceSize * (size - 5)) / 64, angle, 1);
				drawText (widget, gc, posX, posY, tempBuff, colOut, 1);
			}
			break;
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  F O N T  S I Z E                                                                                           *
 *  =======================                                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get the font size from a font def.
 *  \param fontName String to read the size from.
 *  \result The font size.
 */
int getFontSize (char *fontName)
{
	int size = 0, i = strlen (fontName), n = 1;
	
	while (i)
	{
		i --;
		if (fontName[i] >= '0' && fontName[i] <= '9')
		{
			size += (fontName[i] - '0') * n;
			n *= 10;
		}
		else
			break;
	}
	return size;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D R A W  T E X T                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display text.
 *  \param widget GTK context.
 *  \param gc GDK context.
 *  \param posX Position X.
 *  \param posY Position Y.
 *  \param dispString String to display.
 *  \param colour Colour.
 *  \param scale Size.
 *  \result None.
 */
void drawText (GtkWidget * widget, GdkGC *gc, int posX, int posY, char *dispString, int colour, int scale)
{
	int fontSize = 0;
	gint posW, posH;

	if ((fontSize = getFontSize (fontName)) == 0)
		fontSize = fontSizes[(faceSize / 64) - 1];
	if (scale)
		fontSize = (fontSize * 10) / 12;

	layout = gtk_widget_create_pango_layout (mainWindow, NULL);
	fontDesc = pango_font_description_from_string (fontName);
	pango_font_description_set_size (fontDesc, PANGO_SCALE * fontSize);
 	pango_layout_set_font_description (layout, fontDesc); 
	pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
	pango_layout_set_text (layout, dispString, -1);
	pango_layout_get_pixel_size (layout, &posW, &posH);
	gdk_draw_layout_with_colors (widget->window, gc,
			posX - (posW / 2), posY - (posH / 2), layout, 
			&faceColours[colour == -1 ? TEXT__COLOUR : colour], 
			&faceColours[FACE4_COLOUR]);

	g_object_unref (layout);
	pango_font_description_free (fontDesc);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D R A W  F A C E                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Draw the gauge face.
 *  \param widget What to draw on.
 *  \param gc GDK context.
 *  \param face Which face.
 *  \param posX Position X.
 *  \param posY Position Y.
 *  \result None.
 */
gboolean 
drawFace (GtkWidget * widget, GdkGC *gc, int face, int posX, int posY)
{
	int i, maxVal, minVal;
	FACE_SETTINGS *faceSetting = faceSettings[face];
	int centerX = posX + (faceSize >> 1), centerY = posY + (faceSize >> 1);

	/*--------------------------------------------------------------------------------------------*
	 * Draw the face, it is made up of 3 overlapping circles                                      *
	 *--------------------------------------------------------------------------------------------*/	
	gdk_gc_set_foreground (gc, &faceColours[(weHaveFocus && face == currentFace ? FACE1_COLOUR : FACE2_COLOUR)]);
	gdk_draw_rectangle (widget->window, gc, TRUE, posX, posY, faceSize, faceSize);

	drawCircle (widget, gc, centerX, centerY, 62, FACE3_COLOUR, -1);
	drawCircle (widget, gc, centerX, centerY, 60, FACE4_COLOUR, -1);

	/*------------------------------------------------------------------------------------------------*
	 * Draw the hot and cold markers                                                                  *
	 *------------------------------------------------------------------------------------------------*/
	if (faceSetting -> faceFlags & FACE_SHOWHOT)
	{
		int colour = (faceSetting -> faceFlags & FACE_HC_REVS) ? COLD__COLOUR : HOT___COLOUR;

		gdk_gc_set_foreground (gc, &faceColours[colour]);
		gdk_gc_set_line_attributes (gc, faceSize / 18, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);

		gdk_draw_arc (widget->window, gc, FALSE, centerX - ((faceSize * 54) / 128), centerY - ((faceSize * 54) / 128), 
				(faceSize * 54) / 64, (faceSize * 54) / 64, 0, -8 * 360);
	}
	if (faceSetting -> faceFlags & FACE_SHOWCOLD)
	{
		int colour = (faceSetting -> faceFlags & FACE_HC_REVS) ? HOT___COLOUR : COLD__COLOUR;

		gdk_gc_set_foreground (gc, &faceColours[colour]);
		gdk_gc_set_line_attributes (gc, faceSize / 18, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
		gdk_draw_arc (widget->window, gc, FALSE, centerX - ((faceSize * 54) / 128), centerY - ((faceSize * 54) / 128), 
				(faceSize * 54) / 64, (faceSize * 54) / 64, 32 * 360, 8 * 360);
	}
	gdk_gc_set_line_attributes (gc, (faceSize/256) + 1, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER);

	/*------------------------------------------------------------------------------------------------*
	 * Add the text, ether the date or the timezone, plus an AM/PM indicator                          *
	 *------------------------------------------------------------------------------------------------*/
	drawText (widget, gc, centerX, posY + ((faceSize * 5) >> 4), faceSetting -> titleTop, -1, 0);
	drawText (widget, gc, centerX, posY + ((faceSize * 11) >> 4), faceSetting -> titleBot, -1, 0);

	/*------------------------------------------------------------------------------------------------*
	 * Draw the hour markers                                                                          *
	 *------------------------------------------------------------------------------------------------*/
	for (i = 0; i <= 20 ; ++i)
	{
		int markAngle = i * 30;
		drawMark (widget, gc, centerX , centerY, markAngle, 29, QFILL_COLOUR, QMARK_COLOUR, i * 5,
				faceSetting -> faceScaleMin, faceSetting -> faceScaleMax);
		if (!(i & 1))
			drawMinute (widget, gc, centerX , centerY, 29, 1, markAngle, QMARK_COLOUR);
	}
	
	/*------------------------------------------------------------------------------------------------*
	 * Draw the hands                                                                                 *
	 *------------------------------------------------------------------------------------------------*/
	maxVal = faceSetting -> savedMaxMin.shownMaxValue;
	minVal = faceSetting -> savedMaxMin.shownMinValue;
	if (minVal != -1)
	{
		drawHand (widget, gc, centerX, centerY, minVal,
					&handStyle[(faceSetting -> faceFlags & FACE_HC_REVS) ? HAND_MAX : HAND_MIN]);
	}
	if (maxVal != -1)
	{
		drawHand (widget, gc, centerX, centerY, maxVal,
					&handStyle[(faceSetting -> faceFlags & FACE_HC_REVS) ? HAND_MIN : HAND_MAX]);
	}
	if (faceSetting -> shownSecondValue != DONT_SHOW)
	{
		drawHand (widget, gc, centerX, centerY, faceSetting -> shownSecondValue, &handStyle[HAND_SECOND]);
	}
	if (faceSetting -> shownFirstValue != DONT_SHOW)
	{
		drawHand (widget, gc, centerX, centerY, faceSetting -> shownFirstValue, &handStyle[HAND_FIRST]);
	}
	drawCircle (widget, gc, centerX, centerY, 4, CFILL_COLOUR, CIRC__COLOUR);

	if (face == currentFace)
	{
		gtk_window_set_title (GTK_WINDOW (mainWindow), faceSetting -> titleWin);
	}
	gdk_gc_set_line_attributes (gc, (faceSize/256) + 1, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER);

	return TRUE;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C L O C K  E X P O S E                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called to display the clock.
 *  \param widget What to draw on.
 *  \result None.
 */
void clockExpose (GtkWidget * widget)
{
	int i, j, face = 0;
	GdkGCValues saved;
	GdkGC *gc;

	/*--------------------------------------------------------------------------------------------*
	 * Reset the color and stuff back to the default.                                             *
	 *--------------------------------------------------------------------------------------------*/
	gc = widget->style->fg_gc[GTK_WIDGET_STATE (widget)];
	gdk_gc_get_values (gc, &saved);

	for (j = 0; j < faceHeight; j++)
	{
		for (i = 0; i < faceWidth; i++)
		{
			drawFace (widget, gc, face++, (i * faceSize) - 1, (j * faceSize) - 1);
		}
	}

	/*--------------------------------------------------------------------------------------------*
	 * Reset the color and stuff back to the default.                                             *
	 *--------------------------------------------------------------------------------------------*/
	gdk_gc_set_values (gc, &saved, 
			GDK_GC_FOREGROUND | GDK_GC_BACKGROUND | GDK_GC_LINE_WIDTH | GDK_GC_LINE_STYLE);
}

