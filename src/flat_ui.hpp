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
static bool flatCombo(int id){return id==BUTTONBOX||id==MODBOX||id==TRIGGERBOX||id==EXECBOX||id==MODEBOX||id==LANGBOX||id==THEMEBOX||id==UPDATEBOX||id==SHAPESELECT;}
static bool flatCheck(int id){return id==ENABLEBOX||id==INHERITBOX||id==OVERBOX||id==CLOSEBOX||id==STARTBOX||id==CONFIRMBOX||id==INDICATORBOX||id==STARTWINBOX||id==ADMINBOX||id==TOPBOX||id==TASKBARBOX||id==FULLSCREENBOX||id==SELECTOR_ENABLED||id==SELECTOR_INHERIT;}
static bool flatFrame(int id){return flatEdit(id)||flatList(id)||id==TRAIN;}
static void fillSolid(HDC dc,RECT r,DWORD c){if(r.right<=r.left||r.bottom<=r.top)return;H b=CreateSolidBrush(c);if(b){FillRect(dc,&r,b);DeleteObject(b);}}
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
 CornerBitmapInfo bi{40,rad,-rad,1,32,0,(DWORD)(rad*rad*4),0,0,0,0};
 // UI-thread-only scratch storage; avoids allocations and large stack frames.
 static u32 pixels[rounded_aa::max_radius*rounded_aa::max_radius];
 for(int corner=0;corner<4;++corner){
  for(int y=0;y<rad;++y)for(int x=0;x<rad;++x){
   int sy=(corner&2)?rad-1-y:y,sx=(corner&1)?rad-1-x:x,k=sy*rad+sx;
   pixels[y*rad+x]=rounded_aa::pixel(background,border,fill,mask.outer[k],mask.inner[k]);
  }
  StretchDIBits(dc,(corner&1)?r.right-rad:r.left,(corner&2)?r.bottom-rad:r.top,rad,rad,0,0,rad,rad,pixels,&bi,0,0xcc0020);
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
static LR CB flatProc(HWND h,UINT m,WP w,LP l){int id=GetDlgCtrlID(h);if(id<0||id>=164||!flatOld[id])return DefWindowProcW(h,m,w,l);PROC orig=flatOld[id];bool face=flatCombo(id)||flatCheck(id);
 if(m==0x200&&!flatHover[id]){flatHover[id]=true;TrackMouse t{sizeof(t),2,h,0};TrackMouseEvent(&t);InvalidateRect(h,nullptr,0);}else if(m==0x2a3){flatHover[id]=false;InvalidateRect(h,nullptr,0);}else if(m==0x201){flatDown[id]=true;InvalidateRect(h,nullptr,0);}else if(m==0x202||m==0x215){flatDown[id]=false;InvalidateRect(h,nullptr,0);}
 if(m==0x18&&!w){flatHover[id]=false;flatDown[id]=false;}if(face&&m==0x14)return 1;
 if(face&&m==0xf){PAINT p{};HDC dc=BeginPaint(h,&p);flatFace(h,dc,id);EndPaint(h,&p);return 0;}
 if(face&&m==0x318){flatFace(h,(HDC)w,id);return 0;}
 // Native processing still owns selections, notifications, focus, keyboard and popup.
 LR result=CallWindowProcW(orig,h,m,w,l);
 if(face&&(m==0x7||m==0x8||m==0xa||m==0xc||m==0x30||m==0x5||m==0x100||m==0x101||m==0x104||m==0x105||m==0xf1||m==0x14e||m==0x143||m==0x14b||m==0x14f||m==0x201||m==0x202||m==0x207||m==0x20a||m==0x215))InvalidateRect(h,nullptr,0);
 return result;
}
static void flatSubclass(int id){flatOld[id]=(PROC)SetWindowLongPtrW(ctrl(id),-4,(ip)flatProc);}
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
static void aaFillTri(u32* pix,int w,int h,float x1,float y1,float x2,float y2,float x3,float y3,DWORD rgb){int minx=maxn(0,(int)minn(int(x1),minn(int(x2),int(x3)))-1),maxx=minn(w-1,(int)maxn(int(x1),maxn(int(x2),int(x3)))+1),miny=maxn(0,(int)minn(int(y1),minn(int(y2),int(y3)))-1),maxy=minn(h-1,(int)maxn(int(y1),maxn(int(y2),int(y3)))+1);for(int y=miny;y<=maxy;y++)for(int x=minx;x<=maxx;x++){float px=x+0.5f,py=y+0.5f;if(aaPointInTri(px,py,x1,y1,x2,y2,x3,y3))aaBlend(pix[y*w+x],255,rgb);}}
static void aaLine(u32* pix,int w,int h,float x1,float y1,float x2,float y2,float radius,DWORD rgb){float vx=x2-x1,vy=y2-y1,den=vx*vx+vy*vy;if(den<1e-4f)den=1;int minx=maxn(0,(int)(minn(int(x1),int(x2))-radius-1)),maxx=minn(w-1,(int)(maxn(int(x1),int(x2))+radius+1));int miny=maxn(0,(int)(minn(int(y1),int(y2))-radius-1)),maxy=minn(h-1,(int)(maxn(int(y1),int(y2))+radius+1));for(int y=miny;y<=maxy;y++)for(int x=minx;x<=maxx;x++){float px=x+0.5f,py=y+0.5f,t=((px-x1)*vx+(py-y1)*vy)/den;if(t<0)t=0;else if(t>1)t=1;float dx=px-(x1+t*vx),dy=py-(y1+t*vy),cover=radius+0.5f-aaLen(dx,dy);if(cover<=0)continue;aaBlend(pix[y*w+x],(u8)minn(255,maxn(0,(int)(cover*255))),rgb);}}
static void aaRect(u32* pix,int w,int h,int l,int t,int r,int b,DWORD rgb){l=maxn(0,l);t=maxn(0,t);r=minn(w,r);b=minn(h,b);for(int y=t;y<b;y++)for(int x=l;x<r;x++)aaBlend(pix[y*w+x],255,rgb);}
static void aaDownsampleBlit(HDC dc,int left,int top,int w,int h,u32* hi,int scale){auto* lo=(u32*)allocRaw((up)w*h*4);if(!lo)return;for(int y=0;y<h;y++)for(int x=0;x<w;x++){u32 ar=0,ag=0,ab=0;for(int yy=0;yy<scale;yy++)for(int xx=0;xx<scale;xx++){u32 p=hi[(y*scale+yy)*(w*scale)+(x*scale+xx)];ar+=p&255;ag+=(p>>8)&255;ab+=(p>>16)&255;}u32 n=scale*scale;lo[y*w+x]=mgpaint::dib(((ar+n/2)/n)|(((ag+n/2)/n)<<8)|(((ab+n/2)/n)<<16));}CornerBitmapInfo bi{40,w,-h,1,32,0,(DWORD)(w*h*4),0,0,0,0};StretchDIBits(dc,left,top,w,h,0,0,w,h,lo,&bi,0,0xcc0020);freep(lo);} 
static void aaPolylineToSurface(u32* pix,int w,int h,const POINT* pts,int count,float radius,DWORD rgb){for(int i=1;i<count;i++)aaLine(pix,w,h,(float)pts[i-1].x,(float)pts[i-1].y,(float)pts[i].x,(float)pts[i].y,radius,rgb);} 
static bool drawPolylineAA(HDC dc,RECT bounds,const POINT* pts,int count,const POINT* tip,int tipCount,DWORD fg,DWORD bg,int thickness){if(count<2)return false;const int scale=8;int w=maxn(1,bounds.right-bounds.left),h=maxn(1,bounds.bottom-bounds.top),sw=w*scale,sh=h*scale;auto* hi=(u32*)allocRaw((up)sw*sh*4);if(!hi)return false;for(int i=0;i<sw*sh;i++)hi[i]=bg;auto mapPoint=[&](const POINT& p){POINT q{};q.x=(p.x-bounds.left)*scale+scale/2;q.y=(p.y-bounds.top)*scale+scale/2;return q;};float radius=maxn(1,thickness*scale)*0.5f;POINT linePts[512];int n=count>512?512:count;for(int i=0;i<n;i++)linePts[i]=mapPoint(pts[i]);aaPolylineToSurface(hi,sw,sh,linePts,n,radius,fg);if(tip&&tipCount>=2){POINT tipPts[8];int m=tipCount>8?8:tipCount;for(int i=0;i<m;i++)tipPts[i]=mapPoint(tip[i]);aaPolylineToSurface(hi,sw,sh,tipPts,m,radius,fg);if(m>=3)aaFillTri(hi,sw,sh,(float)tipPts[0].x,(float)tipPts[0].y,(float)tipPts[1].x,(float)tipPts[1].y,(float)tipPts[2].x,(float)tipPts[2].y,fg);}aaDownsampleBlit(dc,bounds.left,bounds.top,w,h,hi,scale);freep(hi);return true;}
static void drawFolderArrowAA(HDC dc,int left,int top,int size,DWORD fg,DWORD bg,bool closed){const int scale=8;int sw=size*scale,sh=size*scale;auto* hi=(u32*)allocRaw((up)sw*sh*4);if(!hi)return;for(int i=0;i<sw*sh;i++)hi[i]=bg; if(closed)aaFillTri(hi,sw,sh,sw*0.28f,sh*0.20f,sw*0.72f,sh*0.50f,sw*0.28f,sh*0.80f,fg);else aaFillTri(hi,sw,sh,sw*0.18f,sh*0.32f,sw*0.50f,sh*0.68f,sw*0.82f,sh*0.32f,fg); aaDownsampleBlit(dc,left,top,size,size,hi,scale);freep(hi);}
#include "folder_icon.hpp"
static void drawFolderIconAA(HDC dc,RECT icon,DWORD bg){
 int w=icon.right-icon.left,h=icon.bottom-icon.top;if(w<=0||h<=0||w>256||h>256||!StretchDIBits)return;
 u32 pixels[256];u32* out=w*h<=256?pixels:(u32*)allocRaw((up)w*h*4);if(!out)return;
 for(int y=0;y<h;++y)for(int x=0;x<w;++x)out[y*w+x]=mgfolder::render(x,y,w,h,bg);
 CornerBitmapInfo bi{40,w,-h,1,32,0,(DWORD)(w*h*4),0,0,0,0};
 StretchDIBits(dc,icon.left,icon.top,w,h,0,0,w,h,out,&bi,0,0xcc0020);if(out!=pixels)freep(out);
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
static bool paintGesturePreview(HDC dc,const Gesture&g,RECT r,DWORD color,DWORD bg){
 if(g.count<=0||!StretchDIBits)return false;
 constexpr int ss=mgpaint::samples;int w=r.right-r.left,h=r.bottom-r.top;
 if(w<=0||h<=0||w>256||h>256)return false;
 int sw=w*ss,sh=h*ss,arrow=maxn(2,px(3))*ss,thickness=maxn(1,px(1))*ss;
 PreviewKey key{g.samples[0],w,h,arrow,thickness,color,bg};
 CornerBitmapInfo bi{40,w,-h,1,32,0,(DWORD)(w*h*4),0,0,0,0};
 if(const auto* pixels=previewCache.find(key))return StretchDIBits(dc,r.left,r.top,w,h,0,0,w,h,pixels,&bi,0,0xcc0020)>0;
 P points[N],head[3];
 if(!mgpreview::fit(g.samples[0],points,head,sw,sh,arrow,arrow+thickness))return false;
 auto* mask=(u8*)allocRaw((up)sw*sh);auto* pixels=(u32*)allocRaw((up)w*h*4);
 if(!mask||!pixels){freep(mask);freep(pixels);return false;}
 memset(mask,0,(up)sw*sh);
 for(int i=1;i<N;++i)mgpaint::segment(mask,sw,sh,points[i-1].x,points[i-1].y,points[i].x,points[i].y,thickness/2);
 for(int i=1;i<3;++i)mgpaint::segment(mask,sw,sh,head[i-1].x,head[i-1].y,head[i].x,head[i].y,thickness/2);
 for(int y=0;y<h;++y)for(int x=0;x<w;++x)pixels[y*w+x]=mgpaint::downsample(mask,sw,x,y,bg,color);
 int result=StretchDIBits(dc,r.left,r.top,w,h,0,0,w,h,pixels,&bi,0,0xcc0020);
 freep(mask);if(result<=0||!previewCache.remember(key,pixels))freep(pixels);return result>0;
}
static DWORD rowSelectionFill(bool hc){return hc?accent:mgtheme::mix(fieldFill,RGB(0,120,212),darkTheme?20:12);}
static DWORD rowSelectionText(bool hc){return hc?accentText:ink;}
static void paintRow(DRAWITEM*d){RECT r=d->rect;bool listbox=d->type==2;bool selected=(d->state&1)!=0;bool hc=highContrastTheme;DWORD rowBg=selected?rowSelectionFill(hc):fieldFill,rowFg=selected?rowSelectionText(hc):ink;FillRect(d->dc,&r,fieldBrush);if(d->item==(UINT)-1)return;W b[1024]{};UINT lenmsg=listbox?0x18a:0x149,txtmsg=listbox?0x189:0x148;LR n=SendMessageW(d->hwnd,lenmsg,d->item,0);if(n>=0&&n<1024)SendMessageW(d->hwnd,txtmsg,d->item,(LP)b);RECT q=r;q.left+=px(1);q.right-=px(1);q.top+=px(1);q.bottom-=px(1);if(selected)rounded(d->dc,q,px(4),rowBg,rowBg,false,fieldFill);RECT t=r;t.left+=px(7);t.right-=px(6);if(listbox&&(d->id==LIST||d->id==APPLIST)&&((int)d->data)<=-2){int c=-2-(int)d->data;int yy=(r.top+r.bottom)/2;DWORD fg=selected&&hc?accentText:muted;bool shut=c>=0&&c<cfg.nc&&cfg.categories[c].collapsed;int arrowSize=px(9);drawFolderArrowAA(d->dc,t.left,yy-arrowSize/2,arrowSize,fg,rowBg,shut);RECT icon{t.left+px(11),yy-px(6),t.left+px(11)+px(16),yy-px(6)+px(12)};drawFolderIconAA(d->dc,icon,rowBg);t.left+=px(30);}if(listbox&&d->id==LIST&&page==2&&!settings&&(int)d->data>=0&&d->data<(up)cfg.ne&&*cfg.e[static_cast<int>(d->data)].folder)t.left+=px(16);if(listbox&&d->id==ACTLIST&&d->data<(up)edit.steps&&elevationAction(edit.actions[d->data].type)){elevationIcon(d->dc,t.left,(r.top+r.bottom-px(14))/2,rowFg);t.left+=px(19);}if(listbox&&d->id==LIST&&gstview()&&d->data<(up)cfg.ng){const Gesture&g=cfg.g[static_cast<int>(d->data)];int column=px(26);if(t.right-t.left>column+px(12)){RECT preview{t.right-column,q.top,t.right,q.bottom};DWORD pfg=selected&&hc?accentText:g.enabled?accent:muted;DWORD pbg=rowBg;if(paintGesturePreview(d->dc,g,preview,pfg,pbg))t.right=preview.left-px(3);}}textFlat(d->dc,b,t,rowFg);}
static void paintButtonCore(DRAWITEM*d){RECT r=d->rect;bool tab=d->id>=TAB0&&d->id<=TAB2;bool sel=tab&&d->id==TAB0+page;bool primary=d->id==SAVEBTN||d->id==132;bool down=(d->state&1)||flatDown[d->id],focus=(d->state&0x10)!=0;bool enabled=!(d->state&4);DWORD bg=primary?(down?(legacyDarkTheme?RGB(0,87,160):mgtheme::mix(accent,accentText,14)):flatHover[d->id]?(legacyDarkTheme?RGB(0,105,190):mgtheme::mix(accent,accentText,7)):accent):tab?((sel||flatHover[d->id])?hoverFill:appFill):down?pressedFill:flatHover[d->id]?hoverFill:buttonFill;
 FillRect(d->dc,&r,white);if(tab){RECT t=r;t.bottom-=px(3);if(sel||flatHover[d->id])rounded(d->dc,t,px(4),bg,bg,false);if(sel){RECT u=r;u.top=u.bottom-px(2);rounded(d->dc,u,px(1),accent,accent,false);}}else rounded(d->dc,r,px(4),bg,primary?bg:focus?accent:frame);
 W b[256];GetWindowTextW(d->hwnd,b,256);RECT t=r;t.left+=px(tab?6:4);t.right-=px(tab?6:4);HFONT face=sel&&tabBold?tabBold:font;H old=SelectObject(d->dc,face);POINT extent{};GetTextExtentPoint32W(d->dc,b,len(b),&extent);SelectObject(d->dc,old);
 DWORD tc=primary?accentText:enabled?ink:muted;
 if(extent.x>t.right-t.left&&t.bottom-t.top>=px(30)){RECT measure=t;measure.bottom=measure.top;H o=SelectObject(d->dc,face);DrawTextW(d->dc,b,-1,&measure,0x400|0x10|0x800|1);SelectObject(d->dc,o);int hh=measure.bottom-measure.top;if(hh<=t.bottom-t.top-px(2)){t.top+=(t.bottom-t.top-hh)/2;textFlat(d->dc,b,t,tc,1|0x10,face);return;}}
 // Anchor tab text at the same left inset for both font weights.
 textFlat(d->dc,b,t,tc,(tab?0:1)|4|0x20|0x8000,face);
}
static void paintButton(DRAWITEM*d){int w=d->rect.right-d->rect.left,h=d->rect.bottom-d->rect.top;if(w<1||h<1)return;HDC mem=CreateCompatibleDC(d->dc);H bmp=mem?CreateCompatibleBitmap(d->dc,w,h):nullptr;if(!bmp){if(mem)DeleteDC(mem);paintButtonCore(d);return;}H old=SelectObject(mem,bmp);DRAWITEM copy=*d;copy.dc=mem;copy.rect={0,0,w,h};paintButtonCore(&copy);BitBlt(d->dc,d->rect.left,d->rect.top,w,h,mem,0,0,0xcc0020);SelectObject(mem,old);DeleteObject(bmp);DeleteDC(mem);
}
