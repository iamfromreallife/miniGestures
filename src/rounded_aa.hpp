#pragma once
// 8x8 subpixel coverage, computed only for the corner of a rounded rectangle.
// Coordinates are integer device pixels. The remaining straight spans need no AA.
namespace rounded_aa {
constexpr int samples=8, coverage=samples*samples, max_radius=64;
struct Mask { unsigned char outer[max_radius*max_radius], inner[max_radius*max_radius]; };
inline void make_mask(Mask& m,int r,int stroke) {
 const int c=r*samples*2, outerR=c, innerR=(r-stroke)*samples*2;
 for(int y=0;y<r;++y) for(int x=0;x<r;++x) {
  int o=0,i=0;
  for(int sy=0;sy<samples;++sy) for(int sx=0;sx<samples;++sx) {
   const int dx=c-(2*(x*samples+sx)+1),dy=c-(2*(y*samples+sy)+1);
   const int d=dx*dx+dy*dy;
   o+=d<=outerR*outerR;
   i+=innerR>0&&d<=innerR*innerR;
  }
  m.outer[y*r+x]=(unsigned char)o;m.inner[y*r+x]=(unsigned char)i;
 }
}
// COLORREF is 0x00BBGGRR; a 32-bit BI_RGB DIB pixel is 0x00RRGGBB.
inline unsigned pixel(unsigned bg,unsigned border,unsigned fill,int o,int i) {
 unsigned out=0;
 for(int shift=0;shift<24;shift+=8) {
  const unsigned v=(((bg>>shift)&255)*(coverage-o)+((border>>shift)&255)*(o-i)+((fill>>shift)&255)*i+coverage/2)/coverage;
  out|=v<<(16-shift);
 }
 return out;
}
}
