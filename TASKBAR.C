
/****************************************************************************

    FM/2 Taskbar is a WPS enhancement
    $Id: TASKBAR.C,v 1.6 2003/11/01 08:13:55 root Exp $

    Copyright (c) 2001 by Mark Kimes
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

Revisions	19 Aug 01 MK - Baseline
		19 Apr 03 SHL - main: disable Keep up debug code
		19 Apr 03 SHL - SwitchButtonProc: Add missing background paint
		27 May 03 SHL - LoadTasklist: Delete WPS suppress code to restore v1.01 behavior
		27 May 03 SHL - TabIconProc: add missing background paint
		30 May 03 SHL - TabProc: force show to to ensure old icon erased
		31 May 03 SHL - TabProc: try again - use blank icon.  Hide corrupts title
		01 Nov 03 SHL - Clean up odd version number scheme

*****************************************************************************/

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_WIN
#define INCL_GPI

#define DEFINE_GLOBALS

#include <os2.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>

#include "taskhook.h"
#include "taskbar.h"
#include "procstat.h"

//#pragma data_seg(DATA1)

#pragma alloc_text(ONCE,main,QuitThread,CleanUp,StartUpProc,AddToList,LoadExcludes)
#pragma alloc_text(MISC1,OpenObject,OpenPgm,NormalizeAll,strip_trail_char,strip_lead_char,RunProc)
#pragma alloc_text(MISC2,SetupProc)
#pragma alloc_text(STATUS,StatusProc)
#pragma alloc_text(TAB,TabProc,TabIconProc)

/* strip all leading spaces and tabs from a string */
#define lstrip(s)         strip_lead_char(" \t",(s))
/* strip all trailing spaces and tabs from a string */
#define rstrip(s)         strip_trail_char(" \t",(s))
/*strip all trailing linefeeds and carriage returns from a string */
#define stripcr(s)        strip_trail_char("\r\n",(s))

static PFNWP PFNWPStaticProc;


CHAR * strip_trail_char (register CHAR *strip,register CHAR *a) {

  register CHAR *p;

  if(a && *a && strip && *strip) {
    p = a + (strlen(a) - 1);
    while (*a && strchr(strip,*p) != NULL) {
      *p = 0;
      p--;
    }
  }
  return a;
}


CHAR * strip_lead_char (register CHAR *strip,register CHAR *a) {

  register CHAR *p = a;

  if(a && *a && strip && *strip) {
    while(*p && strchr(strip,*p) != NULL)
      p++;
    if(p != a)
      memmove(a,p,strlen(p) + 1);
  }
  return a;
}


INT AddToList (CHAR *string,CHAR ***list,INT *numfiles,INT *numalloced) {

  /* Add a string to an array of strings */

  CHAR **test;

  if(string) {
    if((*numfiles + 2) > *numalloced) {
      if((test = realloc(*list,(((*numalloced) + 4) * sizeof(CHAR *)))) == NULL)
        return 1;
      (*numalloced) += 4;
      *list = test;
    }
    if(((*list)[*numfiles] = malloc(strlen(string) + 1)) == NULL)
      return 2;
    strcpy((*list)[*numfiles],string);
    (*numfiles)++;
    (*list)[*numfiles] = NULL;
    (*list)[(*numfiles) + 1] = NULL;
  }
  return 0;
}


CHAR **LoadExcludes (CHAR *filename) {

  /* Build a list of titles */

  FILE  *fp;
  INT    numnames = 0,numalloc = 0;
  CHAR **names = NULL,input[CCHMAXPATH];

  if(!filename || !*filename)
    filename = "TEXCLUDE.LST";
  fp = fopen(filename,"r");
  if(fp) {
    while(!feof(fp)) {
      if(!fgets(input,CCHMAXPATH,fp))
        break;
      input[CCHMAXPATH - 1] = 0;
      stripcr(input);
      if(*input == ';')
        continue;
      lstrip(rstrip(input));
      if(*input)
        if(AddToList(input,&names,&numnames,&numalloc))
          break;
    }
    if(names && numnames && numnames + 1 < numalloc) /* pare memory down */
      names = realloc(names,sizeof(CHAR *) * (numnames + 1));
    fclose(fp);
  }
  return names;
}


VOID QuitThread (VOID *args) {

  HAB       hab2;
  HMQ       hmq2;
  KILLWIN  *kw = args;

  if(kw) {
    hab2 = WinInitialize(0);
    if(hab2) {
      hmq2 = WinCreateMsgQueue(hab2,0);
      if(hmq2) {
        WinCancelShutdown(hmq2,TRUE);
        PostMsg(kw->hwnd,WM_SAVEAPPLICATION,MPVOID,MPVOID);
        DosSleep(100L);
        PostMsg(kw->hwnd,WM_SYSCOMMAND,MPFROM2SHORT(SC_CLOSE,0),MPVOID);
        DosSleep(kw->delay);
        if(WinIsWindow(hab2,kw->hwnd))
          DosKillProcess(DKP_PROCESS,kw->pid);
        WinDestroyMsgQueue(hmq2);
      }
      WinTerminate(hab2);
    }
    free(kw);
    _heapmin();
  }
}


BOOL OpenPgm (HWND hwnd,CHAR *exename,CHAR *args,CHAR *progtype) {

  CHAR settings[1024 + CCHMAXPATH + 80];
  BOOL ret;

  sprintf(settings,"EXENAME=%s;PROGTYPE=%s;PARAMETERS=%s;OPEN=DEFAULT",
          exename,progtype,args);
  if(hwnd)
    WinSetFocus(HWND_DESKTOP,HWND_DESKTOP);
  ret = WinCreateObject("WPProgram",NULL,settings,"<WP_NOWHERE>",
                        CO_REPLACEIFEXISTS);
  if(!ret && hwnd)
    WinSetWindowPos(hwnd,HWND_TOP,0,0,0,0,SWP_ZORDER | SWP_ACTIVATE);
  return ret;
}


BOOL OpenObject (HWND hwnd,CHAR *name,CHAR *type) {

  HOBJECT hWPSObject;
  CHAR    s[512];

  hWPSObject = WinQueryObject(name);
  if(hWPSObject != NULLHANDLE) {
    WinSetFocus(HWND_DESKTOP,HWND_DESKTOP);
    sprintf(s,"OPEN=%s",(type) ? type : "DEFAULT");
    if(!WinSetObjectData(hWPSObject,s)) {
      if(hwnd)
        WinSetWindowPos(hwnd,HWND_TOP,0,0,0,0,SWP_ACTIVATE | SWP_ZORDER);
    }
    else
      return TRUE;
  }
  return FALSE;
}


VOID UpdateVals (HWND hwnd) {

  xScreen     = WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN);
  xIcon       = WinQuerySysValue(HWND_DESKTOP,SV_CXICON);
  yScreen     = WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN);
  yIcon       = WinQuerySysValue(HWND_DESKTOP,SV_CYICON);
  FindDesktop(WinQueryAnchorBlock(hwnd));
}


VOID SetPresParams (HWND hwnd,RGB2 *back,RGB2 *fore,RGB2 *border,CHAR *font) {

  if(font)
    WinSetPresParam(hwnd,PP_FONTNAMESIZE,strlen(font) + 1L,
                    (PVOID)font);
  if(back)
    WinSetPresParam(hwnd,PP_BACKGROUNDCOLOR,sizeof(RGB2),
                    (PVOID)back);
  if(fore)
    WinSetPresParam(hwnd,PP_FOREGROUNDCOLOR,sizeof(RGB2),
                    (PVOID)fore);
  if(border)
    WinSetPresParam(hwnd,PP_BORDERCOLOR,sizeof(RGB2),
                    (PVOID)border);
}


VOID PaintRecessedWindow (HWND hwnd,HPS hps,BOOL outtie,BOOL dbl) {

  /*
   * paint a recessed box around the window
   * two pixels width required around window for painting...
   */
  BOOL releaseme = FALSE;

  if(!hps) {
    hps = WinGetPS(WinQueryWindow(hwnd,QW_PARENT));
    releaseme = TRUE;
  }
  if(hps) {

    POINTL ptl;
    SWP    swp;

    WinQueryWindowPos(hwnd,&swp);
    ptl.x = swp.x - 1;
    ptl.y = swp.y - 1;
    GpiMove(hps,&ptl);
    if(!outtie)
      GpiSetColor(hps,CLR_WHITE);
    else
      GpiSetColor(hps,CLR_DARKGRAY);
    ptl.x = swp.x + swp.cx;
    GpiLine(hps,&ptl);
    ptl.y = swp.y + swp.cy;
    GpiLine(hps,&ptl);
    if(dbl) {
      ptl.x = swp.x - 2;
      ptl.y = swp.y - 2;
      GpiMove(hps,&ptl);
      ptl.x = swp.x + swp.cx + 1;
      GpiLine(hps,&ptl);
      ptl.y = swp.y + swp.cy + 1;
      GpiLine(hps,&ptl);
    }
    if(!outtie)
      GpiSetColor(hps,CLR_DARKGRAY);
    else
      GpiSetColor(hps,CLR_WHITE);
    if(dbl) {
      ptl.x = swp.x - 2;
      GpiLine(hps,&ptl);
      ptl.y = swp.y - 2;
      GpiLine(hps,&ptl);
      ptl.x = swp.x + swp.cx;
      ptl.y = swp.y + swp.cy;
      GpiMove(hps,&ptl);
    }
    ptl.x = swp.x - 1;
    GpiLine(hps,&ptl);
    ptl.y = swp.y - 1;
    GpiLine(hps,&ptl);
    GpiSetColor(hps,CLR_PALEGRAY);
    ptl.x = swp.x - (2 + (dbl != FALSE));
    ptl.y = swp.y - (2 + (dbl != FALSE));
    GpiMove(hps,&ptl);
    ptl.x = swp.x + swp.cx + (1 + (dbl != FALSE));
    GpiLine(hps,&ptl);
    ptl.y = swp.y + swp.cy + (1 + (dbl != FALSE));
    GpiLine(hps,&ptl);
    ptl.x = swp.x - (2 + (dbl != FALSE));
    GpiLine(hps,&ptl);
    ptl.y = swp.y - (2 + (dbl != FALSE));
    GpiLine(hps,&ptl);
    if(releaseme)
      WinReleasePS(hps);
  }
}


VOID FreeTasklist (SWENTRYLIST *head) {

  SWENTRYLIST *info,*next;

  info = head;
  while(info) {
    next = info->next;
    free(info);
    info = next;
  }
  _heapmin();
}


ULONG LoadTasklist (PID pid,SWENTRYLIST **prevhead,SWENTRYLIST **prevtail) {

  PSWBLOCK       pswb;
  ULONG          ulSize;
  SWENTRYLIST   *info,*last = NULL;
  register INT   i,x;
  register ULONG ulCount;

  if(*prevhead) {
    FreeTasklist(*prevhead);
    *prevhead = NULL;
  }
  *prevtail = NULL;
  /* Get the switch list information */
  ulCount = WinQuerySwitchList(0,NULL,0);
  ulSize = sizeof(SWBLOCK) + sizeof(HSWITCH) + (ulCount + 4L) *
           (LONG)sizeof(SWENTRY);
  /* Allocate memory for list */
  if((pswb = malloc((unsigned)ulSize)) != NULL) {
    /* Put the info in the list */
    WinQuerySwitchList(0,pswb,ulSize - sizeof(SWENTRY));
    /* do the dirty deed */
    ulCount = 0;
    for(i = 0;i < pswb->cswentry;i++) {
      if (pswb->aswentry[i].swctl.uchVisibility == SWL_VISIBLE &&
          pswb->aswentry[i].swctl.idProcess != pid)
      {
        if(texclude) {
          for(x = 0;texclude[x];x++) {
            if(!strncmp(texclude[x],pswb->aswentry[i].swctl.szSwtitle,
                        strlen(texclude[x])))
              break;
          }
          if(texclude[x])
            continue;
        }
        info = malloc(sizeof(SWENTRYLIST));
        if(info) {
          ulCount++;
          memcpy(&info->swe,&(pswb->aswentry[i]),sizeof(SWENTRY));
          info->id = 0;
          if(!last) {
            *prevhead = *prevtail = info;
            info->prev = NULL;
          }
          else {
            last->next = info;
            info->prev = last;
          }
          info->next = NULL;
          *prevtail = last = info;
        }
      }
    }
    free(pswb);
    _heapmin();
  }
  return ulCount;
}


SWENTRYLIST *FindTasklistID (SWENTRYLIST *head,USHORT id) {

  SWENTRYLIST *info;

  info = head;
  while(info) {
    if(info->id == id)
      break;
    info = info->next;
  }
  return info;
}


SWENTRYLIST *FindTasklistEntry (SWENTRYLIST *head,HSWITCH hswitch) {

  SWENTRYLIST *info;

  info = head;
  while(info) {
    if(info->swe.hswitch == hswitch)
      break;
    info = info->next;
  }
  return info;
}


ULONG DeleteTasklistEntry (SWENTRYLIST **prevhead,SWENTRYLIST **prevtail,
                           HSWITCH hswitch) {

  SWENTRYLIST   *info;
  register ULONG ulCount = 0;

  info = FindTasklistEntry(*prevhead,hswitch);
  if(info) {
    if(info == *prevhead)
      *prevhead = info->next;
    if(info == *prevtail)
      *prevtail = info->prev;
    if(info->prev)
      info->prev->next = info->next;
    if(info->next)
      info->next->prev = info->prev;
    free(info);
  }
  /* count how many we have left */
  info = *prevhead;
  while(info) {
    ulCount++;
    info = info->next;
  }
  return ulCount;
}


ULONG AddTasklistEntry (SWENTRYLIST **prevhead,SWENTRYLIST **prevtail,
                        HSWITCH hswitch) {

  SWENTRYLIST   *info;
  SWCNTRL        swctl;
  register ULONG ulCount = 0;

  if(!WinQuerySwitchEntry(hswitch,&swctl)) {
    info = FindTasklistEntry(*prevhead,hswitch);
    if(!info) { /* doesn't exist, add entry, else just change entry */
      info = malloc(sizeof(SWENTRYLIST));
      if(info) {
        info->id = 0;
        info->swe.hswitch = hswitch;
        if(!*prevhead)
          *prevhead = info;
        info->prev = *prevtail;
        if(*prevtail)
          (*prevtail)->next = info;
        *prevtail = info;
        info->next = NULL;
      }
    }
    if(info)
      memcpy(&(info->swe.swctl),&swctl,sizeof(SWCNTRL));
  }
  /* count how many we have */
  info = *prevhead;
  while(info) {
    ulCount++;
    info = info->next;
  }
  return ulCount;
}



VOID NormalizeWindow (HWND hwnd) {

  SWP  swp;
  LONG oldx,oldy;

  if(!fDisableDesktops && WinQueryWindowPos(hwnd,&swp)) {
    if(swp.fl & (SWP_HIDE | SWP_MINIMIZE)) {
      swp.x = WinQueryWindowUShort(hwnd,QWS_XRESTORE);
      swp.y = WinQueryWindowUShort(hwnd,QWS_YRESTORE);
    }
    oldx = swp.x;
    oldy = swp.y;
    while(swp.x > xScreen)
      swp.x -= xScreen;
    while(swp.y > yScreen)
      swp.y -= yScreen;
    while(swp.x + swp.cx < 4)
      swp.x += xScreen;
    while(swp.y + swp.cy < 4)
      swp.y += yScreen;
    if(oldx != swp.x || oldy != swp.y) {
      if(swp.fl & (SWP_HIDE | SWP_MINIMIZE)) {
        WinSetWindowUShort(hwnd,QWS_XRESTORE,swp.x);
        WinSetWindowUShort(hwnd,QWS_YRESTORE,swp.y);
      }
      else
        WinSetWindowPos(hwnd,HWND_TOP,swp.x,swp.y,0,0,
                        SWP_MOVE | SWP_NOADJUST);
    }
  }
}


VOID NormalizeAll (TASKDATA *td,BOOL foldersonly) {

  HENUM henum;            /* Window handle of WC_FRAME class Desktop */
  HWND  hwnd;             /* Window handles of enumerated application */
  CHAR  ucClassname[24];  /* Class name of enumerated application */
  CHAR  ucWindowText[33]; /* Window text of enumerated application */

  if(fDisableDesktops)
    return;
  /* Enumerate all descendants of HWND_DESKTOP,
     which are the frame windows seen on Desktop,
     but not having necessarily the class WC_FRAME */
  henum = WinBeginEnumWindows(HWND_DESKTOP);
  if(henum != (HENUM)0) {
    while((hwnd = WinGetNextWindow(henum)) != (HWND)0) {
      WinQueryClassName(hwnd,sizeof(ucClassname),ucClassname);
      WinQueryWindowText(hwnd,sizeof(ucWindowText),ucWindowText);
      /* Only move WC_FRAME class (#1) windows. */
      if(strcmp(ucClassname,"#1") && strcmp(ucClassname,"wpFolder window"))
        continue;
      if(foldersonly && !strcmp(ucClassname,"#1"))
        continue;
      NormalizeWindow(hwnd);
    }
    WinEndEnumWindows(henum);    /* End enumeration */
  }

  if(td && td->swehead) {

    SWENTRYLIST *info;

    info = td->swehead;
    while(info) {
      if(info->swe.swctl.hwnd != hwndDesktop &&
         info->swe.swctl.hwnd != hwndWPS &&
         info->swe.swctl.hwnd != hwndBottom)
        NormalizeWindow(info->swe.swctl.hwnd);
      info = info->next;
    }
  }
}

MRESULT EXPENTRY TabIconProc (HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{
  PFNWP oldproc = (PFNWP)WinQueryWindowPtr(hwnd,0);

  switch(msg) {
    case WM_DESTROY:
      return 0;
  } // switch
  return oldproc(hwnd,msg,mp1,mp2);
}


MRESULT EXPENTRY TabProc (HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{

  TASKDATA           *td;
  static SWENTRYLIST *start;

  switch(msg) {
    case WM_INITDLG:
      if(!(WinGetKeyState(HWND_DESKTOP, VK_CTRL) & 0x8000) ||
         !(WinGetKeyState(HWND_DESKTOP, VK_ALT) & 0x8000) ||
         !mp2)
      {
        WinDismissDlg(hwnd,0);
        DosBeep(50,100);
        return 0;
      }
      WinSetWindowPtr(hwnd,0,mp2);
      td = mp2;
      {
        HWND      hwndActive;
        PFNWP     oldproc;
        HPOINTER  hptr;
        INT       x;

        for(x = TAB_ICON0;x < TAB_ICON2 + 1;x++) {
          hptr = (HPOINTER)WinSendDlgItemMsg(hwnd,x,SM_QUERYHANDLE,
                                             MPVOID,MPVOID);
          if(hptr)
            WinDestroyPointer(hptr);
          oldproc = WinSubclassWindow(WinWindowFromID(hwnd,x),
                                      (PFNWP)TabIconProc);
          if(oldproc)
            WinSetWindowPtr(WinWindowFromID(hwnd,x),0,(PVOID)oldproc);
        }
        hwndActive = WinQueryActiveWindow(HWND_DESKTOP);
        start = td->swehead;
        while(start) {
          if(start->swe.swctl.hwnd == hwndActive)
            break;
          start = start->next;
        }
        if(!start)
          start = td->swehead;
        if(!PostMsg(hwnd,UM_TASKLIST,MPVOID,MPVOID))
          WinSendMsg(hwnd,UM_TASKLIST,MPVOID,MPVOID);
      }
      break;

    case UM_TASKLIST:
    case UM_TASKLIST + 1:
      td = WinQueryWindowPtr(hwnd,0);
      if(!td) {
        WinDismissDlg(hwnd,0);
        DosBeep(50,100);
        return 0;
      }
      if(!start) {
        DosBeep(50,100);
        WinDismissDlg(hwnd,0);
      }
      else
      {
        HPOINTER      hptr;
        SWENTRYLIST  *next,*prev;
        CHAR          s[CCHMAXPATH],*p;

	/* Do 2 stage update bacause the obvious hide/show solution
	 * corrupts the title backgroud
	 * Appears to be a PM dialog bug until proven otherwise
	 */
        strcpy(s,start->swe.swctl.szSwtitle);
        p = s;
        while(*p) {
          if(*p == '\n' || *p == '\r')
            *p = ' ';
          p++;
        }
	if (msg == UM_TASKLIST + 1)
          WinSetDlgItemText(hwnd,TAB_NAME,s);
	if (msg == UM_TASKLIST)
          hptr = hptrBlank;
	else
	{
          hptr = (HPOINTER)WinSendMsg(start->swe.swctl.hwnd,
                                      WM_QUERYICON,MPVOID,MPVOID);
          if(hptr == (HPOINTER)0)
            hptr = hptrApp;
	}
        WinSendDlgItemMsg(hwnd,TAB_ICON1,SM_SETHANDLE, MPFROMLONG(hptr),MPVOID);
        prev = start->prev;
        if(!prev)
          prev = td->swetail;
        if(prev == start)
          prev = NULL;
        if (!prev || msg == UM_TASKLIST)
          hptr = hptrBlank;
        else
	{
          hptr = (HPOINTER)WinSendMsg(prev->swe.swctl.hwnd,
                                      WM_QUERYICON,MPVOID,MPVOID);
          if(hptr == (HPOINTER)0)
            hptr = hptrApp;
        }
        WinSendDlgItemMsg(hwnd,TAB_ICON0,SM_SETHANDLE, MPFROMLONG(hptr),MPVOID);
        next = start->next;
        if(!next)
          next = td->swehead;
        if(next == start)
          next = NULL;
        if(!next || msg == UM_TASKLIST)
          hptr = hptrBlank;
        else
	{
          hptr = (HPOINTER)WinSendMsg(next->swe.swctl.hwnd,
                                      WM_QUERYICON,MPVOID,MPVOID);
          if(hptr == (HPOINTER)0)
            hptr = hptrApp;
        }
        WinSendDlgItemMsg(hwnd,TAB_ICON2,SM_SETHANDLE, MPFROMLONG(hptr),MPVOID);
	if (msg == UM_TASKLIST)
          WinPostMsg(hwnd,UM_TASKLIST+1,MPVOID,MPVOID);
      }
      return 0;

    case WM_CHAR:
      td = WinQueryWindowPtr(hwnd,0);
      if(!td)
      {
        WinDismissDlg(hwnd,0);
        DosBeep(50,100);
      }
      else
      {
        USHORT  usFlags    = SHORT1FROMMP(mp1);
        UCHAR   ucScanCode = CHAR4FROMMP(mp1);

        /* Scan code usage
         * 1	Esc
         * 15	Tab
         * 51	, <
         * 52	. >
         * 44	Z
         * 45	X
         */

        if(!td ||
	   (usFlags & (KC_SCANCODE | KC_CTRL | KC_ALT)) != (KC_SCANCODE | KC_CTRL | KC_ALT) ||
	   ucScanCode == 1)
        {
          if(!td || ucScanCode == 1)
	  {
            WinDismissDlg(hwnd,0);
	    return 0;
	  }
	  else
	  {
	    // Ctrl or Alt - released - set new current program
            {
              SWP   swp;
              ULONG fl = SWP_SHOW | SWP_ZORDER | SWP_ACTIVATE;

              WinQueryWindowPos(start->swe.swctl.hwnd,&swp);
              if(swp.fl & SWP_MINIMIZE)
                fl |= SWP_RESTORE;
              WinSetWindowPos(start->swe.swctl.hwnd,HWND_TOP,0,0,0,0,fl);
              WinQueryWindowPos(start->swe.swctl.hwnd,&swp);
              if(!fDisableDesktops &&
                 !WinIsWindowShowing(start->swe.swctl.hwnd)) {
                /* calculate the "desktop" in which the window lies */

                LONG   lx,ly,offx,offy;
                double dx,dy;

                /* where is it from current desktop? */
                dx = (double)(swp.x + (swp.cx / 2)) / (double)xScreen;
                dy = (double)(swp.y + (swp.cy / 2)) / (double)yScreen;
                lx = floor(dx);
                ly = floor(dy);
                /* "quadrant" of current desktop? */
                offx = (xVirtual / xScreen) + 2;
                offy = (yVirtual / yScreen) + 2;
                /* add together and we have new desktop we want */
                lx += offx;
                ly += offy;
                WinSendMsg(WinWindowFromID(hwndTaskbar,MAIN_VIRTUAL),
                           UM_MOVEDESKTOP,MPFROMLONG(lx),MPFROMLONG(ly));
              }
              WinQueryWindowPos(start->swe.swctl.hwnd,&swp);
              if(fReposMouse)
                WinSetPointerPos(HWND_DESKTOP,swp.x + (swp.cx / 2),
                                 swp.y + (swp.cy / 2));
            }
            WinSwitchToProgram(start->swe.hswitch);
            WinDismissDlg(hwnd,1);
          }
        }
        if(usFlags & KC_KEYUP)  /* ignore key releases */
          break;
        if((fCtrlTab && ucScanCode == 15) ||
           (!fLeftHot && (ucScanCode == 51 || ucScanCode == 52)) ||
           (fLeftHot && (ucScanCode == 44 || ucScanCode == 45)))
        {
            /* set icons */
            switch(ucScanCode) {
            case 45:
            case 52:
              start = start->next;
              if(!start)
                start = td->swehead;
              if(!start) {
                DosBeep(50,100);
                WinDismissDlg(hwnd,0);
                return 0;
              }
              break;
            case 15:
            case 44:
            case 51:
              start = start->prev;
              if(!start)
                start = td->swetail;
              if(!start) {
                DosBeep(50,100);
                WinDismissDlg(hwnd,0);
                return 0;
              }
              break;
          }
          WinSendMsg(hwnd,UM_TASKLIST,MPVOID,MPVOID);
        }
      }
      break;

    case WM_COMMAND:
      return 0;
  }
  return WinDefDlgProc(hwnd,msg,mp1,mp2);
}


MRESULT EXPENTRY SetupProc (HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2) {

  switch(msg) {
    case WM_INITDLG:
      WinSendDlgItemMsg(hwnd,SET_HEIGHT,SPBM_SETTEXTLIMIT,MPFROMSHORT(2L),
                        MPVOID);
      WinSendDlgItemMsg(hwnd,SET_HEIGHT,SPBM_OVERRIDESETLIMITS,MPFROMLONG(99L),
                        MPFROMLONG(1L));
      PostMsg(hwnd,UM_SETUP,MPVOID,MPVOID);
      break;

    case UM_SETUP:
      WinCheckButton(hwnd,SET_LEFT,fLeft);
      WinCheckButton(hwnd,SET_RIGHT,fRight);
      WinCheckButton(hwnd,SET_CENTER,fCenter);
      WinCheckButton(hwnd,SET_NOZORDERCHANGE,fNoZorderChange);
      WinCheckButton(hwnd,SET_SLIDINGFOCUS,fSlidingFocus);
      WinCheckButton(hwnd,SET_REPOSMOUSE,fReposMouse);
      WinCheckButton(hwnd,SET_MOVEFOLDERS,fMoveFolders);
      WinCheckButton(hwnd,SET_B2BOTTOM,fB2Bottom);
      WinCheckButton(hwnd,SET_B3CLOSE,fB3Close);
      WinCheckButton(hwnd,SET_B3DBLCLK,fB3DblClk);
      WinCheckButton(hwnd,SET_DISABLEDESKTOPS,fDisableDesktops);
      WinCheckButton(hwnd,SET_ANIMATE,(ulAnimate != 0));
      WinCheckButton(hwnd,SET_NOHOT,(fNoHot != 0));
      WinCheckButton(hwnd,SET_LEFTHOT,(fLeftHot != 0));
      WinCheckButton(hwnd,SET_WRAPMOUSE,(fWrapMouse != 0));
      WinCheckButton(hwnd,SET_SHOWMINIWINS,(fShowMiniwins != 0));
      WinCheckButton(hwnd,SET_CTRLTAB,(fCtrlTab != 0));
      WinCheckButton(hwnd,SET_PROCS,(fProcs != 0));
      WinSendDlgItemMsg(hwnd,SET_HEIGHT,SPBM_SETCURRENTVALUE,
                        MPFROMLONG(yHi),MPVOID);
      return 0;

    case WM_CONTROL:
      switch(SHORT1FROMMP(mp1)) {
        case SET_LEFT:
        case SET_RIGHT:
        case SET_CENTER:
          {
            BOOL left,right,center;

            left   = WinQueryButtonCheckstate(hwnd,SET_LEFT);
            right  = WinQueryButtonCheckstate(hwnd,SET_RIGHT);
            center = WinQueryButtonCheckstate(hwnd,SET_CENTER);
            if(!left && !right && !center) {
              WinCheckButton(hwnd,SET_LEFT,TRUE);
              WinCheckButton(hwnd,SET_RIGHT,TRUE);
              WinCheckButton(hwnd,SET_CENTER,TRUE);
            }
          }
          break;
      }
      return 0;

    case WM_COMMAND:
      switch(SHORT1FROMMP(mp1)) {
        case SET_UNDO:
          PostMsg(hwnd,UM_SETUP,MPVOID,MPVOID);
          break;

        case DID_OK:
          fLeft   = WinQueryButtonCheckstate(hwnd,SET_LEFT);
          fRight  = WinQueryButtonCheckstate(hwnd,SET_RIGHT);
          fCenter = WinQueryButtonCheckstate(hwnd,SET_CENTER);
          WinSendDlgItemMsg(hwnd,SET_HEIGHT,SPBM_QUERYVALUE,MPFROMP(&yHi),
                            MPFROM2SHORT(0,SPBQ_DONOTUPDATE));
          fNoZorderChange = WinQueryButtonCheckstate(hwnd,SET_NOZORDERCHANGE);
          fSlidingFocus = WinQueryButtonCheckstate(hwnd,SET_SLIDINGFOCUS);
          fReposMouse = WinQueryButtonCheckstate(hwnd,SET_REPOSMOUSE);
          fMoveFolders = WinQueryButtonCheckstate(hwnd,SET_MOVEFOLDERS);
          fB2Bottom = WinQueryButtonCheckstate(hwnd,SET_B2BOTTOM);
          fB3Close = WinQueryButtonCheckstate(hwnd,SET_B3CLOSE);
          fB3DblClk = WinQueryButtonCheckstate(hwnd,SET_B3DBLCLK);
          fDisableDesktops = WinQueryButtonCheckstate(hwnd,SET_DISABLEDESKTOPS);
          fNoHot = WinQueryButtonCheckstate(hwnd,SET_NOHOT);
          fLeftHot = WinQueryButtonCheckstate(hwnd,SET_LEFTHOT);
          fWrapMouse = WinQueryButtonCheckstate(hwnd,SET_WRAPMOUSE);
          fShowMiniwins = WinQueryButtonCheckstate(hwnd,SET_SHOWMINIWINS);
          fCtrlTab = WinQueryButtonCheckstate(hwnd,SET_CTRLTAB);
          fProcs = WinQueryButtonCheckstate(hwnd,SET_PROCS);
          ulAnimate = WinQueryButtonCheckstate(hwnd,SET_ANIMATE);
          if(ulAnimate)
            ulAnimate = WS_ANIMATE;
          WinSetWindowBits(hwndTaskbar,QWL_STYLE,ulAnimate,WS_ANIMATE);
          if(tbarprof) {
            PrfWriteProfileData(tbarprof,"FM/3","LeftShow",
                                &fLeft,sizeof(BOOL));
            PrfWriteProfileData(tbarprof,"FM/3","RightShow",
                                &fRight,sizeof(BOOL));
            PrfWriteProfileData(tbarprof,"FM/3","CenterShow",
                                &fCenter,sizeof(BOOL));
            PrfWriteProfileData(tbarprof,"FM/3","HeightShow",
                                &yHi,sizeof(LONG));
            PrfWriteProfileData(tbarprof,"FM/3","NoZorderChange",
                                &fNoZorderChange,sizeof(BOOL));
            PrfWriteProfileData(tbarprof,"FM/3","SlidingFocus",
                                &fSlidingFocus,sizeof(BOOL));
            PrfWriteProfileData(tbarprof,"FM/3","ReposMouse",
                                &fReposMouse,sizeof(BOOL));
            PrfWriteProfileData(tbarprof,"FM/3","B2Bottom",
                                &fB2Bottom,sizeof(BOOL));
            PrfWriteProfileData(tbarprof,"FM/3","B3Close",
                                &fB3Close,sizeof(BOOL));
            PrfWriteProfileData(tbarprof,"FM/3","B3DblClk",
                                &fB3DblClk,sizeof(BOOL));
            PrfWriteProfileData(tbarprof,"FM/3","MoveFolders",
                                &fMoveFolders,sizeof(BOOL));
            PrfWriteProfileData(tbarprof,"FM/3","DisableDesktops",
                                &fDisableDesktops,sizeof(BOOL));
            PrfWriteProfileData(tbarprof,"FM/3","NoHot",
                                &fNoHot,sizeof(BOOL));
            PrfWriteProfileData(tbarprof,"FM/3","LeftHot",
                                &fLeftHot,sizeof(BOOL));
            PrfWriteProfileData(tbarprof,"FM/3","WrapMouse",
                                &fWrapMouse,sizeof(BOOL));
            PrfWriteProfileData(tbarprof,"FM/3","ShowMiniwins",
                                &fShowMiniwins,sizeof(BOOL));
            PrfWriteProfileData(tbarprof,"FM/3","CtrlAltTab",
                                &fCtrlTab,sizeof(BOOL));
            PrfWriteProfileData(tbarprof,"FM/3","Animate",
                                &ulAnimate,sizeof(ULONG));
            PrfWriteProfileData(tbarprof,"FM/3","Procs",
                                &fProcs,sizeof(BOOL));
            if(!fMoveFolders) {

              TASKDATA *td;

              td = WinQueryWindowPtr(WinQueryWindow(hwnd,QW_PARENT),0);
              if(td)
                NormalizeAll(td,TRUE);
            }
          }
          WinDismissDlg(hwnd,1);
          break;

        case DID_CANCEL:
          WinDismissDlg(hwnd,0);
          break;
      }
      return 0;
  }
  return WinDefDlgProc(hwnd,msg,mp1,mp2);
}


MRESULT EXPENTRY RunProc (HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2) {

  static CHAR lastrun[CCHMAXPATH] = "",lastargs[1024 - CCHMAXPATH] = "",
              lastdir[CCHMAXPATH] = "";

  switch(msg) {
    case WM_INITDLG:
      WinSendDlgItemMsg(hwnd,RUN_NAME,EM_SETTEXTLIMIT,
                        MPFROM2SHORT(CCHMAXPATH,0),MPVOID);
      WinSendDlgItemMsg(hwnd,RUN_DIR,EM_SETTEXTLIMIT,
                        MPFROM2SHORT(CCHMAXPATH,0),MPVOID);
      WinSendDlgItemMsg(hwnd,RUN_ARGS,EM_SETTEXTLIMIT,
                        MPFROM2SHORT(1024 - CCHMAXPATH,0),MPVOID);
      WinSetDlgItemText(hwnd,RUN_NAME,lastrun);
      WinSetDlgItemText(hwnd,RUN_ARGS,lastargs);
      WinSetDlgItemText(hwnd,RUN_DIR,lastdir);
      WinSendDlgItemMsg(hwnd,RUN_NAME,EM_SETSEL,
                        MPFROM2SHORT(0,CCHMAXPATH),MPVOID);
      WinSendDlgItemMsg(hwnd,RUN_DIR,EM_SETSEL,
                        MPFROM2SHORT(0,CCHMAXPATH),MPVOID);
      WinSendDlgItemMsg(hwnd,RUN_ARGS,EM_SETSEL,
                        MPFROM2SHORT(0,1024),MPVOID);
      if(!*lastrun)
        WinEnableWindow(WinWindowFromID(hwnd,DID_OK),FALSE);
      PostMsg(hwnd,UM_TEST,MPVOID,MPVOID);
      break;

    case UM_TEST:
      {
        CHAR  appname[CCHMAXPATH];
        ULONG apptype;

        *appname = 0;
        WinQueryDlgItemText(hwnd,RUN_NAME,CCHMAXPATH,appname);
        lstrip(rstrip(appname));
        if(*appname) {
          if(!DosQueryAppType(appname,&apptype) &&
            !(apptype & (FAPPTYP_DLL     | FAPPTYP_PHYSDRV |
                         FAPPTYP_PROTDLL | FAPPTYP_VIRTDRV))) {
            WinEnableWindow(WinWindowFromID(hwnd,RUN_DIR),TRUE);
            WinEnableWindow(WinWindowFromID(hwnd,RUN_DIRHDR),TRUE);
            WinEnableWindow(WinWindowFromID(hwnd,RUN_ARGS),TRUE);
            WinEnableWindow(WinWindowFromID(hwnd,RUN_ARGSHDR),TRUE);
            break;
          }
        }
        WinSetDlgItemText(hwnd,RUN_DIR,"");
        WinSetDlgItemText(hwnd,RUN_ARGS,"");
        WinEnableWindow(WinWindowFromID(hwnd,RUN_DIR),FALSE);
        WinEnableWindow(WinWindowFromID(hwnd,RUN_DIRHDR),FALSE);
        WinEnableWindow(WinWindowFromID(hwnd,RUN_ARGS),FALSE);
        WinEnableWindow(WinWindowFromID(hwnd,RUN_ARGSHDR),FALSE);
      }
      return 0;

    case WM_CONTROL:
      switch(SHORT1FROMMP(mp1)) {
        case RUN_NAME:
          switch(SHORT2FROMMP(mp1)) {
            case EN_KILLFOCUS:
              WinSendMsg(hwnd,UM_TEST,MPVOID,MPVOID);
              break;
            case EN_CHANGE:
              {
                CHAR test[CCHMAXPATH];

                *test = 0;
                WinQueryDlgItemText(hwnd,RUN_NAME,CCHMAXPATH,test);
                lstrip(rstrip(test));
                WinEnableWindow(WinWindowFromID(hwnd,DID_OK),(*test != 0));
              }
              break;
          }
          break;
      }
      return 0;

    case WM_COMMAND:
      switch(SHORT1FROMMP(mp1)) {
        case DID_OK:
          {
            CHAR    build[1024],appname[CCHMAXPATH];
            HOBJECT hWPSObject;

            *appname = 0;
            WinQueryDlgItemText(hwnd,RUN_NAME,CCHMAXPATH,appname);
            lstrip(rstrip(appname));
            if(!*appname) {
              WinSetDlgItemText(hwnd,RUN_NAME,"");
              WinEnableWindow(WinWindowFromID(hwnd,DID_OK),FALSE);
              WinSetFocus(HWND_DESKTOP,WinWindowFromID(hwnd,RUN_NAME));
              DosBeep(50,100);
              break;
            }
            strcpy(lastrun,appname);
            WinSetDlgItemText(hwnd,RUN_NAME,lastrun);
            *lastargs = 0;
            WinQueryDlgItemText(hwnd,RUN_ARGS,1024 - CCHMAXPATH,lastargs);
            *lastdir = 0;
            WinQueryDlgItemText(hwnd,RUN_DIR,CCHMAXPATH,lastdir);
            sprintf(build,"OPEN=DEFAULT%s%s%s%s",
                    (*lastargs) ? ";PARAMETERS=" : "",
                    lastargs,
                    (*lastdir) ? ";STARTDIR=" : "",
                    lastdir);
            hWPSObject = WinQueryObject(lastrun);
            if(hWPSObject != NULLHANDLE) {
              WinSetFocus(HWND_DESKTOP,HWND_DESKTOP);
              WinSetObjectData(hWPSObject,build);
              WinSetWindowPos(hwnd,HWND_TOP,0,0,0,0,
                              SWP_ACTIVATE | SWP_ZORDER);
            }
          }
          WinDismissDlg(hwnd,1);
          break;

        case RUN_FIND:
          {
            FILEDLG fdlg;
            CHAR    drive[3] = " :",*pdrive = drive,*p;
            ULONG   apptype;

            memset(&fdlg,0,sizeof(FILEDLG));
            fdlg.cbSize =       (ULONG)sizeof(FILEDLG);
            fdlg.fl     =       FDS_CENTER | FDS_OPEN_DIALOG;
            fdlg.pszTitle =     "Select something to run/open";
            fdlg.pszOKButton =  "Okay";
            if(isalpha(*lastrun))
              *drive = toupper(*lastrun);
            else
              *drive = 'C';
            fdlg.pszIDrive = pdrive;
            strcpy(fdlg.szFullFile,lastrun);
            p = strrchr(fdlg.szFullFile,'\\');
            if(p) {
              p++;
              *p = 0;
            }
            else
              strcat(fdlg.szFullFile,"\\");
            strcat(fdlg.szFullFile,"*");
            if(*lastrun) {
              p = strrchr(lastrun,'\\');
              if(p) {
                p = strrchr(p,'.');
                if(p) {
                  if(*(p + 1))
                    strcat(fdlg.szFullFile,p);
                }
              }
            }
            if(WinFileDlg(HWND_DESKTOP,hwnd,&fdlg)) {
              if(fdlg.lReturn != DID_CANCEL && !fdlg.lSRC) {
                WinSetDlgItemText(hwnd,RUN_NAME,fdlg.szFullFile);
                strcat(lastrun,fdlg.szFullFile);
                if(!DosQueryAppType(fdlg.szFullFile,&apptype) &&
                  !(apptype & (FAPPTYP_DLL     | FAPPTYP_PHYSDRV |
                               FAPPTYP_PROTDLL | FAPPTYP_VIRTDRV))) {
                  p = fdlg.szFullFile;
                  while(*p) {
                    if(*p == '/')
                      *p = '\\';
                    p++;
                  }
                  p = strrchr(fdlg.szFullFile,'\\');
                  if(p)
                    *p = 0;
                  WinSetDlgItemText(hwnd,RUN_DIR,fdlg.szFullFile);
                  WinSendDlgItemMsg(hwnd,RUN_DIR,EM_SETSEL,
                                    MPFROM2SHORT(0,CCHMAXPATH),MPVOID);
                  WinSendDlgItemMsg(hwnd,RUN_NAME,EM_SETSEL,
                                    MPFROM2SHORT(((p) ? p -
                                                        fdlg.szFullFile : 0),
                                                 CCHMAXPATH),
                                    MPVOID);
                  WinSetFocus(HWND_DESKTOP,WinWindowFromID(hwnd,RUN_ARGS));
                }
                else
                  WinSetFocus(HWND_DESKTOP,WinWindowFromID(hwnd,RUN_NAME));
                WinSendMsg(hwnd,UM_TEST,MPVOID,MPVOID);
              }
            }
          }
          break;

        case DID_CANCEL:
          WinDismissDlg(hwnd,0);
          break;
      }
      return 0;
  }
  return WinDefDlgProc(hwnd,msg,mp1,mp2);
}


MRESULT EXPENTRY VirtualProc (HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2) {

  switch(msg) {
    case WM_CONTEXTMENU:
      {
        POINTL ptl;

        if(hwndAppMenu) {
          WinQueryPointerPos(HWND_DESKTOP,&ptl);
          ptl.y = yNext + yIcon + 5;
          WinMapWindowPoints(HWND_DESKTOP,hwnd,&ptl,1L);
          if(WinPopupMenu(hwnd,hwnd,hwndAppMenu,ptl.x,ptl.y,0,
                          PU_HCONSTRAIN | PU_VCONSTRAIN |
                          PU_KEYBOARD   | PU_MOUSEBUTTON1))
            fMenuShowing = TRUE;
        }
      }
      break;

    case UM_MENUEND:
      fMenuShowing = FALSE;
      return 0;

    case WM_MENUEND:
      PostMsg(hwnd,UM_MENUEND,mp1,mp2);
      break;

    case WM_BUTTON1CLICK:
      if(!fDisableDesktops) {

        LONG   xpos,ypos,virtx,virty,virtorigx,virtorigy;
        double px,py;
        SWP    swp;

        xpos = SHORT1FROMMP(mp1);
        ypos = SHORT2FROMMP(mp1);
        WinQueryWindowPos(hwnd, &swp);
        /* determine "desktop" picked */
        px = (double)xpos / (double)swp.cx;
        py = (double)ypos / (double)swp.cy;
        virtx = (px > .33333) ? ((px > .66666) ? 3 : 2) : 1;
        virty = (py > .33333) ? ((py > .66666) ? 3 : 2) : 1;
        /* calculate x and y offsets to get from this "desktop" to that */
        virtorigx = (virtx - 2) * xScreen;
        virtorigy = (virty - 2) * yScreen;
        if(virtorigx != xVirtual || virtorigy != yVirtual)
          WinSendMsg(hwnd,UM_MOVEDESKTOP,MPFROMLONG(virtx),
                     MPFROMLONG(virty));
      }
      break;

    case UM_MOVEDESKTOP:
      if(mp1 && mp2) {

        /* move desktop */
        HENUM   henum;            /* Window handle of WC_FRAME class Desktop */
        HWND    hwndApplication;  /* Window handles of enumerated application */
        HWND    hwndFrame;        /* our frame handle */
        CHAR    ucClassname[24];  /* Class name of enumerated application */
        CHAR    ucWindowText[33]; /* Window text of enumerated application */
        ULONG   ulAppCount = 0;
        LONG    virtorigx,virtorigy,virtx,virty;
        SWP    *swpApps;
        register INT x;

        virtx = ((LONG)mp1);
        virty = ((LONG)mp2);
        if(virtx < 1)
          virtx = 1;
        if(virtx > 3)
          virtx = 3;
        if(virty < 1)
          virty = 1;
        if(virty > 3)
          virty = 3;
        virtorigx = (virtx - 2) * xScreen;
        virtorigy = (virty - 2) * yScreen;
        swpApps = malloc(sizeof(SWP) * 256);
        if(!swpApps)
          break;
        hwndFrame = WinQueryWindow(WinQueryWindow(hwnd,QW_PARENT),QW_PARENT);
        /* Enumerate all descendants of HWND_DESKTOP,
           which are the frame windows seen on Desktop,
           but not having necessarily the class WC_FRAME */
        henum = WinBeginEnumWindows(HWND_DESKTOP);
        /* Begin with offset 0 in first iteration */
        ulAppCount = 0;
        while((hwndApplication = WinGetNextWindow(henum)) != (HWND)0) {
          /* Don't move desktop or us */
          if(hwndApplication != hwndFrame &&
             hwndApplication != hwndDesktop &&
             hwndApplication != hwndWPS &&
             hwndApplication != hwndBottom) {
            WinQueryWindowPos(hwndApplication,
                              &swpApps[ulAppCount]);
            WinQueryClassName(hwndApplication, sizeof(ucClassname),
                              ucClassname);
            WinQueryWindowText(hwndApplication, sizeof(ucWindowText),
                               ucWindowText);
            /* Only move WC_FRAME class (#1) and wpFolder class windows.
               If it is such a window, overwrite current offset
               with next enumeration so that array only contains
               windows that must be moved */
            if((!strcmp(ucClassname,"#1") ||
                (fMoveFolders && !strcmp(ucClassname,"wpFolder window"))) &&
               *ucWindowText && strcmp(ucWindowText,"Window List")) {
              if(vexclude) {
                for(x = 0;vexclude[x];x++) {
                  if(!strncmp(vexclude[x],ucWindowText,strlen(vexclude[x])))
                    break;
                }
                if(vexclude[x])
                  continue;
              }
              /* Only move windows */
              swpApps[ulAppCount].fl = SWP_MOVE | SWP_NOADJUST;
              if(virtorigx > xVirtual) {
                swpApps[ulAppCount].x -= xScreen;
                if(virtorigx > (xVirtual + xScreen))
                  swpApps[ulAppCount].x -= xScreen;
              }
              else if(virtorigx < xVirtual) {
                swpApps[ulAppCount].x += xScreen;
                if(virtorigx < (xVirtual - xScreen))
                  swpApps[ulAppCount].x += xScreen;
              }
              if(virtorigy > yVirtual) {
                swpApps[ulAppCount].y -= yScreen;
                if(virtorigy > (yVirtual + yScreen))
                  swpApps[ulAppCount].y -= yScreen;
              }
              else if(virtorigy < yVirtual) {
                swpApps[ulAppCount].y += yScreen;
                if(virtorigy < (yVirtual - yScreen))
                  swpApps[ulAppCount].y += yScreen;
              }
              ulAppCount++;
              if(ulAppCount >= 256)
                break;
            }
          }
        }
        WinEndEnumWindows(henum);    /* End enumeration */
        /* Now move all windows */
        if(ulAppCount)
          WinSetMultWindowPos(WinQueryAnchorBlock(hwnd),swpApps,ulAppCount);
        free(swpApps);
        _heapmin();
/*
        {
          CHAR s[160];

          sprintf(s,"x = %ld, y = %ld, vx = %ld, vy = %ld, xv = %ld, yv = %ld",
                  virtx,virty,virtorigx,virtorigy,xVirtual,yVirtual);
          WinMessageBox(HWND_DESKTOP,hwnd,s,"Debug",0,
                        MB_ICONASTERISK | MB_ENTER | MB_MOVEABLE);
        }
*/
        xVirtual = virtorigx;
        yVirtual = virtorigy;
        WinInvalidateRect(hwnd,NULL,FALSE);
      }
      return 0;

    case WM_PAINT:
      {
        HPS   hps;
        RECTL rc;

        hps = WinBeginPaint(hwnd, NULLHANDLE, &rc);
        if(hps) {
          /* Fill background of client area gray */
          WinFillRect(hps, &rc, CLR_PALEGRAY);
          WinEndPaint(hps);
        }
        PaintRecessedWindow(hwnd,(HPS)0,FALSE,FALSE);
        PostMsg(hwnd,UM_PAINT,MPVOID,MPVOID);
      }
      break;

    case UM_PAINT:            /* Draw overview of virtual Desktop */
      {
        HPS     hps;
        SWP     swp;
        double  fScaleX;      /* Reduce factor to reduce horizontal size of
                                 virtual Desktop to horizonal client window
                                 size */
        double  fScaleY;
        POINTL  ptl[2];       /* Point of lines,... */
        POINTL  ptlOrigin;    /* Coordinates (0|0) within client area */
        ULONG   ulColor = 1;  /* Use least significant 7 bits for window
                                 colors */
        LONG    lTemp;
        HENUM   henum;           /* Window handle of WC_FRAME class Desktop */
        HWND    hwndApplication; /* Window handles of enumerated application */
        ULONG   ulAppCount = 0;
        SWP     swpApps[128];
        CHAR    ucClassname[24];    /* Class name of enumerated application */
        CHAR    ucWindowText[33];   /* Window text of enumerated application */
        HWND    hwndFrame;

        hwndFrame = WinQueryWindow(WinQueryWindow(hwnd,QW_PARENT),QW_PARENT);
        /* Get a cached presentation space */
        hps = WinGetPS(hwnd);
        if(hps) {
          /* Get the client area size */
          WinQueryWindowPos(hwnd, &swp);
          /* Now get scale factor to scale virtual Desktop to client area */
          fScaleX = ((double)swp.cx - 1.0) / (3.0 * xScreen);
          fScaleY = (double)swp.cy / (3.0 * yScreen);
          /* Get coordinates (0|0) origin */
          ptlOrigin.x = xScreen * fScaleX;
          ptlOrigin.y = yScreen * fScaleY;
          /* Draw the borders of the 3 * 3 Desktops */
          GpiSetColor(hps, CLR_BLACK);
          for(lTemp = 0; lTemp <= 3; lTemp++)  {
            ptl[0].x = 0;
            ptl[0].y = ptlOrigin.y * lTemp;
            GpiMove(hps, &ptl[0]);
            ptl[0].x = swp.cx;
            GpiLine(hps, &ptl[0]);
            ptl[0].x = ptlOrigin.x * lTemp;
            ptl[0].y = 0;
            GpiMove(hps, &ptl[0]);
            ptl[0].y = swp.cy;
            GpiLine(hps, &ptl[0]);
          }
          /* Get physical Desktop origin */
          ptlOrigin.x += (double)xVirtual * fScaleX;
          ptlOrigin.y += (double)yVirtual * fScaleY;
          /* Now display the physical Desktop */
          GpiSetColor(hps, CLR_WHITE);
          ptl[0].x = ptl[1].x = ptlOrigin.x;
          ptl[0].y = ptl[1].y = ptlOrigin.y;
          ptl[1].x += (double)xScreen * fScaleX;
          ptl[1].y += (double)yScreen * fScaleY;
          GpiMove(hps, &ptl[0]);
          GpiBox(hps, DRO_OUTLINEFILL, &ptl[1], 0, 0);
          if(!fDisableDesktops && fShowMiniwins) {
            /* enumerate all desktop windows */
            henum = WinBeginEnumWindows(HWND_DESKTOP);
            /* Begin with offset 0 in first iteration */
            ulAppCount = 0;
            while((hwndApplication = WinGetNextWindow(henum)) != (HWND)0) {
              if(hwndApplication != hwndDesktop && hwndApplication != hwndWPS &&
                 hwndApplication != hwndBottom && hwndApplication != hwndFrame) {
                WinQueryWindowText(hwndApplication,sizeof(ucWindowText),
                                   ucWindowText);
                if(*ucWindowText && strcmp(ucWindowText,"Window List")) {
                  WinQueryClassName(hwndApplication,sizeof(ucClassname),
                                    ucClassname);
                  if(!strcmp(ucClassname,"#1") ||
                     !strcmp(ucClassname,"wpFolder window")) {
                    WinQueryWindowPos(hwndApplication,
                                      &swpApps[ulAppCount]);
                    if(!(swp.fl & (SWP_HIDE | SWP_MINIMIZE))) {
                      ulAppCount++;
                      if(ulAppCount >= 128)
                        break;
                    }
                  }
                }
              }
            }
            WinEndEnumWindows(henum);    /* End enumeration */
            /* Now display the windows from topmost to bottommost */
            for(lTemp = ulAppCount; lTemp >= 0; lTemp--) {
              if((ulColor & 15) == CLR_PALEGRAY)
                ulColor++;
              /*
              if((ulColor & 15) == CLR_RED) {
              WinQueryWindowText(swpApps[lTemp].hwnd,sizeof(ucWindowText),ucWindowText);
              WinSetWindowText(WinWindowFromID(WinQueryWindow(hwnd,QW_PARENT),MAIN_DATETIME),ucWindowText);
              }
              */
              GpiSetColor(hps,1 + (ulColor & 15));
              ptl[0].x = (double)ptlOrigin.x +
                         ((double)swpApps[lTemp].x * fScaleX);
              ptl[0].y = (double)ptlOrigin.y +
                         ((double)swpApps[lTemp].y * fScaleY);
              ptl[1].x = ptl[0].x +
                         ((double)swpApps[lTemp].cx * fScaleX);
              ptl[1].y = ptl[0].y + ((double)swpApps[lTemp].cy *
                         fScaleY);
              GpiMove(hps, &ptl[0]);
              GpiBox(hps, DRO_OUTLINE, &ptl[1], 0, 0);
              ulColor++;
            }
          }
          WinReleasePS(hps);
        }
      }
      return 0;

    case WM_COMMAND:
      {
        HWND hwndParent = WinQueryWindow(hwnd,QW_PARENT);

        switch(SHORT1FROMMP(mp1)) {
          case IDM_RUN:
            fDlgShowing = TRUE;
            WinDlgBox(HWND_DESKTOP,hwnd,RunProc,0,RUN_FRAME,0);
            fDlgShowing = FALSE;
            break;
          case IDM_OPENFM2:
            OpenObject(hwndParent,"<FM/2>","DEFAULT");
            break;
          case IDM_OPENFM2FOLDER:
            OpenObject(hwndParent,"<FM3_Folder>","DEFAULT");
            break;
          case IDM_OPENLAUNCHPAD:
            OpenObject(hwndParent,"<WP_LAUNCHPAD>","DEFAULT");
            break;
          case IDM_OPENCLOCK:
            OpenObject(hwndParent,"<WP_CLOCK>","DEFAULT");
            break;
          case IDM_OPENDRIVES:
            OpenObject(hwndParent,"<WP_DRIVES>","DEFAULT");
            break;
          case IDM_OPENFONTPALETTE:
            OpenObject(hwndParent,"<WP_FNTPAL>","DEFAULT");
            break;
          case IDM_OPENSOLIDCOLOR:
            if(!OpenObject(hwndParent,"<WP_CLRPAL>","DEFAULT"))
              OpenObject(hwndParent,"<WP_LORESCLRPAL>","DEFAULT");
            break;
          case IDM_OPENHIRESCOLOR:
            OpenObject(hwndParent,"<WP_HIRESCLRPAL>","DEFAULT");
            break;
          case IDM_OPENTEMPLATES:
            OpenObject(hwndParent,"<WP_TEMPS>","DEFAULT");
            break;
          case IDM_OPENSETUP:
            OpenObject(hwndParent,"<WP_CONFIG>","DEFAULT");
            break;

          case IDM_OPENOS2FULL:
            OpenPgm(hwndParent,"*","","FULLSCREEN");
            break;
          case IDM_OPENOS2WIN:
            OpenPgm(hwndParent,"*","","WINDOWABLEVIO");
            break;
          case IDM_OPENDOSFULL:
            OpenPgm(hwndParent,"*","","VDM");
            break;
          case IDM_OPENDOSWIN:
            OpenPgm(hwndParent,"*","","WINDOWEDVDM");
            break;
          case IDM_OPENWINOS2:
            OpenPgm(hwndParent,"WINOS2.COM","","VDM");
            break;

          case IDM_RESETDESKTOPS:
            xVirtual = yVirtual = 0;
            NormalizeAll(WinQueryWindowPtr(WinQueryWindow(hwnd,QW_PARENT),0),
                         FALSE);
            break;

          case IDM_RESTOREALL:
          case IDM_MINIMIZEALL:
            {
              PSWBLOCK      pswb;
              ULONG         ulSize,ulCount,fl;
              register INT  i;

              /* Get the switch list information */
              ulCount = WinQuerySwitchList(0,NULL,0);
              ulSize = sizeof(SWBLOCK) + sizeof(HSWITCH) + (ulCount + 4L) *
                       (LONG)sizeof(SWENTRY);
              /* Allocate memory for list */
              if((pswb = malloc((unsigned)ulSize)) != NULL) {
                /* Put the info in the list */
                ulCount = WinQuerySwitchList(0,pswb,
                                             ulSize - sizeof(SWENTRY));
                /* do the dirty deed */
                for(i = 0;i < pswb->cswentry;i++) {
                  if(pswb->aswentry[i].swctl.hwnd != hwndWPS &&
                     pswb->aswentry[i].swctl.hwnd != hwndDesktop &&
                     pswb->aswentry[i].swctl.hwnd != hwndBottom &&
                     pswb->aswentry[i].swctl.uchVisibility == SWL_VISIBLE) {
                    fl = (SHORT1FROMMP(mp1) == IDM_MINIMIZEALL) ?
                          (SWP_MINIMIZE | SWP_HIDE) : (SWP_SHOW | SWP_RESTORE);
                    WinSetWindowPos(pswb->aswentry[i].swctl.hwnd,HWND_TOP,
                                    0,0,0,0,fl);
                  }
                }
                free(pswb);
                _heapmin();
              }
            }
            break;
        }
      }
      PostMsg(WinQueryWindow(hwnd,QW_PARENT),UM_HIDE,MPVOID,MPVOID);
      return 0;

    case WM_DESTROY:
      xVirtual = yVirtual = 0;
      NormalizeAll(WinQueryWindowPtr(WinQueryWindow(hwnd,QW_PARENT),0),
                   FALSE);
      if(hwndAppMenu)
        WinDestroyWindow(hwndAppMenu);
      break;
  }

  return WinDefWindowProc(hwnd,msg,mp1,mp2);
}


MRESULT EXPENTRY BubbleProc (HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2) {

  PFNWP oldproc = (PFNWP)WinQueryWindowPtr(hwnd,0);

  switch(msg) {
    case WM_SETFOCUS:
      if(mp2)
        PostMsg(hwnd,UM_FOCUSME,mp1,MPVOID);
      break;

    case UM_FOCUSME:
      WinSetFocus(HWND_DESKTOP,(HWND)mp1);
      return 0;

    case WM_PAINT:
      PostMsg(hwnd,UM_PAINT,MPVOID,MPVOID);
      break;

    case UM_PAINT:
      {
        HPS     hps;
        SWP     swp;
        POINTL  ptl;

        WinQueryWindowPos(hwnd,&swp);
        if(!(swp.fl & (SWP_HIDE | SWP_MINIMIZE)) && swp.cx > 6 && swp.cy > 6) {
          hps = WinGetPS(hwnd);
          if(hps) {
            GpiSetColor(hps,CLR_WHITE);
            ptl.x = 2;
            ptl.y = 1;
            GpiMove(hps,&ptl);
            ptl.x = swp.cx - 2;
            GpiLine(hps,&ptl);
            ptl.y = swp.cy - 3;
            GpiLine(hps,&ptl);
            ptl.x = 2;
            GpiLine(hps,&ptl);
            ptl.y = 1;
            GpiLine(hps,&ptl);
            GpiSetColor(hps,CLR_BROWN);
            ptl.x = 1;
            ptl.y = 2;
            GpiMove(hps,&ptl);
            ptl.x = swp.cx - 3;
            GpiLine(hps,&ptl);
            ptl.y = swp.cy - 2;
            GpiLine(hps,&ptl);
            ptl.x = 1;
            GpiLine(hps,&ptl);
            ptl.y = 2;
            GpiLine(hps,&ptl);
            WinReleasePS(hps);
          }
        }
      }
      return 0;

    case WM_CLOSE:
      WinDestroyWindow(hwnd);
      return 0;
  }
  return (oldproc) ? oldproc(hwnd,msg,mp1,mp2) :
                     WinDefWindowProc(hwnd,msg,mp1,mp2);
}


MRESULT EXPENTRY SwitchButtonProc (HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2) {

  static HWND hwndBubble = (HWND)0,hwndMenu = (HWND)0;

  switch(msg) {
    case WM_CREATE:
      break;

    case WM_BUTTON1CLICK:
      PostMsg(hwnd,WM_COMMAND,MPFROM2SHORT(IDM_SHOW,0),MPVOID);
      break;

    case WM_CONTEXTMENU:
      {
        POINTL       ptl;
        USHORT       id = WinQueryWindowUShort(hwnd,QWS_ID);
        TASKDATA    *td = WinQueryWindowPtr(WinQueryWindow(hwnd,
                                         QW_PARENT),0);
        SWP          swp;
        SWENTRYLIST *info = WinQueryWindowPtr(hwnd,0);

        if(info) {
          WinSetCapture(HWND_DESKTOP,NULLHANDLE);
          WinSendMsg(hwndBubble,WM_CLOSE,MPVOID,MPVOID);
          hwndBubble = (HWND)0;
          if(!hwndMenu) {
            hwndMenu = WinLoadMenu(HWND_DESKTOP,0,MAIN_FIRST);
#ifdef DEBUG
            if(hwndMenu) {

              MENUITEM mi;

              memset(&mi,0,sizeof(MENUITEM));
              mi.iPosition = MIT_END;
              mi.afStyle = MIS_TEXT;
              mi.id = IDM_DEBUGINFO;
              WinSendMsg(hwndMenu,MM_INSERTITEM,MPFROMP(&mi),
                         MPFROMP("Debug info"));
            }
#endif
          }
          if(hwndMenu) {
            if(hwndBubble) {
              WinSetCapture(HWND_DESKTOP,NULLHANDLE);
              WinSendMsg(hwndBubble,WM_CLOSE,MPVOID,MPVOID);
              hwndBubble = (HWND)0;
            }
            WinQueryPointerPos(HWND_DESKTOP,&ptl);
            ptl.y = yNext + yIcon + 5;
            WinMapWindowPoints(HWND_DESKTOP,hwnd,&ptl,1L);
            FindTasklistID(td->swehead,id);
            if(info->swe.swctl.idProcess == dtPid) {
              WinEnableMenuItem(hwndMenu,IDM_KILL,FALSE);
              if(info->swe.swctl.hwnd == hwndDesktop ||
                 info->swe.swctl.hwnd == hwndWPS ||
                 info->swe.swctl.hwnd == hwndBottom) {
                WinEnableMenuItem(hwndMenu,IDM_CLOSE,FALSE);
                WinEnableMenuItem(hwndMenu,IDM_MOVEDESKTOP,FALSE);
                WinEnableMenuItem(hwndMenu,IDM_JUMPABLE,FALSE);
              }
              else {
                WinEnableMenuItem(hwndMenu,IDM_CLOSE,TRUE);
                WinEnableMenuItem(hwndMenu,IDM_MOVEDESKTOP,TRUE);
                WinEnableMenuItem(hwndMenu,IDM_JUMPABLE,TRUE);
              }
            }
            else {
              WinEnableMenuItem(hwndMenu,IDM_CLOSE,TRUE);
              WinEnableMenuItem(hwndMenu,IDM_KILL,TRUE);
              WinEnableMenuItem(hwndMenu,IDM_MOVEDESKTOP,TRUE);
              WinEnableMenuItem(hwndMenu,IDM_JUMPABLE,TRUE);
            }
            WinQueryWindowPos(info->swe.swctl.hwnd,&swp);
            WinEnableMenuItem(hwndMenu,IDM_SHOW,
                              (swp.fl & (SWP_HIDE | SWP_MINIMIZE)) != 0);
            WinEnableMenuItem(hwndMenu,IDM_HIDE,
                              (swp.fl & (SWP_HIDE | SWP_MINIMIZE)) == 0);
            WinEnableMenuItem(hwndMenu,IDM_MOVEDESKTOP,(!fDisableDesktops));
            WinCheckMenuItem(hwndMenu,IDM_JUMPABLE,(info->swe.swctl.fbJump ==
                             SWL_JUMPABLE));
            if(WinPopupMenu(hwnd,hwnd,hwndMenu,ptl.x,ptl.y,0,
                            PU_HCONSTRAIN | PU_VCONSTRAIN |
                            PU_KEYBOARD   | PU_MOUSEBUTTON1))
              fMenuShowing = TRUE;
          }
        }
      }
      break;

    case UM_MENUEND:
      fMenuShowing = FALSE;
      return 0;

    case WM_MENUEND:
      PostMsg(hwnd,UM_MENUEND,mp1,mp2);
      break;

    case WM_BUTTON1DOWN:
    case WM_BUTTON2DOWN:
    case WM_BUTTON3DOWN:
    case WM_MOUSEMOVE:
      {
        HWND           hwndCap = WinQueryCapture(HWND_DESKTOP);
        SHORT          posx    = SHORT1FROMMP(mp1),
                       posy    = SHORT2FROMMP(mp1);
        SWP            swp;
        SWENTRYLIST   *info    = WinQueryWindowPtr(hwnd,0);
        CHAR          *s;
        register CHAR *p;

        if(info) {
          WinQueryWindowPos(hwnd,&swp);
          if(hwndBubble && hwndCap == hwnd) {
            if(msg != WM_MOUSEMOVE || posx < 0 || posy < 0 ||
               posx > swp.cx || posy > swp.cy) {
              WinSetCapture(HWND_DESKTOP,NULLHANDLE);
              WinSendMsg(hwndBubble,WM_CLOSE,MPVOID,MPVOID);
              hwndBubble = (HWND)0;
            }
          }
          else if(hwndCap == NULLHANDLE && msg == WM_MOUSEMOVE &&
                  !fMenuShowing) {
            if(hwndBubble)
              WinSendMsg(hwndBubble,WM_CLOSE,MPVOID,MPVOID);
              hwndBubble = (HWND)0;
              s = malloc(strlen(info->swe.swctl.szSwtitle) + 32);
            if(s) {
              strcpy(s,info->swe.swctl.szSwtitle);
              p = s;
              while(*p) {
                if(*p == '\r' || *p == '\n')
                  *p = ' ';
                p++;
              }
              hwndBubble = WinCreateWindow(HWND_DESKTOP,WC_STATIC,s,
                                           SS_TEXT | DT_CENTER | DT_VCENTER,
                                           0, 0, 0, 0,
                                           WinQueryWindow(hwnd,QW_PARENT),
                                           HWND_TOP,MAIN_HELP,NULL,NULL);
              if(hwndBubble) {

                PFNWP  oldproc;
                RGB2   rgb2F,rgb2;
                HPS    hps;
                POINTL aptl[TXTBOX_COUNT];
                LONG   sx,sy;

                memset(&rgb2,0,sizeof(RGB2));
                rgb2F.bRed = 255;
                rgb2F.bGreen = 255;
                rgb2F.bBlue = 198;
                rgb2F.fcOptions = 0;
                SetPresParams(hwndBubble,&rgb2F,&rgb2,&rgb2,"8.Helv");
                hps = WinGetPS(hwndBubble);
                GpiQueryTextBox(hps,strlen(s),s,TXTBOX_COUNT,aptl);
                WinReleasePS(hps);
                sy = swp.y + swp.cy + 8;
                if(swp.x > (xScreen / 2))
                  sx = (swp.x + (swp.cx / 2)) - aptl[TXTBOX_TOPRIGHT].x;
                else
                  sx = swp.x + (swp.cx / 2);
                if(sx < 0)
                  sx = 0;
                oldproc = WinSubclassWindow(hwndBubble,(PFNWP)BubbleProc);
                if(oldproc) {
                  WinSetWindowPtr(hwndBubble,0,(PVOID)oldproc);
                  WinSetWindowPos(hwndBubble,HWND_TOP,sx,sy,
                                  aptl[TXTBOX_TOPRIGHT].x + 12,
                                  aptl[TXTBOX_TOPLEFT].y + 12,
                                  SWP_DEACTIVATE | SWP_SHOW | SWP_ZORDER |
                                  SWP_MOVE | SWP_SIZE);
                  WinSetCapture(HWND_DESKTOP,hwnd);
                }
                else {
                  WinDestroyWindow(hwndBubble);
                  hwndBubble = (HWND)0;
                }
              }
              free(s);
              _heapmin();
            }
          }
        }
      }
      break;

    case WM_PAINT:
      {
        MRESULT      mr;
        HWND         hwndActive;
        BOOL         outtie = FALSE;
        HPS    hps;
        RECTL  rclUpdate;
        POINTL ptl;
        SWENTRYLIST *info = WinQueryWindowPtr(hwnd,0);

        if(WinIsWindowShowing(hwnd)) {
	  hps = WinBeginPaint(hwnd,NULLHANDLE,(PRECTL)&rclUpdate);
	  if (hps) {
	    WinFillRect(hps,(PRECTL)&rclUpdate,CLR_PALEGRAY);
	    WinEndPaint(hps);
            WinInvalidateRect(hwnd,&rclUpdate,FALSE);
	  }
	  mr = PFNWPStaticProc(hwnd,msg,mp1,mp2);	// Draw icon
          if (info) {
            hwndActive = WinQueryActiveWindow(HWND_DESKTOP);
            if(hwndActive == info->swe.swctl.hwnd)
              outtie = TRUE;
          }
          PaintRecessedWindow(hwnd,(HPS)0,outtie,!outtie);
        }
        return mr;
      }

    case WM_COMMAND:
      if(!fStayUp)
        WinSetWindowPos(WinQueryWindow(hwnd,QW_PARENT),HWND_BOTTOM,0,0,0,0,
                        SWP_ZORDER | SWP_HIDE | SWP_ACTIVATE);
      {
        TASKDATA    *td = WinQueryWindowPtr(WinQueryWindow(hwnd,
                                            QW_PARENT),0);
        SWENTRYLIST *info = WinQueryWindowPtr(hwnd,0);

        if(!info || !td)
          return 0;
        switch(SHORT1FROMMP(mp1)) {
#ifdef DEBUG
          case IDM_DEBUGINFO:
            {
              CHAR  class[24];
              CHAR  title[256];
              CHAR  message[1024];
              SWP   swp;

              *class = *title = 0;
              WinQueryClassName(info->swe.swctl.hwnd,
                                sizeof(class),class);
              WinQueryWindowText(info->swe.swctl.hwnd,
                                 sizeof(title),title);
              WinQueryWindowPos(info->swe.swctl.hwnd,
                                &swp);
              sprintf(message,"Class = \"%s\"\n\nTitle = \"%s\"\n\n"
                              "STitle = \"%s\"\n\nProgtype = %lu\n\n"
                              "Jump = %lu\n\nposition: %ldx, %ldy\n\n"
                              "hwndIcon = %lu, id = %hu",
                      class,title,
                      info->swe.swctl.szSwtitle,
                      info->swe.swctl.bProgType,
                      info->swe.swctl.fbJump,
                      swp.x,swp.y,
                      info->swe.swctl.hwndIcon,
                      WinQueryWindowUShort(info->swe.swctl.hwndIcon,QWS_ID));
              WinMessageBox(HWND_DESKTOP,hwnd,message,"Debug",0,
                            MB_ICONASTERISK | MB_ENTER | MB_MOVEABLE);
            }
            break;
#endif
          case IDM_SHOW:
ShowMe:
            {
              SWP   swp;
              ULONG fl = SWP_SHOW | SWP_ZORDER | SWP_ACTIVATE;

              WinQueryWindowPos(info->swe.swctl.hwnd,
                                &swp);
              if(swp.fl & SWP_MINIMIZE)
                fl |= SWP_RESTORE;
              WinSetWindowPos(info->swe.swctl.hwnd,
                              HWND_TOP,0,0,0,0,fl);
              WinQueryWindowPos(info->swe.swctl.hwnd,
                                &swp);
              if(!fDisableDesktops &&
                 !WinIsWindowShowing(info->swe.swctl.hwnd)) {
                /* calculate the "desktop" in which the window lies */

                LONG   x,y,offx,offy;
                double dx,dy;

                /* where is it from current desktop? */
                dx = (double)(swp.x + (swp.cx / 2)) / (double)xScreen;
                dy = (double)(swp.y + (swp.cy / 2)) / (double)yScreen;
                x = floor(dx);
                y = floor(dy);
                /* "quadrant" of current desktop? */
                offx = (xVirtual / xScreen) + 2;
                offy = (yVirtual / yScreen) + 2;
                /* add together and we have new desktop we want */
                x += offx;
                y += offy;
                WinSendMsg(WinWindowFromID(WinQueryWindow(hwnd,QW_PARENT),
                           MAIN_VIRTUAL),UM_MOVEDESKTOP,
                           MPFROMLONG(x),MPFROMLONG(y));
              }
              WinQueryWindowPos(info->swe.swctl.hwnd,&swp);
              if(fReposMouse)
                WinSetPointerPos(HWND_DESKTOP,swp.x + (swp.cx / 2),
                                 swp.y + (swp.cy / 2));
            }
            WinSwitchToProgram(info->swe.hswitch);
            break;
          case IDM_HIDE:
            WinSetWindowPos(info->swe.swctl.hwnd,
                            HWND_BOTTOM,0,0,0,0,SWP_HIDE | SWP_ZORDER);
            break;
          case IDM_REMOVEFROMLIST:
            fDlgShowing = TRUE;
            if(WinMessageBox(HWND_DESKTOP,HWND_DESKTOP,
                             "Are you sure you want to remove this from "
                             "the Window List and Taskbar?  The removal "
                             "is permanent (for the duration of the window).",
                             (info->swe.swctl.fbJump !=
                              SWL_JUMPABLE) ?
                             "Confirm (Warning:  window is not jumpable!)" :
                             "Confirm",
                             0,MB_ICONEXCLAMATION | MB_MOVEABLE |
                               MB_YESNOCANCEL) == MBID_YES) {
              info->swe.swctl.uchVisibility =
                SWL_INVISIBLE;
              WinChangeSwitchEntry(info->swe.hswitch,
                                   &info->swe.swctl);
            }
            fDlgShowing = FALSE;
            break;
          case IDM_JUMPABLE:
            info->swe.swctl.fbJump =
              (info->swe.swctl.fbJump == SWL_JUMPABLE) ?
                SWL_NOTJUMPABLE : SWL_JUMPABLE;
            WinChangeSwitchEntry(info->swe.hswitch,
                                 &info->swe.swctl);
            break;
          case IDM_CLOSE:
            PostMsg(info->swe.swctl.hwnd,
                       WM_SAVEAPPLICATION,MPVOID,MPVOID);
            PostMsg(info->swe.swctl.hwnd,
                       WM_SYSCOMMAND,MPFROM2SHORT(SC_CLOSE,0),MPVOID);
            break;
          case IDM_KILL:
            {
              KILLWIN *kw;

              kw = malloc(sizeof(KILLWIN));
              if(kw) {
                memset(kw,0,sizeof(KILLWIN));
                kw->size = sizeof(KILLWIN);
                kw->hwnd = info->swe.swctl.hwnd;
                kw->pid  = info->swe.swctl.idProcess;
                if(info->swe.swctl.bProgType == PROG_DEFAULT ||
                   info->swe.swctl.bProgType == PROG_FULLSCREEN ||
                   info->swe.swctl.bProgType == PROG_WINDOWABLEVIO ||
                   info->swe.swctl.bProgType == PROG_REAL ||
                   info->swe.swctl.bProgType == PROG_VDM ||
                   info->swe.swctl.bProgType == PROG_WINDOWEDVDM)
                  kw->delay = 500L;
                else
                  kw->delay = 5000L;
                if(_beginthread(QuitThread,NULL,16384,kw) == -1) {
                  free(kw);
                  _heapmin();
                }
              }
            }
            break;
          case IDM_MOVEDESKTOP:
            NormalizeWindow(info->swe.swctl.hwnd);
            goto ShowMe;
        }
      }
      PostMsg(WinQueryWindow(hwnd,QW_PARENT),UM_HIDE,MPVOID,MPVOID);
      return 0;

    case WM_CLOSE:
      WinShowWindow(hwnd,FALSE);
      if(fStayUp) {

        RECTL Rectl;

        WinQueryWindowRect(hwnd,&Rectl);
        Rectl.xLeft -= 2;
        Rectl.xRight += 2;
        Rectl.yBottom -= 2;
        Rectl.yTop += 2;
        WinInvalidateRect(WinQueryWindow(hwnd,QW_PARENT),&Rectl,FALSE);
      }
      WinDestroyWindow(hwnd);
      return 0;

    case WM_DESTROY:
      if(hwndBubble) {
        WinSendMsg(hwndBubble,WM_CLOSE,MPVOID,MPVOID);
        hwndBubble = (HWND)0;
      }
      return 0;
  }
  return PFNWPStaticProc(hwnd,msg,mp1,mp2);
}


MRESULT EXPENTRY DateTimeProc (HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2) {

  PFNWP oldproc = (PFNWP)WinQueryWindowPtr(hwnd,0);

  switch(msg) {
    case WM_CONTEXTMENU:
      WinSendMsg(WinQueryWindow(hwnd,QW_PARENT),UM_CONTEXTMENU,mp1,mp2);
      break;

    case WM_BUTTON3CLICK:
      PostMsg(hwnd,WM_COMMAND,MPFROM2SHORT(IDM_WPSCLOCK,0),MPVOID);
      break;

    case WM_BUTTON1DBLCLK:
      PostMsg(hwnd,WM_COMMAND,MPFROM2SHORT(IDM_SETDATETIME,0),MPVOID);
      break;

    case WM_COMMAND:
      switch(SHORT1FROMMP(mp1)) {
        case IDM_SETDATETIME:
          OpenObject(hwnd,"<WP_CLOCK>","SETTINGS");
          break;
        case IDM_WPSCLOCK:
          OpenObject(hwnd,"<WP_CLOCK>","DEFAULT");
          break;
      }
      PostMsg(WinQueryWindow(hwnd,QW_PARENT),UM_HIDE,MPVOID,MPVOID);
      return 0;

    case WM_PAINT:
      {
        MRESULT mr;

        mr = oldproc(hwnd,msg,mp1,mp2);
        PaintRecessedWindow(hwnd,(HPS)0,FALSE,FALSE);
        return mr;
      }

    case WM_CLOSE:
      WinDestroyWindow(hwnd);
      return 0;

    case WM_DESTROY:
      break;
  }
  return oldproc(hwnd,msg,mp1,mp2);
}


MRESULT EXPENTRY StatusProc (HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2) {

  PFNWP oldproc = (PFNWP)WinQueryWindowPtr(hwnd,0);

  switch(msg) {
    case WM_CONTEXTMENU:
      WinSendMsg(WinQueryWindow(hwnd,QW_PARENT),UM_CONTEXTMENU,mp1,mp2);
      break;

    case WM_BUTTON3CLICK:
      OpenObject(WinQueryWindow(hwnd,QW_PARENT),"<FM/2_SYSINFO>","DEFAULT");
      PostMsg(WinQueryWindow(hwnd,QW_PARENT),UM_HIDE,MPVOID,MPVOID);
      break;

    case WM_BUTTON1DBLCLK:
      OpenObject(WinQueryWindow(hwnd,QW_PARENT),"<FM/2_KILLPROC>","DEFAULT");
      PostMsg(WinQueryWindow(hwnd,QW_PARENT),UM_HIDE,MPVOID,MPVOID);
      break;

    case WM_COMMAND:
      switch(SHORT1FROMMP(mp1)) {
        default:
          break;
      }
      PostMsg(WinQueryWindow(hwnd,QW_PARENT),UM_HIDE,MPVOID,MPVOID);
      return 0;

    case WM_PAINT:
      {
        MRESULT mr;

        mr = oldproc(hwnd,msg,mp1,mp2);
        PaintRecessedWindow(hwnd,(HPS)0,FALSE,FALSE);
        return mr;
      }

    case WM_CLOSE:
      WinDestroyWindow(hwnd);
      return 0;

    case WM_DESTROY:
      break;
  }
  return oldproc(hwnd,msg,mp1,mp2);
}


MRESULT EXPENTRY StartUpProc (HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2) {

  static BOOL timerstarted;

  switch(msg) {
    case WM_INITDLG:
      timerstarted = FALSE;
      WinSetWindowPtr(hwnd,0,mp2);
      {
        CHAR s[80];

        sprintf(s,"v%s",VERSION);
        WinSetDlgItemText(hwnd,START_VERSION,s);
      }
      if(!mp2) {
        if(WinStartTimer(WinQueryAnchorBlock(hwnd),hwnd,START_TIMER,1500)) {
          timerstarted = TRUE;
          WinShowWindow(WinWindowFromID(hwnd,DID_OK),FALSE);
        }
      }
      break;

    case WM_TIMER:
      WinDismissDlg(hwnd,0);
      break;

    case WM_COMMAND:
      if(WinQueryWindowPtr(hwnd,0))
        WinDismissDlg(hwnd,0);
      return 0;

    case WM_DESTROY:
      if(timerstarted)
        WinStopTimer(WinQueryAnchorBlock(hwnd),hwnd,START_TIMER);
      timerstarted = FALSE;
      break;
  }
  return WinDefDlgProc(hwnd,msg,mp1,mp2);
}


MRESULT EXPENTRY MainWndProc (HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2) {

  TASKDATA   *td;
  static BOOL timerstarted = FALSE;

  switch(msg) {
    case WM_CREATE:
      td = malloc(sizeof(TASKDATA));
      if(td) {

        TID           tid;
        PID           pid;
        HWND          hwndTemp;
        PFNWP         oldproc;

        memset(td,0,sizeof(TASKDATA));
        td->size = sizeof(TASKDATA);
        WinSetWindowPtr(hwnd,0,(PVOID)td);
        /* get our process ID */
        if(WinQueryWindowProcess(hwnd,&pid,&tid))
          td->pid = pid;
        hwndTemp = WinCreateWindow(hwnd,WC_STATIC,"Time\rDate",
                      WS_VISIBLE | SS_TEXT | DT_CENTER | DT_WORDBREAK,
                      4,4,74,yIcon,hwnd,
                      HWND_TOP,MAIN_DATETIME,NULL,NULL);
        if(hwndTemp) {

          RGB2   rgb2F,rgb2;

          memset(&rgb2,0,sizeof(RGB2));
          rgb2F.bRed = rgb2F.bGreen = rgb2F.bBlue = 204;
          rgb2F.fcOptions = 0;
          SetPresParams(hwndTemp,&rgb2F,&rgb2,&rgb2,"8.Helv");
          if(!WinStartTimer(WinQueryAnchorBlock(hwnd),hwnd,MAIN_TIMER,2000)) {
            DosBeep(50,100);
            WinDestroyWindow(hwnd);
          }
          else {
            timerstarted = TRUE;
            oldproc = WinSubclassWindow(hwndTemp,(PFNWP)DateTimeProc);
            if(oldproc)
              WinSetWindowPtr(hwndTemp,0,(PVOID)oldproc);
            PostMsg(hwnd,UM_TIMER,MPVOID,MPVOID);
          }
        }
        else {
          WinDestroyWindow(hwnd);
          break;
        }

        hwndTemp = WinCreateWindow(hwnd,WC_STATIC,"Procs\rThreads",
                      WS_VISIBLE | SS_TEXT | DT_CENTER | DT_WORDBREAK,
                      82,4,74,yIcon,hwnd,
                      HWND_TOP,MAIN_PROCS,NULL,NULL);
        if(hwndTemp) {

          RGB2   rgb2F,rgb2;

          memset(&rgb2,0,sizeof(RGB2));
          rgb2F.bRed = rgb2F.bGreen = rgb2F.bBlue = 204;
          rgb2F.fcOptions = 0;
          SetPresParams(hwndTemp,&rgb2F,&rgb2,&rgb2,"8.Helv");
          oldproc = WinSubclassWindow(hwndTemp,(PFNWP)StatusProc);
          if(oldproc)
            WinSetWindowPtr(hwndTemp,0,(PVOID)oldproc);
        }
        else
          fProcs = FALSE;

        hwndTemp = WinCreateWindow(hwnd,"Virtual Desktops","",WS_VISIBLE,
                                   (74 * ((fProcs != 0) + 1)) + 8 +
                                    ((fProcs != 0) * 4),4,
                                   xIcon,yIcon,hwnd,HWND_TOP,
                                   MAIN_VIRTUAL,NULL,NULL);
        if(hwndTemp) {
          UpdateVals(hwnd);
          fDlgShowing = TRUE;
          WinDlgBox(HWND_DESKTOP,hwnd,StartUpProc,0,START_FRAME,NULL);
          fDlgShowing = FALSE;
        }
        else
          WinDestroyWindow(hwnd);
      }
      else
        WinDestroyWindow(hwnd);
      break;

    case UM_TIMER:
    case WM_TIMER:
      td = WinQueryWindowPtr(hwnd,0);
      if(td && td->killme) {
        if(!PostMsg(hwnd,WM_CLOSE,MPVOID,MPVOID))
          WinSendMsg(hwnd,WM_CLOSE,MPVOID,MPVOID);
      }
      else if(msg == UM_TIMER ||
              WinIsWindowVisible(WinWindowFromID(hwnd,MAIN_DATETIME))) {

        CHAR     s[24];
        DATETIME dt;

        if(!DosGetDateTime(&dt)) {
          sprintf(s,"%02hu:%02hu:%02hu\r%04u/%02u/%02u",dt.hours,
                  dt.minutes,dt.seconds,dt.year,dt.month,dt.day);
          WinSetDlgItemText(hwnd,MAIN_DATETIME,s);
        }
      }
      if(msg == UM_TIMER)
        return 0;
      break;

    case UM_TASKLIST:
      if(fStayUp) {
WinSendMsg(hwnd,UM_SHOW,MPFROMLONG(1L),MPVOID);
return 0;
        td = WinQueryWindowPtr(hwnd,0);
        if(td) {

          ULONG        ulWas;
          SWENTRYLIST *info;

          ulWas = td->ulCount;
          info = FindTasklistEntry(td->swehead,(HSWITCH)mp2);
          switch((ULONG)mp1) {
            case 3:
              if(info) {
                if(info->id)
                  WinDestroyWindow(WinWindowFromID(hwnd,info->id));
                td->ulCount = DeleteTasklistEntry(&(td->swehead),
                                                  &(td->swetail),
                                                  (HSWITCH)mp2);

                /* TEMPORARY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

                WinSendMsg(hwnd,UM_SHOW,MPFROMLONG(1L),MPVOID);
              }
              break;

            case 0x00010001:
            case 0x00000001:
              td->ulCount = AddTasklistEntry(&(td->swehead),&(td->swetail),
                                             (HSWITCH)mp2);
              if(td->ulCount > ulWas) {

                /* TEMPORARY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

                WinSendMsg(hwnd,UM_SHOW,MPFROMLONG(1L),MPVOID);

              }
              break;
          }
        }
      }
      return 0;

    case UM_FREE:   /* free switchlist info */
      td = WinQueryWindowPtr(hwnd,0);
      if(td) {

        /* kill off "buttons" */
        HENUM  henum;
        HWND   hwndC;
        USHORT id;

        henum = WinBeginEnumWindows(hwnd);
        while((hwndC = WinGetNextWindow(henum)) != NULLHANDLE) {
          id = WinQueryWindowUShort(hwndC,QWS_ID);
          if(id >= MAIN_FIRST)
            WinSendMsg(hwndC,WM_CLOSE,MPVOID,MPVOID);
        }
        WinEndEnumWindows(henum);
        if(!mp1) {
          /* free switchlist memory */
          FreeTasklist(td->swehead);
          /* zero switchlist 'counter' */
          td->ulCount = 0L;
          td->swehead = td->swetail = NULL;
        }
      }
      return 0;

    case UM_ALTTAB: /* user pressed hook's switch hotkeys */
      td = WinQueryWindowPtr(hwnd,0);
      fKeyShowing = TRUE;
      WinSendMsg(hwnd,UM_HIDE,MPVOID,MPVOID); /* dismiss window if showing */
      if(td) {
        if(!fStayUp)
          WinSendMsg(hwnd,UM_GETLIST,MPVOID,MPVOID);  /* get switchlist info */
        if(td->ulCount && td->swehead)  /* if procs available, call dialog */
          WinDlgBox(HWND_DESKTOP,HWND_DESKTOP,TabProc,0,TAB_FRAME,(PVOID)td);
      }
      if(!fStayUp)
        PostMsg(hwnd,UM_FREE,MPVOID,MPVOID);  /* free info */
      fKeyShowing = FALSE;                    /* so hook knows we're done */
      return 0;

    case UM_GETLIST:
      td = WinQueryWindowPtr(hwnd,0);
      if(td) {
        UpdateVals(hwnd);  /* screen size, icon size, etc. */
        /* remove any buttons already in window */
        WinSendMsg(hwnd,UM_FREE,MPVOID,MPVOID);
        /* Get the switch list information */
        td->ulCount = LoadTasklist(td->pid,&(td->swehead),&(td->swetail));
      }
      return 0;

    case UM_HIDE: /* hide window */
      if(!fStayUp) {
        WinSetWindowPos(hwnd,HWND_TOP,0,0,0,0,SWP_HIDE);
        fWinShowing = FALSE;  /* so hook will know we're hidden */
        WinSendMsg(hwnd,UM_FREE,MPVOID,MPVOID); /* free switchlist memory */
      }
      return 0;

    case UM_PROCS:
      {
        PROCESSINFO  *ppi;
        BUFFHEADER   *pbh;
        MODINFO      *pmi;
        ULONG         numprocs = 0,numthreads = 0;
        CHAR          s[128];

        pbh = malloc(USHRT_MAX);
        if(pbh) {
          if(!DosQProcStatus(pbh,USHRT_MAX)) {
            ppi = pbh->ppi;
            while(ppi->ulEndIndicator != PROCESS_END_INDICATOR) {
              pmi = pbh->pmi;
              while(pmi && ppi->hModRef != pmi->hMod)
                pmi = pmi->pNext;
              if(pmi) {
                numprocs++;
                numthreads += ppi->usThreadCount;
              }
              ppi = (PPROCESSINFO)(ppi->ptiFirst + ppi->usThreadCount);
            }
            sprintf(s,"Proc: %hu\rThrd: %hu",numprocs,numthreads);
            WinSetDlgItemText(hwnd,MAIN_PROCS,s);
          }
          else {
            WinSetDlgItemText(hwnd,MAIN_PROCS,"???\r???");
            fProcs = FALSE;
          }
          free(pbh);
          _heapmin();
        }
      }
      return 0;

    case UM_SHOW: /* show window */
      td = WinQueryWindowPtr(hwnd,0);
      if(td) {

        ULONG         xNext;
        register INT  i;
        HWND          hwndTemp;
        HPOINTER      hptr;
        SWENTRYLIST  *info;

        /* Get the switch list information */
        WinSendMsg(hwnd,UM_FREE,mp1,MPVOID);
        if(!mp1)
          WinSendMsg(hwnd,UM_GETLIST,MPVOID,MPVOID);
        yNext = 4;
        xNext = ((76L * ((fProcs != FALSE) + 1)) + 12L) + xIcon;
        if(td->ulCount && td->swehead) {
          /* Calculate size of taskbar window */
          info = td->swehead;
          for(i = 0;i < td->ulCount && info;i++) {
            xNext += (xIcon + 4);
            if(i + 1 < td->ulCount && xNext + xIcon >= xScreen) {
              xNext = 4L;
              yNext += (yIcon + 8);
            }
            info = info->next;
          }
          /* set taskbar window size */
          WinSetWindowPos(hwnd,HWND_TOP,0,0,xScreen,yNext + yIcon + 4,
                          SWP_SIZE);
          /* Create "buttons" */
          yNext = 4;
          xNext = ((76L * ((fProcs != FALSE) + 1)) + 12L) + xIcon;
          info = td->swehead;
          for(i = 0; i < td->ulCount; i++) {
            hwndTemp = WinCreateWindow(hwnd,"Task Buttons","#3",
                          WS_VISIBLE | SS_ICON,
                          xNext,yNext,xIcon,yIcon,hwnd,
                          HWND_TOP,MAIN_FIRST + i,NULL,NULL);
            if(hwndTemp) {
              info->id = MAIN_FIRST + i;
              WinSetWindowPtr(hwndTemp,0,info);
              xNext += (xIcon + 4);
              if(i + 1 < td->ulCount && info->next &&
                 xNext + xIcon >= xScreen) {
                xNext = 4L;
                yNext += (yIcon + 8);
              }
              hptr = (HPOINTER)WinSendMsg(hwndTemp,SM_QUERYHANDLE,
                                          MPVOID,MPVOID);
              if(hptr)
                WinDestroyPointer(hptr);
              hptr = (HPOINTER)WinSendMsg(info->swe.swctl.hwnd,
                                          WM_QUERYICON,MPVOID,MPVOID);
              if(hptr == (HPOINTER)0)
                hptr = hptrApp;
              WinSendMsg(hwndTemp,SM_SETHANDLE,MPFROMLONG(hptr),MPVOID);
            }
            info = info->next;
          }
          WinSetWindowPos(WinWindowFromID(hwnd,MAIN_DATETIME),HWND_TOP,4,4,
                          74,yIcon,SWP_SIZE | SWP_MOVE | SWP_NOADJUST);
          if(fProcs)
            WinSetWindowPos(WinWindowFromID(hwnd,MAIN_PROCS),HWND_TOP,82,4,
                            74,yIcon,SWP_SHOW | SWP_SIZE | SWP_MOVE |
                            SWP_NOADJUST);
          else
            WinShowWindow(WinWindowFromID(hwnd,MAIN_PROCS),FALSE);
          WinSetWindowPos(WinWindowFromID(hwnd,MAIN_VIRTUAL),HWND_TOP,
                          (74 * ((fProcs != 0) + 1)) + 8 +
                           ((fProcs != 0) * 4),4,
                          xIcon,yIcon,SWP_SIZE | SWP_MOVE | SWP_NOADJUST);
          /* show taskbar */
          WinSetWindowPos(hwnd,HWND_TOP,0,0,xScreen,yNext + yIcon + 4,
                          SWP_SHOW | SWP_ACTIVATE | SWP_ZORDER);
          fWinShowing = TRUE;   /* so hook knows we're showing */
          PostMsg(hwnd,UM_TIMER,MPVOID,MPVOID);  /* update clock */
          if(fProcs)
            PostMsg(hwnd,UM_PROCS,MPVOID,MPVOID);
        }
        else {
          if(!fStayUp)
            fWinShowing = FALSE;
          else {
            WinSetWindowPos(WinWindowFromID(hwnd,MAIN_DATETIME),HWND_TOP,4,4,
                            74,yIcon,SWP_SIZE | SWP_MOVE | SWP_NOADJUST);
            if(fProcs)
              WinSetWindowPos(WinWindowFromID(hwnd,MAIN_PROCS),HWND_TOP,82,4,
                              74,yIcon,SWP_SHOW | SWP_SIZE | SWP_MOVE |
                              SWP_NOADJUST);
            else
              WinShowWindow(WinWindowFromID(hwnd,MAIN_PROCS),FALSE);
            WinSetWindowPos(WinWindowFromID(hwnd,MAIN_VIRTUAL),HWND_TOP,
                            (74 * ((fProcs != 0) + 1)) + 8 +
                             ((fProcs != 0) * 4),4,
                            xIcon,yIcon,SWP_SIZE | SWP_MOVE | SWP_NOADJUST);
            fWinShowing = TRUE;   /* so hook knows we're showing */
            PostMsg(hwnd,UM_TIMER,MPVOID,MPVOID);  /* update clock */
            if(fProcs)
              PostMsg(hwnd,UM_PROCS,MPVOID,MPVOID);
            WinSetWindowPos(hwnd,HWND_TOP,0,0,xScreen,yNext + yIcon + 4,
                            SWP_SIZE | SWP_SHOW);
          }
        }
      }
      return MRFROMLONG(fWinShowing);  /* hook checks return */

    case WM_PAINT:
      {
        HPS    hps;
        RECTL  rclUpdate;
        POINTL ptl;
        SWP    swp;

        hps = WinBeginPaint(hwnd,NULLHANDLE,(PRECTL)&rclUpdate);
        if(hps) {
          WinFillRect(hps,(PRECTL)&rclUpdate,CLR_PALEGRAY);
          WinQueryWindowPos(hwnd,&swp);
          ptl.x = 0;
          ptl.y = 0;
          GpiMove(hps,&ptl);
          GpiSetColor(hps,CLR_DARKGRAY);
          ptl.x = swp.cx - 1;
          GpiLine(hps,&ptl);
          ptl.y = swp.cy - 1;
          GpiLine(hps,&ptl);
          GpiSetColor(hps,CLR_WHITE);
          ptl.x = 0;
          GpiLine(hps,&ptl);
          ptl.y = 0;
          GpiLine(hps,&ptl);
          WinEndPaint(hps);
        }
      }
      break;

    case WM_BUTTON1CLICK:
    case WM_BUTTON3CLICK:
      WinSetWindowPos(hwnd,HWND_TOP,0,0,0,0,SWP_ZORDER | SWP_ACTIVATE);
      WinInvalidateRect(hwnd,NULL,TRUE);
      break;

    case UM_CONTEXTMENU:
    case WM_CONTEXTMENU:
      td = WinQueryWindowPtr(hwnd,0);
      {
        POINTL ptl;

        if(!td->hwndMenu)
          td->hwndMenu = WinLoadMenu(HWND_DESKTOP,0,MAIN_ID);
        if(td->hwndMenu) {
          WinQueryPointerPos(HWND_DESKTOP,&ptl);
          ptl.y = yNext + yIcon + 5;
          WinMapWindowPoints(HWND_DESKTOP,hwnd,&ptl,1L);
          if(WinPopupMenu(hwnd,hwnd,td->hwndMenu,ptl.x,ptl.y,0,
                          PU_HCONSTRAIN | PU_VCONSTRAIN |
                          PU_KEYBOARD   | PU_MOUSEBUTTON1))
            fMenuShowing = TRUE;
        }
      }
      if(msg == UM_CONTEXTMENU)
        return 0;
      break;

    case UM_MENUEND:
      fMenuShowing = FALSE;
      return 0;

    case WM_MENUEND:
      PostMsg(hwnd,UM_MENUEND,mp1,mp2);
      break;

    case WM_COMMAND:
      switch(SHORT1FROMMP(mp1)) {
        case IDM_LOGO:
          fDlgShowing = TRUE;
          WinDlgBox(HWND_DESKTOP,hwnd,StartUpProc,0,START_FRAME,(PVOID)&hwnd);
          fDlgShowing = FALSE;
          break;
        case IDM_HELP:
          WinMessageBox(HWND_DESKTOP,HWND_DESKTOP,
                        "Ain't no steenking help.  Request a context menu in "
                        "various places for the \"right\" commands for those "
                        "places.  Click window icons to show the windows.  "
                        "Double-click the date/time window to set the time.  "
                        "Click in one of the virtual desktop areas to switch "
                        "desktops or CTRL-ALT-> and CTRL-ALT-< to switch "
                        "programs via keyboard (if not disabled).",
                        pgmname,0,
                        MB_ICONASTERISK | MB_ENTER | MB_MOVEABLE);
          break;
        case IDM_ABOUT:
          {
            CHAR s[256];

            sprintf(s,"%s mini-app\n"
                        "Free software from Mark Kimes.\n"
                        "A handy companion to the OS/2 Window List and "
                        "Launchpad.\n\n"
                        "This copy of FM/2 Taskbar is version %s",
                        pgmname,VERSION);
            WinMessageBox(HWND_DESKTOP,HWND_DESKTOP,s,pgmname,0,
                          MB_ICONASTERISK | MB_ENTER | MB_MOVEABLE);
          }
          break;
        case IDM_CLOSE:
          WinDestroyWindow(hwnd);
          break;

        case IDM_SETUP:
          fDlgShowing = TRUE;
          WinDlgBox(HWND_DESKTOP,hwnd,SetupProc,0,SET_FRAME,NULL);
          fDlgShowing = FALSE;
          break;

        case IDM_EDITTEXCLUDES:
        case IDM_EDITVEXCLUDES:
          {
            CHAR *name = "TEXCLUDE.LST";
            CHAR  fullname[CCHMAXPATH];
            FILE *fp;

            if(SHORT1FROMMP(mp1) == IDM_EDITVEXCLUDES)
              name = "VEXCLUDE.LST";
            if(DosQueryPathInfo(name,FIL_QUERYFULLNAME,fullname,
                                sizeof(fullname)))
              strcpy(fullname,name);
            fp = fopen(fullname,"a+");
            if(fp) {
              fseek(fp,0L,SEEK_END);
              if(!ftell(fp))
                fprintf(fp,";\n;Add items to exclude from %s to this file,\n"
                           ";one per line, as desired.  The text you add should be\n"
                           ";the title bar text (can be partial).\n;\n",
                           (SHORT1FROMMP(mp1) == IDM_EDITVEXCLUDES) ?
                           "being moved on virtual desktops" : "the Taskbar");
              fclose(fp);
            }
            OpenObject(hwnd,fullname,"DEFAULT");
          }
          break;

        case IDM_RELOADTEXCLUDES:
          if(texclude) {

            INT x;

            for(x = 0;texclude[x];x++)
              free(texclude[x]);
            free(texclude);
            _heapmin();
            texclude = NULL;
          }
          texclude = LoadExcludes(NULL);
          break;

        case IDM_RELOADVEXCLUDES:
          if(vexclude) {

            INT x;

            for(x = 0;vexclude[x];x++)
              free(vexclude[x]);
            free(vexclude);
            _heapmin();
            vexclude = NULL;
          }
          vexclude = LoadExcludes("VEXCLUDE.LST");
          break;
      }
      PostMsg(hwnd,UM_HIDE,MPVOID,MPVOID);
      return 0;

    case WM_CLOSE:
      break;

    case WM_DESTROY:
      if(timerstarted)
        WinStopTimer(WinQueryAnchorBlock(hwnd),hwnd,MAIN_TIMER);
      PostMsg((HWND)0,WM_QUIT,MPVOID,MPVOID);
      break;
  }
  return WinDefWindowProc(hwnd,msg,mp1,mp2);
}


VOID APIENTRY CleanUp (ULONG why) {

  if(!StopHooks())
    DosBeep(50,100);
  if(tbarprof)
    PrfCloseProfile(tbarprof);
  tbarprof = (HINI)0;
  DosExitList(EXLST_REMOVE,CleanUp);
}


int main (void) {

  HAB               hab;
  HMQ               hmq;
  QMSG              qmsg;
  static CLASSINFO  clinfo;

  // fStayUp = TRUE;			// Enable for debug

  pgmname = "FM/2 Taskbar";
  hab = WinInitialize(0);
  if(hab) {
    UpdateVals((HWND)0);
    hmq = WinCreateMsgQueue(hab,384);
    if(hmq) {
      if(WinQueryClassInfo(hab,WC_STATIC,&clinfo) && clinfo.pfnWindowProc) {
        PFNWPStaticProc = clinfo.pfnWindowProc;
        if(InitDLL(hab) && StartInputHook()) {
          StartSendHook();
          DosExitList(EXLST_ADD,CleanUp);
          UpdateVals((HWND)0);
          tbarprof = PrfOpenProfile(hab,"FTASKBAR.INI");
          if(tbarprof) {

            ULONG size;

            size = sizeof(BOOL);
            PrfQueryProfileData(tbarprof,"FM/3","ReposMouse",
                                (PVOID)&fReposMouse,&size);
            size = sizeof(BOOL);
            PrfQueryProfileData(tbarprof,"FM/3","Procs",
                                (PVOID)&fProcs,&size);
            size = sizeof(BOOL);
            PrfQueryProfileData(tbarprof,"FM/3","SlidingFocus",
                                (PVOID)&fSlidingFocus,&size);
            size = sizeof(BOOL);
            PrfQueryProfileData(tbarprof,"FM/3","NoZorderChange",
                                (PVOID)&fNoZorderChange,&size);
            size = sizeof(BOOL);
            PrfQueryProfileData(tbarprof,"FM/3","MoveFolders",
                                (PVOID)&fMoveFolders,&size);
            size = sizeof(BOOL);
            PrfQueryProfileData(tbarprof,"FM/3","B2Bottom",
                                (PVOID)&fB2Bottom,&size);
            size = sizeof(BOOL);
            PrfQueryProfileData(tbarprof,"FM/3","B3Close",
                                (PVOID)&fB3Close,&size);
            size = sizeof(BOOL);
            PrfQueryProfileData(tbarprof,"FM/3","B3DblClk",
                                (PVOID)&fB3DblClk,&size);
            size = sizeof(BOOL);
            PrfQueryProfileData(tbarprof,"FM/3","LeftShow",
                                (PVOID)&fLeft,&size);
            size = sizeof(BOOL);
            PrfQueryProfileData(tbarprof,"FM/3","RightShow",
                                (PVOID)&fRight,&size);
            size = sizeof(BOOL);
            PrfQueryProfileData(tbarprof,"FM/3","CenterShow",
                                (PVOID)&fCenter,&size);
            size = sizeof(LONG);
            PrfQueryProfileData(tbarprof,"FM/3","HeightShow",
                                (PVOID)&yHi,&size);
            size = sizeof(LONG);
            PrfQueryProfileData(tbarprof,"FM/3","Animate",
                                (PVOID)&ulAnimate,&size);
            if(ulAnimate)
              ulAnimate = WS_ANIMATE;
            size = sizeof(BOOL);
            PrfQueryProfileData(tbarprof,"FM/3","DisableDesktops",
                                (PVOID)&fDisableDesktops,&size);
            size = sizeof(BOOL);
            PrfQueryProfileData(tbarprof,"FM/3","NoHot",
                                (PVOID)&fNoHot,&size);
            size = sizeof(BOOL);
            PrfQueryProfileData(tbarprof,"FM/3","LeftHot",
                                (PVOID)&fLeftHot,&size);
            size = sizeof(BOOL);
            PrfQueryProfileData(tbarprof,"FM/3","WrapMouse",
                                (PVOID)&fWrapMouse,&size);
            size = sizeof(BOOL);
            fShowMiniwins = TRUE;
            PrfQueryProfileData(tbarprof,"FM/3","ShowMiniwins",
                                (PVOID)&fShowMiniwins,&size);
            size = sizeof(BOOL);
            PrfQueryProfileData(tbarprof,"FM/3","CtrlAltTab",
                                (PVOID)&fCtrlTab,&size);
            texclude = LoadExcludes(NULL);
            vexclude = LoadExcludes("VEXCLUDE.LST");
          }
          WinRegisterClass(hab,pgmname,MainWndProc,CS_CLIPCHILDREN,
                           sizeof(PVOID));
          WinRegisterClass(hab,"Virtual Desktops",VirtualProc,0,sizeof(PVOID));
          WinRegisterClass(hab,"Task Buttons",SwitchButtonProc,0,sizeof(PVOID));
          hptrApp = WinLoadPointer(HWND_DESKTOP,0,APP_POINTER);
          hptrBlank = WinLoadPointer(HWND_DESKTOP,0,BLANK_POINTER);
          hwndAppMenu = WinLoadMenu(HWND_DESKTOP,0,MAIN_VIRTUAL);
          hwndTaskbar = WinCreateWindow(HWND_DESKTOP,pgmname,
                                        pgmname,
                                        ulAnimate,
                                        0,0,xScreen,yIcon + 8,
                                        HWND_DESKTOP,HWND_TOP,
                                        MAIN_ID,NULL,NULL);
          if(hwndTaskbar) {
            DosSetPriority(PRTYS_THREAD,PRTYC_REGULAR,31L,0L);
            if(fStayUp)
              PostMsg(hwndTaskbar,UM_SHOW,MPVOID,MPVOID);
            while(WinGetMsg(hab,&qmsg,(HWND)0,0,0))
              WinDispatchMsg(hab,&qmsg);
            DosSetPriority(PRTYS_THREAD,PRTYC_REGULAR,1L,0L);
          }
        }
      }
      CleanUp(0L);
      WinDestroyMsgQueue(hmq);
    }
    WinTerminate(hab);
  }
  return 0;
}
