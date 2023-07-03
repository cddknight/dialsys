/**********************************************************************************************************************
 *                                                                                                                    *
 *  P A R S E  Z O N E . H                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File ParseZone.h part of TzClock is free software: you can redistribute it and/or modify it under the terms of    *
 *  the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or  *
 *  (at your option) any later version.                                                                               *
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
 *  \brief Functions for parsing timezone information.
 */
#define FIRST_CITY 28
#define GMT_ZERO 13

int parseZone				();
void aboutCallback			(guint data);
void onTopCallback			(guint data);
void stickCallback			(guint data);
void lockCallback			(guint data);
void calendarCallback		(guint data);
void fontCallback			(guint data);
void colourCallback			(guint data);
void alarmCallback			(guint data);
void setTimeZoneCallback	(guint data);
void copyCallback			(guint data);
void stopwatchCallback		(guint data);
void timerCallback			(guint data);
void subSecondCallback		(guint data);
void showTimeCallback		(guint data);
void showSecondsCallback	(guint data);
void swStartCallback		(guint data);
void swResetCallback		(guint data);
void tmStartCallback		(guint data);
void tmResetCallback		(guint data);
void timerSetCallback		(guint data);
void zoomCallback			(guint data);
void faceCallback			(guint data);
void quitCallback			(guint data);
void markerCallback			(guint data);
void stepCallback			(guint data);
void configSaveCallback		(guint data);
void dialSaveCallback		(guint data);

