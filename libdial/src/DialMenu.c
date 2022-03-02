/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I A L  M E N U . C                                                                                              *
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
 *  \brief Handle dial menus.
 */
#include <gtk/gtk.h>
#include <libintl.h>
#include <string.h>
#include "dialsys.h"

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C R E A T E  M E N U                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Create the menu .
 *  \param createMenuDesc Create a menu, read from template.
 *  \param accelGroup Create a menu.
 *  \param bar Is this to be a menubar.
 *  \result None.
 */
GtkWidget *createMenu (MENU_DESC *createMenuDesc, GtkAccelGroup *accelGroup, int bar)
{
	int i = 0;
	GtkWidget *menuItem, *thisMenu;

	if (bar)
		thisMenu = gtk_menu_bar_new ();
	else
		thisMenu = gtk_menu_new ();

	while (createMenuDesc[i].menuName)
	{
		if (!(createMenuDesc[i].disable))
		{
			if (strcmp (createMenuDesc[i].menuName, "-") == 0)
			{
				menuItem = gtk_separator_menu_item_new ();
				gtk_menu_shell_append (GTK_MENU_SHELL (thisMenu), menuItem);
				gtk_widget_show (menuItem);
			}
			else
			{
				if (createMenuDesc[i].checkbox)
				{
					menuItem = gtk_check_menu_item_new_with_label (gettext(createMenuDesc[i].menuName));
					gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuItem),
							createMenuDesc[i].checked ? TRUE : FALSE);
				}
				else if (createMenuDesc[i].stockItem)
				{
#if GTK_MINOR_VERSION >= 10
					menuItem = gtk_menu_item_new_with_label (gettext(createMenuDesc[i].menuName));
#else
					menuItem = gtk_image_menu_item_new_from_stock  (createMenuDesc[i].stockItem,
							accelGroup);
#endif
				}
				else
				{
					menuItem = gtk_menu_item_new_with_label (gettext(createMenuDesc[i].menuName));
				}

				gtk_menu_shell_append (GTK_MENU_SHELL (thisMenu), menuItem);
				if (createMenuDesc[i].funcCallBack)
				{
					g_signal_connect_swapped (menuItem, "activate", G_CALLBACK (createMenuDesc[i].funcCallBack),
							(gpointer)createMenuDesc[i].param);
					if (createMenuDesc[i].accelKey)
					{
						gtk_widget_add_accelerator (menuItem, "activate", accelGroup,
								createMenuDesc[i].accelKey, GDK_CONTROL_MASK,
								GTK_ACCEL_VISIBLE);
					}
				}
				else
				{
					GtkWidget *nextMenu = createMenu (createMenuDesc[i].subMenuDesc, accelGroup, FALSE);
					gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuItem), nextMenu);
				}
			}
			gtk_widget_show (menuItem);
		}
		++i;
	}
	return thisMenu;
}

