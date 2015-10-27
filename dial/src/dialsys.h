#ifndef INCLUDE_DIALSYS_H
#define INCLUDE_DIALSYS_H
 
#include <gtk/gtk.h>
#include <stdbool.h>

/*----------------------------------------------------------------------------------------------------*
 * Prototypes for dial list store                                                                     *
 *----------------------------------------------------------------------------------------------------*/
void *queueCreate (void);
void  queueDelete (void *queueHandle);
void *queueGet (void *queueHandle);
void  queuePut (void *queueHandle, void *putData);
void  queuePutSort (void *queueHandle, void *putData, 
                        int(*Compare)(void *item1, void *item2));
void  queuePush (void *queueHandle, void *putData);
void *queueRead (void *queueHandle, int item);
void queueSetFreeData (void *queueHandle, unsigned long setData);
unsigned long queueGetFreeData (void *queueHandle);
unsigned long queueGetItemCount (void *queueHandle);

/*----------------------------------------------------------------------------------------------------*
 * Prototypes for dial config store                                                                   *
 *----------------------------------------------------------------------------------------------------*/
int configLoad (const char *configFile);
int configSave (const char *configFile);
void configFree ();
int configSetValue (const char *configName, char *configValue);
int configSetIntValue (const char *configName, int configValue);
int configSetBoolValue (const char *configName, bool configValue);
int configGetValue (const char *configName, char *value, int maxLen);
int configGetIntValue (const char *configName, int *configValue);
int configGetBoolValue (const char *configName, bool *configValue);

/*----------------------------------------------------------------------------------------------------*
 * Structure to store menu items                                                                      *
 *----------------------------------------------------------------------------------------------------*/
typedef struct _menuDesc
{
	char *menuName;
	void (*funcCallBack) (guint data);
	struct _menuDesc *subMenuDesc;
	unsigned long param;
	char *stockItem;
	unsigned int accelKey;
	unsigned char disable;
	unsigned char checkbox;
	unsigned char checked;
}
MENU_DESC;

/*----------------------------------------------------------------------------------------------------*
 * Prototypes for dial menu                                                                           *
 *----------------------------------------------------------------------------------------------------*/
GtkWidget *createMenu (MENU_DESC *subMenuDesc, GtkAccelGroup *accelGroup, int bar);

/*----------------------------------------------------------------------------------------------------*
 * Defines used by dial display                                                                       *
 *----------------------------------------------------------------------------------------------------*/
#define SCALE_1				300
#define SCALE_2				600
#define SCALE_3				900
#define SCALE_4				1200
#define MAX_FACES 			50
#define _(String) 			gettext (String)
#define __(String) 			(String)

/*----------------------------------------------------------------------------------------------------*
 * Structure to store display items                                                                   *
 *----------------------------------------------------------------------------------------------------*/
typedef struct _colourDetails
{
	char *shortName;
	char *longName;
	char defColour[62];
#if GTK_MAJOR_VERSION == 2
	GdkColor dialColour;
#else
	GdkRGBA dialColour;
#endif
}
COLOUR_DETAILS;

typedef struct _handStyle
{
	int style;
	int length;
	int tail;
	int line;
	int fill;
	bool fillIn;
	bool gauge;
}
HAND_STYLE;

/*----------------------------------------------------------------------------------------------------*
 * Prototypes for dial display                                                                        *
 *----------------------------------------------------------------------------------------------------*/
GtkWidget *dialInit 	(GtkWindow *mainWindowIn, void(*update)(void), COLOUR_DETAILS *colourDetails, int start, void *dialSave);
void dialDrawStart 		(cairo_t *cr, int posX, int posY);
void dialDrawFinish 	(void);

void dialDrawMinute		(int size, int len, int angle, int colour);
void dialDrawCircle		(int size, int colFill, int colOut);
void dialDrawSquare		(int size, int colFill, int colOut);
void dialHotCold		(int size, int colFill, int cold);
void dialDrawHand		(int angle, HAND_STYLE *handStyle);
void dialDrawMark		(int angle, int size, int colFill, int colOut, char *text);
void dialDrawText		(int posn, char *string1, int colour);
void dialCircleGradient (int size, int colFill, int style);
void dialSquareGradient (int size, int colFill, int style);

void dialDrawMinuteX	(int posX, int posY, int size, int len, int angle, int colour);
void dialDrawCircleX	(int posX, int posY, int size, int colFill, int colOut);
void dialDrawSquareX	(int posX, int posY, int size, int colFill, int colOut);
void dialHotColdX		(int posX, int posY, int size, int colFill, int cold);
void dialDrawHandX		(int posX, int posY, int angle, HAND_STYLE *handStyle);
void dialDrawMarkX		(int posX, int posY, int angle, int size, int colFill, int colOut, char *text);
void dialDrawTextX		(int posX, int posY, char *string1, int colour, int scale);
void dialCircleGradientX (int posX, int posY, int size, int colFill, int style);
void dialSquareGradientX (int posX, int posY, int size, int colFill, int style);

int dialSin 			(int number, int angle);
int dialCos 			(int number, int angle);
float dialGetFontSize 	(char *fontName);

void dialFontCallback 	(guint data);
void dialColourCallback	(guint data);
void dialFixFaceSize	(void);
void dialZoomCallback	(guint data);
void dialMarkerCallback	(guint data);
void dialStepCallback	(guint data);
void dialAddDelCallback	(guint data);
void dialSaveCallback	(guint data);

#endif

