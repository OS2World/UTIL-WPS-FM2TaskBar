/****************************************************************************
    taskhook.c  - input hook DLL

    FM/2 Taskbar is a WPS enhancement
    copyright (c) 2001 by Mark Kimes

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

Revisions	19 Aug 01 MK - Release
		25 Sep 01 SHL - Add missing #includes

*****************************************************************************/


#define INCL_DOS
#define INCL_WIN

#include <os2.h>

#include <string.h>

#include "taskbar.h"

#pragma alloc_text(ONCE,InitDLL,StartInputHook,StartSendHook,StopHooks)

typedef struct HOOKED {
  HWND           hwnd;
  PFNWP          oldproc;
  struct HOOKED *next;
} HOOKED;

/***********************************************************************/
/*  Global variables.                                                  */
/***********************************************************************/
HAB     habDLL;
HWND    hwndTaskbar,hwndDesktop,hwndWPS,hwndBottom;
HMODULE hMod;
PFN     pfnInput,pfnSend;
BOOL    fWinShowing,fMenuShowing,fSlidingFocus,fNoZorderChange,
        fB2Bottom,fB3Close,fLeft = TRUE,fRight,fCenter = TRUE,
        fDlgShowing,fB3DblClk,fKeyShowing,fNoHot,fLeftHot,fWrapMouse,
        fCtrlTab = TRUE,fStayUp;
LONG    xScreen,yScreen,xIcon,yIcon,yNext,yHi = 4;
PID     dtPid,myPid;


#define KILLMSG()  {pqMsg->hwnd = NULLHANDLE; return(TRUE);}


BOOL PostMsg (HWND h, ULONG msg, MPARAM mp1, MPARAM mp2) {

  BOOL rc = WinPostMsg(h,msg,mp1,mp2);

  if(!rc) {

    PIB *ppib;
    TIB *ptib;

    if(!DosGetInfoBlocks(&ptib,&ppib)) {

      PID pid;
      TID tid;

      if(WinQueryWindowProcess(h,&pid,&tid)) {
        if(pid != ppib->pib_ulpid || tid != ptib->tib_ptib2->tib2_ultid) {
          for(;;) {
            DosSleep(1L);
            rc = WinPostMsg(h,msg,mp1,mp2);
            if(!rc) {
              if(!WinIsWindow((HAB)0,h))
                break;
            }
            else
              break;
          }
        }
      }
    }
  }
  return rc;
}


/***********************************************************************/
/*  InitDLL: This function sets up the DLL and sets all variables      */
/***********************************************************************/
BOOL EXPENTRY InitDLL (HAB hab) {

  habDLL = hab;

  yIcon = WinQuerySysValue(HWND_DESKTOP,SV_CYICON);

  if(!DosLoadModule(NULL,0,"TASKHOOK",&hMod) &&
     !DosQueryProcAddr(hMod,4,"InputProc",&pfnInput) &&
     !DosQueryProcAddr(hMod,34,"SendProc",&pfnSend))
    return TRUE;
  return FALSE;
}


/***********************************************************************/
/*  StartInputHook: This function starts the hook filtering.           */
/***********************************************************************/
BOOL EXPENTRY StartInputHook (void) {

  return WinSetHook(habDLL,NULLHANDLE,HK_INPUT,pfnInput,hMod);
}


/***********************************************************************/
/*  StartSendHook: This function starts the hook filtering.            */
/***********************************************************************/
BOOL EXPENTRY StartSendHook (void) {

  return WinSetHook(habDLL,NULLHANDLE,HK_SENDMSG,pfnSend,hMod);
}


/***********************************************************************/
/*  StopHooks: This function stops the hook filtering.             */
/***********************************************************************/
BOOL EXPENTRY StopHooks (void) {

  if(WinReleaseHook(habDLL,NULLHANDLE,HK_INPUT,pfnInput,hMod) &&
     (!pfnSend || WinReleaseHook(habDLL,NULLHANDLE,HK_SENDMSG,pfnSend,hMod)) &&
     !DosFreeModule(hMod))
    return TRUE;
  return FALSE;
}


/***********************************************************************/
/*  InputProc: This is the input filter routine.                       */
/*  While the hook is active, all messages come here                   */
/*  before being dispatched.                                           */
/***********************************************************************/
BOOL EXPENTRY InputProc (HAB hab, PQMSG pqMsg, ULONG fs) {

  /* mouse is captured -- ignore msg */
  if(WinQueryCapture(HWND_DESKTOP) != NULLHANDLE)
    return FALSE;
  switch(pqMsg->msg) {
    case WM_MOUSEMOVE:
      switch(fWinShowing) {
        case FALSE:
          if(!fDlgShowing && !fKeyShowing) {
            if(pqMsg->ptl.y < yHi && ((fLeft && fRight && fCenter) ||
               (fLeft && pqMsg->ptl.x < (xScreen / 4)) ||
               (fRight && pqMsg->ptl.x > (xScreen / 4) * 3) ||
               (fCenter && (pqMsg->ptl.x > (xScreen / 4) - 4) &&
                            pqMsg->ptl.x < ((xScreen / 4) * 3) + 4))) {
              fWinShowing = (BOOL)WinSendMsg(hwndTaskbar,UM_SHOW,
                                             MPVOID,MPVOID);
              break;
            }
            if(fWrapMouse && (pqMsg->ptl.x <= 0 ||
                              pqMsg->ptl.x >= xScreen - 1 ||
                              pqMsg->ptl.y <= 0 ||
                              pqMsg->ptl.y >= yScreen - 1)) {
              if(pqMsg->ptl.x <= 0)
                pqMsg->ptl.x = xScreen - 2;
              else if(pqMsg->ptl.x >= xScreen - 1)
                pqMsg->ptl.x = 1;
              else if(pqMsg->ptl.y <= 0)
                pqMsg->ptl.y = yScreen - 2;
              else if(pqMsg->ptl.y >= yScreen - 1)
                pqMsg->ptl.y = 1;
              WinSetPointerPos(HWND_DESKTOP,pqMsg->ptl.x,pqMsg->ptl.y);
              break;
            }
          }
          /*                                                                                      *\
           * If enabled, here we catch all mouse movements, to set the window under the mouse     *
           * pointer as the active one, if it isn't currently active or the window list or        *
           * optionally the Desktop window.                                                       *
          \*                                                                                      */
          /* If enabled, use sliding focus to activate window
             under the mouse pointer (with some exceptions).
             Caution! Menus have a class WC_MENU, but their
             parent is not the frame window WC_FRAME but the
             Desktop itself. */
          while(fSlidingFocus) {

            static UCHAR    ucClassname[7];     /* Window class f.e. #1 for WC_FRAME */
            static UCHAR    ucWindowText[33];   /* Window name f.e. OS/2 2.0 Desktop */
            static HWND     hwndActive;         /* Window handle of active frame class window on Desktop */
            static HWND     hwndApplication;    /* Window handle of application under mouse pointer */
            static HWND     hwndTitlebar;
                                                /* Window handle of applications parent window */
            static HWND     hwndApplicationParent;
            static HWND     hwndTop;            /* Window handle on top of zorder */
            BOOL            ctrldown = FALSE;
            HENUM           henum;

            if(WinGetKeyState(HWND_DESKTOP, VK_SHIFT) & 0x8000)
                return(FALSE);
            if(WinGetKeyState(HWND_DESKTOP, VK_CTRL) & 0x8000)
                ctrldown = TRUE;
            /* Query the currently active window, where HWND_DESKTOP
               is the parent window. It will be a WC_FRAME class
               window */
            hwndActive = WinQueryActiveWindow(HWND_DESKTOP);
            if(fNoZorderChange && ctrldown) {
              henum = WinBeginEnumWindows(HWND_DESKTOP);
              hwndTop = WinGetNextWindow(henum);
              WinEndEnumWindows(henum);
            }
            else
              hwndTop = (HWND)0;
            WinQueryWindowText(hwndActive,sizeof(ucWindowText),
                               ucWindowText);
            /* Don't switch away from the WC_FRAME class tasklist */
            if(!strcmp(ucWindowText,"Window List"))
              break;
            /* Get message target window */
            hwndApplication = pqMsg->hwnd;
            /* If the window under the mouse pointer is one of the
               Desktops, don't do any changes */
            if((hwndApplication == hwndDesktop) ||
               (hwndApplication == hwndWPS))
              break;
            /* Get parent window of current window */
            hwndApplicationParent = WinQueryWindow(hwndApplication,
                                                   QW_PARENT);
            /* Loop until we get the Desktop window handle. The
               previous child window of the Desktop is then the
               WC_FRAME class window of the point under the mouse
               pointer which is not the Desktop. */
            while(hwndApplicationParent != hwndDesktop) {
              hwndApplication = hwndApplicationParent;
              hwndApplicationParent = WinQueryWindow(hwndApplication,
                                                     QW_PARENT);
            }
            /* Query the class of the frame window of the
               designated target of WM_MOUSEMOVE */
            WinQueryClassName(hwndApplication, sizeof(ucClassname),
                              ucClassname);
            /* Don't switch to menu windows */
            if(!strcmp(ucClassname,"#4"))
              return(FALSE);
            /* Don't switch to a combobox's listbox windows */
            if(!strcmp(ucClassname,"#7"))
              return(FALSE);
            /* Query the frame window name of the designated
               target of WM_MOUSEMOVE */
            WinQueryWindowText(hwndApplication, sizeof(ucWindowText),
                               ucWindowText);
            /* Don't switch to seamless Win-OS2 menus */
            if(strstr(ucWindowText,"Seamless"))
              return(FALSE);
            /* Sort with expected descending probability, to avoid
               unnecessary cpu load */
            for (;;) {
              /* Don't switch if previous windows equals current one */
              if(hwndActive == hwndApplication) {
                if(hwndTop == hwndApplication || !fNoZorderChange ||
                   !ctrldown)
                break;
              }
              /* Only switch to WC_FRAME class windows */
              if(strcmp(ucClassname,"#1"))
                break;
              if(!ctrldown && fNoZorderChange) {
                /* Change focus, but preserve Z-order */
                /* Don't send WM_ACTIVATE to window with new focus */
                WinFocusChange(HWND_DESKTOP,WinWindowFromID(hwndApplication,
                               FID_CLIENT),FC_NOSETACTIVE);
                /* Activate new window */
                if((hwndTitlebar = WinWindowFromID(hwndApplication,
                                                   FID_TITLEBAR)) != (HWND)0)
                 PostMsg(hwndApplication, WM_ACTIVATE, MPFROMSHORT(TRUE),
                         MPFROMHWND(hwndTitlebar));
              }
              else if(ctrldown && fNoZorderChange)
                WinSetActiveWindow(HWND_DESKTOP,hwndApplication);
              else
                /* Switch to the new frame window. It will generate
                   all messages of deactivating old and activating
                   new frame window */
                WinSetFocus(HWND_DESKTOP,WinWindowFromID(hwndApplication,
                            FID_CLIENT));
              /* We changed the focus, don't pass this message to
                 the next hook in the chain */
              return(TRUE);
            }
            break;                              /* Exit loop now */
          }
          break;

        default:
          if(!fStayUp && !fDlgShowing && !fMenuShowing && !fKeyShowing &&
             pqMsg->ptl.y > yNext + yIcon + 4)
            fWinShowing = (BOOL)WinSendMsg(hwndTaskbar,UM_HIDE,MPVOID,MPVOID);
          break;
      }
      break;

    case WM_BUTTON2DOWN:
      if(!fDlgShowing && fB2Bottom) {

        static HWND hwndFrame;

        hwndFrame = WinQueryWindow(pqMsg->hwnd,QW_PARENT);
        /* If we click on a titlebar the current window handle
           equals the parent's titlebar window handle. If the
           Shift key is pressed we pass this message to the titlebar
           instead of processing to change its z-order */
        if((pqMsg->hwnd == WinWindowFromID(hwndFrame,FID_TITLEBAR)) &&
           (!(WinGetKeyState(HWND_DESKTOP,VK_SHIFT) & 0x8000))) {
          /* Set it to the bottom of all windows */
          WinSetWindowPos(hwndFrame,HWND_BOTTOM,0,0,0,0,
                          SWP_ZORDER | SWP_DEACTIVATE);
          KILLMSG();
        }
      }
      break;

    case WM_BUTTON3UP:
      if(fB3DblClk && !(WinGetKeyState(HWND_DESKTOP,VK_SHIFT) & 0x8000))
        KILLMSG();
      break;

    case WM_BUTTON3DOWN:
      if(!fDlgShowing && (fB3Close || fB3DblClk) &&
         !(WinGetKeyState(HWND_DESKTOP,VK_SHIFT) & 0x8000)) {

        static HWND hwndFrame;

        if(fB3Close) {
          hwndFrame = WinQueryWindow(pqMsg->hwnd,QW_PARENT);
          /* If we click on a titlebar the current window handle
             equals the parent's titlebar window handle. If the
             Shift key is pressed we pass this message to the titlebar
             instead of telling it to die. */
          if(pqMsg->hwnd == WinWindowFromID(hwndFrame,FID_TITLEBAR)) {
            /* close it */
            PostMsg(hwndFrame,WM_SYSCOMMAND,
                    MPFROM2SHORT(SC_CLOSE,0),MPVOID);
            KILLMSG();
            break;
          }
        }
        if(fB3DblClk) {

          static HWND hwndFrame;

          hwndFrame = WinQueryWindow(pqMsg->hwnd,QW_PARENT);
          /* If we click on a titlebar the current window handle
             equals the parent's titlebar window handle. If the
             Shift key is pressed we pass this message to the titlebar
             instead of processing to change its z-order */
          if(pqMsg->hwnd == WinWindowFromID(hwndFrame,FID_TITLEBAR)) {
//            PostMsg(pqMsg->hwnd,WM_BUTTON1DOWN,pqMsg->mp1,pqMsg->mp2);
            PostMsg(pqMsg->hwnd,WM_BUTTON1DBLCLK,pqMsg->mp1,pqMsg->mp2);
//            PostMsg(pqMsg->hwnd,WM_OPEN,pqMsg->mp1,pqMsg->mp2);
          }
          else {
            PostMsg(pqMsg->hwnd,WM_BUTTON1DOWN,pqMsg->mp1,pqMsg->mp2);
            PostMsg(pqMsg->hwnd,WM_BUTTON1UP,pqMsg->mp1,pqMsg->mp2);
            PostMsg(pqMsg->hwnd,WM_BUTTON1CLICK,pqMsg->mp1,pqMsg->mp2);
            PostMsg(pqMsg->hwnd,WM_SINGLESELECT,pqMsg->mp1,pqMsg->mp2);
            PostMsg(pqMsg->hwnd,WM_BUTTON1DBLCLK,pqMsg->mp1,pqMsg->mp2);
            PostMsg(pqMsg->hwnd,WM_OPEN,pqMsg->mp1,pqMsg->mp2);
            PostMsg(pqMsg->hwnd,WM_BUTTON1UP,pqMsg->mp1,pqMsg->mp2);
          }
          KILLMSG();
//          pqMsg->msg = WM_BUTTON1DBLCLK;  /* this sorta works */
        }
      }
      break;

    case WM_CHAR:
      if(!fNoHot && !fKeyShowing && !fDlgShowing) {

        USHORT usFlags    = SHORT1FROMMP(pqMsg->mp1);
        UCHAR  ucScanCode = CHAR4FROMMP(pqMsg->mp1);

        if(usFlags & KC_KEYUP)
          break;
        if((usFlags & (KC_SCANCODE | KC_CTRL | KC_ALT)) !=
           (KC_SCANCODE | KC_CTRL | KC_ALT))
          break;
        if((fCtrlTab && ucScanCode == 15) ||
          (fLeftHot && (ucScanCode == 44 || ucScanCode == 45)) ||
          (!fLeftHot && (ucScanCode == 51 || ucScanCode == 52))) {
          fKeyShowing = TRUE;
          if(!PostMsg(hwndTaskbar,UM_ALTTAB,MPFROMLONG((LONG)ucScanCode),
                      MPVOID))
            fKeyShowing = FALSE;
        }
      }
      break;

    case WM_TASKLIST:
      PostMsg(hwndTaskbar,UM_TASKLIST,pqMsg->mp1,pqMsg->mp2);
      break;
  }
  /* Pass the message on to the next hook in line. */
  return FALSE;
}


VOID EXPENTRY SendProc (HAB hab,PSMHSTRUCT psmh,BOOL fInterTask) {

  static UCHAR ucClassname[7];     /* Window class f.e. #1 for WC_FRAME */

  switch(psmh->msg) {
    case WM_CREATE:
      WinQueryClassName(psmh->hwnd,sizeof(ucClassname),ucClassname);
      if(!strcmp(ucClassname,"#1") || !strcmp(ucClassname,"Folder")) {

      }
      break;

    case WM_DESTROY:
      break;
  }
}
