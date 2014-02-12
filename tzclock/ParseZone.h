/*----------------------------------------------------------------------------------------------------*
 *                                                                                                    *
 *  P A R S E  Z O N E . H                                                                            *
 *  ======================                                                                            *
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
 *  @version $Id: ParseZone.h 1434 2012-04-06 07:19:33Z chris $
 */
#define FIRST_CITY 28
#define GMT_ZERO 13

int parseZone 				();
void aboutCallback 			(guint data);
void onTopCallback 			(guint data);
void stickCallback 			(guint data);
void lockCallback 			(guint data);
void calendarCallback 		(guint data);
void fontCallback	 		(guint data);
void colourCallback	 		(guint data);
void alarmCallback	 		(guint data);
void setTimeZoneCallback 	(guint data);
void copyCallback 			(guint data);
void stopwatchCallback		(guint data);
void subSecondCallback		(guint data);
void showSecondsCallback	(guint data);
void swStartCallback 		(guint data);
void zoomCallback 			(guint data);
void faceCallback			(guint data);
void swResetCallback 		(guint data);
void quitCallback 			(guint data);
void markerCallback			(guint data);
void stepCallback			(guint data);
void configSaveCallback		(guint data);
void dialSaveCallback 		(guint data);

