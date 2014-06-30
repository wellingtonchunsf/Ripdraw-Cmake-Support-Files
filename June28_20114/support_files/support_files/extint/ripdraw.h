/* ripdraw.h
 * 
 * supports Windows/Linux only
 * supports little-endian CPU only
 * 
 * function prototypes and data types
 */
#ifndef _RIPDRAW_H_
#define _RIPDRAW_H_

#ifdef  __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <stdio.h>
#include <memory.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <memory.h>
#include <unistd.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#define RDAPI extern
#define RDAPI_INLINE static __inline

#define	RD_SLEEP(x)	Sleep(x);

#else
#define RDAPI extern
#define RDAPI_INLINE static inline

#define	RD_SLEEP(x)	sleep(x);
#endif

/* ================================================================== */
/* data types for commands */
typedef unsigned char RD_BYTE;
typedef unsigned short RD_ID;
typedef unsigned short RD_UWORD;

typedef enum _RD_FLAG
{
    RD_FALSE = 0, RD_TRUE = 1
} RD_FLAG;
typedef enum _RD_HDIRECTION
{
    RD_HDIRECTION_LEFT = 0, RD_HDIRECTION_RIGHT = 1
} RD_HDIRECTION;
typedef enum _RD_DIRECTION
{
    RD_DIRECTION_HORIZONTAL = 0, RD_DIRECTION_VERTICAL = 1
} RD_DIRECTION;
typedef enum _RD_GET_VERSION_TYPE
{
    RD_GET_VERSION_TYPE_DEVAPP = 0, RD_GET_VERSION_TYPE_HARDWARE = 1, RD_GET_VERSION_TYPE_OS = 2
} RD_GET_VERSION_TYPE;
typedef enum _RD_EVENT_TYPE
{
    RD_EVENT_TYPE_ANIMATION = 0, RD_EVENT_TYPE_TOUCH = 1
} RD_EVENT_TYPE;

typedef struct _RD_POSITION
{
    RD_UWORD x;
    RD_UWORD y;
} RD_POSITION;
typedef struct _RD_SIZE
{
    RD_UWORD width;
    RD_UWORD height;
} RD_SIZE;
typedef struct _RD_COLOR
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
} RD_COLOR;

RDAPI_INLINE RD_POSITION Rd_Position(RD_UWORD x, RD_UWORD y)
{
    RD_POSITION ret;
    ret.x = x;
    ret.y = y;
    return ret;
}
RDAPI_INLINE RD_SIZE Rd_Size(RD_UWORD width, RD_UWORD height)
{
    RD_SIZE ret;
    ret.width = width;
    ret.height = height;
    return ret;
}
RDAPI_INLINE RD_COLOR Rd_Color(RD_BYTE red, RD_BYTE green, RD_BYTE blue, RD_BYTE alpha)
{
    RD_COLOR ret;
    ret.red = red;
    ret.green = green;
    ret.blue = blue;
    ret.alpha = alpha;
    return ret;
}


/* ================================================================== */
/* command ids */
typedef enum _RD_COMMAND_IDS
{
    /* Layer */
    Cmd_SetLayerEnable = 0x3131,
    Cmd_SetLayerOriginAndSize = 0x3132,
    Cmd_SetLayerBackColor = 0x3133,
    Cmd_SetLayerTransparency = 0x3134,
    Cmd_LayerClear = 0x3135,
    Cmd_LayerMove = 0x3136,
    Cmd_LayerWriteRawPixels =  0x3137,
    Cmd_ComposeLayersToPage = 0x3138,
    Cmd_PageToScreen = 0x3139,
    Cmd_PartialComposeLayersToScreen = 0x03141,
    /* Image */
    Cmd_ImageLoad = 0x3231,
    Cmd_ImageRelease = 0x3232,
    Cmd_ImageWrite = 0x3233,
    Cmd_ImageDelete = 0x3234,
    Cmd_ImageMove = 0x3235,
    Cmd_ImageListLoad = 0x3236,
    Cmd_ImageListRelease = 0x3237,
    Cmd_ImageListWrite = 0x3238,
    Cmd_ImageListReplace = 0x3239,
    Cmd_ImageListDelete = 0x3240,
    Cmd_AnimationPlay = 0x3241,
    Cmd_AnimationStop = 0x3242,
    Cmd_AnimationContinue = 0x3243,
    Cmd_AnimationDelete = 0x3244,
    /* Text */
    Cmd_FontLoad = 0x3331,
    Cmd_FontRelease = 0x3332,
    Cmd_SetFontPadding = 0x3333,
    Cmd_StringWrite = 0x3334,
    Cmd_StringReplace = 0x3335,
    Cmd_StringDelete = 0x3336,
    Cmd_CharacterWrite = 0x3337,
    Cmd_CharacterReplace = 0x3338,
    Cmd_CharacterDelete = 0x3339,
    Cmd_TextWindowCreate = 0x3341,
    Cmd_TextWindowSetInsertionPoint = 0x3342,
    Cmd_TextWindowInsertText = 0x03343,
    Cmd_TextWindowDelete = 0x3344,
    /* Graph */
    Cmd_LineGraphCreateWindow = 0x3431,
    Cmd_LineGraphInsertPoints = 0x3432,
    Cmd_LineGraphMove = 0x3433,
    Cmd_LineGraphDeleteWindow = 0x3434,
    Cmd_BarGraphCreateWindow = 0x3435,
    Cmd_BarGraphInsertStacks = 0x3436,
    Cmd_BarGraphRemoveStacks = 0x3437,
    Cmd_BarGraphDeleteWindow = 0x3438,
    /* Touch */
    Cmd_TouchMapRectangle = 0x3531,
    Cmd_TouchMapCircle = 0x3532,
    Cmd_TouchMapDelete = 0x3533,
    Cmd_TouchMapClear = 0x3534,
    /* Information */
    Cmd_SystemInfo = 0x3631,
    /* Configuration */
    Cmd_GetMaxBackLightBrightness = 0x3731,
    Cmd_GetBackLightBrightness = 0x3732,
    Cmd_SetBackLightBrightness = 0x3733,
    /* Flash */
    Cmd_FlashWriteEnable = 0x4031,
    Cmd_FlashImage = 0x4032,
    Cmd_FlashData = 0x4033,
    Cmd_FlashDelete = 0x4034,
    Cmd_FlashDeleteAll = 0x4035,
    /* Other */
    Cmd_Reset = 0x3831,
    Cmd_EventMessage = 0x3832,
    Cmd_TestEcho = 0x3931
} RD_COMMAND_IDS;

/* ================================================================== */
/* data types for serial interface */
typedef struct _RD_INTERFACE_BUFFER
{
    int capacity;
    int size;
    RD_BYTE* ptr;
} RD_INTERFACE_BUFFER;

typedef struct _RD_INTERFACE
{
	void* extint;
    int is_open;
	int verbose;
    int seq_no;
	int last_cmd_id;
    RD_UWORD last_response_status;
    RD_INTERFACE_BUFFER request;
    RD_INTERFACE_BUFFER response;
} RD_INTERFACE;

typedef struct _RD_EVENT
{
    RD_BYTE event_type;
    RD_BYTE* data;
    RD_UWORD has_more_data;
} RD_EVENT;

/* ================================================================== */
/* function prototype for interface */
#define RD_DBG(vl, ...) \
	if (rd_interface->verbose >= vl) \
	{ \
		printf(__VA_ARGS__); \
	}

/* open the serial port interface and other initialization
   returns interface pointer on success OR NULL on failed */
RDAPI RD_INTERFACE* RdInterfaceInit(const char* port_name);
/* close interface */
RDAPI int RdInterfaceClose(RD_INTERFACE* rd_interface);
/* free data */
RDAPI int RdFreeData(void* data);

/* ================================================================== */
/* Layer commands */
/* Set Layer Enable */
RDAPI int Rd_SetLayerEnable(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_FLAG enable);
/* Set Layer Origin And Size
   It creates new area and clear layer by filling background color */
RDAPI int Rd_SetLayerOriginAndSize(RD_INTERFACE* rd_interface, RD_ID layer_id,
RD_POSITION position, RD_SIZE size);
/* Set Layer Back Color */
RDAPI int Rd_SetLayerBackColor(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_COLOR back_color);
/* Set Layer Transparency */
RDAPI int Rd_SetLayerTransparency(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_BYTE transparency_percentage);
/* Layer Clear
   It fills the background color */
RDAPI int Rd_LayerClear(RD_INTERFACE* rd_interface, RD_ID layer_id);
/* Move Layer */
RDAPI int Rd_LayerMove(RD_INTERFACE* rd_interface, RD_ID layer_id,
RD_UWORD move_left, RD_UWORD move_top, RD_UWORD move_right, RD_UWORD move_bottom);
/* Write Raw Pixels */
RDAPI int Rd_LayerWriteRawPixels(RD_INTERFACE* rd_interface, RD_ID layer_id,
RD_POSITION position, RD_SIZE pixel_size, const RD_COLOR* pixels);
/* Compose all Layers */
RDAPI int Rd_ComposeLayersToPage(RD_INTERFACE* rd_interface, RD_ID page_id);
/* shows page to screen */
RDAPI int Rd_PageToScreen(RD_INTERFACE* rd_interface, RD_ID page_id);
/* partial compose */
RDAPI int Rd_PartialComposeLayersToScreen(RD_INTERFACE* rd_interface, RD_ID layer_id);

/* ================================================================== */
/* Image commands */
/* Load image of given name
   Returns image id of newly loaded image */
RDAPI int Rd_ImageLoad(RD_INTERFACE* rd_interface, const char* image_label, RD_ID* image_id);
/* Image Release */
RDAPI int Rd_ImageRelease(RD_INTERFACE* rd_interface, RD_ID image_id);
/* Image write */
RDAPI int Rd_ImageWrite(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_ID image_id,
RD_POSITION position, RD_ID* image_write_id);
/* Image Delete */
RDAPI int Rd_ImageDelete(RD_INTERFACE* rd_interface, RD_ID image_write_id);
/* Image Move */
RDAPI int Rd_ImageMove(RD_INTERFACE* rd_interface, RD_ID image_write_id, RD_POSITION position);
/* Returns image id of newly loaded image list */
RDAPI int Rd_ImageListLoad(RD_INTERFACE* rd_interface, const char* prefix, RD_UWORD index_start,
RD_UWORD index_step,
RD_UWORD index_count, RD_ID* image_list_id);
/* Image list Release */
RDAPI int Rd_ImageListRelease(RD_INTERFACE* rd_interface, RD_ID image_list_id);
/* Image write from list */
RDAPI int Rd_ImageListWrite(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_POSITION position,
RD_ID image_list_id, RD_UWORD image_index, RD_ID* image_list_write_id);
/* Image Replace */
RDAPI int Rd_ImageListReplace(RD_INTERFACE* rd_interface, RD_ID image_list_write_id, RD_UWORD image_index);
/* Image List Delete */
RDAPI int Rd_ImageListDelete(RD_INTERFACE* rd_interface, RD_ID image_list_write_id);
/* Animation play */
RDAPI int Rd_AnimationPlay(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_POSITION position,
RD_ID image_list_id, RD_UWORD frequency, RD_ID* animation_play_id);
/* Animation stop */
RDAPI int Rd_AnimationStop(RD_INTERFACE* rd_interface, RD_ID animation_play_id, RD_UWORD stop_index);
/* Animation continue */
RDAPI int Rd_AnimationContinue(RD_INTERFACE* rd_interface, RD_ID animation_play_id);
/* Animation Delete */
RDAPI int Rd_AnimationDelete(RD_INTERFACE* rd_interface, RD_ID animation_play_id);

/* ================================================================== */
/* Text commands */
/* Load image of given name */
/* Returns image id of newly loaded image */
RDAPI int Rd_FontLoad(RD_INTERFACE* rd_interface, const char* font_label, RD_UWORD* font_id);
/* Font Release */
RDAPI int Rd_FontRelease(RD_INTERFACE* rd_interface, RD_ID font_id);
RDAPI int Rd_SetFontPadding(RD_INTERFACE* rd_interface, RD_ID font_id, const RD_BYTE padding);
/* String write */
RDAPI int Rd_StringWrite(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_POSITION position,
RD_ID font_id, RD_COLOR color, RD_HDIRECTION hdirection, const char* data, RD_ID* string_write_id);
/* String Replace */
RDAPI int Rd_StringReplace(RD_INTERFACE* rd_interface, RD_ID string_write_id, const char* data);
/* String delete */
RDAPI int Rd_StringDelete(RD_INTERFACE* rd_interface, RD_ID string_write_id);
/* Character write */
RDAPI int Rd_CharacterWrite(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_POSITION position,
RD_ID font_id, RD_COLOR color, RD_BYTE c, RD_ID* character_write_id);
/* Character replace */
RDAPI int Rd_CharacterReplace(RD_INTERFACE* rd_interface, RD_ID character_write_id, RD_BYTE c);
/* Character delete */
RDAPI int Rd_CharacterDelete(RD_INTERFACE* rd_interface, RD_ID character_write_id);
/* Create text window */
RDAPI int Rd_TextWindowCreate(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_POSITION position,
RD_SIZE size, RD_ID font_id, RD_COLOR fontcolor, RD_HDIRECTION scroll_direction, RD_UWORD* text_window_id);
/* Insert point for text window */
RDAPI int Rd_TextWindowSetInsertionPoint(RD_INTERFACE* rd_interface, RD_ID text_window_id, RD_POSITION position);
/* Insert text in text window */
RDAPI int Rd_TextWindowInsertText(RD_INTERFACE* rd_interface, RD_ID text_window_id, const char* stringData);
/* text window delete */
RDAPI int Rd_TextWindowDelete(RD_INTERFACE* rd_interface, RD_ID text_window_id);

/* ================================================================== */
/* Graph commands */
/* create line graph window */
/*  Returns graph id of newly created graph */
RDAPI int Rd_LineGraphCreateWindow(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_POSITION position,
RD_SIZE size, RD_BYTE line_width, RD_BYTE line_glow_width, RD_FLAG autocompose, RD_ID* graph_id);
/* set the line graph start point */
RDAPI int Rd_LineGraphInsertPoints(RD_INTERFACE* rd_interface, RD_ID graph_id, RD_COLOR point_color,
RD_UWORD point_length, const RD_POSITION* points);
/* set line graph shift point */
RDAPI int Rd_LineGraphMove(RD_INTERFACE* rd_interface, RD_ID graph_id,
RD_UWORD left, RD_UWORD top, RD_UWORD right, RD_UWORD bottom);
/* delete Line Graph window*/
RDAPI int Rd_LineGraphDeleteWindow(RD_INTERFACE* rd_interface, RD_ID graph_id);
/* create bar graph window returns graph id */
RDAPI int Rd_BarGraphCreateWindow(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_POSITION position, RD_SIZE size,
RD_BYTE stack_size, RD_DIRECTION stack_direction, RD_FLAG autocompose, RD_ID* graph_id);
/* insert stacks */
RDAPI int Rd_BarGraphInsertStacks(RD_INTERFACE* rd_interface, RD_ID graph_id, RD_BYTE no_of_stack, RD_ID image_id);
/* remove stacks */
RDAPI int Rd_BarGraphRemoveStacks(RD_INTERFACE* rd_interface, RD_ID graph_id, RD_BYTE no_of_stack);
/* delete Bar Graph window*/
RDAPI int Rd_BarGraphDeleteWindow(RD_INTERFACE* rd_interface, RD_ID graph_id);

/* ================================================================== */
/* Touch map commands */
/* create MapRectangle
   Returns MapRectangle id */
RDAPI int Rd_TouchMapRectangle(RD_INTERFACE* rd_interface, RD_POSITION position, RD_SIZE size, const char* label, RD_ID* touch_id);
/* create MapCircle
   Returns MapCircle id */
RDAPI int Rd_TouchMapCircle(RD_INTERFACE* rd_interface, RD_POSITION position,
RD_UWORD outer_circle_radius, RD_UWORD inner_circle_radius, const char* label, RD_ID* touch_id);
/* TouchMap delete shape */
RDAPI int Rd_TouchMapDelete(RD_INTERFACE* rd_interface, RD_ID touch_id);
/* TouchMap Clear all */
RDAPI int Rd_TouchMapClear(RD_INTERFACE* rd_interface);

/* ================================================================== */
/* Information commands */
/* Get Version */
RDAPI int Rd_SystemInfo(RD_INTERFACE* rd_interface, RD_GET_VERSION_TYPE type, char** output);

/* ================================================================== */
/* Other commands */
/* Reset all */
RDAPI int Rd_Reset(RD_INTERFACE* rd_interface);
/* Test Echo*/
RDAPI int Rd_TestEcho(RD_INTERFACE* rd_interface, const char* label, char** output);
/* Event Message */
RDAPI int Rd_EventMessage(RD_INTERFACE* rd_interface, RD_EVENT** event, RD_UWORD* count);

/* ================================================================== */
/* Flash commands */
/* Set FlashWrite Enable */
RDAPI int Rd_FlashWriteEnable(RD_INTERFACE* rd_interface, RD_FLAG enable);
/* Start file transfer
   Returns Transfer id */
RDAPI int Rd_FlashImage(RD_INTERFACE* rd_interface, RD_UWORD type, const char* filename, RD_UWORD length, RD_ID* transfer_id);
/* File transfer chunk */
RDAPI int Rd_FlashData(RD_INTERFACE* rd_interface, RD_ID transfer_id, RD_UWORD type, const char* data);
/* File delete */
RDAPI int Rd_FlashDelete(RD_INTERFACE* rd_interface, RD_UWORD type, const char* filename);
/* File delete all */
RDAPI int Rd_FlashDeleteAll(RD_INTERFACE* rd_interface);

/* ================================================================== */
/* Configuration */
/* Get Max BackLight Brightness */
RDAPI int Rd_GetMaxBackLightBrightness(RD_INTERFACE* rd_interface, RD_UWORD* max_backlight_brightness);
/* Get BackLight Brightness */
RDAPI int Rd_GetBackLightBrightness(RD_INTERFACE* rd_interface, RD_UWORD* backlight_brightness);
/* Set BackLight Brightness */
RDAPI int Rd_SetBackLightBrightness(RD_INTERFACE* rd_interface, RD_UWORD backlight_brightness);

/* ================================================================== */
/* helper macros */
#define _RD_CHECK_INTERFACE()\
    if (rd_interface == NULL)\
    {\
        perror("\ninterface should not NULL\n");\
        return -1;\
    }\
    if (rd_interface->is_open != 1)\
    {\
        perror("\nport not opened\n");\
        return -1;\
    }

#ifdef  __cplusplus
}
#endif

#endif

