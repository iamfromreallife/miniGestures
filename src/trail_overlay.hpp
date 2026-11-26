#pragma once
// Submit complete premultiplied frames atomically, without visible resize/erase/paint.
static HDC trailDC;static H trailBitmap,trailOldBitmap;static u32* trailPixels;static int trailW,trailH;static bool trailVisible;static TrailBounds trailArea{},lastNeeded{};static int renderedN,renderedColor,renderedWidth;static TrailStamp trailStamp;
static void releaseTrailSurface(){if(trailDC&&trailOldBitmap&&trailOldBitmap!=PTR(-1))SelectObject(trailDC,trailOldBitmap);if(trailBitmap)DeleteObject(trailBitmap);if(trailDC)DeleteDC(trailDC);trailDC=nullptr;trailBitmap=trailOldBitmap=nullptr;trailPixels=nullptr;trailW=trailH=0;}
static void clearTrailOverlay(){if(overlaywnd&&trailVisible)ShowWindow(overlaywnd,0);trailVisible=false;trailArea={};lastNeeded={};renderedN=0;// Keep only a small reusable DIB after capture; large frames rarely match the next stroke.
if((u64)trailW*trailH>256*1024)releaseTrailSurface();}
static void destroyTrailOverlay(){clearTrailOverlay();if(overlaywnd){DestroyWindow(overlaywnd);overlaywnd=nullptr;}releaseTrailSurface();}
static bool ensureTrailSurface(TrailBounds next,bool& preserve){
 int w=next.w,h=next.h;if(trailDC&&trailW==w&&trailH==h)return true;
 // Preserve only a bounded previous surface. Fall back to the original full
 // rerasterization if the old surface is large or cannot be copied.
 preserve=preserve&&trailPixels&&trailArea.w==trailW&&trailArea.h==trailH&&
  (up)trailW*trailH<=1024*1024&&trailContains(next,trailArea);
 if(!preserve)releaseTrailSurface();
 if(w<=0||h<=0||(u64)w*h>64000000){releaseTrailSurface();return false;}
 using DIB=H(CB*)(HDC,const void*,UINT,void**,H,DWORD);
 static DIB create=(DIB)GetProcAddress(GetModuleHandleW(L"gdi32.dll"),"CreateDIBSection");
 if(!create){releaseTrailSurface();return false;}
 struct Info{DWORD size;int w,h;u16 planes,bits;DWORD compression,image;int xp,yp;DWORD used,important;};
 Info info{40,w,-h,1,32,0,0,0,0,0,0};HDC dc=CreateCompatibleDC(nullptr);u32* pixels=nullptr;
 H bmp=dc?create(dc,&info,0,(void**)&pixels,nullptr,0):nullptr;H old=bmp?SelectObject(dc,bmp):nullptr;
 if(!dc||!bmp||!pixels||!old||old==PTR(-1)){if(bmp)DeleteObject(bmp);if(dc)DeleteDC(dc);releaseTrailSurface();return false;}
 if(preserve){memset(pixels,0,(up)w*h*4);preserve=copyTrailPixels(pixels,next,trailPixels,trailArea);}
 releaseTrailSurface();trailDC=dc;trailBitmap=bmp;trailOldBitmap=old;trailPixels=pixels;trailW=w;trailH=h;return true;
}
static void renderTrailOverlay(const P* points,int n,int color,int width){width=clamp(width,1,24);// Only append-only frames with unchanged width can reuse the exact
 // previous bounds. Compaction, new strokes and width changes rescan.
 TrailBounds needed;
 if(lastNeeded.w&&n>renderedN&&renderedN>=2&&renderedWidth==width){
  auto extra=trailBounds(points+renderedN,n-renderedN,width);
  int right=maxn(lastNeeded.x+lastNeeded.w,extra.x+extra.w),bottom=maxn(lastNeeded.y+lastNeeded.h,extra.y+extra.h);
  int x=minn(lastNeeded.x,extra.x),y=minn(lastNeeded.y,extra.y);needed={x,y,right-x,bottom-y};
 }else needed=trailBounds(points,n,width);
 bool incremental=trailArea.w&&n>renderedN&&renderedN>=2&&renderedColor==color&&renderedWidth==width;auto b=trailArea.w&&trailContains(trailArea,needed)?trailArea:trailGrow(needed,trailArea);if(!ensureTrailSurface(b,incremental)){trailArea={};lastNeeded={};renderedN=0;return;}if(!incremental)memset(trailPixels,0,(up)b.w*b.h*4);int first=incremental?renderedN-1:0;rasterTrail(trailPixels,b,points+first,n-first,width,color,&trailStamp);trailArea=b;lastNeeded=needed;renderedN=n;renderedColor=color;renderedWidth=width;if(!overlaywnd)overlaywnd=CreateWindowExW(0x80000|0x20|0x80|0x08000000|8,L"miniGesturesTrail",L"",0x80000000,0,0,1,1,nullptr,nullptr,inst,nullptr);if(!overlaywnd)return;struct Blend{u8 op,flags,alpha,format;};using Update=B(CB*)(HWND,HDC,const POINT*,const POINT*,HDC,const POINT*,DWORD,const Blend*,DWORD);static Update update=nullptr;if(!update)update=(Update)GetProcAddress(GetModuleHandleW(L"user32.dll"),"UpdateLayeredWindow");if(!update)return;POINT pos{b.x,b.y},size{b.w,b.h},zero{};Blend blend{0,0,255,1};if(update(overlaywnd,nullptr,&pos,&size,trailDC,&zero,0,&blend,2)&&!trailVisible){ShowWindow(overlaywnd,4);trailVisible=true;}}
