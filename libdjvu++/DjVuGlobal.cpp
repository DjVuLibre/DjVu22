//C- Copyright © 1999-2001 LizardTech, Inc. All Rights Reserved.
//C- 
//C- This software (the "Original Code") is subject to, and may be
//C- distributed under, the GNU General Public License, Version 2.
//C- You may obtain a copy of the license from the Free Software
//C- Foundation at http://www.fsf.org .
//C- 
//C- With respect to the Original Code, and subject to any third party
//C- intellectual property claims, LizardTech grants recipient a worldwide,
//C- royalty-free, non-exclusive license under patent claims infringed by
//C- making, using, or selling Original Code which are now or hereafter
//C- owned or controlled by LizardTech, but solely to the extent that any
//C- such patent is reasonably necessary to enable you to make, have made,
//C- practice, sell, or otherwise dispose of Original Code (or portions
//C- thereof) and not to any greater extent that may be necessary to utilize
//C- further modifications or combinations.
//C- 
//C- The Original Code is provided "AS IS" WITHOUT WARRANTY OF ANY KIND,
//C- EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
//C- OF NON-INFRINGEMENT, OR ANY IMPLIED WARRANTY OF MERCHANTIBILITY OF
//C- FITNESS FOR A PARTICULAR PURPOSE.
//C-




// ----------------------------------------

#define NEED_DJVU_MEMORY_IMPLEMENTATION
#include "DjVuGlobal.h"


// ----------------------------------------

#ifdef NEED_DJVU_MEMORY
#include "GException.h"

static djvu_new_callback *newptr = (djvu_new_callback*) & ::operator new;
static djvu_delete_callback *delptr = (djvu_delete_callback*) & ::operator delete;

void *
_djvu_new(size_t sz)
{
  void *addr = (*newptr)(sz);
  if (! addr) THROW(GException::outofmemory);
  return addr;
}

void  
_djvu_delete(void *addr)
{
  if (addr) 
    (*delptr)(addr);
}

void 
_djvu_memory_callback(djvu_delete_callback *dp, djvu_new_callback *np)
{
  newptr = ( np ? np : (djvu_new_callback*) &::operator new );
  delptr = ( dp ? dp : (djvu_delete_callback*) & ::operator delete );
}

#endif



// ----------------------------------------

#ifdef NEED_DJVU_PROGRESS
#include "GOS.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static DjVuProgressScale       *p_scale  = 0;
static djvu_progress_callback  *p_cb     = 0;
static unsigned long            p_start  = 0;
static FILE                    *p_log    = 0;

void 
_djvu_end_progress()
{
  p_scale = 0;
  p_cb = 0;
  if (p_log && p_log!=stderr) 
    fclose(p_log);
  p_log = 0;
}

void 
_djvu_start_progress(DjVuProgressScale *s, djvu_progress_callback *c)
{
  _djvu_end_progress();
  p_scale = s;
  p_cb = c;
  p_log = 0;
}

void 
_djvu_start_progress(DjVuProgressScale *s, const char *logname)
{
  _djvu_end_progress();
  p_scale = s;
  p_cb = 0;
  p_start = GOS::ticks();
  p_log = stderr;
  if (logname) 
    p_log = fopen(logname,"w");
  if (p_log) 
    fprintf(p_log, "------------------------------\n");
}


// Internal stuff
void 
_djvu_progress(const char *filename, const char *tag, int index)
{
  // Log current call
  if (p_log)
    fprintf(p_log, "  { \"%s\", \"%s\", %d },\t// time=%lu\n", 
            filename,tag,index, GOS::ticks()-p_start );
  // Search for a match
  DjVuProgressScale *scale = p_scale;
  for (; scale && scale->percent>0 && scale->percent<100; scale++)
    {
      // Check for a match
      if (scale->match_filename && strcmp(filename, scale->match_filename))
        continue;
      if (scale->match_tag && strcmp(tag, scale->match_tag))
        continue;
      if (index < scale->match_index)
        break;
      // We have a match
      if (p_log)
        fprintf(p_log, "  // got %d %%\n", scale->percent );
      if (p_cb)
        (*p_cb) (scale->percent);
      // Go to next entry
      for (scale++; scale->percent>0 && scale->percent<100; scale++)
        {
          if (scale->match_filename && strcmp(filename, scale->match_filename))
            break;
          if (scale->match_tag && strcmp(tag, scale->match_tag))
            break;
          if (index <= scale->match_index)
            break;
        }
      p_scale = scale;
      break;
    }
}

#endif
