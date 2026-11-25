#pragma once
// UI-only rasterization helpers. COLORREF is 0xBBGGRR; BI_RGB is 0xRRGGBB.
namespace mgpaint {
constexpr unsigned dib(unsigned c){return ((c&255u)<<16)|(c&0xff00u)|((c>>16)&255u);}
constexpr unsigned blendDib(unsigned bg,unsigned fg,unsigned covered,unsigned total){
 unsigned c=0;for(unsigned s=0;s<24;s+=8)c|=((((bg>>s)&255u)*(total-covered)+((fg>>s)&255u)*covered+total/2)/total)<<(16-s);return c;
}
constexpr int samples=8;
inline void segment(unsigned char* mask,int w,int h,int ax,int ay,int bx,int by,int radius){
 if(!mask||w<=0||h<=0||radius<=0)return;
 int l=(ax<bx?ax:bx)-radius-1,r=(ax>bx?ax:bx)+radius+1,t=(ay<by?ay:by)-radius-1,b=(ay>by?ay:by)+radius+1;
 if(l<0)l=0;if(t<0)t=0;if(r>w)r=w;if(b>h)b=h;
 const double vx=bx-ax,vy=by-ay,den=vx*vx+vy*vy,rr=double(radius)*radius;
 for(int y=t;y<b;++y)for(int x=l;x<r;++x){
  auto& p=mask[y*w+x];if(p)continue;
  double dx=x+0.5-ax,dy=y+0.5-ay,u=den?(dx*vx+dy*vy)/den:0;
  if(u<0)u=0;else if(u>1)u=1;dx-=u*vx;dy-=u*vy;
  if(dx*dx+dy*dy<=rr)p=1;
 }
}
inline unsigned downsample(const unsigned char* mask,int stride,int x,int y,unsigned bg,unsigned fg){
 unsigned n=0;for(int j=0;j<samples;++j)for(int i=0;i<samples;++i)n+=mask[(y*samples+j)*stride+x*samples+i];
 return blendDib(bg,fg,n,samples*samples);
}
}
