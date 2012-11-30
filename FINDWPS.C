#define INCL_DOS
#define INCL_WIN

#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "taskbar.h"


VOID FindDesktop (HAB hab) {

  CHAR  Class[24];
  HENUM henum;
  TID   tid;

  /* Get to bottommost window handle of the desktop */
  hwndBottom = WinQueryWindow(HWND_DESKTOP, QW_BOTTOM);
  /* Enumerate all windows at desktop z-order */
  henum = WinBeginEnumWindows(hwndBottom);
  hwndWPS = (HWND)0;
  while((hwndWPS = WinGetNextWindow(henum)) != NULLHANDLE) {
    /* Now get the class name of that window handle */
    WinQueryClassName(hwndWPS,sizeof(Class),(PCH)Class);
    if(!strcmp(Class,"#37"))          /* desktop class */
      break;
  }
  WinEndEnumWindows(henum);

  /* Without the WPS installed we can only get the desktop window handle */
  hwndDesktop = WinQueryDesktopWindow(hab,NULLHANDLE);

  /* find the PID of the desktop process */
  WinQueryWindowProcess((hwndWPS) ? hwndWPS : (hwndDesktop) ?
                        hwndDesktop : hwndBottom,&dtPid,&tid);
}
