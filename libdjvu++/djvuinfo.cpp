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



/** @name djvuinfo

    {\bf Synopsis}
    \begin{verbatim}
        djvuinfo <... iff_file_names ...>
    \end{verbatim}

    {\bf Description} ---
    File #"djvuinfo.cpp"# uses the facilities provided by \Ref{IFFByteStream.h}
    to display an indented representation of the chunk structure of an
    ``EA IFF 85'' file.  Each line represent contains a chunk ID followed by the
    chunk size.  Additional information about the chunk is provided when
    program #djvuinfo.cpp# recognizes the chunk name and knows how to summarize
    the chunk data.  Lines are indented in order to reflect the hierarchical
    structure of the IFF files.

    {\bf Example}
    \begin{verbatim}
    % djvuinfo graham1.djvu 
    graham1.djvu:
      FORM:DJVU [32553] 
        INFO [5]            2325x3156, version 20, 300 dpi, gamma 2.2
        Sjbz [17692]        JB2 data, no header
        BG44 [2570]         #1 - 74 slices - v1.2 (color) - 775x1052
        FG44 [1035]         #1 - 100 slices - v1.2 (color) - 194x263
        BG44 [3048]         #2 - 10 slices 
        BG44 [894]          #3 - 4 slices 
        BG44 [7247]         #4 - 9 slices 
    \end{verbatim}

    {\bf References} ---
    EA IFF 85 file format specification:\\
    \URL{http://www.cica.indiana.edu/graphics/image_specs/ilbm.format.txt}
    or \URL{http://www.tnt.uni-hannover.de/soft/compgraph/fileformats/docs/iff.pre}

    @memo
    Prints the structure of an IFF file.
    @author
    L\'eon Bottou <leonb@research.att.com>
    @version
    #$Id: djvuinfo.cpp,v 1.3 2001-01-04 22:04:54 bcr Exp $# */
//@{
//@}

#include <stdio.h>
#include "GException.h"
#include "GString.h"
#include "ByteStream.h"
#include "IFFByteStream.h"
#include "DjVuImage.h"



// ---------- ROUTINES FOR SUMMARIZING CHUNK DATA

void
display_djvu_info(IFFByteStream &iff, const GString &head, size_t size)
{
  struct DjVuInfo info;
  info.decode(iff);
  if (size >= 4)
    printf("%dx%d", info.width, info.height);
  if (size >= 5)
    printf(", version %d", info.version);
  if (size >= 8)
    printf(", %d dpi", info.dpi);
  if (size >= 8)
    printf(", gamma %3.1f", info.gamma);
}

void
display_sjbz_info(IFFByteStream &iff, const GString &head, size_t size)
{
  printf("JB2 data, no header");
}

void
display_iw4_info(IFFByteStream &iff, const GString &head, size_t size)
{
  struct PrimaryHeader {
    unsigned char serial;
    unsigned char slices;
  } primary;

  struct SecondaryHeader {
    unsigned char major;
    unsigned char minor;
    unsigned char xhi, xlo;
    unsigned char yhi, ylo;
  } secondary;
  
  if (iff.readall((void*)&primary, sizeof(primary)) == sizeof(primary))
    {
      printf("#%d - %d slices ", primary.serial+1, primary.slices);
      if (primary.serial==0)
        if (iff.readall((void*)&secondary, sizeof(secondary)) == sizeof(secondary))
          {
            printf("- v%d.%d (%s) - %dx%d", secondary.major&0x7f, secondary.minor,
                   (secondary.major & 0x80 ? "b&w" : "color"),
                   (secondary.xhi<<8)+secondary.xlo,
                   (secondary.yhi<<8)+secondary.ylo  );
          }
    }
}


struct displaysubr
{
  const char *id;
  void (*subr)(IFFByteStream &iff, const GString &head, size_t size);
} 
disproutines[] = 
{
  { "DJVU.INFO", display_djvu_info },
  { "DJVU.Sjbz", display_sjbz_info },
  { "DJVU.FG44", display_iw4_info },
  { "DJVU.BG44", display_iw4_info },
  { "BM44.BM44", display_iw4_info },
  { "PM44.PM44", display_iw4_info },
  { 0, 0 },
};


// ---------- ROUTINES FOR DISPLAYING CHUNK STRUCTURE

void
display_chunks(IFFByteStream &iff, const GString &head)
{
  size_t size;
  GString id, fullid;
  GString head2 = head + "  ";
  
  while ((size = iff.get_chunk(id)))
    {
      GString msg;
      msg.format("%s%s [%d] ", (const char *)head, (const char *)id, size);
      printf("%s", (const char *)msg);
      // Test chunk type
      int ok = 1;
      iff.full_id(fullid);
      for (int i=0; disproutines[i].id; i++)
        if (fullid == disproutines[i].id)
          {
            int n = msg.length();
            while (n++ < 30) putchar(' ');
            (*disproutines[i].subr)(iff, head2, size);
            ok = 0;
            break;
          }
      // Default display of composite chunk
      printf("\n");
      if (ok && iff.composite())
        display_chunks(iff, head2);
      // Terminate
      iff.close_chunk();
    }
}




void
display(const char *s)
{
  StdioByteStream bs(s,"rb");
  IFFByteStream iff(bs);
  printf("%s:\n", s);
  GString head = "  ";
  display_chunks(iff, head);
}


void
usage()
{
  fprintf(stderr,
          "DJVUINFO -- Describes IFF85 files\n"
          "%s\nUsage: djvuinfo <iff_filenames>\n",
          "Copyright (c) 1999-2000 LizardTech, Inc. All Rights Reserved.");
  exit(1);
}

int 
main(int argc, char **argv)
{
  TRY
    {
      
      if (argc<=1)
        usage();
      for (int i=1; i<argc; i++)
        display(argv[i]);
    }
  CATCH(ex)
    {
      ex.perror();
      exit(1);
    }
  ENDCATCH;
  return 0;
}
