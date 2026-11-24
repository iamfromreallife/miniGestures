#pragma once
#include "center_geometry.hpp"
static inline bool centerWindow(HWND h,bool ordered){
 // Remember the destination monitor before restoring a maximized window.
 H monitor=MonitorFromWindow(h,2);MONITOR mi{sizeof(mi)};
 if(!monitor||!GetMonitorInfoW(monitor,&mi))return false;
 if(IsZoomed(h)||IsIconic(h))ShowWindow(h,4);
 RECT outer{},visible{};if(!GetWindowRect(h,&outer))return false;
 visible=outer;
 // DWM's extended bounds exclude invisible resize borders, and both bounds
 // are physical screen coordinates in this per-monitor-v2-aware process.
 H dwm=LoadLibraryExW(L"dwmapi.dll",nullptr,0x800);
 if(dwm){using GetBounds=long(CB*)(HWND,DWORD,void*,DWORD);auto fn=(GetBounds)GetProcAddress(dwm,"DwmGetWindowAttribute");RECT r{};
  if(fn&&fn(h,9,&r,sizeof(r))>=0&&r.right>r.left&&r.bottom>r.top)visible=r;
  FreeLibrary(dwm);
 }
 int x,y;if(!centeredOrigin(mi.rcWork,outer,visible,x,y))return false;
 return SetWindowPos(h,nullptr,x,y,0,0,1|4|16|(ordered?0:0x4000))!=0;
}
