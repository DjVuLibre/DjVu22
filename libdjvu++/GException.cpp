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


#ifdef __GNUC__
#pragma implementation
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <new.h>
#include "GException.h"
#ifdef DEBUGLVL
#include "debug.h"
#endif

// File "$Id: GException.cpp,v 1.2 2001-01-04 22:04:53 bcr Exp $"
// - Author: Leon Bottou, 05/1997

GException::GException() 
  : cause(0), file(0), func(0), line(0)
{
}

const char * const
GException::outofmemory = "Out of memory";

GException::GException(const GException & exc) 
  : cause(0), file(exc.file), func(exc.func), line(exc.line)
{
  if (exc.cause && exc.cause!=outofmemory) 
    {
      char *s = new char[strlen(exc.cause)+1];
      strcpy(s, exc.cause);
      cause = s;
    }
  else
    {
      cause = exc.cause;
    }
}

GException::GException (const char *xcause, const char *file, int line, const char *func)
  : cause(0), file(file), func(func), line(line)
{
  // good place to set a breakpoint and DEBUG message too. 
  // It'd hard to track exceptions which seem to go from nowhere
#ifdef DEBUG_MSG
  DEBUG_MSG("GException::GException(): cause=" << (cause ? cause : "unknown") << "\n");
#endif
  if (xcause && xcause!=outofmemory) 
    {
      char *s = new char[strlen(xcause)+1];
      strcpy(s, xcause);
      cause = s;
    }
  else
    {
      cause = xcause;
    }
}

GException::~GException(void)
{
  if (cause && cause!=outofmemory ) 
    delete [] (char*)cause; 
  cause=file=func=0;
}

GException & 
GException::operator=(const GException & exc)
{
  if (cause && cause!=outofmemory) 
    delete [] (char*)cause;
  cause = 0;
  file = exc.file;
  func = exc.func;
  line = exc.line;
  if (exc.cause && exc.cause!=outofmemory) 
    {
      char *s = new char[strlen(exc.cause)+1];
      strcpy(s, exc.cause);
      cause = s;
    }
  else
    {
      cause = exc.cause;
    }
  return *this;
}

void
GException::perror(const char *msg) const
{
  fflush(NULL);
  fprintf(stderr, "*** %s", get_cause());
  if (file && line>0)
    fprintf(stderr, "\n*** (%s:%d)", file, line);    
  else if (file)
    fprintf(stderr, "\n*** (%s)", file);        
  if (msg) 
    fprintf(stderr, " %s", msg);
  if (func)
    fprintf(stderr, "\n*** %s", func);
  fprintf(stderr,"\n");
}

const char* 
GException::get_cause(void) const
{
  if (! cause)
    return "Invalid exception";
  return cause;
}


#ifdef USE_EXCEPTION_EMULATION

GExceptionHandler *GExceptionHandler::head = 0;

void
GExceptionHandler::emthrow(const GException &gex)
{
  if (head)
    {
      head->current = gex;
      longjmp(head->jump, -1);
    }
  else
    {
      gex.perror("Unhandled exception");
      abort();
    }
}


#endif



// ------ MEMORY MANAGEMENT HANDLER

#ifndef NEED_DJVU_MEMORY
// This is not activated when C++ memory management
// is overidden.  The overriding functions handle
// memory exceptions by themselves.
#if defined(_MSC_VER)
// Microsoft is different!
static int throw_memory_error(size_t) { THROW(GException::outofmemory); return 0; }
static int (*old_handler)(size_t) = _set_new_handler(throw_memory_error);
#else // !_MSC_VER
// Standard C++
static void throw_memory_error() { THROW(GException::outofmemory); }
static void (*old_handler)() = set_new_handler(throw_memory_error);
#endif // !_MSC_VER
#endif // !NEED_DJVU_MEMORY
