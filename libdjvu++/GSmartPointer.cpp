//C- -*- C++ -*-
//C- Copyright (c) 1999-2000 LizardTech, Inc. All Rights Reserved.
//C- 
//C- This software (the "Original Code") is subject to, and may be
//C- distributed under, the GNU General Public License, Version 2.
//C- You may obtain a copy of the license from the Free Software
//C- Foundation at http://www.fsf.org.
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

// File "$Id: GSmartPointer.cpp,v 1.1 2000-08-17 23:23:29 bcr Exp $"
// - Author: Leon Bottou, 05/1997

/* Put this into *one* file, which instantiates all the required containers
#ifdef __GNUC__
#pragma implementation
#endif
*/

#include "GThreads.h"
#include "GSmartPointer.h"
#include "GException.h"


// ------ STATIC CRITICAL SECTION

static GCriticalSection gcsCounter;


// ------ GPENABLED


GPEnabled::~GPEnabled()
{
  if (count)
    THROW("Suspicious destruction of referenced GPEnabled object");
}

void
GPEnabled::destroy()
{
  if (count)
    THROW("Suspicious destruction of referenced GPEnabled object");
  delete this;
}

void 
GPEnabled::ref()
{
  gcsCounter.lock();
  count++;
  gcsCounter.unlock();
}

void 
GPEnabled::unref()
{
  gcsCounter.lock();
  int newcount = --count;
  gcsCounter.unlock();
  if (newcount==0)
    destroy();
}


// ------ GPBASE


GPBase&
GPBase::assign (GPEnabled *nptr)
{
  gcsCounter.lock();
  if (nptr) 
    {
      nptr->count++;
    }
  if (ptr)
    {
      GPEnabled *old = ptr;
      ptr = nptr;
      int newcount = --old->count;
      gcsCounter.unlock();      
      if (newcount == 0)
        old->destroy();
    }
  else
    {
      ptr = nptr;
      gcsCounter.unlock();
    }
  return *this;
}


