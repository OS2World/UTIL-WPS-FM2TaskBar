#define INCL_DOS
#define INCL_WIN

#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "taskbar.h"

/* strip all leading spaces and tabs from a string */
#define lstrip(s)         strip_lead_char(" \t",(s))

/* strip all trailing spaces and tabs from a string */
#define rstrip(s)         strip_trail_char(" \t",(s))

/*strip all trailing linefeeds and carriage returns from a string */
#define stripcr(s)        strip_trail_char("\r\n",(s))

#pragma alloc_text(MENU,strip_trail_char,strip_lead_char,FreeStartData)
#pragma alloc_text(MENU,FreeMenu,SubmenuLoad,MenuLoad,MenuSave)


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


VOID FreeStartData (STARTDATA *sd) {

  if(sd) {
    if(sd->PgmTitle)
      free(sd->PgmTitle);
    if(sd->PgmName)
      free(sd->PgmName);
    if(sd->PgmInputs)
      free(sd->PgmInputs);
    if(sd->Environment)
      free(sd->Environment);
    if(sd->IconFile)
      free(sd->IconFile);
    if(sd->ObjectBuffer)
      free(sd->ObjectBuffer);
    free(sd);
  }
}


VOID FreeMenu (MENUSTUFF *menu) {

  MENUSTUFF *next,*info;

  if(!menu)
    menu = menuhead;
  info = menu;
  while(info) {
    next = info->next;
    if(info->type == MIS_SUBMENU && info->sub)
      FreeMenu(info->sub);
    if(info->sd)
      FreeStartData(info->sd);
    free(info);
    info = next;
  }
}


VOID SubmenuLoad (MENUSTUFF *head,FILE *fp) {

  MENUSTUFF  *info;
  CHAR        s[1024],*p;
  INT         state;

  while(!feof(fp)) {
    if(!fgets(s,1024,fp))
      break;
    s[1023] = 0;
    stripcr(s);
    lstrip(rstrip(s));
    if(!*s || *s == ';')
      continue;
    if(!strcmp(s,"SUBMENU END"))
      break;
    if(!strcmp(s,"MENUITEM"))
      state = 0;
    else if(!strcmp(s,"MENUCONTROL"))
      state = MIS_BREAK;
    else
      state = MIS_SUBMENU;
    /* get item title */
    if(!fgets(s,1023,fp))
      break;
    stripcr(s);
    lstrip(rstrip(s));
    if(!strncmp(s,"Title:",6)) {
      p += 6;
      while(*p == ' ')
        p++;
      if(*p) {

      }
    }
  }
}


MENUSTUFF * MenuLoad (CHAR *filename) {

  MENUSTUFF  *head = NULL;
  FILE       *fp;

  fp = fopen(filename,"r");
  if(fp) {
    head = malloc(sizeof(MENUSTUFF));
    if(head) {
      memset(head,0,sizeof(MENUSTUFF));
      SubmenuLoad(head,fp);
    }
    fclose(fp);
  }
  return head;
}


BOOL MenuSave (MENUSTUFF *menu,CHAR *filename) {

  MENUSTUFF *info;
  FILE      *fp;
  BOOL       ret = FALSE;

  fp = fopen(filename,"w");
  if(fp) {
    if(!menu)
      menu = menuhead;
    info = menu;
    while(info) {

      info = info->next;
    }
    fclose(fp);
  }
  return ret;
}

