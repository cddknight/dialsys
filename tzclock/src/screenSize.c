/*----------------------------------------------------------------------------------------------------*
 *                                                                                                    *
 *  S C R E E N  S I Z E . C                                                                          *
 *  ========================                                                                          *
 *                                                                                                    *
 *  screenSize developed by Chris Knight.                                                             * 
 *                                                                                                    *
 *----------------------------------------------------------------------------------------------------*
 *                                                                                                    *
 *  screenSize is free software; you can redistribute it and/or modify it under the terms of the GNU  *
 *  General Public License version 2 as published by the Free Software Foundation.  Note that I       *
 *  am not granting permission to redistribute or modify screenSize under the terms of any later      *
 *  version of the General Public License.                                                            *
 *                                                                                                    *
 *  screenSize is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without   *
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
 *  @brief Application to calculate the size of screen in pixels.
 *  @version $Id: screenSize.c 1453 2012-05-02 15:02:27Z ukchkn $
 */
#include <stdio.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I A L  S A V E                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Dummy fuction to keep library happy.
 *  \result None.
 */
void dialSave() {}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  S C R E E N  S I Z E                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief New method to get the sreen size.
 *  \param width Return width.
 *  \param height Return height.
 *  \result None.
 */
void getScreenSize (int *width, int *height)
{
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION >= 22
        GdkDisplay *display;
        GdkMonitor *monitor;
        GdkRectangle monitor_geometry;

        display = gdk_display_get_default ();
        monitor = gdk_display_get_monitor (display, 0);
        gdk_monitor_get_geometry (monitor, &monitor_geometry);

        if (width != NULL)
        {
                *width = monitor_geometry.width;
        }
        if (height != NULL)
        {
                *height = monitor_geometry.height;
        }
#else
        if (width != NULL)
        {
                *width = gdk_screen_width();
        }
        if (height != NULL)
        {
                *height = gdk_screen_height();
        }
#endif
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  M A I N                                                                                                           *
 *  =======                                                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Start of the program called by the library.
 *  \param argc Number of arguments, passed to init.
 *  \param argv Command line arguments, passed to init.
 *  \result Nothing.
 */
int main (int argc, char *argv[])
{
	int width = 0, height = 0;

	gtk_init (&argc, &argv);
	getScreenSize (&width, &height);
	printf ("XPOS=%d\nYPOS=%d\n", width, height);
	return 0;
}
