#ifdef VITA

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <psp2/rtc.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/clib.h>
#include "vita_compat.h"

#include <stdlib.h>
#include <string.h>

int _newlib_heap_size_user = 1 * 1024 * 1024;

// Allocations override

void *memset(void * ptr, int value, size_t num) {
	return sceClibMemset(ptr, value, num);
}

void *memmove(void * destination, const void * source, size_t num) {
	return sceClibMemmove(destination, source, num);
}

void *memcpy(void * destination, const void * source, size_t num) {
	return sceClibMemcpy(destination, source, num);
}

void *malloc(size_t size) {
	return sceLibcMalloc(size);
}

void *calloc(size_t nitems, size_t size) {
	return sceLibcCalloc(nitems, size);
}

void *realloc(void *ptr, size_t size) {
	return sceLibcRealloc(ptr, size);
}

void *memalign(size_t blocksize, size_t bytes) {
	return sceLibcMemalign(blocksize, bytes);
}

void free(void *ptr) {
	sceLibcFree(ptr);
}

int _vita_getcwd(char *buf, size_t size)
{
    const char *cwd = "ux0:data/RE3";

    if (strlen(cwd)+1 > size)
    {
        return -ERANGE;
    }

    strncpy(buf, cwd, size);
    return 0;
}

/* Taken from glibc */
char *realpath(const char *name, char *resolved)
{
   char *rpath, *dest = NULL;
   const char *start, *end, *rpath_limit;
   long int path_max;

   /* As per Single Unix Specification V2 we must return an error if
      either parameter is a null pointer.  We extend this to allow
      the RESOLVED parameter to be NULL in case the we are expected to
      allocate the room for the return value.  */
   if (!name)
      return NULL;

   /* As per Single Unix Specification V2 we must return an error if
      the name argument points to an empty string.  */
   if (name[0] == '\0')
      return NULL;

   path_max = PATH_MAX;

   if (!resolved)
   {
      rpath = malloc(path_max);
      if (!rpath)
         return NULL;
   }
   else
      rpath = resolved;
   rpath_limit = rpath + path_max;

   if (name[0] != '/')
   {
      if (!_vita_getcwd(rpath, path_max))
      {
         rpath[0] = '\0';
         goto error;
      }
      dest = memchr(rpath, '\0', path_max);
   }
   else
   {
      rpath[0] = '/';
      dest = rpath + 1;
   }

   for (start = end = name; *start; start = end)
   {
      /* Skip sequence of multiple path-separators.  */
      while (*start == '/')
         ++start;

      /* Find end of path component.  */
      for (end = start; *end && *end != '/'; ++end)
         /* Nothing.  */;

      if (end - start == 0)
         break;
      else if (end - start == 1 && start[0] == '.')
         /* nothing */;
      else if (end - start == 2 && start[0] == '.' && start[1] == '.')
      {
         /* Back up to previous component, ignore if at root already.  */
         if (dest > rpath + 1)
            while ((--dest)[-1] != '/')
               ;
      }
      else
      {
         size_t new_size;

         if (dest[-1] != '/')
            *dest++ = '/';

         if (dest + (end - start) >= rpath_limit)
         {
            ptrdiff_t dest_offset = dest - rpath;
            char *new_rpath;

            if (resolved)
            {
               if (dest > rpath + 1)
                  dest--;
               *dest = '\0';
               goto error;
            }
            new_size = rpath_limit - rpath;
            if (end - start + 1 > path_max)
               new_size += end - start + 1;
            else
               new_size += path_max;
            new_rpath = (char *)realloc(rpath, new_size);
            if (!new_rpath)
               goto error;
            rpath = new_rpath;
            rpath_limit = rpath + new_size;

            dest = rpath + dest_offset;
         }

         dest = memcpy(dest, start, end - start);
         *dest = '\0';
      }
   }
   if (dest > rpath + 1 && dest[-1] == '/')
      --dest;
   *dest = '\0';

   return rpath;

error:
   if (!resolved)
      free(rpath);
   return NULL;
}

int _vita_clock_gettime(int clk_id, struct timespec *tp) {
    if (clk_id == CLOCK_MONOTONIC)
    {
        SceKernelSysClock ticks;
        sceKernelGetProcessTime(&ticks);

        tp->tv_sec = ticks.quad_t/(1000*1000);
        tp->tv_nsec = (ticks.quad_t * 1000) % (1000*1000*1000);

        return 0;
    }

    else if (clk_id == CLOCK_REALTIME)
    {
        time_t seconds;
        SceDateTime time;
        sceRtcGetCurrentClockLocalTime(&time);

        sceRtcGetTime_t(&time, &seconds);

        tp->tv_sec = seconds;
        tp->tv_nsec = time.microsecond * 1000;
        return 0;
    }

    return -ENOSYS;
}
#endif