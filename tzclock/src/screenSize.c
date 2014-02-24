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

void dialSave() {}

/*----------------------------------------------------------------------------------------------------*
 *                                                                                                    *
 *  M A I N                                                                                           *
 *  =======                                                                                           *
 *                                                                                                    *
 *----------------------------------------------------------------------------------------------------*/
/**
 *  @brief Start of the program called by the library.
 *  @param argc Number of arguments, passed to init.
 *  @param argv Command line arguments, passed to init.
 *  @result Nothing.
 */
int main (int argc, char *argv[])
{
//	GtkWidget *mainWindow;

	gtk_init (&argc, &argv);

/*	g_set_application_name ("copyclip");
	gtk_window_set_default_icon_name ("copyclip");
	mainWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_show_all (mainWindow);
	gtk_main ();
*/
	printf ("XPOS=%d\nYPOS=%d\n",
			gdk_screen_width(), gdk_screen_height());
	return 0;
}
