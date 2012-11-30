/*
 * FM/2 Taskbar installation program
 */
'@Echo off'
'cls'
/* tell the nice people who we are */
say ''
say ' 旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커'
say ' �                     FM/2 Taskbar installation program                      �'
say ' 읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸'
say ''
/* get current directory */
curdir = directory()
/* see if we have what we need to run */
rc = stream('taskhook.dll','c','query exists')
if rc = '' then
do
  say 'Sorry, TASKHOOK.DLL not found.  Must not be right directory.  Terminating.'
  exit
end
rc = stream('taskbar.exe','c','query exists')
if rc = '' then
do
  say 'Sorry, TASKBAR.EXE not found.  Must not be right directory.  Terminating.'
  exit
end
/* load system utility functions */
call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
call SysLoadFuncs
/* ask where they want the program object */
call charout ,"Create object on [D]esktop or in [S]tartup folder? [D/S] "
/* get input */
dummy = ''
do forever
  dummy = SysGetKey('NOECHO')
  parse upper var dummy dummy
  if dummy = 'D' then leave
  if dummy = 'S' then leave
end
/* got input; echo it */
call charout ,dummy
say ''
/* now do it */
if dummy = 'D' then location = '<WP_DESKTOP>'
if dummy \= 'D' then location = '<WP_START>'
title = 'FM/2 Taskbar'
classname = 'WPProgram'
setup = 'OBJECTID=<FM/2 Taskbar>;EXENAME='curdir'\TASKBAR.EXE;STARTUPDIR='curdir';PARAMETERS=%'
rc = SysCreateObject(classname,title,location,setup,u)
if rc = 0 then
do
  call SysDestroyObject "<FM/2 Taskbar>"
  call SysCreateObject classname,title,location,setup,u
end
/* tell the nice people that we're done and how to run it */
say 'FM/2 taskbar installation complete.  Start with WPS object when ready.'
