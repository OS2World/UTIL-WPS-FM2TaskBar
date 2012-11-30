
/****************************************************************************

    FM/2 Taskbar Common Includes
    $Id: TASKBAR.H,v 1.7 2003/11/01 08:14:27 root Exp $

    Copyright (c) 1996, 2001 Mark Kimes
    Copyright (c) 2001, 2003 Steven Levine and Associates, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Revisions	03 Apr 96 MK - Baseline
		19 Apr 03 SHL - Version 1.07a
		28 May 03 SHL - Version 1.07b
		30 May 03 SHL - Version 1.07c
		30 May 03 SHL - Version 1.07d
		31 Oct 03 SHL - Version 1.08

*****************************************************************************/

#define VERSION             "1.08"

#define MAIN_ID             1
#define MAIN_HELP           2
#define APP_POINTER         3
#define MAIN_TIMER          4
#define MAIN_DATETIME       5
#define MAIN_STARTUP        6
#define MAIN_VIRTUAL        7
#define BLANK_POINTER       8
#define MAIN_PROCS          9

#define START_TIMER         10
#define START_FRAME         11
#define START_BITMAP        12
#define START_ICON          13
#define FM2_ICON            14
#define START_VERSION       15

#define MAIN_FIRST          100

#define RUN_FRAME           200
#define RUN_FIND            201
#define RUN_NAME            202
#define RUN_DIR             203
#define RUN_ARGS            204
#define RUN_DIRHDR          205
#define RUN_ARGSHDR         206

#define SET_FRAME           300
#define SET_LEFT            301
#define SET_CENTER          302
#define SET_RIGHT           303
#define SET_HEIGHT          304
#define SET_NOZORDERCHANGE  305
#define SET_SLIDINGFOCUS    306
#define SET_REPOSMOUSE      307
#define SET_MOVEFOLDERS     308
#define SET_B2BOTTOM        309
#define SET_B3CLOSE         310
#define SET_B3DBLCLK        311
#define SET_UNDO            312
#define SET_DISABLEDESKTOPS 313
#define SET_ANIMATE         314
#define SET_NOHOT           315
#define SET_LEFTHOT         316
#define SET_WRAPMOUSE       317
#define SET_SHOWMINIWINS    318
#define SET_CTRLTAB         319
#define SET_PROCS           320

#define TAB_FRAME           400
#define TAB_ICON0           401
#define TAB_ICON1           402
#define TAB_ICON2           403
#define TAB_NAME            404

#define IDM_HELP            999
#define IDM_CLOSE           1000
#define IDM_SHOW            1001
#define IDM_HIDE            1002
#define IDM_KILL            1003
#define IDM_MOVEDESKTOP     1005
#define IDM_SETDATETIME     1007
#define IDM_WPSCLOCK        1008
#define IDM_ABOUT           1009
#define IDM_OPENFM2         1010
#define IDM_OPENFM2FOLDER   1011
#define IDM_OPENOS2WIN      1012
#define IDM_OPENOS2FULL     1013
#define IDM_OPENDOSWIN      1014
#define IDM_OPENDOSFULL     1015
#define IDM_OPENWINOS2      1016
#define IDM_OPENDRIVES      1017
#define IDM_OPENFONTPALETTE 1018
#define IDM_OPENSOLIDCOLOR  1019
#define IDM_OPENHIRESCOLOR  1020
#define IDM_OPENTEMPLATES   1021
#define IDM_OPENSETUP       1022
#define IDM_MINIMIZEALL     1026
#define IDM_RESTOREALL      1027
#define IDM_RESETDESKTOPS   1028
#define IDM_DEBUGINFO       1029
#define IDM_MOVEFOLDERS     1030
#define IDM_RUN             1031
#define IDM_SETUP           1032
#define IDM_OPENLAUNCHPAD   1034
#define IDM_OPENCLOCK       1036
#define IDM_RELOADTEXCLUDES 1037
#define IDM_RELOADVEXCLUDES 1038
#define IDM_EDITTEXCLUDES   1039
#define IDM_EDITVEXCLUDES   1040
#define IDM_LOGO            1045
#define IDM_REMOVEFROMLIST  1046
#define IDM_JUMPABLE        1047

#define MENU_FIRST          10000

#define UM_SHOW             (WM_USER + 1)
#define UM_HIDE             (WM_USER + 2)
#define UM_FOCUSME          (WM_USER + 3)
#define UM_MENUEND          (WM_USER + 4)
#define UM_TIMER            (WM_USER + 5)
#define UM_CONTEXTMENU      (WM_USER + 6)
#define UM_CLOSE            (WM_USER + 7)
#define UM_FREE             (WM_USER + 8)
#define UM_TEST             (WM_USER + 9)
#define UM_SETUP            (WM_USER + 10)
#define UM_MOVEDESKTOP      (WM_USER + 11)
#define UM_ALTTAB           (WM_USER + 12)
#define UM_GETLIST          (WM_USER + 13)
#define UM_PAINT            (WM_USER + 14)
#define UM_PROCS            (WM_USER + 15)
#define UM_TASKLIST         (WM_USER + 16)

#define WM_TASKLIST         0x00002f43

typedef struct SWENTRYLIST {
  SWENTRY             swe;
  USHORT              id;
  struct SWENTRYLIST *prev;
  struct SWENTRYLIST *next;
} SWENTRYLIST;

typedef struct {
  USHORT       size;
  USHORT       flags;
  PID          pid;
  ULONG        ulCount;
  SWENTRYLIST *swehead;
  SWENTRYLIST *swetail;
  HWND         hwndMenu;
  BOOL         killme;
} TASKDATA;

typedef struct {
  USHORT size;
  USHORT dummy;
  PID    pid;
  HWND   hwnd;
  ULONG  delay;
} KILLWIN;

typedef struct MENUSTUFF {
  USHORT            size;   // size of structure
  USHORT            type;   // MIS_* constants
  USHORT            id;     // id of menu item
  USHORT            dummy;
  STARTDATA        *sd;     // program to start, if any
  struct MENUSTUFF *next;   // points to next menu item, if any
  struct MENUSTUFF *prev;   // points to previous menu item, if any
  struct MENUSTUFF *parent; // points to parent menu, if any
  struct MENUSTUFF *sub;    // if type indicates submenu, this is it
} MENUSTUFF;

#ifdef DEFINE_GLOBALS
  #define DATADEF
#else
  #define DATADEF extern
#endif

DATADEF HPOINTER   hptrApp,hptrBlank;
DATADEF LONG       xVirtual,yVirtual;
DATADEF ULONG      ulAnimate;
DATADEF BOOL       fReposMouse,fMoveFolders,fExtravagent,fDisableDesktops,
                   fShowMiniwins,fProcs;
DATADEF HWND       hwndAppMenu;
DATADEF HINI       tbarprof;
DATADEF MENUSTUFF *menuhead;
DATADEF CHAR     **texclude,**vexclude;
DATADEF CHAR      *pgmname;

extern LONG        xScreen,yScreen,xIcon,yIcon,yNext,yHi;
extern PID         dtPid;
extern HWND        hwndDesktop,hwndWPS,hwndBottom,hwndTaskbar;
extern BOOL        fWinShowing,fMenuShowing,fSlidingFocus,fNoZorderChange,
                   fB2Bottom,fB3Close,fLeft,fRight,fCenter,fDlgShowing,
                   fB3DblClk,fKeyShowing,fNoHot,fLeftHot,fWrapMouse,
                   fCtrlTab,fStayUp;

/* findwps.c */
VOID FindDesktop (HAB hab);

/* The end */
