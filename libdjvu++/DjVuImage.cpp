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

#include "DjVuImage.h"
#include "GScaler.h"
#include <stdarg.h>

// ----------------------------------------
// CLASS DJVUINFO


#define STRINGIFY(x) STRINGIFY_(x)
#define STRINGIFY_(x) #x


DjVuInfo::DjVuInfo()
  : width(0), height(0), 
    version(DJVUVERSION),
    dpi(300), gamma(2.2), reserved(0)
{
}

void 
DjVuInfo::decode(ByteStream &bs)
{
  // Set to default values
  width = 0;
  height = 0;
  version = DJVUVERSION;
  dpi = 300;
  gamma = 2.2;
  reserved = 0;
  // Read data
  unsigned char buffer[10];
  int  size = bs.readall((void*)buffer, sizeof(buffer));
  if (size < 5)
    THROW("DjVu Decoder: Corrupted file (truncated INFO chunk)");
  // Analyze data with backward compatibility in mind!
  if (size>=2)
    width = (buffer[0]<<8) + buffer[1];
  if (size>=4)
    height = (buffer[2]<<8) + buffer[3];
  if (size>=5)
    version = buffer[4];
  if (size>=6 && buffer[5]!=0xff)
    version = (buffer[5]<<8) + buffer[4];
  if (size>=8 && buffer[7]!=0xff)
    dpi = (buffer[7]<<8) + buffer[6];
  if (size>=9)
    gamma = 22.0 * buffer[8];
  if (size>=10)
    reserved = buffer[9];
  // Consistency checks
  if (width<0 || height<0)
    THROW("DjVu Decoder: Corrupted file (image size is zero)");
  if (version >= DJVUVERSION_TOO_NEW)
    THROW("DjVu Decoder: Cannot decode DjVu files with version >= "
          STRINGIFY(DJVUVERSION_TOO_NEW) );
  // Fixup
  if (gamma <= 0.3 || gamma >= 5.0)
    gamma = 2.2;
  if (dpi < 25 || dpi > 6000)
    dpi = 300;
}

void 
DjVuInfo::encode(ByteStream &bs)
{
  bs.write16(width);
  bs.write16(height);
  bs.write8(version & 0xff);
  bs.write8(version >> 8);
  bs.write8(dpi & 0xff);
  bs.write8(dpi >> 8);
  bs.write8((int)(10.0*gamma+0.5) );
  bs.write8(reserved);
}

unsigned int 
DjVuInfo::get_memory_usage() const
{
  return sizeof(DjVuInfo);
}



// ----------------------------------------
// CLASS DJVUANNO


DjVuAnno::DjVuAnno()
{
}

void 
DjVuAnno::decode(ByteStream &bs)
{
  GCriticalSectionLock lock(&mutex);
  char buf[512];
  int len = sizeof(buf);
  while (len>0) {
    len = bs.readall((void*)buf, sizeof(buf));
    raw = raw + GString(buf, len);
  } 
}

void 
DjVuAnno::encode(ByteStream &bs)
{
  GCriticalSectionLock lock(&mutex);
  bs.writall((const void*)(const char*)raw, raw.length());
}

unsigned int 
DjVuAnno::get_memory_usage() const
{
  return sizeof(DjVuAnno) + raw.length();
}



// ----------------------------------------
// CLASS DJVUINTERFACE

void 
DjVuInterface::notify_chunk(const char *chkid, const char *msg)
{
  // Noop
}

void 
DjVuInterface::notify_relayout(void)
{
  // Noop
}

void 
DjVuInterface::notify_redisplay(void)
{
  // Noop
}


// ----------------------------------------
// CLASS DJVUIMAGE


//// DJVUIMAGE: CONSTRUCTION

DjVuImage::DjVuImage()
{
}
 
void 
DjVuImage::init()
{
  info = 0;
  anno = 0;
  bg44 = 0;
  fgpm = 0;
  fgjb = 0;
  mimetype = GString();
  description = GString();
  filesize = 0;
}


//// DJVUIMAGE: UTILITIES

unsigned int 
DjVuImage::get_memory_usage() const
{
  unsigned int usage = sizeof(DjVuImage);
  // Components
  if (info) 
    usage += info->get_memory_usage();
  if (anno)
    usage += anno->get_memory_usage();
  if (bg44) 
    usage += bg44->get_memory_usage();
  if (fgpm) 
    usage += fgpm->get_memory_usage();
  if (fgjb)
    usage += fgjb->get_memory_usage();
  // This one should not count!
  usage += description.length();
  // Return
  return usage;
}

GString 
DjVuImage::get_short_description() const
{
  GString msg = "Empty";
  int width = get_width();
  int height = get_height();
  if (width && height)
    if (filesize > 100)
      msg.format("%dx%d in %0.1f Kb", width, height, filesize/1024.0);
    else
      msg.format("%dx%d", width, height);      
  return msg;
}

GString 
DjVuImage::get_long_description() const
{
  // Tab characters '\t' sometimes look really ugly.
  // This code replace them with spaces so that every column 
  // is really aligned.  Lines that do not contain tabs are 
  // left unchanged.
  GString result = description;
  // Loop on tabulations
  for(;;)
    {
      // Search position of first tab in each line
      int tab_num = 0;
      int tab_pos = 0;
      const char *s=result;
      for (s=result; *s; s++)
        {
          const char *line_start = s;
          for (; *s && *s!='\n' && *s!='\t'; s++) /**/;
          if (*s == '\t')
            {
              int pos = s-line_start;
              tab_pos = ( (pos>tab_pos) ? pos : tab_pos );
              tab_num += 1;
            }
          for (; *s && *s!='\n'; s++) /**/;
        }
      // Check whether all tabs have been removed
      if (tab_num <= 0)
        break;
      // Replace tab 
      GString tmp;
      char *d = tmp.getbuf(result.length() + tab_num * tab_pos + 1);
      s = result;
      for (s=result; *s; *d++=*s++)
        {
          const char *line_start = d;
          for (; *s && *s!='\n' && *s!='\t'; *d++=*s++) /**/;
          if (*s == '\t')
            for(s++; d<line_start+tab_pos+2; *d++=' ') /**/;
          for (; *s && *s!='\n'; *d++=*s++) /**/;
        }
      *d++ = 0;
      result = tmp;
    }
  // All tabs have been replaced.
  return result;
}


//// DJVUIMAGE: CHECKING

static int
compute_red(int w, int h, int rw, int rh)
{
  for (int red=1; red<16; red++)
    if (((w+red-1)/red==rw) && ((h+red-1)/red==rh))
      return red;
  return 16;
}


int 
DjVuImage::is_legal_bilevel() const
{
  // Components
  GP<DjVuInfo> info = get_info();
  GP<JB2Image> fgjb = get_fgjb();
  GP<IWPixmap> bg44 = get_bg44();
  GP<GPixmap>  fgpm = get_fgpm();
  // Check info
  if (! info)
    return 0;
  int width = info->width;
  int height = info->height;
  if (! (width>0 && height>0))
    return 0;
  // Check fgjb
  if (!fgjb)
    return 0;
  if (fgjb->get_width()!=width || fgjb->get_height()!=height)
    return 0;
  // Check that color information is not present.
  if (bg44 || fgpm)
    return 0;
  // Ok.
  return 1;
}

int 
DjVuImage::is_legal_photo() const
{
  // Components
  GP<DjVuInfo> info = get_info();
  GP<JB2Image> fgjb = get_fgjb(); 
  GP<IWPixmap> bg44 = get_bg44();
  GP<GPixmap>  fgpm = get_fgpm();
  // Check info
  if (! info)
    return 0;
  int width = info->width;
  int height = info->height;
  if (! (width>0 && height>0))
    return 0;
  // Check bg44
  if (!bg44)
    return 0;
  if (bg44->get_width()!=width || bg44->get_height()!=height)
    return 0;
  // Check that extra information is not present.
  if (fgjb || fgpm)
    return 0;
  // Ok.
  return 1;
}

int 
DjVuImage::is_legal_compound() const
{
  // Components
  GP<DjVuInfo> info = get_info();
  GP<JB2Image> fgjb = get_fgjb();
  GP<IWPixmap> bg44 = get_bg44();
  GP<GPixmap>  fgpm = get_fgpm();
  // Check size
  if (! info)
    return 0;
  int width = info->width;
  int height = info->height;
  if (! (width>0 && height>0))
    return 0;
  // Check fgjb
  if (!fgjb)
    return 0;
  if (fgjb->get_width()!=width || fgjb->get_height()!=height)
    return 0;
  // Check background
  int bgred = 0;
  if (bg44)
    bgred = compute_red(width, height, bg44->get_width(), bg44->get_height());
  if (bgred<1 || bgred>12)
    return 0;
  // Check foreground colors
  int fgred = 0;
  if (fgpm)
    fgred = compute_red(width, height, fgpm->columns(), fgpm->rows());
  if (fgred<1 || fgred>12)
    return 0;
  // Check that all components are present
  if (fgjb && bgred && fgred)
    return 1;
  // Unrecognized
  return 0;
}





//// DJVUIMAGE: LOW LEVEL RENDERING

GP<GBitmap>
DjVuImage::get_bitmap(const GRect &rect, 
                      int subsample, int align) const
{
  // Access image size
  int width = get_width();
  int height = get_height();
  GP<JB2Image> fgjb = get_fgjb();
  if ( width && height && fgjb && 
       (fgjb->get_width() == width) && 
       (fgjb->get_height() == height) ) 
    {
      return fgjb->get_bitmap(rect, subsample, align);
    }
  return 0;
}

GP<GPixmap>
DjVuImage::get_bg_pixmap(const GRect &rect, 
                         int subsample, double gamma) const
{
  GP<GPixmap> pm = 0;
  // Access image size
  int width = get_width();
  int height = get_height();
  GP<DjVuInfo> info = get_info();
  if (width<=0 || height<=0 || !info) return 0;
  GP<IWPixmap> bg44 = get_bg44();
  // Compute gamma_correction
  double gamma_correction = 1.0;
  if (gamma > 0)
    gamma_correction = gamma / info->gamma;
  if (gamma_correction < 0.1)
    gamma_correction = 0.1;
  else if (gamma_correction > 10)
    gamma_correction = 10;
  
  // CASE1: Incremental BG IWPixmap
  if (bg44)
    {
      int w = bg44->get_width();
      int h = bg44->get_height();
      // Avoid silly cases
      if (w==0 || h==0 || width==0 || height==0)
        return 0;
      // Determine how much bg44 is reduced
      int red = compute_red(width,height,w,h);
      if (red<1 || red>12)
        return 0;
      // Handle pure downsampling cases
      if (subsample == red)
        pm = bg44->get_pixmap(1,rect);
      else if (subsample == 2*red)
        pm = bg44->get_pixmap(2,rect);    
      else if (subsample == 4*red)
        pm = bg44->get_pixmap(4,rect); 
      else if (subsample == 8*red)
        pm = bg44->get_pixmap(8,rect); 
      // Handle fractional downsampling case
      else if (red*4 == subsample*3)
        {
          GRect nrect = rect;
          GRect xrect = rect;
          xrect.xmin = (xrect.xmin/3)*4;
          xrect.ymin = (xrect.ymin/3)*4;
          xrect.xmax = ((xrect.xmax+2)/3)*4;
          xrect.ymax = ((xrect.ymax+2)/3)*4;
          nrect.translate(-xrect.xmin*3/4, -xrect.ymin*3/4);
          if (xrect.xmax > w) 
            xrect.xmax = w;
          if (xrect.ymax > h) 
            xrect.ymax = h;
          GP<GPixmap> ipm = bg44->get_pixmap(1,xrect);
          pm = new GPixmap();
          pm->downsample43(ipm, &nrect);
        }
      // Handle all other cases with pixmapscaler
      else
        {
          // find suitable power of two
          int po2 = 16;
          while (po2>1 && subsample<po2*red)
            po2 >>= 1;
          // setup pixmap scaler
          int inw = (w+po2-1)/po2;
          int inh = (h+po2-1)/po2;
          int outw = (width+subsample-1)/subsample;
          int outh = (height+subsample-1)/subsample;
          GPixmapScaler ps(inw, inh, outw, outh);
          ps.set_horz_ratio(red*po2, subsample);
          ps.set_vert_ratio(red*po2, subsample);
          // run pixmap scaler
          GRect xrect;
          ps.get_input_rect(rect,xrect);
          GP<GPixmap> ipm = bg44->get_pixmap(po2,xrect);
          pm = new GPixmap();
          ps.scale(xrect, *ipm, rect, *pm);
        }
      // Apply gamma correction
      if (pm && gamma_correction!=1.0)
        pm->color_correct(gamma_correction);
      return pm;
    }

  // FAILURE
  return 0;
}

int  
DjVuImage::stencil(GPixmap *pm, const GRect &rect, 
                         int subsample, double gamma) const
{
  // Access components
  int width = get_width();
  int height = get_height();
  GP<DjVuInfo> info = get_info();
  if (width<=0 || height<=0 || !info) return 0;
  GP<JB2Image> fgjb = get_fgjb();
  GP<GPixmap> fgpm = get_fgpm();
  // Compute gamma_correction
  double gamma_correction = 1.0;
  if (gamma > 0)
    gamma_correction = gamma / info->gamma;
  if (gamma_correction < 0.1)
    gamma_correction = 0.1;
  else if (gamma_correction > 10)
    gamma_correction = 10;

  // CASE1: JB2 stencil and FG pixmap
  if (fgjb && fgpm)
    {
      GP<GBitmap> bm = get_bitmap(rect, subsample);
      if (bm && pm)
        {
          int w = fgpm->columns();
          int h = fgpm->rows();
          // Determine foreground reduction
          int red = compute_red(width,height,w,h);
          if (red<1 || red>12)
            return 0;
          // Try supersampling foreground pixmap by an integer factor
          int supersample = ( red>subsample ? red/subsample : 1);
          int wantedred = supersample*subsample;
          // Try simple foreground upsampling
          if (red == wantedred)
            {
              // Simple foreground upsampling is enough.
              pm->stencil(bm, fgpm, supersample, &rect, gamma_correction);
              return 1;
            }
          else 
            {
              // Must rescale foreground pixmap first
              GP<GPixmap> nfg;
              int desw = (w*red+wantedred-1)/wantedred;
              int desh = (h*red+wantedred-1)/wantedred;
              // Cache rescaled fgpm for speed
              static const DjVuImage *tagimage  = 0;
              static const GPixmap *tagfgpm   = 0;
              static GP<GPixmap> cachednfg = 0;
              // Check whether cached fgpm applies.
              if ( cachednfg && this==tagimage && fgpm==tagfgpm
                   && desw==(int)cachednfg->columns()
                   && desh==(int)cachednfg->rows() )
                {
                  nfg = cachednfg;
                }
              else
                {
                  GPixmapScaler ps(w,h,desw,desh);
                  ps.set_horz_ratio(red, wantedred);
                  ps.set_vert_ratio(red, wantedred);
                  nfg = new GPixmap;
                  GRect provided(0,0,w,h);
                  GRect desired(0,0,desw,desh);
                  ps.scale(provided, *fgpm, desired, *nfg);
                }
              // Compute
              pm->stencil(bm, nfg, supersample, &rect, gamma_correction);
              // Cache
              tagimage = this;
              tagfgpm = fgpm;
              cachednfg = nfg;
              return 1;
            }
        }
    }
  // FAILURE
  return 0;
}

GP<GPixmap>
DjVuImage::get_fg_pixmap(const GRect &rect, 
                         int subsample, double gamma) const
{
  // Obtain white background pixmap
  GP<GPixmap> pm = 0;
  // Access components
  int width = get_width();
  int height = get_height();
  if (width && height)
    {
      pm = new GPixmap(rect.height(),rect.width(), &GPixel::WHITE);
      if (stencil(pm, rect, subsample, gamma))
        return pm;
    }
  return 0;
}

GP<GPixmap>
DjVuImage::get_pixmap(const GRect &rect, int subsample, double gamma) const
{
  // Get background
  GP<GPixmap> pm = get_bg_pixmap(rect, subsample, gamma);
  // Superpose foreground
  if (! stencil(pm, rect, subsample, gamma))
    // Avoid ugly progressive display (hack)
    if (get_fgjb()) return 0;
  // Return
  return pm;
}


//// DJVUIMAGE: RENDERING (ARBITRARY SCALE)

typedef GP<GBitmap>(DjVuImage::*BImager)(const GRect &, int, int) const;
typedef GP<GPixmap>(DjVuImage::*PImager)(const GRect &, int, double) const;

static GP<GBitmap>
do_bitmap(const DjVuImage &dimg, BImager get,
          const GRect &rect, const GRect &all, int align )
{
  // Sanity
  if (! ( all.contains(rect.xmin, rect.ymin) &&
          all.contains(rect.xmax-1, rect.ymax-1) ))
    THROW("(DjVuImage::get_pixmap) Illegal target rectangles");
  // Check for integral reduction
  int red;
  int w = dimg.get_width();
  int h = dimg.get_height();
  int rw = all.width();
  int rh = all.height();
  GRect zrect = rect; 
  zrect.translate(-all.xmin, -all.ymin);
  for (red=1; red<=15; red++)
    if (rw*red>w-red && rw*red<w+red && rh*red>h-red && rh*red<h+red)
      return (dimg.*get)(zrect, red, align);
  // Find best reduction
  for (red=15; red>1; red--)
    if ( (rw*red < w && rh*red < h) ||
         (rw*red*3 < w || rh*red*3 < h) )
      break;
  // Setup bitmap scaler
  GBitmapScaler bs;
  bs.set_input_size( (w+red-1)/red, (h+red-1)/red );
  bs.set_output_size( rw, rh );
  bs.set_horz_ratio( rw*red, w );
  bs.set_vert_ratio( rh*red, h );
  // Scale
  GRect srect;
  bs.get_input_rect(zrect, srect);
  GP<GBitmap> sbm = (dimg.*get)(srect, red, 1);
  if (!sbm) return 0;
  int border = ((zrect.width() + align - 1) & ~(align - 1)) - zrect.width();
  GP<GBitmap> bm = new GBitmap(zrect.height(), zrect.width(), border);
  bs.scale(srect, *sbm, zrect, *bm);
  return bm;
}

static GP<GPixmap>
do_pixmap(const DjVuImage &dimg, PImager get,
          const GRect &rect, const GRect &all, double gamma=0 )
{
  // Sanity
  if (! ( all.contains(rect.xmin, rect.ymin) &&
          all.contains(rect.xmax-1, rect.ymax-1) ))
    THROW("(DjVuImage::get_pixmap) Illegal target rectangles");
  // Check for integral reduction
  int red;
  int w = dimg.get_width();
  int h = dimg.get_height();
  int rw = all.width();
  int rh = all.height();
  GRect zrect = rect; 
  zrect.translate(-all.xmin, -all.ymin);
  for (red=1; red<=15; red++)
    if (rw*red>w-red && rw*red<w+red && rh*red>h-red && rh*red<h+red)
      return (dimg.*get)(zrect, red, gamma);
  // These reductions usually go faster (improve!)
  static int fastred[] = { 12,6,4,3,2,1 };
  // Find best reduction
  for (int i=0; (red=fastred[i])>1; i++)
    if ( (rw*red < w && rh*red < h) ||
         (rw*red*3 < w || rh*red*3 < h) )
      break;
  // Setup pixmap scaler
  GPixmapScaler ps;
  ps.set_input_size( (w+red-1)/red, (h+red-1)/red );
  ps.set_output_size( rw, rh );
  ps.set_horz_ratio( rw*red, w );
  ps.set_vert_ratio( rh*red, h );
  // Scale
  GRect srect;
  ps.get_input_rect(zrect, srect);
  GP<GPixmap> spm = (dimg.*get)(srect, red, gamma);
  if (!spm) return 0;
  GP<GPixmap> pm = new GPixmap;
  ps.scale(srect, *spm, zrect, *pm);
  return pm;
}

GP<GPixmap>  
DjVuImage::get_pixmap(const GRect &rect, const GRect &all, double gamma) const
{
  return do_pixmap(*this, & DjVuImage::get_pixmap, rect, all, gamma);
}

GP<GBitmap>  
DjVuImage::get_bitmap(const GRect &rect, const GRect &all, int align) const
{
  return do_bitmap(*this, & DjVuImage::get_bitmap, rect, all, align);
}

GP<GPixmap>  
DjVuImage::get_bg_pixmap(const GRect &rect, const GRect &all, double gamma) const
{
  return do_pixmap(*this, & DjVuImage::get_bg_pixmap, rect, all, gamma);
}

GP<GPixmap>  
DjVuImage::get_fg_pixmap(const GRect &rect, const GRect &all, double gamma) const
{
  return do_pixmap(*this, & DjVuImage::get_fg_pixmap, rect, all, gamma);
}


//// DJVUIMAGE: DECODING

void
DjVuImage::decode(ByteStream &bs, DjVuInterface *notifier)
{
  // Reset everything
  init();
  // Process main chunk header
  IFFByteStream iff(bs);
  GString chkid;
  if (! iff.get_chunk(chkid))
    THROW("EOF");

  // ------------------------------
  // DJVU IMAGE
  // ------------------------------
  if (chkid == "FORM:DJVU")
    {
      mimetype = "image/djvu";
      // Decode DJVU chunks
      GString desc;
      int chksize;
      while ((chksize = iff.get_chunk(chkid)))
        {
          // Check that first chunk is an INFO chunk.
          if (info==0 && chkid!="INFO")
            THROW("DjVuDecoder:: Corrupted file (Does not start with INFO chunk)");

          // --- CHUNK 'INFO'
          if (chkid=="INFO")
            {
              if (info)
                THROW("DjVu Decoder: Corrupted file (Duplicate INFO chunk)"); 
              info = new DjVuInfo;
              info->decode(iff);
              if (notifier) notifier->notify_relayout();
              // Describe this chunk
              desc.format(" %0.1f Kb\t'%s'\tPage information.\n", 
                          chksize/1024.0, (const char*)chkid );
              
            }
          // --- CHUNK "ANTa"
          else if (chkid=="ANTa")
            {
              if (! anno) anno=new DjVuAnno;
              anno->decode(iff);
              desc.format(" %0.1f Kb\t'%s'\tPage annotation.\n",
                          chksize/1024.0, (const char*)chkid);
            }
          // --- CHUNK "Sjbz"
          else if (chkid == "Sjbz")
            {
              if (fgjb)
                THROW("DjVu Decoder: Corrupted data (Duplicate FGxx chunk)");
              GP<JB2Image> jimg = new JB2Image;
              jimg->decode(iff);
              fgjb = jimg;
              if (notifier) notifier->notify_redisplay();
              desc.format(" %0.1f Kb\t'%s'\tJB2 foreground mask (%dx%d)\n",
                          chksize/1024.0, (const char*)chkid,
                          fgjb->get_width(), fgjb->get_height() );
            }
          //--- CHUNK "BG44"
          else if (chkid == "BG44")
            {
              if (! bg44)
                {
                  // First chunk
                  GP<IWPixmap> img44 = new IWPixmap;
                  img44->decode_chunk(iff);
                  bg44 = img44;
                  if (notifier) notifier->notify_redisplay();
                  desc.format(" %0.1f Kb\t'%s'\tIW44 background (%dx%d)\n",
                              chksize/1024.0, (const char*)chkid,
                              bg44->get_width(), bg44->get_height() );
                }
              else
                {
                  // Refinement chunks
                  bg44->decode_chunk(iff);
                  if (notifier) notifier->notify_redisplay();
                  desc.format(" %0.1f Kb\t'%s'\tIW44 background (part %d).\n",
                              chksize/1024.0, (const char*)chkid,
                              bg44->get_serial() );
                  
                }
            }
          // --- CHUNK "FG44"
          else if (chkid == "FG44")
            {
              if (fgpm)
                THROW("DjVu Decoder: Corrupted data (Duplicate foreground layer)");
              IWPixmap fg44;
              fg44.decode_chunk(iff);
              fgpm = fg44.get_pixmap();
              if (notifier) notifier->notify_redisplay();
              desc.format(" %0.1f Kb\t'%s'\tIW44 foreground colors (%dx%d)\n",
                          chksize/1024.0, (const char*)chkid,
                          fg44.get_width(), fg44.get_height() );
            }
          // --- IMPLEMENT THS AS SOON AS DEFINED
          else if (chkid == "BG2k")
            {
              if (bg44)
                THROW("DjVu Decoder: Corrupted data (Duplicate background layer)");
              desc.format(" %0.1f Kb\t'%s'\tJPEG2000 background (Not yet implemented).\n", 
                          chksize/1024.0, (const char*)chkid);
            }
          else if (chkid == "FG2k")
            {
              if (fgpm)
                THROW("DjVu Decoder: Corrupted data (Duplicate foreground layer)");
              desc.format(" %0.1f Kb\t'%s'\tJPEG2000 foreground colors (Not yet implemented).\n", 
                          chksize/1024.0, (const char*)chkid);
            }
          // --- TENTATIVE CHUNKS
          else if (chkid == "Smmr")
            {
              if (fgjb)
                THROW("DjVu Decoder: Corrupted data (Duplicate background layer)");
              desc.format(" %0.1f Kb\t'%s'\tG4/MMR foreground mask (Tentative - Unsupported).\n", 
                          chksize/1024.0, (const char*)chkid);
            }
          else if (chkid == "BGjp")
            {
              if (bg44)
                THROW("DjVu Decoder: Corrupted data (Duplicate background layer)");
              desc.format(" %0.1f Kb\t'%s'\tJPEG background (Tentative - Unsupported).\n", 
                          chksize/1024.0, (const char*)chkid);
            }
          else if (chkid == "FGjp")
            {
              if (fgpm)
                THROW("DjVu Decoder: Corrupted data (Duplicate foreground layer)");
              desc.format(" %0.1f Kb\t'%s'\tJPEG foreground colors (Tentative - Unsupported).\n", 
                          chksize/1024.0, (const char*)chkid);
            }
          else
            {
              desc.format(" %0.1f Kb\t'%s'\tUnknown chunk (Ignored).\n",
                          chksize/1024.0, (const char*)chkid);
            }
          // Update description and notify
          description = description + desc;
          if (notifier) notifier->notify_chunk(chkid, desc);
          // Close chunk
          iff.close_chunk();
        }
      // Record file size
      filesize = iff.tell();
      if (bg44)
        bg44->close_codec();
      // Complete description
      if (info)
        {
          desc.format("DJVU Image (%dx%d) version %d:\n\n", 
                      info->width, info->height, info->version);
          description = desc + description;
          int rawsize = info->width * info->height * 3;
          desc.format("\nCompression ratio: %0.f (%0.1f Kb)\n",
                      (double)rawsize/filesize, filesize/1024.0 );
          description = description + desc;
        }
      else
        THROW("DjVu Decoder: Corrupted data (Missing INFO chunk)");
    }

  // ------------------------------
  // IW44 IMAGE
  // ------------------------------
  else if (chkid == "FORM:PM44" || chkid == "FORM:BM44" )
    {
      mimetype = "image/iw44";
      // Decode IW44 chunks
      GString desc;
      int chksize;
      GP<IWPixmap> img44;
      while ((chksize = iff.get_chunk(chkid)))
        {
          
          // --- CHUNK 'PM44' OR 'BM44'
          if (chkid=="PM44" || chkid=="BM44")
            {
              if (! img44)
                {
                  // First chunk
                  img44 = new IWPixmap;
                  img44->decode_chunk(iff);
                  info = new DjVuInfo;
                  info->width = img44->get_width();
                  info->height = img44->get_height();
                  info->dpi = 100;
                  bg44 = img44;
                  if (notifier) notifier->notify_relayout();
                }
              else
                {
                  // Refinwement chunks
                  img44->decode_chunk(iff);
                  if (notifier) notifier->notify_redisplay();
                }
              // Describe
              desc.format(" %0.1f Kb\t'%s'\tIW44 wavelet data (part %d)\n", 
                          chksize/1024.0, (const char*)chkid, 
                          img44->get_serial());
            }
          // --- CHUNK 'ANTa'
          else if (chkid=="ANTa")
            {
              if (! anno) anno=new DjVuAnno;
              anno->decode(iff);
              desc.format(" %0.1f Kb\t'%s'\tPage annotation.\n",
                          chksize/1024.0, (const char*)chkid);
            }
          // --- UNKNOWN CHUNK
          else
            {
              desc.format(" %0.1f Kb\t'%s'\tUnknown chunk (Ignored).\n",
                          chksize/1024.0, (const char*)chkid);
            }
          // Update description and notify
          description = description + desc;
          if (notifier) notifier->notify_chunk(chkid, desc);
          // Close chunk
          iff.close_chunk();
        }
      // Record file size
      filesize = iff.tell();
      // Complete description
      if (info)
        {
          desc.format("IW44 Image (%dx%d) :\n\n", 
                      info->width, info->height);
          description = desc + description;
          int rawsize = info->width * info->height * 3;
          desc.format("\nCompression ratio: %0.f (%0.1f Kb)\n",
                      (double)rawsize/filesize, filesize/1024.0 );
          description = description + desc;
        }
      else
        THROW("DjVu Decoder: Corrupted data (Missing IW44 data chunks)");
    }
  else
    {
      // ------------------------------
      // UNKNOWN IMAGE
      // ------------------------------
      THROW("DejaVu decoder: a DJVU or IW44 image was expected");
    }
}
