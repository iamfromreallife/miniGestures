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
static inline TrayCommand trayCommand(unsigned msg,bool& skipDoubleRelease){
 if(msg==0x203){skipDoubleRelease=true;return TrayNone;} // first click already toggled
 if(msg==0x201){skipDoubleRelease=false;return TrayNone;}
 if(msg==0x202){if(skipDoubleRelease){skipDoubleRelease=false;return TrayNone;}return TrayToggleEditor;}
 if(msg==0x208)return TrayToggleEngine;
 if(msg==0x205)return TrayContext;
 return TrayNone;
}
