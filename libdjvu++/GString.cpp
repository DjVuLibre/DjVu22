//C- Copyright � 1999-2001 LizardTech, Inc. All Rights Reserved.
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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <new.h>

#include "GString.h"

// File "$Id: GString.cpp,v 1.2 2001-01-04 22:04:53 bcr Exp $"
// - Author: Leon Bottou, 04/1997

static GStringRep nullstring;

GStringRep *
GStringRep::xnew(unsigned int sz)
{
  if (sz > 0)
    {
      void *vma = ::operator new(sizeof(GStringRep) + sz);
      GStringRep *addr = new (vma) GStringRep;
      addr->size = sz;
      return addr;
    }
  return &nullstring;
}

void
GStringRep::destroy()
{
  if ( (void*)this != (void*)&nullstring )
    GPEnabled::destroy();
}

GStringRep::~GStringRep()
{
  count = 0;
}


GString::GString()
  : GP<GStringRep> ( &nullstring )
{
}

GString::GString(const char *str)
{
  if (str)
    {
      (*this) =  GStringRep::xnew(strlen(str));
      strcpy((*this)->data,str);
    }
  else
    {
      (*this) = &nullstring;
    }
}

GString::GString(const char *str, unsigned int len)
{
  if (str)
    {
      unsigned int nlen = 0;
      for (nlen=0; nlen<len && str[nlen]; nlen++) /**/;
      (*this) = GStringRep::xnew(nlen);
      memcpy((*this)->data,str,nlen);
      (*this)->data[nlen] = 0;
    }
  else
    {
      (*this) = &nullstring;
    }
}

GString::GString(const GString& gs, int from, unsigned int len)
{
  if (from < 0)
    from += gs->size;
  if (from<0 || from>=(int)gs->size)
    from = gs->size;
  if (len > gs->size - from)
    len = gs->size - from;
  (*this) = GStringRep::xnew(len);
  memcpy((*this)->data, &gs->data[from],len);
  (*this)->data[len] = 0;
}

GString::GString(const int number) 
{
  this->format("%d",number);
}
  
GString::GString(const double number) 
{
  this->format("%f",number);
}

GString& 
GString::operator= (const char *str)
{
  *this = GStringRep::xnew(strlen(str));
  strcpy((*this)->data,str);
  return *this;
}

GString 
GString::upcase() const
{
  GString ret;
  ret = GStringRep::xnew((*this)->size);
  unsigned char *src = (unsigned char*)((*this)->data);
  unsigned char *dst = (unsigned char*)(ret->data);
  for (int i = 0; i<(int)(*this)->size; i++)
    if (islower(src[i]))
      dst[i] = toupper(src[i]);
    else
      dst[i] = src[i];
  dst[(*this)->size] = 0;
  return ret;
}

GString
GString::downcase() const
{
  GString ret;
  ret = GStringRep::xnew((*this)->size);
  unsigned char *src = (unsigned char*)((*this)->data);
  unsigned char *dst = (unsigned char*)(ret->data);
  for (int i = 0; i<(int)(*this)->size; i++)
    if (isupper(src[i]))
      dst[i] = tolower(src[i]);
    else
      dst[i] = src[i];
  dst[(*this)->size] = 0;
  return ret;
}

char *
GString::getbuf(int n)
{
  // Compute buffer length
  (*this)->data[(*this)->size] = 0;
  if (n < 0) n = strlen((*this)->data);
  // Save string
  GString old = *this;
  (*this) = GStringRep::xnew(n);
  // Copy string
  char *s = old->data;
  char *d = (*this)->data;
  while (d < (*this)->data + n)
    if ((*d++ = *s)) 
      s += 1;
  (*this)->data[(*this)->size] = 0;
  return (*this)->data;
}

void
GString::empty()
{
  (*this) = &nullstring;
}

void
GString::setat(int n, char ch)
{
  if (n < 0) 
    {
      n += (*this)->size;
      if (n < 0)
        THROW("Subscript too small in GString::setat");
    }
  if (n >= (int)(*this)->size)
    {
      if (n > (int)(*this)->size)
        THROW("Subscript too large in GString::setat");      
      if (ch == 0)
        return;
      getbuf((*this)->size + 1);
      (*this)->data[n] = ch;
    }
  else
    {
      if ((*this)->count > 1)
        getbuf((*this)->size);
      (*this)->data[n] = ch;
      if (ch == 0)
        (*this)->size = n;
    }
}

void
GString::format(const char *fmt, ... )
{
  char buffer[4096];
  // Format string
  va_list args;
  va_start(args, fmt);
  buffer[sizeof(buffer)-1] = 0;
  vsprintf(buffer, fmt, args);
  va_end(args);
  if (buffer[sizeof(buffer)-1])
    abort();
  // Go altering the string
  (*this) = buffer;
}

int 
GString::search(char c, int from) const
{
  if (from<0)
    from += (*this)->size;
  if (from<0 || from>=(int)(*this)->size)
    return -1;
  char *s = strchr(&(*this)->data[from], c);
  return (s ? s - (*this)->data : -1);
}
  
int 
GString::search(const char *str, int from) const
{
  if (from<0)
    from += (*this)->size;
  if (from<0 || from>=(int)(*this)->size)
    return -1;
  char *s = strstr(&(*this)->data[from], str);
  return (s ? s - (*this)->data : -1);
}


int 
GString::rsearch(char c, int from) const
{
  if (from<0)
    from += (*this)->size;
  if (from<0 || from>=(int)(*this)->size)
    return -1;
  int ans = -1;
  char *s = (*this)->data;
  while ((s=strchr(s,c)) && (s<=(*this)->data + from))
    {
      ans = s - (*this)->data;
      s += 1;
    }
  return ans;
}

int 
GString::rsearch(const char *str, int from) const
{
  if (from<0)
    from += (*this)->size;
  if (from<0 || from>=(int)(*this)->size)
    return -1;
  int ans = -1;
  char *s = (*this)->data;
  while ((s=strstr(s,str)) && (s<=(*this)->data + from))
    {
      ans = s - (*this)->data;
      s += 1;
    }
  return ans;
}

GString& 
GString::operator+= (const char *str)
{
  GString old = *this;
  (*this) = GStringRep::xnew(strlen(old->data) + strlen(str));
  strcpy((*this)->data, old->data);
  strcat((*this)->data, str);
  return *this;
}

GString
GString::concat(const char *str1, const char *str2)
{
  GStringRep *resrep = GStringRep::xnew(strlen(str1) + strlen(str2));
  strcpy(resrep->data, str1);
  strcat(resrep->data, str2);
  GString res(resrep);
  return res;
}

unsigned int 
hash(const GString &str)
{
  unsigned int x = 0;
  const char *s = str->data;
  while (*s) 
    x = (x<<6) ^ (unsigned char)(*s++);
  return x;
}



