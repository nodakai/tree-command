/* $Copyright: $
 * Copyright (c) 1996 - 2014 by Steve Baker (ice@mama.indstate.edu)
 * All Rights reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "tree.h"

extern bool dflag, lflag, pflag, sflag, Fflag, aflag, fflag, uflag, gflag;
extern bool Dflag, inodeflag, devflag, Rflag, duflag, pruneflag, gitflag;
extern bool noindent, force_color, xdev, nolinks, flimit;

extern void (*listdir)(char *, int *, int *, u_long, dev_t);
extern int (*cmpfunc)();
extern FILE *outfile;
extern int Level, *dirs, maxdirs;

extern bool colorize, linktargetcolor;
extern char *endcode;

int file_exist(const char *filename)
{
  struct stat buffer;
  return (stat (filename, &buffer) == 0);
}

static void show_git_branch(char* dir)
{
  const char *branch_file = strcat(dir, "/.git/HEAD");
  const char *branch = NULL;

  char buf[4096];
  char out_buf[4096];
  memset(&buf[0], 0, sizeof(buf));
  memset(&out_buf[0], 0, sizeof(out_buf));

  if(file_exist(branch_file))
  {
    FILE *file;
    file = fopen(branch_file, "r");

    if (file)
    {
      fscanf (file, "%s", buf);

      if(!strncmp("ref:", buf, sizeof "ref:"))//git branch at HEAD
      {
        memset(&buf[0], 0, sizeof(buf));
        fscanf (file, "%s", buf);

        char delim[] = " /";
        bool refs = FALSE;
        bool heads = FALSE;
        char *token;

        for(token = strtok(buf, delim); token != NULL; token = strtok(NULL, delim))
        {
          if(!refs && !strncmp("refs", token, sizeof "refs"))
          {
            refs = TRUE;
            continue;
          }
          if(!heads && !strncmp("heads", token, sizeof "heads"))
          {
            heads = TRUE;
            continue;
          }
          if(refs && heads)
          {
            if(out_buf[0] != '\0')
            {
              strcat(out_buf, "/");
            }
            strcat(out_buf, token);
          }
        }
        branch = &out_buf;
      }
      else //git branch detached from HEAD
      {
        rewind(file);
        strcat(out_buf, "HEAD detached at ");
        strncat(out_buf, buf, 7);
        branch = &out_buf;
      }

      if (ferror(file))
      {
          /* deal with error */
      }
      fclose(file);
    }
  }
  else // .git/HEAD doeasn't exist
  {
    branch = NULL;
  }

  if(branch)
  {
    printf(" [%s]", branch);
  }
}

off_t unix_listdir(char *d, int *dt, int *ft, u_long lev, dev_t dev)
{
  char *path;
  char* git_path;
  bool nlf = FALSE, colored = FALSE;
  long pathsize = 0;
  struct _info **dir, **sav;
  struct stat sb;
  int n, c;
  
  if ((Level >= 0) && (lev > Level)) {
    fputc('\n',outfile);
    return 0;
  }
  
  if (xdev && lev == 0) {
    stat(d,&sb);
    dev = sb.st_dev;
  }
  
  sav = dir = read_dir(d,&n);
  if (!dir && n) {
    fprintf(outfile," [error opening dir]\n");
    return 0;
  }
  if (!n) {
    fputc('\n', outfile);
    free_dir(sav);
    return 0;
  }
  if (flimit > 0 && n > flimit) {
    fprintf(outfile," [%d entries exceeds filelimit, not opening dir]\n",n);
    free_dir(sav);
    return 0;
  }

  if (cmpfunc) qsort(dir,n,sizeof(struct _info *), cmpfunc);
  if (lev >= maxdirs-1) {
    dirs = xrealloc(dirs,sizeof(int) * (maxdirs += 1024));
    memset(dirs+(maxdirs-1024), 0, sizeof(int) * 1024);
  }
  dirs[lev] = 1;
  if (!*(dir+1)) dirs[lev] = 2;
  fprintf(outfile,"\n");
  
  path = malloc(pathsize=4096);
  git_path = malloc(pathsize=4096);

  while(*dir) {
    if (!noindent) indent(lev);

    fillinfo(path,*dir);
    if (path[0] == ' ') {
      path[0] = '[';
      fprintf(outfile, "%s]  ",path);
    }
    
    if (colorize) {
      if ((*dir)->lnk && linktargetcolor) colored = color((*dir)->lnkmode,(*dir)->name,(*dir)->orphan,FALSE);
      else colored = color((*dir)->mode,(*dir)->name,(*dir)->orphan,FALSE);
    }
    
    if (fflag) {
      if (sizeof(char) * (strlen(d)+strlen((*dir)->name)+2) > pathsize)
	path=xrealloc(path,pathsize=(sizeof(char) * (strlen(d)+strlen((*dir)->name)+1024)));
      if (!strcmp(d,"/")) sprintf(path,"%s%s",d,(*dir)->name);
      else sprintf(path,"%s/%s",d,(*dir)->name);
    } else {
      if (sizeof(char) * (strlen((*dir)->name)+1) > pathsize)
	path=xrealloc(path,pathsize=(sizeof(char) * (strlen((*dir)->name)+1024)));
      sprintf(path,"%s",(*dir)->name);
    }

    printit(path);

    if (colored) fprintf(outfile,"%s",endcode);
    if (Fflag && !(*dir)->lnk) {
      if ((c = Ftype((*dir)->mode))) fputc(c, outfile);
    }

    if(gitflag) {
    	sprintf(git_path,"%s/%s",d,path);
    	show_git_branch(git_path);
    }

    if ((*dir)->lnk) {
      fprintf(outfile," -> ");
      if (colorize) colored = color((*dir)->lnkmode,(*dir)->lnk,(*dir)->orphan,TRUE);
      printit((*dir)->lnk);
      if (colored) fprintf(outfile,"%s",endcode);
      if (Fflag) {
	if ((c = Ftype((*dir)->lnkmode))) fputc(c, outfile);
      }
    }
    
    if ((*dir)->isdir) {
      if ((*dir)->lnk) {
	if (lflag && !(xdev && dev != (*dir)->dev)) {
	  if (findino((*dir)->inode,(*dir)->dev)) {
	    fprintf(outfile,"  [recursive, not followed]");
	  } else {
	    saveino((*dir)->inode, (*dir)->dev);
	    if (*(*dir)->lnk == '/')
	      listdir((*dir)->lnk,dt,ft,lev+1,dev);
	    else {
	      if (strlen(d)+strlen((*dir)->lnk)+2 > pathsize) path=xrealloc(path,pathsize=(strlen(d)+strlen((*dir)->name)+1024));
	      if (fflag && !strcmp(d,"/")) sprintf(path,"%s%s",d,(*dir)->lnk);
	      else sprintf(path,"%s/%s",d,(*dir)->lnk);
	      listdir(path,dt,ft,lev+1,dev);
	    }
	    nlf = TRUE;
	  }
	}
      } else if (!(xdev && dev != (*dir)->dev)) {
	if (strlen(d)+strlen((*dir)->name)+2 > pathsize) path=xrealloc(path,pathsize=(strlen(d)+strlen((*dir)->name)+1024));
	if (fflag && !strcmp(d,"/")) sprintf(path,"%s%s",d,(*dir)->name);
	else sprintf(path,"%s/%s",d,(*dir)->name);
	saveino((*dir)->inode, (*dir)->dev);
	listdir(path,dt,ft,lev+1,dev);
	nlf = TRUE;
      }
      *dt += 1;
    } else *ft += 1;
    if (*(dir+1) && !*(dir+2)) dirs[lev] = 2;
    if (nlf) nlf = FALSE;
    else fprintf(outfile,"\n");
    dir++;
  }
  dirs[lev] = 0;
  free(git_path);
  free(path);
  free_dir(sav);
  return 0;
}

off_t unix_rlistdir(char *d, int *dt, int *ft, u_long lev, dev_t dev)
{
  struct _info **dir;
  off_t size = 0;
  char *err;
  
  dir = getfulltree(d, lev, dev, &size, &err);

  memset(dirs, 0, sizeof(int) * maxdirs);

  r_listdir(dir, d, dt, ft, lev);

  return size;
}

void r_listdir(struct _info **dir, char *d, int *dt, int *ft, u_long lev)
{
  char *path;
  long pathsize = 0;
  struct _info **sav = dir;
  bool nlf = FALSE, colored = FALSE;
  int c;
  
  if (dir == NULL) return;

  dirs[lev] = 1;
  if (!*(dir+1)) dirs[lev] = 2;
  fprintf(outfile,"\n");

  path = malloc(pathsize=4096);

  while(*dir) {
    if (!noindent) indent(lev);
    
    fillinfo(path,*dir);
    if (path[0] == ' ') {
      path[0] = '[';
      fprintf(outfile, "%s]  ",path);
    }
    
    if (colorize) {
      if ((*dir)->lnk && linktargetcolor) colored = color((*dir)->lnkmode,(*dir)->name,(*dir)->orphan,FALSE);
      else colored = color((*dir)->mode,(*dir)->name,(*dir)->orphan,FALSE);
    }
    
    if (fflag) {
      if (sizeof(char) * (strlen(d)+strlen((*dir)->name)+2) > pathsize)
	path=xrealloc(path,pathsize=(sizeof(char) * (strlen(d)+strlen((*dir)->name)+1024)));
      if (!strcmp(d,"/")) sprintf(path,"%s%s",d,(*dir)->name);
      else sprintf(path,"%s/%s",d,(*dir)->name);
    } else {
      if (sizeof(char) * (strlen((*dir)->name)+1) > pathsize)
	path=xrealloc(path,pathsize=(sizeof(char) * (strlen((*dir)->name)+1024)));
      sprintf(path,"%s",(*dir)->name);
    }
    
    printit(path);
    
    if (colored) fprintf(outfile,"%s",endcode);
    if (Fflag && !(*dir)->lnk) {
      if ((c = Ftype((*dir)->mode))) fputc(c, outfile);
    }
    
    if ((*dir)->lnk) {
      fprintf(outfile," -> ");
      if (colorize) colored = color((*dir)->lnkmode,(*dir)->lnk,(*dir)->orphan,TRUE);
      printit((*dir)->lnk);
      if (colored) fprintf(outfile,"%s",endcode);
      if (Fflag) {
	if ((c = Ftype((*dir)->lnkmode))) fputc(c, outfile);
      }
    }
    
    if ((*dir)->err) {
      fprintf(outfile," [%s]", (*dir)->err);
      free((*dir)->err);
      (*dir)->err = NULL;
    }
    if ((*dir)->child) {
      if (fflag) {
	if (strlen(d)+strlen((*dir)->name)+2 > pathsize) path=xrealloc(path,pathsize=(strlen(d)+strlen((*dir)->name)+1024));
	if (!strcmp(d,"/")) sprintf(path,"%s%s",d,(*dir)->name);
	else sprintf(path,"%s/%s",d,(*dir)->name);
      }
      r_listdir((*dir)->child, fflag? path : NULL, dt, ft, lev+1);
      nlf = TRUE;
      *dt += 1;
    } else {
      if ((*dir)->isdir) *dt += 1;
      else *ft += 1;
    }

    if (*(dir+1) && !*(dir+2)) dirs[lev] = 2;
    if (nlf) nlf = FALSE;
    else fprintf(outfile,"\n");
    dir++;
  }
  dirs[lev] = 0;
  free(path);
  free_dir(sav);
}
