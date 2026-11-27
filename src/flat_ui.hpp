#pragma once
// Modern flat native GDI drawing; no GDI+ startup or background thread.
// No additional runtime DLLs are distributed. Native controls retain input,
// keyboard navigation, clipboard, accessibility and combo popup behavior.
API int GetDlgCtrlID(HWND);
API B IsWindowEnabled(HWND);
struct TrackMouse { DWORD size,flags; HWND hwnd; DWORD hoverTime; };
API B TrackMouseEvent(TrackMouse*);
API B GetTextExtentPoint32W(HDC,const W*,int,POINT*);
static_assert(sizeof(TrackMouse)==24);

static PROC flatOld[164]{};
static bool flatHover[164]{},flatDown[164]{};

static UINT argb(DWORD rgb){return 0xff000000|((rgb&255)<<16)|(rgb&0xff00)|((rgb>>16)&255);}
#include "theme_palette.hpp"
#include "preview_raster.hpp"
static DWORD ink,muted,accent,accentText,frame,fieldFill,buttonFill,hoverFill,pressedFill,appFill,shapeFill,categoryFill;
static HBRUSH fieldBrush=nullptr;static bool darkTheme=false,legacyDarkTheme=false,highContrastTheme=false;
static bool flatEdit(int id){switch(id){case APPSEARCH:case SEARCHBOX:case NAMEBOX:case FOLDERBOX:case KEYBOX:case SENSEBOX:case ARGBOX:case EXPATHBOX:case EXEBOX:case CLASSBOX:case TITLEBOX:case THRESHBOX:case TRAILCOLOR:case TRAILWIDTH:case INDICATORSECONDS:case GESTURETIMEOUT:return true;default:return false;}}
static bool flatList(int id){return id==LIST||id==ACTLIST||id==APPLIST;}
static bool flatCombo(int id){return id==BUTTONBOX||id==MODBOX||id==TRIGGERBOX||id==EXECBOX||id==MODEBOX||id==LANGBOX||id==THEMEBOX||id==UPDATEBOX;}
static bool flatCheck(int id){return id==ENABLEBOX||id==INHERITBOX||id==OVERBOX||id==CLOSEBOX||id==STARTBOX||id==CONFIRMBOX||id==INDICATORBOX||id==STARTWINBOX||id==ADMINBOX||id==TOPBOX||id==TASKBARBOX||id==FULLSCREENBOX||id==REMOTEINPUTBOX||id==SELECTOR_ENABLED||id==SELECTOR_INHERIT;}
static bool flatFrame(int id){return flatEdit(id)||flatList(id)||id==TRAIN;}
static void fillSolid(HDC dc,RECT r,DWORD c){
 if(r.right<=r.left||r.bottom<=r.top)return;
 using ColorFn=DWORD(CB*)(HDC,DWORD);
 static auto set=(ColorFn)GetProcAddress(GetModuleHandleW(L"gdi32.dll"),"SetDCBrushColor");
 if(set){H b=GetStockObject(18);if(b){DWORD old=set(dc,c);if(old!=0xffffffffu){FillRect(dc,&r,b);set(dc,old);return;}}}
 H b=CreateSolidBrush(c);if(b){FillRect(dc,&r,b);DeleteObject(b);}
}
// Antialias just the four corners. All straight spans stay native GDI fills.
// No GDI+ startup, DLL additions, per-pixel GDI calls, or full-control image buffers.
#include "rounded_aa.hpp"
struct CornerBitmapInfo { DWORD size; int width,height; u16 planes,bits; DWORD compression,imageSize; int xppm,yppm; DWORD used,important; };
static_assert(sizeof(CornerBitmapInfo)==40);
// Eight reusable mask slots cover the normal control radii without a large cache.
struct CornerCache { int radius,mode; rounded_aa::Mask mask; };
static CornerCache cornerCache[8]{};static unsigned nextCorner=0;
static const rounded_aa::Mask& cachedCorner(int rad,int mode){
 for(auto& e:cornerCache)if(e.radius==rad&&e.mode==mode)return e.mask;
 auto& e=cornerCache[nextCorner++%8];rounded_aa::make_mask(e.mask,rad,mode);e.radius=rad;e.mode=mode;return e.mask;
}
struct GlyphKey{int kind,w,h;DWORD fg,bg,extra;};
struct GlyphCache{
 struct Entry{GlyphKey key{};u32* pixels=nullptr;up age=0;};
 Entry slots[24]{};up tick=0,pixelCount=0;
 static constexpr up budget=65536;
 static bool same(const GlyphKey&a,const GlyphKey&b){return a.kind==b.kind&&a.w==b.w&&a.h==b.h&&a.fg==b.fg&&a.bg==b.bg&&a.extra==b.extra;}
 const u32* find(const GlyphKey&k){for(auto& e:slots)if(e.pixels&&same(e.key,k)){e.age=++tick;return e.pixels;}return nullptr;}
 void drop(Entry& e){if(e.pixels){pixelCount-=(up)e.key.w*e.key.h;freep(e.pixels);e.pixels=nullptr;}}
 bool remember(const GlyphKey& k,u32* pixels){if(!pixels||k.w<=0||k.h<=0)return false;up n=(up)k.w*k.h;if(n>budget)return false;
  auto victim=[&](){Entry* v=&slots[0];for(auto& e:slots){if(!e.pixels)return &e;if(e.age<v->age)v=&e;}return v;};
  while(pixelCount+n>budget){Entry* v=nullptr;for(auto& e:slots)if(e.pixels&&(!v||e.age<v->age))v=&e;if(!v)return false;drop(*v);}
  auto* v=victim();drop(*v);v->key=k;v->pixels=pixels;v->age=++tick;pixelCount+=n;return true;
 }
 void clear(){for(auto& e:slots)drop(e);tick=0;}
};
static GlyphCache glyphCache;
static void glyphBlit(HDC dc,int left,int top,int w,int h,const u32* pixels){CornerBitmapInfo bi{40,w,-h,1,32,0,(DWORD)(w*h*4),0,0,0,0};StretchDIBits(dc,left,top,w,h,0,0,w,h,pixels,&bi,0,0xcc0020);}
template<class Paint> static void cachedGlyph(HDC dc,int left,int top,GlyphKey k,Paint paint){
 if(!StretchDIBits||k.w<=0||k.h<=0)return;
 if(auto* pixels=glyphCache.find(k)){glyphBlit(dc,left,top,k.w,k.h,pixels);return;}
 auto* pixels=(u32*)allocRaw((up)k.w*k.h*4);if(!pixels)return;
 if(paint(pixels)){glyphBlit(dc,left,top,k.w,k.h,pixels);if(glyphCache.remember(k,pixels))return;}freep(pixels);
}
static void rounded(HDC dc,RECT r,int radius,DWORD fill,DWORD border,bool outline=true,DWORD background=appFill){
 int w=r.right-r.left,h=r.bottom-r.top;if(w<=0||h<=0)return;
 int rad=minn(rounded_aa::max_radius,minn(maxn(0,radius),minn(w,h)/2));
 if(!outline)border=fill;
 if(rad==0){fillSolid(dc,r,border);if(outline&&w>2&&h>2)fillSolid(dc,{r.left+1,r.top+1,r.right-1,r.bottom-1},fill);return;}
 if(!StretchDIBits){H b=CreateSolidBrush(fill),p=CreatePen(0,1,border);if(!b||!p){if(b)DeleteObject(b);if(p)DeleteObject(p);fillSolid(dc,r,fill);return;}H ob=SelectObject(dc,b),op=SelectObject(dc,p);RoundRect(dc,r.left,r.top,r.right,r.bottom,rad*2,rad*2);SelectObject(dc,op);SelectObject(dc,ob);DeleteObject(p);DeleteObject(b);return;}
 // Fill only the straight spans. The corner blocks are composited separately.
 fillSolid(dc,{r.left+rad,r.top,r.right-rad,r.bottom},border);
 fillSolid(dc,{r.left,r.top+rad,r.right,r.bottom-rad},border);
 if(outline){
  fillSolid(dc,{r.left+rad,r.top+1,r.right-rad,r.bottom-1},fill);
  fillSolid(dc,{r.left+1,r.top+rad,r.right-1,r.bottom-rad},fill);
 }
 int mode=outline?1:0;const auto& mask=cachedCorner(rad,mode);
 for(int corner=0;corner<4;++corner){
  GlyphKey key{10+4*mode+corner,rad,rad,border,background,fill};
  cachedGlyph(dc,(corner&1)?r.right-rad:r.left,(corner&2)?r.bottom-rad:r.top,key,[&](u32* pixels){
   for(int y=0;y<rad;++y)for(int x=0;x<rad;++x){
    int sy=(corner&2)?rad-1-y:y,sx=(corner&1)?rad-1-x:x,k=sy*rad+sx;
    pixels[y*rad+x]=rounded_aa::pixel(background,border,fill,mask.outer[k],mask.inner[k]);
   }return true;
  });
 }
}
static void flatLine(HDC dc,const POINT* pts,int count,DWORD c,int thickness){if(count<2)return;H p=CreatePen(0,maxn(1,thickness),c);if(!p)return;H o=SelectObject(dc,p);Polyline(dc,pts,count);SelectObject(dc,o);DeleteObject(p);}
static void textFlat(HDC dc,const W* s,RECT r,DWORD color,UINT flags=0x20|4|0x8000,HFONT face=nullptr){H old=SelectObject(dc,face?face:font);SetBkMode(dc,1);SetTextColor(dc,color);DrawTextW(dc,s,-1,&r,flags|0x800);SelectObject(dc,old);}
static void elevationIcon(HDC dc,int x,int y,DWORD fg);
static constexpr int adminIconInset=19;
// Paint a control into a temporary bitmap: one blit, no background flash.
static void flatFace(HWND h,HDC dest,int id){RECT r{};GetClientRect(h,&r);if(r.right<2||r.bottom<2)return;HDC mem=CreateCompatibleDC(dest);H bmp=mem?CreateCompatibleBitmap(dest,r.right,r.bottom):nullptr;H old=bmp?SelectObject(mem,bmp):nullptr;HDC dc=bmp?mem:dest;
 FillRect(dc,&r,white);bool enabled=IsWindowEnabled(h)!=0,focus=GetFocus()==h;DWORD color=enabled?ink:muted;
 if(flatCombo(id)){
  bool dropped=SendMessageW(h,0x157,0,0)!=0;DWORD bg=!enabled?buttonFill:dropped?pressedFill:flatHover[id]?hoverFill:buttonFill;
  rounded(dc,r,px(4),bg,focus||dropped?accent:frame);W value[512]{};int n=(int)SendMessageW(h,0x147,0,0);if(n>=0&&SendMessageW(h,0x149,n,0)<512)SendMessageW(h,0x148,n,(LP)value);
  RECT t=r;t.left+=px(8);t.right-=px(28);textFlat(dc,value,t,color);int x=r.right-px(14),y=r.bottom/2;POINT a[]={{x-px(4),y-px(2)},{x,y+px(2)},{x+px(4),y-px(2)}};flatLine(dc,a,3,color,px(1));
 }else if(flatCheck(id)){
  int size=px(15),y=(r.bottom-size)/2;RECT q{px(1),y,px(1)+size,y+size};bool on=SendMessageW(h,0xf0,0,0)==1;rounded(dc,q,px(3),on?accent:flatHover[id]?hoverFill:fieldFill,focus||on?accent:frame);
  if(on){POINT tick[]={{q.left+px(3),q.top+px(7)},{q.left+px(6),q.top+px(10)},{q.left+px(12),q.top+px(4)}};flatLine(dc,tick,3,accentText,px(2));}
  W s[256];GetWindowTextW(h,s,256);RECT t=r;t.left=q.right+px(7);if(id==ADMINBOX){elevationIcon(dc,t.left,(r.bottom-px(14))/2,color);t.left+=px(adminIconInset);}textFlat(dc,s,t,color);
 }
 if(bmp){BitBlt(dest,0,0,r.right,r.bottom,mem,0,0,0xcc0020);SelectObject(mem,old);DeleteObject(bmp);}if(mem)DeleteDC(mem);
}
static void installComboPopup(HWND combo);
static LR CB flatProc(HWND h,UINT m,WP w,LP l){int id=GetDlgCtrlID(h);if(id<0||id>=164||!flatOld[id])return DefWindowProcW(h,m,w,l);PROC orig=flatOld[id];bool face=flatCombo(id)||flatCheck(id);
 if(m==0x200&&!flatHover[id]){flatHover[id]=true;TrackMouse t{sizeof(t),2,h,0};TrackMouseEvent(&t);InvalidateRect(h,nullptr,0);}else if(m==0x2a3){flatHover[id]=false;InvalidateRect(h,nullptr,0);}else if(m==0x201){flatDown[id]=true;InvalidateRect(h,nullptr,0);}else if(m==0x202||m==0x215){flatDown[id]=false;InvalidateRect(h,nullptr,0);}
 if(m==0x18&&!w){flatHover[id]=false;flatDown[id]=false;}if(face&&m==0x14)return 1;
 if(face&&m==0xf){PAINT p{};HDC dc=BeginPaint(h,&p);flatFace(h,dc,id);EndPaint(h,&p);return 0;}
 if(face&&m==0x318){flatFace(h,(HDC)w,id);return 0;}
 if(flatCombo(id)&&m==0x134&&fieldBrush){SetTextColor((HDC)w,ink);SetBkColor((HDC)w,fieldFill);return (LR)fieldBrush;}
 // Some native combos may allocate or recreate their list lazily.
 if(flatCombo(id)&&(m==0x201||m==0x100||m==0x104||(m==0x14f&&w)))installComboPopup(h);
 // Native processing still owns selections, notifications, focus, keyboard and popup.
 LR result=CallWindowProcW(orig,h,m,w,l);
 if(face&&(m==0x7||m==0x8||m==0xa||m==0xc||m==0x30||m==0x5||m==0x100||m==0x101||m==0x104||m==0x105||m==0xf1||m==0x14e||m==0x143||m==0x14b||m==0x14f||m==0x201||m==0x202||m==0x207||m==0x20a||m==0x215))InvalidateRect(h,nullptr,0);
 // Focus is transferred inside native mouse processing, before the next
 // dropdown animates. Finish the full face repaint now, not a queued paint.
 if(flatCombo(id)&&(m==0x7||m==0x8))UpdateWindow(h);
 return result;
}
static void flatSubclass(int id){flatOld[id]=(PROC)SetWindowLongPtrW(ctrl(id),-4,(ip)flatProc);if(flatCombo(id))installComboPopup(ctrl(id));}
#include "text_alignment.hpp"
// Native edits keep their keyboard, IME, selection and clipboard behavior.
// EM_GETRECT accounts for the control's own internal formatting offset.
static void alignNativeEdit(HWND edit,HWND parent,RECT outer,int textHeight,int spare){
 if(!edit||!parent)return;RECT win{},format{};if(!GetWindowRect(edit,&win))return;
 POINT pos{win.left,win.top},client{0,0};ScreenToClient(parent,&pos);ClientToScreen(edit,&client);
 SendMessageW(edit,0xb2,0,(LP)&format); // EM_GETRECT
 int nativeTop=client.y-win.top+maxn(0,format.top);
 int h=win.bottom-win.top,margin=1,room=outer.bottom-outer.top;
 if(h>room-2*margin)return;
 auto band=mgtext::editBand(room,textHeight,spare,nativeTop);
 // The actual child size may differ by a pixel after DPI rounding.
 int top=clamp(band.top,margin,room-margin-h);
 if(pos.y!=outer.top+top)SetWindowPos(edit,nullptr,pos.x,outer.top+top,0,0,1|4|16);
}
static void placeNativeEdit(HWND edit,HWND parent,RECT outer,int leftPad,int rightPad,int textHeight,int spare){
 if(!edit)return;auto band=mgtext::editBand(outer.bottom-outer.top,textHeight,spare);
 SetWindowPos(edit,nullptr,outer.left+leftPad,outer.top+band.top,maxn(1,outer.right-outer.left-leftPad-rightPad),band.height,4|16);
 alignNativeEdit(edit,parent,outer,textHeight,spare);
}
static void flatMetrics(){for(int i=0;i<164;i++)if(ctrl(i)){if(flatList(i))SendMessageW(ctrl(i),0x1a0,0,px(21));if(flatCombo(i)){SendMessageW(ctrl(i),0x153,(WP)-1,px(17));SendMessageW(ctrl(i),0x153,0,px(21));}if(flatEdit(i))SendMessageW(ctrl(i),0xd3,3,(LP)(px(2)|(px(2)<<16)));}}

static float aaAbs(float v){return v<0?-v:v;}
static float aaLen(float x,float y){x=aaAbs(x);y=aaAbs(y);float hi=x>y?x:y,lo=x>y?y:x;return hi+lo*0.375f;}
static void aaBlend(u32& dst,u8 a,DWORD rgb){u32 dr=dst&255,dg=(dst>>8)&255,db=(dst>>16)&255;u32 sr=rgb&255,sg=(rgb>>8)&255,sb=(rgb>>16)&255;dr=(dr*(255-a)+sr*a+127)/255;dg=(dg*(255-a)+sg*a+127)/255;db=(db*(255-a)+sb*a+127)/255;dst=(db<<16)|(dg<<8)|dr;}
static bool aaPointInTri(float px,float py,float x1,float y1,float x2,float y2,float x3,float y3){float d1=(px-x2)*(y1-y2)-(x1-x2)*(py-y2);float d2=(px-x3)*(y2-y3)-(x2-x3)*(py-y3);float d3=(px-x1)*(y3-y1)-(x3-x1)*(py-y1);bool n=(d1<0)||(d2<0)||(d3<0),p=(d1>0)||(d2>0)||(d3>0);return !(n&&p);}
static void aaFillTri(u32* pix,int w,int h,float x1,float y1,float x2,float y2,float x3,float y3,DWORD rgb,int firstRow=0){int minx=maxn(0,(int)minn(int(x1),minn(int(x2),int(x3)))-1),maxx=minn(w-1,(int)maxn(int(x1),maxn(int(x2),int(x3)))+1),miny=maxn(firstRow,(int)minn(int(y1),minn(int(y2),int(y3)))-1),maxy=minn(firstRow+h-1,(int)maxn(int(y1),maxn(int(y2),int(y3)))+1);for(int y=miny;y<=maxy;y++)for(int x=minx;x<=maxx;x++){float px=x+0.5f,py=y+0.5f;if(aaPointInTri(px,py,x1,y1,x2,y2,x3,y3))aaBlend(pix[(y-firstRow)*w+x],255,rgb);}}
static void aaLine(u32* pix,int w,int h,float x1,float y1,float x2,float y2,float radius,DWORD rgb,int firstRow=0){float vx=x2-x1,vy=y2-y1,den=vx*vx+vy*vy;if(den<1e-4f)den=1;int minx=maxn(0,(int)(minn(int(x1),int(x2))-radius-1)),maxx=minn(w-1,(int)(maxn(int(x1),int(x2))+radius+1));int miny=maxn(firstRow,(int)(minn(int(y1),int(y2))-radius-1)),maxy=minn(firstRow+h-1,(int)(maxn(int(y1),int(y2))+radius+1));for(int y=miny;y<=maxy;y++)for(int x=minx;x<=maxx;x++){float px=x+0.5f,py=y+0.5f,t=((px-x1)*vx+(py-y1)*vy)/den;if(t<0)t=0;else if(t>1)t=1;float dx=px-(x1+t*vx),dy=py-(y1+t*vy),cover=radius+0.5f-aaLen(dx,dy);if(cover<=0)continue;aaBlend(pix[(y-firstRow)*w+x],(u8)minn(255,maxn(0,(int)(cover*255))),rgb);}}
static void aaRect(u32* pix,int w,int h,int l,int t,int r,int b,DWORD rgb){l=maxn(0,l);t=maxn(0,t);r=minn(w,r);b=minn(h,b);for(int y=t;y<b;y++)for(int x=l;x<r;x++)aaBlend(pix[y*w+x],255,rgb);}
static void aaDownsample(u32* lo,int w,int h,const u32* hi,int scale){for(int y=0;y<h;y++)for(int x=0;x<w;x++){u32 ar=0,ag=0,ab=0;for(int yy=0;yy<scale;yy++)for(int xx=0;xx<scale;xx++){u32 p=hi[(y*scale+yy)*(w*scale)+(x*scale+xx)];ar+=p&255;ag+=(p>>8)&255;ab+=(p>>16)&255;}u32 n=scale*scale;lo[y*w+x]=mgpaint::dib(((ar+n/2)/n)|(((ag+n/2)/n)<<8)|(((ab+n/2)/n)<<16));}}
static void aaPolylineToSurface(u32* pix,int w,int h,const POINT* pts,int count,float radius,DWORD rgb,int firstRow=0){for(int i=1;i<count;i++)aaLine(pix,w,h,(float)pts[i-1].x,(float)pts[i-1].y,(float)pts[i].x,(float)pts[i].y,radius,rgb,firstRow);} 
// Render high-resolution strips into a bounded scratch surface, then resolve
// the complete low-resolution image in one GDI transfer. Pixel coordinates,
// line order, repeated integer alpha blends and rounding are unchanged.
struct RasterScratch{u32* hi=nullptr,*lo=nullptr;up highCap=0,lowCap=0;bool busy=false;
 static bool grow(u32*& p,up& cap,up n){if(p&&cap>=n)return true;freep(p);p=(u32*)allocRaw(n*4);cap=p?n:0;return p!=nullptr;}
 void clear(){if(busy)return;freep(hi);freep(lo);hi=lo=nullptr;highCap=lowCap=0;}
};
static RasterScratch rasterScratch;
static bool drawPolylineAA(HDC dc,RECT bounds,const POINT* pts,int count,const POINT* tip,int tipCount,DWORD fg,DWORD bg,int thickness){
 if(count<2)return false;const int scale=8;
 int w=maxn(1,bounds.right-bounds.left),h=maxn(1,bounds.bottom-bounds.top),sw=w*scale;
 const up rowBytes=(up)sw*scale*4;const int band=maxn(1,minn(h,(int)(4*1024*1024/rowBytes)));
 bool useScratch=!rasterScratch.busy&&RasterScratch::grow(rasterScratch.hi,rasterScratch.highCap,rowBytes*band/4)&&RasterScratch::grow(rasterScratch.lo,rasterScratch.lowCap,(up)w*h);
 auto* hi=useScratch?rasterScratch.hi:(u32*)allocRaw(rowBytes*band);auto* lo=useScratch?rasterScratch.lo:(u32*)allocRaw((up)w*h*4);
 if(!hi||!lo){if(!useScratch){freep(hi);freep(lo);}return false;}if(useScratch)rasterScratch.busy=true;
 auto mapPoint=[&](const POINT& p){return POINT{(p.x-bounds.left)*scale+scale/2,(p.y-bounds.top)*scale+scale/2};};
 float radius=maxn(1,thickness*scale)*0.5f;POINT linePts[512],tipPts[8];
 int n=count>512?512:count,m=tip&&tipCount>=2?minn(8,tipCount):0;
 for(int i=0;i<n;i++)linePts[i]=mapPoint(pts[i]);for(int i=0;i<m;i++)tipPts[i]=mapPoint(tip[i]);
 for(int y=0;y<h;y+=band){
  int rows=minn(band,h-y),sh=rows*scale,firstRow=y*scale;
  std::fill_n(hi,(up)sw*sh,bg);
  aaPolylineToSurface(hi,sw,sh,linePts,n,radius,fg,firstRow);
  if(m>=2){aaPolylineToSurface(hi,sw,sh,tipPts,m,radius,fg,firstRow);
   if(m>=3)aaFillTri(hi,sw,sh,(float)tipPts[0].x,(float)tipPts[0].y,(float)tipPts[1].x,(float)tipPts[1].y,(float)tipPts[2].x,(float)tipPts[2].y,fg,firstRow);
  }
  aaDownsample(lo+(up)y*w,w,rows,hi,scale);
 }
 glyphBlit(dc,bounds.left,bounds.top,w,h,lo);if(useScratch)rasterScratch.busy=false;else{freep(hi);freep(lo);}return true;
}
static void drawFolderArrowAA(HDC dc,int left,int top,int size,DWORD fg,DWORD bg,bool closed){
 cachedGlyph(dc,left,top,{closed?2:1,size,size,fg,bg,0},[&](u32* pixels){
  const int scale=8;int sw=size*scale,sh=size*scale;auto* hi=(u32*)allocRaw((up)sw*sh*4);if(!hi)return false;
  std::fill_n(hi,(up)sw*sh,bg);
  if(closed)aaFillTri(hi,sw,sh,sw*0.28f,sh*0.20f,sw*0.72f,sh*0.50f,sw*0.28f,sh*0.80f,fg);
  else aaFillTri(hi,sw,sh,sw*0.18f,sh*0.32f,sw*0.50f,sh*0.68f,sw*0.82f,sh*0.32f,fg);
  aaDownsample(pixels,size,size,hi,scale);freep(hi);return true;
 });
}
#include "folder_icon.hpp"
static void drawFolderIconAA(HDC dc,RECT icon,DWORD bg){
 int w=icon.right-icon.left,h=icon.bottom-icon.top;if(w<=0||h<=0||w>256||h>256||!StretchDIBits)return;
 cachedGlyph(dc,icon.left,icon.top,{3,w,h,0,bg,0},[&](u32* pixels){
  for(int y=0;y<h;++y)for(int x=0;x<w;++x)pixels[y*w+x]=mgfolder::render(x,y,w,h,bg);return true;
 });
}
struct DRAWITEM{UINT type,id,item,action,state;HWND hwnd;HDC dc;RECT rect;up data;};
struct MEASUREITEM{UINT type,id,item,width,height;up data;};
static_assert(sizeof(DRAWITEM)==64 && sizeof(MEASUREITEM)==32);
static void elevationIcon(HDC dc,int x,int y,DWORD fg){
 using DrawIcon=B(*)(HDC,int,int,HICON,int,int,UINT,HBRUSH,UINT);
 auto draw=(DrawIcon)GetProcAddress(GetModuleHandleW(L"user32.dll"),"DrawIconEx");
 HICON shield=LoadIconW(nullptr,(const W*)(up)32518);
 if(draw&&shield&&draw(dc,x,y,shield,px(14),px(14),0,nullptr,3))return;
 POINT outline[]={{x+px(7),y},{x+px(13),y+px(2)},{x+px(12),y+px(8)},{x+px(7),y+px(13)},{x+px(2),y+px(8)},{x+px(1),y+px(2)},{x+px(7),y}};
 flatLine(dc,outline,7,fg,px(1));POINT cross[]={{x+px(7),y+px(2)},{x+px(7),y+px(10)}};flatLine(dc,cross,2,fg,px(1));
}
#include "gesture_preview.hpp"
// Keep geometry at 1/8-pixel precision until the final coverage resolve.
#include "preview_cache.hpp"
static PreviewCache previewCache;
static bool paintShapePreview(HDC dc,const P* sample,RECT r,DWORD color,DWORD bg,int arrowPx,int thicknessPx){
 if(!sample||!StretchDIBits)return false;
 constexpr int ss=mgpaint::samples;int w=r.right-r.left,h=r.bottom-r.top;
 if(w<=0||h<=0||w>256||h>256)return false;
 int sw=w*ss,sh=h*ss,arrow=arrowPx*ss,thickness=thicknessPx*ss;
 PreviewKey key{sample,w,h,arrow,thickness,color,bg};
 CornerBitmapInfo bi{40,w,-h,1,32,0,(DWORD)(w*h*4),0,0,0,0};
 if(const auto* pixels=previewCache.find(key))return StretchDIBits(dc,r.left,r.top,w,h,0,0,w,h,pixels,&bi,0,0xcc0020)>0;
 P points[N],head[3];
 if(!mgpreview::fit(sample,points,head,sw,sh,arrow,arrow+thickness))return false;
 auto* mask=(u8*)allocRaw((up)sw*sh);auto* pixels=(u32*)allocRaw((up)w*h*4);
 if(!mask||!pixels){freep(mask);freep(pixels);return false;}
 memset(mask,0,(up)sw*sh);
 for(int i=1;i<N;++i)mgpaint::segment(mask,sw,sh,points[i-1].x,points[i-1].y,points[i].x,points[i].y,thickness/2);
 for(int i=1;i<3;++i)mgpaint::segment(mask,sw,sh,head[i-1].x,head[i-1].y,head[i].x,head[i].y,thickness/2);
 for(int y=0;y<h;++y)for(int x=0;x<w;++x)pixels[y*w+x]=mgpaint::downsample(mask,sw,x,y,bg,color);
 int result=StretchDIBits(dc,r.left,r.top,w,h,0,0,w,h,pixels,&bi,0,0xcc0020);
 freep(mask);if(result<=0||!previewCache.remember(key,pixels))freep(pixels);return result>0;
}
static bool paintGesturePreview(HDC dc,const Gesture&g,RECT r,DWORD color,DWORD bg){
 return g.count>0&&paintShapePreview(dc,g.samples[0],r,color,bg,maxn(2,px(3)),maxn(1,px(1)));
}
static DWORD rowSelectionFill(bool hc){return hc?accent:shapeFill;}
static DWORD rowSelectionText(bool hc){return hc?accentText:mgtheme::contrastInk(rowSelectionFill(false));}
static void paintRowCore(DRAWITEM*d){RECT r=d->rect;bool listbox=d->type==2;
 // The current client width is authoritative, even when Windows issues an
 // owner-draw notification using a row rectangle from the previous size.
 if(listbox){RECT client{};if(GetClientRect(d->hwnd,&client)&&client.right>0){r.left=0;r.right=client.right;}}
 if(r.right<=r.left||r.bottom<=r.top)return;
 bool selected=(d->state&1)!=0;bool hc=highContrastTheme;DWORD rowBg=selected?rowSelectionFill(hc):fieldFill,rowFg=selected?rowSelectionText(hc):ink;FillRect(d->dc,&r,fieldBrush);if(d->item==(UINT)-1)return;W b[1024]{};UINT lenmsg=listbox?0x18a:0x149,txtmsg=listbox?0x189:0x148;LR n=SendMessageW(d->hwnd,lenmsg,d->item,0);if(n>=0&&n<1024)SendMessageW(d->hwnd,txtmsg,d->item,(LP)b);RECT q=r;q.left+=px(1);q.right-=px(1);q.top+=px(1);q.bottom-=px(1);if(selected)rounded(d->dc,q,px(4),rowBg,rowBg,false,fieldFill);RECT t=r;t.left+=px(7);t.right-=px(6);if(listbox&&(d->id==LIST||d->id==APPLIST)&&((int)d->data)<=-2){int c=-2-(int)d->data;int yy=(r.top+r.bottom)/2;DWORD fg=selected?rowSelectionText(hc):muted;bool shut=c>=0&&c<cfg.nc&&cfg.categories[c].collapsed;int arrowSize=px(9);drawFolderArrowAA(d->dc,t.left,yy-arrowSize/2,arrowSize,fg,rowBg,shut);RECT icon{t.left+px(11),yy-px(6),t.left+px(11)+px(16),yy-px(6)+px(12)};drawFolderIconAA(d->dc,icon,rowBg);t.left+=px(30);}if(listbox&&d->id==LIST&&page==2&&!settings&&(int)d->data>=0&&d->data<(up)cfg.ne&&*cfg.e[static_cast<int>(d->data)].folder)t.left+=px(16);if(listbox&&d->id==ACTLIST&&d->data<(up)edit.steps&&elevationAction(edit.actions[d->data].type)){elevationIcon(d->dc,t.left,(r.top+r.bottom-px(14))/2,rowFg);t.left+=px(19);}if(listbox&&d->id==LIST&&gstview()&&d->data<(up)cfg.ng){const Gesture&g=cfg.g[static_cast<int>(d->data)];if(g.count>0){
 // Reserve the thumbnail column independently of the label's length.
 // Long names are ellipsized; a drawn gesture never loses its preview
 // merely because the list or the application window becomes narrower.
 int right=r.right-maxn(1,px(6)),column=minn(px(26),maxn(0,right-r.left-px(2)));
 RECT preview{right-column,q.top,right,q.bottom};
 if(column>0&&preview.bottom>preview.top){
  DWORD pfg=hc?(selected?accentText:g.enabled?accent:muted):g.enabled?mgtheme::gestureInk(rowBg):mgtheme::gestureInk(rowBg,muted,30);
  t.right=minn(t.right,preview.left-px(3));
  paintGesturePreview(d->dc,g,preview,pfg,rowBg);
 }
 }}if(t.right>t.left)textFlat(d->dc,b,t,rowFg);}
static void paintRow(DRAWITEM*d);
static void paintButtonCore(DRAWITEM*d){RECT r=d->rect;bool tab=d->id>=TAB0&&d->id<=TAB2;bool sel=tab&&d->id==TAB0+page;bool primary=d->id==SAVEBTN||d->id==132;bool down=(d->state&1)||flatDown[d->id],focus=(d->state&0x10)!=0;bool enabled=!(d->state&4);DWORD bg=primary?(down?mgtheme::mix(accent,highContrastTheme?accentText:RGB(0,0,0),14):flatHover[d->id]?mgtheme::mix(accent,accentText,7):accent):tab?((sel||flatHover[d->id])?hoverFill:appFill):down?pressedFill:flatHover[d->id]?hoverFill:buttonFill;
 FillRect(d->dc,&r,white);if(tab){RECT t=r;t.bottom-=px(3);if(sel||flatHover[d->id])rounded(d->dc,t,px(4),bg,bg,false);if(sel){RECT u=r;u.top=u.bottom-px(2);rounded(d->dc,u,px(1),accent,accent,false);}}else rounded(d->dc,r,px(4),bg,primary?bg:focus?accent:frame);
 W b[256];GetWindowTextW(d->hwnd,b,256);RECT t=r;t.left+=px(tab?6:4);t.right-=px(tab?6:4);HFONT face=sel&&tabBold?tabBold:font;H old=SelectObject(d->dc,face);POINT extent{};GetTextExtentPoint32W(d->dc,b,len(b),&extent);SelectObject(d->dc,old);
 DWORD tc=primary?(highContrastTheme?accentText:mgtheme::contrastInk(bg)):enabled?ink:muted;
 if(extent.x>t.right-t.left&&t.bottom-t.top>=px(30)){RECT measure=t;measure.bottom=measure.top;H o=SelectObject(d->dc,face);DrawTextW(d->dc,b,-1,&measure,0x400|0x10|0x800|1);SelectObject(d->dc,o);int hh=measure.bottom-measure.top;if(hh<=t.bottom-t.top-px(2)){t.top+=(t.bottom-t.top-hh)/2;textFlat(d->dc,b,t,tc,1|0x10,face);return;}}
 // Anchor tab text at the same left inset for both font weights.
 textFlat(d->dc,b,t,tc,(tab?0:1)|4|0x20|0x8000,face);
}
// A bounded, UI-thread-only back buffer. Reset selected objects after each
// paint; the original fallback remains available for reentrancy or allocation failure.
struct ButtonBuffer{HDC dc=nullptr;H bmp=nullptr;int w=0,h=0;bool busy=false;
 void clear(){if(bmp)DeleteObject(bmp);if(dc)DeleteDC(dc);dc=nullptr;bmp=nullptr;w=h=0;}
 bool ensure(HDC target,int width,int height){
  if(dc&&bmp&&w>=width&&h>=height)return true;
  clear();dc=CreateCompatibleDC(target);if(!dc)return false;
  bmp=CreateCompatibleBitmap(target,width,height);if(!bmp){clear();return false;}w=width;h=height;return true;
 }
};
static ButtonBuffer buttonBuffer;
static void paintButton(DRAWITEM*d){
 int w=d->rect.right-d->rect.left,h=d->rect.bottom-d->rect.top;if(w<1||h<1)return;
 bool useCache=!buttonBuffer.busy&&(up)w*h<=131072&&buttonBuffer.ensure(d->dc,w,h);
 HDC mem=useCache?buttonBuffer.dc:CreateCompatibleDC(d->dc);
 H bmp=useCache?buttonBuffer.bmp:mem?CreateCompatibleBitmap(d->dc,w,h):nullptr;
 if(!bmp){if(mem&&!useCache)DeleteDC(mem);paintButtonCore(d);return;}
 if(useCache)buttonBuffer.busy=true;
 H old=SelectObject(mem,bmp);if(!old||old==PTR(-1)){if(useCache){buttonBuffer.busy=false;buttonBuffer.clear();}else{DeleteObject(bmp);DeleteDC(mem);}paintButtonCore(d);return;}
 DRAWITEM copy=*d;copy.dc=mem;copy.rect={0,0,w,h};paintButtonCore(&copy);
 BitBlt(d->dc,d->rect.left,d->rect.top,w,h,mem,0,0,0xcc0020);SelectObject(mem,old);
 if(useCache)buttonBuffer.busy=false;else{DeleteObject(bmp);DeleteDC(mem);}
}

// Native combo popup selection changes can call WM_DRAWITEM directly from
// mouse tracking, outside WM_PAINT. Never expose the intermediate clear,
// rounded selection background, or text to the screen in separate GDI calls.
// Keep normal list boxes and native combo input/navigation unchanged.
static void paintRow(DRAWITEM*d){
 if(d->type!=3){paintRowCore(d);return;}
 // ODS_COMBOBOXEDIT denotes the selection field, not a popup row.
 // USER32 can clear this field synchronously during focus transfer, just
 // before starting the next combo's drop animation. A deferred invalidation
 // leaves it visibly blank until that animation returns to the message loop.
 // Restore the same buffered custom face immediately using the supplied DC.
 // Its native clip region is respected, and the padding matches WM_PAINT.
 if((d->state&0x1000u)&&flatCombo(d->id)){
  if(d->hwnd&&d->dc)flatFace(d->hwnd,d->dc,d->id);
  return;
 }
 // ODA_FOCUS alone changes no pixels in our combo style.
 if(d->action==4)return;
 const int w=d->rect.right-d->rect.left,h=d->rect.bottom-d->rect.top;
 if(w<=0||h<=0)return;
 bool cached=!buttonBuffer.busy&&(up)w*h<=131072&&buttonBuffer.ensure(d->dc,w,h);
 HDC mem=cached?buttonBuffer.dc:CreateCompatibleDC(d->dc);
 H bmp=cached?buttonBuffer.bmp:mem?CreateCompatibleBitmap(d->dc,w,h):nullptr;
 if(!bmp){if(mem&&!cached)DeleteDC(mem);paintRowCore(d);return;}
 if(cached)buttonBuffer.busy=true;
 H old=SelectObject(mem,bmp);
 if(!old||old==PTR(-1)){
  if(cached){buttonBuffer.busy=false;buttonBuffer.clear();}
  else{DeleteObject(bmp);DeleteDC(mem);}
  paintRowCore(d);return;
 }
 DRAWITEM copy=*d;copy.dc=mem;copy.rect={0,0,w,h};
 paintRowCore(&copy);
 BitBlt(d->dc,d->rect.left,d->rect.top,w,h,mem,0,0,0xcc0020);
 SelectObject(mem,old);
 if(cached)buttonBuffer.busy=false;else{DeleteObject(bmp);DeleteDC(mem);}
}


// A CBS_DROPDOWNLIST has a separate native ComboLBox popup. Buffering
// WM_DRAWITEM alone does not cover the background or USER32's WM_PRINT
// animation snapshots. Own those paint paths before the list is ever shown,
// while leaving mouse tracking, selection, scrolling, accessibility and
// keyboard navigation entirely to the native window procedure.
struct MGComboInfo{DWORD cbSize;RECT item,button;DWORD state;HWND combo,edit,list;};
static_assert(sizeof(MGComboInfo)==64);
API B GetComboBoxInfo(HWND,MGComboInfo*);
API H GetPropW(HWND,const W*);
API B SetPropW(HWND,const W*,H);
API H RemovePropW(HWND,const W*);
API int SaveDC(HDC);
API B RestoreDC(HDC,int);
API B OffsetViewportOrgEx(HDC,int,int,POINT*);
API int IntersectClipRect(HDC,int,int,int,int);
API int ExcludeClipRect(HDC,int,int,int,int);
API HDC GetWindowDC(HWND);
struct MGScrollBarInfo{DWORD cbSize;RECT rect;int lineButton,thumbTop,thumbBottom,reserved;DWORD state[6];};
static_assert(sizeof(MGScrollBarInfo)==60);
API B GetScrollBarInfo(HWND,long,MGScrollBarInfo*);
struct MGComboWindowPos{HWND hwnd,after;int x,y,cx,cy;UINT flags;};
static_assert(sizeof(MGComboWindowPos)==40);
static constexpr W comboProcProp[]=L"miniGestures.ComboPopupOriginal";
static constexpr W comboOwnerProp[]=L"miniGestures.ComboPopupOwner";
static constexpr W comboThemeProp[]=L"miniGestures.ComboPopupTheme";

static void comboPopupClient(HWND list,HDC dest){
 if(!dest||!fieldBrush)return;
 RECT area{};if(!GetClientRect(list,&area)||area.right<=0||area.bottom<=0)return;
 const int w=area.right,h=area.bottom;
 bool cached=!buttonBuffer.busy&&(up)w*h<=131072&&buttonBuffer.ensure(dest,w,h);
 HDC mem=cached?buttonBuffer.dc:CreateCompatibleDC(dest);
 H bmp=cached?buttonBuffer.bmp:mem?CreateCompatibleBitmap(dest,w,h):nullptr;
 H old=bmp?SelectObject(mem,bmp):nullptr;
 bool buffered=old&&old!=PTR(-1);
 if(cached&&buffered)buttonBuffer.busy=true;
 HDC dc=buffered?mem:dest;
 FillRect(dc,&area,fieldBrush);
 HWND owner=(HWND)GetPropW(list,comboOwnerProp);
 HFONT face=(HFONT)SendMessageW(list,0x31,0,0);
 if(!face&&owner)face=(HFONT)SendMessageW(owner,0x31,0,0);
 if(!face)face=font;
 int scale=(int)GetDpiForWindow(list);if(scale<=0)scale=96;
 auto unit=[&](int v){return (v*scale+48)/96;};
 int top=(int)SendMessageW(list,0x18e,0,0); // LB_GETTOPINDEX
 int count=(int)SendMessageW(list,0x18b,0,0); // LB_GETCOUNT
 int current=(int)SendMessageW(list,0x188,0,0); // LB_GETCURSEL
 if(top>=0&&count>0)for(int i=top;i<count;i++){
  RECT r{};if(SendMessageW(list,0x198,i,(LP)&r)<0)break; // LB_GETITEMRECT
  if(r.top>=h)break;if(r.bottom<=0)continue;
  if(i==current){RECT q{r.left+unit(1),r.top+unit(1),r.right-unit(1),r.bottom-unit(1)};
   DWORD bg=rowSelectionFill(highContrastTheme);rounded(dc,q,unit(4),bg,bg,false,fieldFill);}
  W value[1024]{};LR n=SendMessageW(list,0x18a,i,0);
  if(n>=0&&n<1024)SendMessageW(list,0x189,i,(LP)value);
  RECT t=r;t.left+=unit(7);t.right-=unit(6);
  if(t.right>t.left)textFlat(dc,value,t,i==current?rowSelectionText(highContrastTheme):ink,0x20|4|0x8000,face);
 }
 if(buffered){BitBlt(dest,0,0,w,h,mem,0,0,0xcc0020);SelectObject(mem,old);}
 if(cached){buttonBuffer.busy=false;if(!buffered)buttonBuffer.clear();}
 else{if(bmp)DeleteObject(bmp);if(mem)DeleteDC(mem);}
}
// The native scrollbar is nonclient area, outside WM_PAINT and the
// owner-drawn rows. Paint the entire frame/scrollbar from our palette,
// using USER32's actual scrollbar geometry so native hit-testing agrees.
static void comboPopupFrame(HWND h,HDC dc){
 if(!dc||!fieldBrush)return;
 RECT outer{},client{};POINT origin{};
 if(!GetWindowRect(h,&outer)||!GetClientRect(h,&client)||!ClientToScreen(h,&origin))return;
 int w=outer.right-outer.left,ht=outer.bottom-outer.top;
 if(w<=0||ht<=0)return;
 int x=origin.x-outer.left,y=origin.y-outer.top;
 int saved=SaveDC(dc);if(!saved)return;
 ExcludeClipRect(dc,x,y,x+client.right,y+client.bottom);
 fillSolid(dc,{0,0,w,ht},highContrastTheme?frame:accent);
 MGScrollBarInfo sb{};sb.cbSize=sizeof(sb);
 // OBJID_VSCROLL = -5; the RECT is in screen coordinates.
 if(GetScrollBarInfo(h,-5,&sb)&&!(sb.state[0]&(0x8000u|0x10000u))){
  RECT r{sb.rect.left-outer.left,sb.rect.top-outer.top,sb.rect.right-outer.left,sb.rect.bottom-outer.top};
  if(r.right>r.left&&r.bottom>r.top){
   DWORD track=highContrastTheme?fieldFill:mgtheme::mix(fieldFill,ink,5);
   fillSolid(dc,r,track);
   int scale=(int)GetDpiForWindow(h);if(scale<=0)scale=96;
   int inset=maxn(2,(3*scale+48)/96),line=maxn(1,sb.lineButton);
   int mid=r.left+(r.right-r.left)/2;
   int top=clamp(r.top+sb.thumbTop,r.top,r.bottom),bottom=clamp(r.top+sb.thumbBottom,top,r.bottom);
   if(bottom>top&&r.right-r.left>2*inset){
    RECT thumb{r.left+inset,top,r.right-inset,bottom};
    DWORD color=highContrastTheme?ink:mgtheme::mix(track,ink,(sb.state[3]&8)?65:45);
    rounded(dc,thumb,maxn(1,inset),color,color,false,track);
   }
   int a=maxn(1,(3*scale+48)/96),upY=r.top+line/2,downY=r.bottom-line/2;
   if(line>=2*a&&r.bottom-r.top>=2*line){
    POINT u[]={{mid-a,upY+1},{mid,upY-a},{mid+a,upY+1}};
    POINT d[]={{mid-a,downY-1},{mid,downY+a},{mid+a,downY-1}};
    flatLine(dc,u,3,muted,1);flatLine(dc,d,3,muted,1);
   }
  }
 }
 RestoreDC(dc,saved);
}
static void refreshComboFrame(HWND h){
 if(!IsWindowVisible(h))return;
 HDC dc=GetWindowDC(h);if(dc){comboPopupFrame(h,dc);ReleaseDC(h,dc);}
}
static void applyComboNativeTheme(HWND h){
 using Fn=long(CB*)(HWND,const W*,const W*);
 static H ux=LoadLibraryExW(L"uxtheme.dll",nullptr,0x800);
 static auto set=ux?(Fn)GetProcAddress(ux,"SetWindowTheme"):nullptr;
 // SetWindowTheme sends WM_THEMECHANGED and may force a paint. Do not
 // reapply it on every keypress or click when the theme is unchanged.
 bool dark=darkTheme&&!highContrastTheme;H marker=PTR(dark?2:1);
 if(set&&GetPropW(h,comboThemeProp)!=marker){
  if(set(h,dark?L"DarkMode_Explorer":nullptr,nullptr)>=0)SetPropW(h,comboThemeProp,marker);
 }
}
static LR CB comboPopupProc(HWND h,UINT m,WP w,LP l){
 PROC original=(PROC)GetPropW(h,comboProcProp);
 if(!original)return DefWindowProcW(h,m,w,l);
 if(m==0x82){ // WM_NCDESTROY: no stale HWND registrations.
  RemovePropW(h,comboThemeProp);RemovePropW(h,comboOwnerProp);RemovePropW(h,comboProcProp);
  SetWindowLongPtrW(h,-4,(ip)original);
  return CallWindowProcW(original,h,m,w,l);
 }
 if(fieldBrush){
  // Never preserve old pixels when the native popup grows or is shown.
  if(m==0x46&&l){auto* p=(MGComboWindowPos*)l;if((p->flags&0x40)||!(p->flags&1))p->flags|=0x100;}
  if(m==0x85){HDC dc=GetWindowDC(h);if(dc){comboPopupFrame(h,dc);ReleaseDC(h,dc);return 0;}}
  if(m==0x86){refreshComboFrame(h);return 1;}
  if(m==0x14){ // WM_ERASEBKGND: never use the native white class brush.
   // Even an explicit erase paints a complete buffered client, rather
   // than exposing an intermediate blank surface in either theme.
   if(w)comboPopupClient(h,(HDC)w);return 1;
  }
  if(m==0xf){ // WM_PAINT
   if(w){comboPopupClient(h,(HDC)w);return 0;}
   PAINT p{};HDC dc=BeginPaint(h,&p);comboPopupClient(h,dc);EndPaint(h,&p);return 0;
  }
  if(m==0x318){comboPopupClient(h,(HDC)w);return 0;} // WM_PRINTCLIENT
  if(m==0x317&&w&&(l&(2|4|8))){ // WM_PRINT: complete themed animation image
   HDC dc=(HDC)w;RECT outer{},client{};POINT origin{};
   if(GetWindowRect(h,&outer)&&GetClientRect(h,&client)&&ClientToScreen(h,&origin)){
    int saved=SaveDC(dc);
    if(saved){
     if(l&2)comboPopupFrame(h,dc); // no native white nonclient snapshot
     if(l&(4|8)){
      int x=origin.x-outer.left,y=origin.y-outer.top;
      OffsetViewportOrgEx(dc,x,y,nullptr);
      IntersectClipRect(dc,0,0,client.right,client.bottom);
      comboPopupClient(h,dc);
     }
     RestoreDC(dc,saved);return 0;
    }
   }
  }
  if(m==0x134){SetTextColor((HDC)w,ink);SetBkColor((HDC)w,fieldFill);return (LR)fieldBrush;}
 }
 LR result=CallWindowProcW(original,h,m,w,l);
 // Native scrollbar input can paint synchronously during tracking; restore
 // our frame immediately after scroll, selection or nonclient input.
 if(m==0x47||m==0x18||m==0x115||m==0x20a||m==0xa0||m==0xa1||m==0xa2||
    m==0x100||m==0x188||m==0x197||m==0x31a)refreshComboFrame(h);
 return result;
}
static void installComboPopup(HWND combo){
 if(!combo)return;MGComboInfo info{};info.cbSize=sizeof(info);
 if(!GetComboBoxInfo(combo,&info)||!info.list)return;
 if(GetPropW(info.list,comboProcProp)){applyComboNativeTheme(info.list);return;}
 PROC old=(PROC)GetWindowLongPtrW(info.list,-4);if(!old)return;
 if(!SetPropW(info.list,comboOwnerProp,combo))return;
 if(!SetPropW(info.list,comboProcProp,(H)old)){RemovePropW(info.list,comboOwnerProp);return;}
 if(!SetWindowLongPtrW(info.list,-4,(ip)comboPopupProc)){
  RemovePropW(info.list,comboProcProp);RemovePropW(info.list,comboOwnerProp);return;
 }
 applyComboNativeTheme(info.list);
}
