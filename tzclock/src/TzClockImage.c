/**********************************************************************************************************************
 *                                                                                                                    *
 *  T Z  C L O C K  I M A G E . C                                                                                     *
 *  =============================                                                                                     *
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
 *  \brief Not used.
 */
#include "config.h"
#include "TzClockDisp.h"
#include "TimeZone.h"

#include <librsvg-2/librsvg/rsvg.h>
#include <librsvg-2/librsvg/rsvg-cairo.h>

extern GtkWidget *mainWindow;
extern GtkWidget *drawingArea;
extern GdkColor clockColours[];
extern int faceSize;
extern int faceWidth;
extern int faceHeight;
extern int weHaveFocus;
extern int currentFace;
extern int timeSetting;
extern int fastSetting;
extern int showBounceSec;
extern time_t forceTime;

extern FACE_SETTINGS faceSettings[];
extern TZ_INFO *timeZones;

static GdkDrawable *windowShapeBitmap;
static char windowTitle[128];
static int needsUpdate = 1;

/*------------------------------------------------------------------------------------------------*
 *                                                                                                *
 *------------------------------------------------------------------------------------------------*/
typedef enum _layerElements
{
	CLOCK_DROP_SHADOW = 0,
	CLOCK_FACE,
	CLOCK_MARKS,
	CLOCK_HOUR_HAND_SHADOW,
	CLOCK_MINUTE_HAND_SHADOW,
	CLOCK_SECOND_HAND_SHADOW,
	CLOCK_HOUR_HAND,
	CLOCK_MINUTE_HAND,
	CLOCK_SECOND_HAND,
	CLOCK_FACE_SHADOW,
	CLOCK_GLASS,
	CLOCK_FRAME,
	CLOCK_ELEMENTS
}
layerElements;

RsvgHandle		*g_pSvgHandles[CLOCK_ELEMENTS];
cairo_surface_t *g_pBackgroundSurface = NULL;
cairo_surface_t *g_pForegroundSurface = NULL;

extern const guint gtk_major_version;
extern const guint gtk_minor_version;
extern const guint gtk_micro_version;

char *g_pFileNames[CLOCK_ELEMENTS] =
{
	"clock-drop-shadow.svg",
	"clock-face.svg",
	"clock-marks.svg",
	"clock-hour-hand-shadow.svg",
	"clock-minute-hand-shadow.svg",
	"clock-second-hand-shadow.svg",
	"clock-hour-hand.svg",
	"clock-minute-hand.svg",
	"clock-second-hand.svg",
	"clock-face-shadow.svg",
	"clock-glass.svg",
	"clock-frame.svg"
};
RsvgDimensionData	g_DimensionData;

/**********************************************************************************************************************
 *                                                                                                                    *
 *  M A K E  W I N D O W  M A S K                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Make a mask to make the face round.
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
	gdk_gc_set_foreground (gc, &clockColours[BLACK_COLOUR]);
	gdk_gc_set_background (gc, &clockColours[WHITE_COLOUR]);

	gdk_draw_rectangle (windowShapeBitmap, gc, TRUE, 0, 0, fullWidth, fullHeight);

	gdk_gc_set_foreground (gc, &clockColours[WHITE_COLOUR]);
	gdk_gc_set_background (gc, &clockColours[BLACK_COLOUR]);

	for (i = 0; i < faceWidth; i++)
	{
		for (j = 0; j < faceHeight; j++)
			gdk_draw_arc (windowShapeBitmap, gc, TRUE, (i * faceSize) - 1, (j * faceSize) - 1,
					faceSize + 1, faceSize + 1, 0, 360 * 64);
/*          gdk_draw_arc (windowShapeBitmap, gc, TRUE, (i * faceSize) + 10, (j * faceSize) + 10,
 *                  faceSize - 21, faceSize - 21, 0, 360 * 64);
 */ }
	gtk_widget_shape_combine_mask (mainWindow, windowShapeBitmap, 0, 0);

	g_object_unref (gc);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D R A W  T E X T                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display text on the face.
 *  \param cr Cairo handle.
 *  \param string1 Tect to display.
 *  \param string2 Extra test to displat.
 *  \param top Display at the top of the face.
 *  \result None.
 */
void drawText (cairo_t *cr, char *string1, char *string2, int top)
{
	int height;
	cairo_text_extents_t textExtents;

	cairo_set_font_size (cr, 4 + ((faceSize / 64) * 4));
	gdk_cairo_set_source_color (cr, &clockColours[TEXT__COLOUR]);

	cairo_save (cr);
	cairo_set_font_size (cr, 7);
	cairo_set_source_rgb (cr, 0.0f, 0.5f, 1.0f);
	cairo_set_line_width (cr, 3.0f);

	cairo_text_extents (cr, "abcdefgABCDEFG", &textExtents);
	height = textExtents.height;

	if (string2[0])
	{
		cairo_text_extents (cr, string1, &textExtents);
		if (top)
		{
			cairo_move_to (cr, -textExtents.width / 2.0f, ((double) -g_DimensionData.height / 4.0f) + (height * 1.4f));
			cairo_show_text (cr, string1);
			cairo_text_extents (cr, string2, &textExtents);
			cairo_move_to (cr, -textExtents.width / 2.0f, ((double) -g_DimensionData.height / 4.0f) + (height * 2.6f));
			cairo_show_text (cr, string2);
		}
		else
		{
			cairo_move_to (cr, -textExtents.width / 2.0f, ((double)	 g_DimensionData.height / 4.0f) - (height * 1.4f));
			cairo_show_text (cr, string1);
			cairo_text_extents (cr, string2, &textExtents);
			cairo_move_to (cr, -textExtents.width / 2.0f, ((double)	 g_DimensionData.height / 4.0f) - (height * 0.2f));
			cairo_show_text (cr, string2);
		}
	}
	else
	{
		cairo_text_extents (cr, string1, &textExtents);
		if (top)
			cairo_move_to (cr, -textExtents.width / 2.0f, ((double) -g_DimensionData.height / 4.0f) + (height * 2.0f));
		else
			cairo_move_to (cr, -textExtents.width / 2.0f, ((double)	 g_DimensionData.height / 4.0f) - (height * 1.2f));
		cairo_show_text (cr, string1);
	}
	cairo_restore (cr);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C A I R O  D R A W  S U R F A C E                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief .
 *  \param cr .
 *  \param backgound .
 *  \result .
 */
void cairoDrawSurface (cairo_t *cr, int backgound)
{
	int i, j;
	cairo_surface_t* pNewSurface = NULL;
	cairo_t* pDrawingContext = NULL;

	if (needsUpdate)
	{
		GError* pError = NULL;

		for (i = 0; i < CLOCK_ELEMENTS; i++)
		{
			char fileName[80];

			strcpy (fileName, "theme/");
			strcat (fileName, g_pFileNames[i]);
			g_pSvgHandles[i] = rsvg_handle_new_from_file (fileName, &pError);
		}
		rsvg_handle_get_dimensions (g_pSvgHandles[CLOCK_DROP_SHADOW], &g_DimensionData);

		cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
		cairo_surface_destroy (backgound ? g_pBackgroundSurface : g_pForegroundSurface);

		pNewSurface = cairo_surface_create_similar (cairo_get_target (cr),
				CAIRO_CONTENT_COLOR_ALPHA, faceWidth * faceSize, faceHeight * faceSize);

		if (cairo_surface_status (pNewSurface) != CAIRO_STATUS_SUCCESS)
			return;

		pDrawingContext = cairo_create (pNewSurface);
		if (cairo_status (pDrawingContext) != CAIRO_STATUS_SUCCESS)
			return;

		cairo_scale (pDrawingContext,
				(double) faceSize / (double) g_DimensionData.width,
				(double) faceSize / (double) g_DimensionData.height);
		cairo_set_source_rgba (pDrawingContext, 1.0f, 1.0f, 1.0f, 0.0f);
		cairo_set_operator (pDrawingContext, CAIRO_OPERATOR_OVER);

		for (j = 0; j < faceHeight; j++)
		{
			for (i = 0; i < faceWidth; i++)
			{
				cairo_save (pDrawingContext);
				cairo_translate (pDrawingContext,
						(double)(g_DimensionData.width * i),
						(double)(g_DimensionData.height * j));
				if (backgound)
				{
					rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_DROP_SHADOW], pDrawingContext);
					rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_FACE], pDrawingContext);
					rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_MARKS], pDrawingContext);
				}
				else
				{
					rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_FACE_SHADOW], pDrawingContext);
					rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_GLASS], pDrawingContext);
					rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_FRAME], pDrawingContext);
				}
				cairo_paint (pDrawingContext);
				cairo_restore (pDrawingContext);
			}
		}
		if (backgound)
		{
			g_pBackgroundSurface = pNewSurface;
		}
		else
		{
			g_pForegroundSurface = pNewSurface;
		}
		cairo_destroy (pDrawingContext);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D R A W  F A C E                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief .
 *  \param cr .
 *  \param face .
 *  \param posX .
 *  \param posY .
 *  \param t .
 *  \param bounce .
 *  \result .
 */
gboolean
drawFace (cairo_t *cr, int face, int posX, int posY, time_t t, int bounce)
{
	int i, j, timeZone = faceSettings[face].currentTZ, tempTime;
	int hourAngle, minuteAngle, secondAngle, swMinAngle, swSecAngle = 0, swPrtAngle;
	char tempString1[51], tempString2[51];
	cairo_text_extents_t textExtents;
	struct tm tm;

	/*------------------------------------------------------------------------------------------------*
     * The alarm check has moved out of here                                                          *
     *------------------------------------------------------------------------------------------------*/
	getTheFaceTime (face, &t, &tm);

	/*------------------------------------------------------------------------------------------------*
     * Handle the smooth scroll time chage                                                            *
     *------------------------------------------------------------------------------------------------*/
	tempTime = (tm.tm_hour * 60) + tm.tm_min;

	if (!fastSetting)
	{
		int gap = 0;

		/*--------------------------------------------------------------------------------------------*
         * Don't scroll at midnight                                                                   *
		 *--------------------------------------------------------------------------------------------*/
		if (tempTime == 0 && faceSettings[face].shownTime == (23 * 60) + 59)
			gap = 1;
		else
			gap = tempTime - faceSettings[face].shownTime;

		if (abs (gap) > 1)
		{
			/*----------------------------------------------------------------------------------------*
			 * The maximum gap is 12 hours unless it is a 24 hour clock                               *
			 *----------------------------------------------------------------------------------------*/
			if (abs(gap) > TWELVEHOURS && !faceSettings[face].show24Hour)
			{
				if (gap > 0)
				{
					faceSettings[face].shownTime += TWELVEHOURS;
					gap -= TWELVEHOURS;
				}
				else
				{
					faceSettings[face].shownTime -= TWELVEHOURS;
					gap += TWELVEHOURS;
				}
			}

			j = 1;
			for (i = 1; i <= faceSettings[face].stepping; i ++)
			{
				j += (i / 3);
				if (j > abs (gap))
					break;
			}
			i /= 3;
			if (gap < 0)
				i *= -1;

			if (faceSettings[face].stepping < 24)
				faceSettings[face].stepping ++;

			tempTime = faceSettings[face].shownTime + i;

			tm.tm_min = tempTime % 60;
			tm.tm_hour = tempTime / 60;

			timeSetting = 1;
		}
		else
			faceSettings[face].stepping = 0;
	}
	faceSettings[face].shownTime = tempTime;

	/*------------------------------------------------------------------------------------------------*
	 * Draw the face, it is made up of 3 overlapping circles                                          *
	 *------------------------------------------------------------------------------------------------*/
	cairo_save (cr);

	/*--------------------------------------------------------------------------------------------*
	 * Draw the face, it is made up of 3 overlapping circles                                      *
	 *--------------------------------------------------------------------------------------------*/
	if (face == 0)
	{
		cairoDrawSurface (cr, 1);
		cairoDrawSurface (cr, 0);
		needsUpdate = 0;

		cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_surface (cr, g_pBackgroundSurface, 0.0f, 0.0f);
		cairo_paint (cr);
	}

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_save (cr);

	cairo_scale (cr,
			(double) faceSize / (double) g_DimensionData.width,
			(double) faceSize / (double) g_DimensionData.height);

	cairo_translate (cr,
			(double)(g_DimensionData.width * posX),
			(double)(g_DimensionData.height * posY));
	cairo_translate (cr, g_DimensionData.width / 2.0f, g_DimensionData.height / 2.0f);

	/*--------------------------------------------------------------------------------------------*
	 * Add the text, ether the date or the timezone, plus an AM/PM indicator                      *
	 *--------------------------------------------------------------------------------------------*/
	getStringValue (tempString1, tempString2, 50, faceSettings[face].stopwatch ?
			(timeZone ? TXT_TOPSW_Z : TXT_TOPSW_L) : (timeZone ? TXT_TOP_Z : TXT_TOP_L), face, t);
	drawText (cr, tempString1, tempString2, 1);

	if (!faceSettings[face].stopwatch)
	{
		getStringValue (tempString1, tempString2, 50, timeZone ? TXT_BOTTOM_Z : TXT_BOTTOM_L, face, t);
		drawText (cr, tempString1, tempString2, 0);
	}

	if (face == currentFace)
	{
		getStringValue (tempString1, tempString2, 50, timeZone ? TXT_TITLE_Z : TXT_TITLE_L, face, t);
		if (strcmp (windowTitle, tempString1))
		{
			strcpy (windowTitle, tempString1);
			gtk_window_set_title (GTK_WINDOW (mainWindow), windowTitle);
		}
	}

	/*--------------------------------------------------------------------------------------------*
	 * Draw the hands: First draw the shadow                                                      *
	 *--------------------------------------------------------------------------------------------*/
	cairo_rotate (cr, -M_PI/2.0f);

	cairo_save (cr);
	cairo_translate (cr, -0.25f, 0.25f);
	cairo_rotate (cr, (M_PI/6.0f) * tm.tm_hour + (M_PI/360.0f) * tm.tm_min);
	rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_HOUR_HAND_SHADOW], cr);
	cairo_restore (cr);

	cairo_save (cr);
	cairo_translate (cr, -0.50f, 0.50f);
	cairo_rotate (cr, (M_PI/30.0f) * tm.tm_min + (M_PI/120.0f) * (tm.tm_sec / 15));
	rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_MINUTE_HAND_SHADOW], cr);
	cairo_restore (cr);

	cairo_save (cr);
	cairo_translate (cr, -0.75f, 0.75f);
	cairo_rotate (cr, (M_PI/30.0f) * tm.tm_sec);
	rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_SECOND_HAND_SHADOW], cr);
	cairo_restore (cr);

	/*--------------------------------------------------------------------------------------------*
	 * Draw the hands: Then draw the hand                                                         *
	 *--------------------------------------------------------------------------------------------*/
	cairo_save (cr);
	cairo_rotate (cr, (M_PI/6.0f) * tm.tm_hour + (M_PI/360.0f) * tm.tm_min);
	rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_HOUR_HAND], cr);
	cairo_restore (cr);

	cairo_save (cr);
	cairo_rotate (cr, (M_PI/30.0f) * tm.tm_min + (M_PI/120.0f) * (tm.tm_sec / 15));
	rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_MINUTE_HAND], cr);
	cairo_restore (cr);

	cairo_save (cr);
	cairo_rotate (cr, (M_PI/30.0f) * tm.tm_sec);
	rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_SECOND_HAND], cr);
	cairo_restore (cr);

	/*--------------------------------------------------------------------------------------------*
	 * Restore everything and draw the clock forground                                            *
	 *--------------------------------------------------------------------------------------------*/
	cairo_restore (cr);

	if (face == 0)
	{
		cairo_set_source_surface (cr, g_pForegroundSurface, 0.0f, 0.0f);
		cairo_paint (cr);
	}

	cairo_restore (cr);
	return TRUE;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C L O C K  E X P O S E                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief .
 *  \param widget .
 *  \result .
 */
void clockExpose (GtkWidget * widget)
{
	int i, j, face = 0, bounce = 0;
	cairo_t *cr = gdk_cairo_create (drawingArea -> window);
	struct timeval tv;
	struct timezone tz;

	gettimeofday(&tv, &tz);
	if (showBounceSec)
	{
		bounce = tv.tv_usec < 50000 ? 1 : 0;
	}

	/*------------------------------------------------------------------------------------------------*
	 * Reset the color and stuff back to the default.                                                 *
	 *------------------------------------------------------------------------------------------------*/
	timeSetting = 0;

	if (forceTime != -1)
	{
		tv.tv_sec = forceTime;
		bounce = 0;
	}

	for (j = 0; j < faceHeight; j++)
	{
		for (i = 0; i < faceWidth; i++)
			drawFace (cr, face++, i, j, tv.tv_sec, bounce);
	}

	/*------------------------------------------------------------------------------------------------*
	 * Reset the color and stuff back to the default.                                                 *
	 *------------------------------------------------------------------------------------------------*/
	cairo_destroy (cr);
	cr = NULL;
}

