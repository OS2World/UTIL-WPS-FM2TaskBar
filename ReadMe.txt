
TASKBAR.EXE version 1.08                            		31 Oct 2003
======================== 


 ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
 ³                FM/2 Taskbar is free software from Mark Kimes             ³
 ³                                  v1.08                                   ³
 ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
 ³                       Read this file before installing                   ³
 ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

The FM/2 Taskbar is a small utility program that works in conjunction
with the WPS Window List, program objects and Launchpad to control
what's running on your system and where it runs.  It's much harder to
explain than to try, and since installation is quick and painless and a
deinstallation program is included, I encourage you to try it and see
what you think.  See sections below for the information you need to get
started.  Warning:  A pointing device _is_ required; the Taskbar would
make little sense without one.


Installation
============

First, unpack the archive into a (preferably empty) directory.  Now run
the TINSTAL.CMD file to create a WPS object for the Taskbar.  You're
given a choice to create the object on the Desktop or in the Startup
folder.  You're done.


Uninstalling
============

If you try the Taskbar and decide you don't like it, run TUINSTAL.CMD
in the Taskbar directory to remove it.


Starting TASKBAR
================

Once the object is created, double-click it to run it.  If the object is
in the Startup folder, OS/2 will start it every time you boot up OS/2.


Using TASKBAR
=============

Here's a simple diagram showing the layout of the Taskbar:

 ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
 ³                                                                           ³
 ³                                                                           ³
 ³                                                                           ³
 ³                                                                           ³
 ³                                                                           ³
 ÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\/\ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁ
                                 Desktop
 ÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ/\/ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂ
 ³                                                                           ³
 ³                                                                           ³
 ³    ÚÄÄTaskbar                                                             ³
 ³    ³                                                                      ³
 ³    ³                                                                      ³
 ÃÄÄÄÄÁÄÂÄÂÄÂÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
 ³ Time/ÃÄÅÄÅÄ´ÚÄÄ¿ ÚÄÄ¿ ÚÄÄ¿ ÚÄÄ¿ ÚÄÄ¿ ÚÄÄ¿ ÚÄÄ¿ ÚÄÄ¿ ÚÄÄ¿ ÚÄÄ¿ ÚÄÄ¿        ³
 ³ Date ÃÄÅÅÅÄ´ÀÄÄÙ ÀÄÄÙ ÀÄÄÙ ÀÄÄÙ ÀÂÄÙ ÀÄÄÙ ÀÄÄÙ ÀÄÄÙ ÀÄÄÙ ÀÄÄÙ ÀÄÂÙ        ³
 ÀÄÄÄÄÄÄÁÄÁÅÁÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÙ
           ÀÄÄVirtual desktops      ÀÄÄIcons of running programsÄÄÄÙ

After starting the Taskbar from the WPS object the TINSTAL.CMD program
creates for you, move the mouse pointer to the bottom of the screen.
The Taskbar pops up and remains up as long as you keep the mouse pointer
within its window.  When you move the mouse outside the window, the
window vanishes until you call it up again.  This "popup" action ensures
that the Taskbar isn't occupying valuable Desktop space when you don't
need it, but is always available when you do.

As you pass the mouse pointer over the icons of running programs, a
caption window appears showing the name of the program the icons
represent.  Click on one of these icons with mouse button one (usually
the left button), and the program is brought to the foreground, and is
restored if it was hidden or minimized.  You can request a context menu
on these icons (click mouse button two, usually the right button) for
more options (Show, Hide, Move to this desktop, Close and Kill).  The
"current" window is shown extruded (an outtie instead of an innie, in
navel terms -- jelly baby, anyone?).

The Time/Date field can be double-clicked to bring up a Settings
notebook to allow you to set the time and date.  A context menu
requested on this field is the same as a context menu requested over an
empty part of the Taskbar, with various options to control how the
Taskbar operates, and to close the Taskbar (we'll get to that in a
moment).  If you have a three-button mouse and a driver that recognizes
the third button, clicking it in this field will display the system
clock (assuming you haven't told the Taskbar to change button three
presses to button one double-clicks in the Taskbar settings -- see
below).

The Virtual desktop field allows you to have nine separate workspaces on
your desktop.  Click mouse button one in one of the nine squares of the
grid, and the desktop "switches" to that workspace.  (If you close the
Taskbar, all programs are brought onto the actual desktop so none are
left stranded where you can't get to them without a lot of work.)
Requesting a context menu on this field allows you to start command
lines or open FM/2 or several WPS objects.  The context menu also allows
you to hide or restore all open windows.

If you have the Settings toggle "Show process and thread counts" turned
on another window appears between the Time/Date window and the Virtual
desktops window that shows the number of processes and threads running
on your system at the time the Taskbar appeared.  Double-clicking this
window causes the FM/2 Process Killer program to be run.  Clicking it
with the third button causes the FM/2 System Information program to be
run (if you don't have button three emulating a button one
double-click).

You can quickly switch programs using the keyboard by pressing and
holding the CTRL and ALT keys, then pressing either the "<" (comma) or
">" (period) keys.  The Taskbar displays a small window showing the icon
and name of the program that would be brought to the foreground if you
release the CTRL and ALT keys.  Use < or > to cycle through open
applications until you find the one you want, then release the CTRL and
ALT keys, or press ESCape to cancel the operation.  You can also use the
CTRL, ALT and Tab keys together to switch tasks unless you disable them.

Requesting a context menu on the Time/Date field or any empty part of
the Taskbar brings up a menu that allows you to change Taskbar settings
or close (exit) the Taskbar.  An About box and (very) brief help are
available here as well.  Some things that might be non-obvious are
detailed below:

  Settings:  Opens a dialog where you can adjust the Taskbar settings.
  In the dialog, you can set what part of the bottom of the screen will
  activate the taskbar, how low the mouse pointer must go in that area,
  and whether the taskbar window is "animated" or not.

    Button 2 send to bottom:  If checked, clicking mouse button two on a
    window's titlebar will send it to the bottom of the window stack
    (all other open windows will appear on top of it).  Hold down the
    Shift key to temporarily override this action.

    Button 3 close:  If checked, clicking mouse button three on a
    window's titlebar will close the application.  Hold down the Shift
    key to temporarily override this action.

    Button 3 = B1 dbl clk:  If checked, the Taskbar translates a click
    of mouse button three (the center button on three-button mice) to a
    double-click of mouse button 1.  The WPS doesn't use button three
    directly, so this may be a way to get some use out of it.  You can
    hold down the Shift key to temporarily override this action.

    Sliding focus:  If checked, windows below the mouse pointer will be
    activated and brought to the top of the window stack.  Holding down
    the Shift key temporarily overrides this action.

    No zorder change:  Works in conjunction with Sliding focus.  If
    checked, windows will be activated but NOT brought to the top of the
    window stack.  Hold CTRL to temporarily override this action.

    Reposition mouse on show:  If checked, when you use the Taskbar
    icons to Show a window, the mouse pointer will be placed in the
    center of the window.

    Wrap mouse pointer at screen edges:  If checked, the mouse pointer
    will wrap at screen edges rather than stop as if a wall was reached.

    Move folders:  If not checked, the Taskbar always keeps WPS folders
    on the current desktop.

    Show detail in mini desktop:  Some programs cause a noticeable
    slowdown when the WinEnumWindows API is called.  If the Taskbar
    comes up, but the miniature virtual desktop window takes a second or
    two to display, try turning this off.

    Disable desktops:  Disables the virtual desktop feature of the
    Taskbar.

    Disable hotkey switching:  Disables CTRL+ALT+< and CTRL+ALT+> quick
    visual task switching.

    Left-hand hotkey switching:  Uses CTRL+ALT+Z and CTRL+ALT+X instead
    of the above keys.

    Also use CTRL-ALT-Tab switching:  The CTRL+ALT+TAB combination can
    also be used to switch tasks via keyboard.

  Reloading and editing exclude lists:  See "Settting" below.
  Normally, you don't need to worry about this at all.

My best advice to you regarding the settings above is to simply try them
and see what you like best.  You can't destroy your computer or OS/2
with them, so you've nothing to lose but the time it takes to turn
something back off or on; relax and experiment.


Settings
========

To exclude windows from the Taskbar display:  Enter their switch list
titles (as shown in the Window List), one per line, in a file called
"TEXCLUDE.LST" in the Taskbar's directory.  You can make this partial;
if the first part of the window's switch list title matches the line
in the file, it won't show on the Taskbar.  Use a standard text editor
to create the file (E.EXE, the system editor, will work fine).

To exclude windows from being moved when you select a virtual desktop,
enter their titlebar text, one per line, in a file called "VEXCLUDE.LST"
in the Taskbar's directory.  You can make this partial as above.

You can reload an exclude list after you've edited it via menu
selection.  Request a context menu on the Time/Date field or on a blank
area of the Taskbar.  You can also open the files from the context menu;
whatever editor you have assigned to the files via WPS associations is
used.  After editing, don't forget to reload it.

TASKBAR saves it's settings in FTASKBAR.INI under the Application name FM/2.
If you suspect these settings may corrupted, use an INI file editor to delete
the suspect keys.  They will be rebuilt the next time you restart TASKBAR.


Troubleshooting
===============

The only thing that I know of that could cause you problems with the
Taskbar is the LIBPATH line in CONFIG.SYS.  This line should contain
".\" to allow TASKHOOK.DLL to be found in TASKBAR.EXE's default
directory.  An example:
  LIBPATH=.\;C:\OS2\APPS\DLL;C:\MMOS2\DLL;C:\OS2\DLL;C:\OS2\MDOS;C:\;
This is the default for OS/2 installation, so you shouldn't encounter
a problem.

The Taskbar is meant to run in conjunction with the WPS.  Some
functionality will be missing if run without it, but it shouldn't blow
up.


Known problems/shortcomings
=========================== 

 - There must be some.


About TASKBAR
=============

TASKBAR was originally written by:

  Mark Kimes
  <hectorplasmic@worldnet.att.net>

He has kindly allowed me to take over maintenance and support of TASKBAR and to 
release the program under the GNU GPL license.  I'm sure he would appreciate 
a Thank You note for his generosity.

See TREADME.ORG for Mark's orginal comments.


Support
=======

Please address support questions and enhancement requests to:

  Steven H. Levine
  steve53@earthlink.net

I also monitor the comp.os.os2.apps newsgroup and others in the 
comp.os.os2.* hierarchy.

Thanks and enjoy.

$Id: ReadMe.txt,v 1.3 2003/11/01 07:35:28 root Exp $
