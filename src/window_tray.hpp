#pragma once
// Geometry in physical screen pixels, including negative coordinates on secondary monitors.
struct SavedWindowBox { int x,y,w,h; };
static inline int geomMin(int a,int b){return a<b?a:b;}
static inline int geomMax(int a,int b){return a>b?a:b;}
static inline SavedWindowBox restoreBox(SavedWindowBox saved,SavedWindowBox work,int minWidth,int minHeight,int defaultWidth,int defaultHeight){
 bool have=saved.w>=300&&saved.h>=300&&saved.w<=100000&&saved.h<=100000;
 int w=geomMin(work.w,geomMax(minWidth,have?saved.w:defaultWidth));
 int h=geomMin(work.h,geomMax(minHeight,have?saved.h:defaultHeight));
 int x=have?saved.x:work.x+(work.w-w)/2;
 int y=have?saved.y:work.y+(work.h-h)/2;
 // Leave the last position intact when it remains fully usable; otherwise recover on the nearest monitor.
 x=geomMax(work.x,geomMin(x,work.x+work.w-w));
 y=geomMax(work.y,geomMin(y,work.y+work.h-h));
 return {x,y,w,h};
}
enum TrayCommand {TrayNone,TrayToggleEditor,TrayToggleEngine,TrayContext};
// Adapt NightLight's edge-based filtering: toggle on each physical press,
// including the second press of a double-click. No debounce/waiting period.
struct TrayClickFilter{
 bool pressPending=false,recentRelease=false;
 unsigned releaseTime=0;
 constexpr TrayCommand command(unsigned msg,unsigned timestamp){
  switch(msg){
   case 0x201:case 0x203:pressPending=true;recentRelease=false;return TrayToggleEditor;
   case 0x202:{
    releaseTime=timestamp;recentRelease=true;
    if(pressPending){pressPending=false;return TrayNone;}
    return TrayToggleEditor;
   }
   case 0x400:
    if(pressPending)return TrayNone;
    if(recentRelease&&timestamp-releaseTime<=150u){recentRelease=false;return TrayNone;}
    recentRelease=false;return TrayToggleEditor;
   case 0x401:return TrayToggleEditor;
   case 0x208:return TrayToggleEngine;
   case 0x205:return TrayContext;
   default:return TrayNone;
  }
 }
};
constexpr bool trayClickRegression(){
 TrayClickFilter f;
 if(f.command(0x201,0)!=TrayToggleEditor||f.command(0x202,10)!=TrayNone)return false;
 if(f.command(0x400,11)!=TrayNone||f.command(0x203,20)!=TrayToggleEditor||f.command(0x202,21)!=TrayNone)return false;
 if(f.command(0x201,22)!=TrayToggleEditor||f.command(0x202,23)!=TrayNone)return false;
 if(f.command(0x203,24)!=TrayToggleEditor||f.command(0x400,25)!=TrayNone||f.command(0x202,26)!=TrayNone)return false;
 TrayClickFilter up,select;
 return up.command(0x202,0)==TrayToggleEditor&&up.command(0x202,1)==TrayToggleEditor&&
  select.command(0x400,0)==TrayToggleEditor&&select.command(0x400,1)==TrayToggleEditor&&
  f.command(0x208,27)==TrayToggleEngine&&f.command(0x205,28)==TrayContext;
}
static_assert(trayClickRegression(),"Every physical tray press must toggle");
