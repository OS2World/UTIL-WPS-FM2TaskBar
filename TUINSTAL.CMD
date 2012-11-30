/*
 * FM/2 Taskbar deinstallation program
 */
'@Echo off'
'cls'
/* tell the nice people who we are */
say ''
say ' 旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커'
say ' �                   FM/2 Taskbar deinstallation program                      �'
say ' 읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸'
say ''
/* get current directory */
curdir = directory()
/* see if we have what we need to kill off */
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
/* be sure they meant it */
call charout ,"Remove FM/2 Taskbar from your system? [Y/N] "
/* get input */
dummy = ''
do forever
  dummy = SysGetKey('NOECHO')
  parse upper var dummy dummy
  if dummy = '1b'x then dummy = 'N'
  if dummy = '0d'x then dummy = 'Y'
  if dummy = 'N' then leave
  if dummy = 'Y' then leave
end
call charout ,dummy
say ''
/* well, did they really mean it? */
if dummy = 'N' then exit
/* disembowel ourself */
call SysDestroyObject "<FM/2 Taskbar>"
'del taskbar.exe'
'del taskhook.dll'
'del taskbar.ini'
'del tinstal.cmd'
'del file_id.diz'
'del texclude.lst'
'del vexclude.lst'
'del tread.me'
cd..
/* tell the nice people we're done */
say 'Deinstallation complete.  Remove directory '
say curdir' if desired with RMDIR.'
/* now blow our brains out all over the wall */
'del 'curdir'\tuinstal.cmd'
