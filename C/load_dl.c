/*************************************************************************
*									 *
*	 YAP Prolog 							 *
*									 *
*	Yap Prolog was developed at NCCUP - Universidade do Porto	 *
*									 *
* Copyright L.Damas, V.S.Costa and Universidade do Porto 1985-1997	 *
*									 *
**************************************************************************
*									 *
* File:		load_dl.c						 *
* comments:	dl based dynamic loaderr of external routines		 *
*               tested on i486-linuxelf					 *
*************************************************************************/

#include "Yap.h"
#include "Yatom.h"
#include "YapHeap.h"
#include "yapio.h"
#include "Foreign.h"

#if LOAD_DL

// use SWI-Prolog code if all else fails
char *
findExecutable(const char *av0, char *buffer);

#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <dlfcn.h>
#endif

 

typedef void (*prismf)(void);

/* only works for dlls */
int
Yap_CallFunctionByName(const char *thing_string);

int
Yap_CallFunctionByName(const char *thing_string)
{
  void * handle = dlopen(NULL, RTLD_LAZY
#ifndef __CYGWIN__
#ifdef RTLD_NOLOAD
			 | RTLD_NOLOAD
#endif
#endif
			 );
  // you could do RTLD_NOW as well.  shouldn't matter
  if (!handle) {
    CACHE_REGS
    Yap_Error(SYSTEM_ERROR_INTERNAL, ARG1, "Dynamic linking on main module : %s\n", dlerror());
  }
  prismf * addr = (prismf *)dlsym(handle, thing_string);
  if (addr)
    (*addr)();
  dlclose(handle);
  return TRUE;
}

/*
 *   YAP_FindExecutable(argv[0]) should be called on yap initialization to
 *   locate the executable of Yap
*/
char *
Yap_FindExecutable(void)
{
#if HAVE_GETEXECNAME
  // Solaris
  return getexecname();
#elif __APPLE__
  char path[1024];
  char *buf;

  uint32_t size = sizeof(path);
  if (!_NSGetExecutablePath(path, &size)) {
    size_t sz = strlen(path);
    buf = malloc(sz+1);
    strncpy(buf, path, sz);
    return buf;
  } else {
    char *rc = malloc(size+1);
    if (_NSGetExecutablePath(rc, &size) == 0)
      return "yap";
    return rc;
  }
#elif defined(__linux__)
  enum { BUFFERSIZE = 1024 };
  char *buf = malloc(BUFFERSIZE);
  ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf)-1);
 
  if (len != -1) {
    buf[len] = '\0';
    return buf;
  }
  // follow through to standard method
#elif defined(__FreeBSD__) ||  defined(__DragonFly__)
  enum { BUFFERSIZE = 1024 };
  char *buf = malloc(BUFFERSIZE);
  ssize_t len = readlink("/proc/curproc/file", buf, sizeof(buf)-1);
 
  if (len != -1) {
    buf[len] = '\0';
    return buf;
  }
  int mib[4];
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PATHNAME;
  mib[3] = -1; // current process
  size_t cb = BUFFERSIZE;
  sysctl(mib, 4, buf, &cb, NULL, 0);
  // follow through to standard method
#endif
  return
    NULL;
}

void *
Yap_LoadForeignFile(char *file, int flags)
{
  CACHE_REGS
  int dlflag;
  void *out;
  

  if (flags &  EAGER_LOADING)
    dlflag = RTLD_NOW;
  else
    dlflag = RTLD_LAZY;
  if (flags &  GLOBAL_LOADING)
    dlflag |= RTLD_GLOBAL;
#ifndef __CYGWIN__
  else 
    dlflag |= RTLD_LOCAL;
#endif
  if (!Yap_TrueFileName(file, LOCAL_FileNameBuf, true)){
    /* use LD_LIBRARY_PATH */
    strncpy(LOCAL_FileNameBuf,file, YAP_FILENAME_MAX-1);
    strncat(LOCAL_FileNameBuf,".", YAP_FILENAME_MAX-1);
    strncat(LOCAL_FileNameBuf, "SO_EXT",  YAP_FILENAME_MAX-1);
  }
  out = (void *)dlopen(LOCAL_FileNameBuf, flags);
  if (out == NULL) {
    char *m_os = dlerror();
    if (m_os) {
      LOCAL_ErrorMessage = dlerror();
    } else {
      LOCAL_ErrorMessage = "dlopen failed";
    }
  }
  return out;
}

int
Yap_CallForeignFile(void *handle, char *f)
{
  YapInitProc proc = (YapInitProc) dlsym(handle, f);
  if (!proc) {
    /* Yap_Error(SYSTEM_ERROR_INTERNAL, ARG1, "dlsym error %s\n", dlerror());*/
    return FALSE;
  }
  (*proc) ();
  return TRUE;
}

int
Yap_CloseForeignFile(void *handle)
{
  if ( dlclose(handle) < 0) {
    CACHE_REGS
    Yap_Error(SYSTEM_ERROR_INTERNAL, ARG1, "dlclose error %s\n", dlerror());
    return -1;
  }
  return 0;
}


/*
 * LoadForeign(ofiles,libs,proc_name,init_proc) dynamically loads foreign
 * code files and libraries and locates an initialization routine
*/
static Int
LoadForeign(StringList ofiles, StringList libs,
	       char *proc_name,	YapInitProc *init_proc)
{
  CACHE_REGS

  while (libs) {
    if (!Yap_TrueFileName((char *)AtomName(libs->name), LOCAL_FileNameBuf, true)) {
      /* use LD_LIBRARY_PATH */
      strncpy(LOCAL_FileNameBuf, (char *)AtomName(libs->name), YAP_FILENAME_MAX);
    }

#ifdef __osf__
    if((libs->handle=dlopen(LOCAL_FileNameBuf,RTLD_LAZY)) == NULL)
#else
    if((libs->handle=dlopen(LOCAL_FileNameBuf,RTLD_LAZY|RTLD_GLOBAL)) == NULL)
#endif
    {
      strcpy(LOCAL_ErrorSay,dlerror());
      return LOAD_FAILLED;
    }
    libs = libs->next;
  }

  while (ofiles) {
    void *handle;

  /* load libraries first so that their symbols are available to
     other routines */

    /* dlopen wants to follow the LD_CONFIG_PATH */
    if (!Yap_TrueFileName((char *)AtomName(ofiles->name), LOCAL_FileNameBuf, TRUE)) {
      strcpy(LOCAL_ErrorSay, "%% Trying to open unexisting file in LoadForeign");
      return LOAD_FAILLED;
    }
#ifdef __osf__
    if((handle=dlopen(LOCAL_FileNameBuf,RTLD_LAZY)) == 0)
#else
    if((handle=dlopen(LOCAL_FileNameBuf,RTLD_LAZY|RTLD_GLOBAL)) == 0)
#endif
    {
      fprintf(stderr,"dlopen of image %s failed: %s\n", LOCAL_FileNameBuf, dlerror());
/*      strcpy(LOCAL_ErrorSay,dlerror());*/
      return LOAD_FAILLED;
    }

    ofiles->handle = handle;

    if (proc_name && !*init_proc)
      *init_proc = (YapInitProc) dlsym(handle,proc_name);

    ofiles = ofiles->next;
  }

  if(! *init_proc) {
    strcpy(LOCAL_ErrorSay,"Could not locate initialization routine");
    return LOAD_FAILLED;
  }

  return LOAD_SUCCEEDED;
}

Int
Yap_LoadForeign(StringList ofiles, StringList libs,
	       char *proc_name,	YapInitProc *init_proc)
{
  return LoadForeign(ofiles, libs, proc_name, init_proc);
}

void 
Yap_ShutdownLoadForeign(void)
{
  ForeignObj *f_code;

  f_code = ForeignCodeLoaded;
  while (f_code != NULL) {
    StringList objs, libs, old;
    ForeignObj *of_code = f_code;

    objs = f_code->objs;
    while (objs != NULL) {
      old = objs;
      if (dlclose(objs->handle) != 0)
      	return; /* ERROR */
      objs = objs->next;
      Yap_FreeCodeSpace((ADDR)old);
    }
    libs = f_code->libs;
    while (libs != NULL) {
      old = libs;
      if (dlclose(libs->handle) != 0)
	return; /* ERROR */
      libs = libs->next;
      Yap_FreeCodeSpace((ADDR)old);
    }
    f_code = f_code->next;
    Yap_FreeCodeSpace((ADDR)of_code);
  }
  /*
    make sure that we don't try to close foreign code several times, eg,
    from within an error handler
  */
  ForeignCodeLoaded = NULL;
}

Int
Yap_ReLoadForeign(StringList ofiles, StringList libs,
	       char *proc_name,	YapInitProc *init_proc)
{
  return(LoadForeign(ofiles,libs, proc_name, init_proc));
}

#endif

#if SIMICS

void dlopen(void)
{
}

void dlclose(void)
{
}

void dlsym(void)
{
}

#endif



