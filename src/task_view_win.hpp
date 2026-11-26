#pragma once
#include "task_view_target.hpp"
#ifdef _WIN32
// Fresh geometry/Z-order at each gesture trigger. No activation, compositor
// flush, sleep, or foreground-window dependency. Keep the native hit when it
// agrees with the topmost eligible window; preserve shell/menu protection.
API HWND GetShellWindow();
API HWND GetTopWindow(HWND);
API HWND GetWindow(HWND,UINT);
API HWND ChildWindowFromPointEx(HWND,POINT,UINT);
struct PointerWindowLookup {
 POINT point;HWND root;DWORD shellPid=0;
 using DwmGet=long(CB*)(HWND,DWORD,void*,DWORD);
 DwmGet dwm=nullptr;
 static bool containsPoint(const RECT& r,POINT p){return r.right>r.left&&r.bottom>r.top&&p.x>=r.left&&p.x<r.right&&p.y>=r.top&&p.y<r.bottom;}
 bool taskView(HWND h,const W* cls){
  if(!shellPid||!h||(!eq(cls,L"XamlExplorerHostIslandWindow")&&!eq(cls,L"MultitaskingViewFrame")&&!eq(cls,L"Windows.UI.Core.CoreWindow")))return false;
  DWORD pid=0;W title[128]{};
  return GetWindowThreadProcessId(h,&pid)&&pid==shellPid&&GetWindowTextW(h,title,128)&&mgtaskview::isTaskView(cls,title,eq);
 }
 static bool protectedSurface(const W* cls){return eq(cls,L"Shell_TrayWnd")||eq(cls,L"Shell_SecondaryTrayWnd")||eq(cls,L"#32768")||eq(cls,L"NotifyIconOverflowWindow")||eq(cls,L"TopLevelWindowForOverflowXamlIsland");}
 bool ready(){
  H lib=GetModuleHandleW(L"dwmapi.dll");dwm=lib?(DwmGet)GetProcAddress(lib,"DwmGetWindowAttribute"):nullptr;
  if(!dwm)return false;
  HWND shell=GetShellWindow();if(shell)GetWindowThreadProcessId(shell,&shellPid);
  // An actual hit on taskbar/menu stays native, even with overlapping bounds.
  W cls[128]{};return !root||!GetClassNameW(root,cls,128)||!protectedSurface(cls);
 }
 HWND first(){return GetTopWindow(nullptr);}
 HWND next(HWND h){return GetWindow(h,2);}
 mgtaskview::Candidate classify(HWND h){
  using namespace mgtaskview;
  if(!IsWindow(h)||!IsWindowVisible(h)||IsIconic(h))return Skip;
  RECT bounds{};if(!GetWindowRect(h,&bounds)||!containsPoint(bounds,point))return Skip;
  DWORD cloaked=0;if(dwm(h,14,&cloaked,sizeof(cloaked))<0||cloaked)return Skip;
  // Native hits may be on legitimate resize borders. Only alternate hits
  // need the visible frame check to reject invisible border overlap.
  RECT frame{};if(h!=root&&dwm(h,9,&frame,sizeof(frame))>=0&&frame.right>frame.left&&frame.bottom>frame.top&&!containsPoint(frame,point))return Skip;
  W cls[128]{};if(!GetClassNameW(h,cls,128))return Stop;
  if(taskView(h,cls))return Skip;
  if(eq(cls,L"Progman")||eq(cls,L"WorkerW")||protectedSurface(cls))return Stop;
  // Preserve ordinary native child hit testing for all actual application hits.
  if(h==root)return UseOriginal;
  DWORD pid=0;if(!GetWindowThreadProcessId(h,&pid)||!pid)return Skip;
  if(shellPid&&pid==shellPid&&!eq(cls,L"CabinetWClass")&&!eq(cls,L"ExploreWClass"))return Stop;
  auto ex=(u32)GetWindowLongPtrW(h,-20);
  // A layered/transparent surface with a different native hit cannot safely
  // be inferred from its rectangular bounds (e.g. tooltips, gesture trails).
  if(ex&(0x20u|0x80000u))return Skip;
  if((ex&0x08000000u)||!IsWindowEnabled(h))return Stop;
  return Application;
 }
 HWND child(HWND h){
  HWND result=h;for(int i=0;i<16;++i){POINT p=point;if(!ScreenToClient(result,&p))break;HWND c=ChildWindowFromPointEx(result,p,7);if(!c||c==result||GetAncestor(c,2)!=h)break;result=c;}return result;
 }
};
static HWND gestureWindowAtHit(POINT point,HWND hit){HWND root=hit?GetAncestor(hit,2):nullptr;PointerWindowLookup lookup{point,root?root:hit};return mgtaskview::resolve(hit,lookup);}
static HWND gestureWindowAt(POINT point){return gestureWindowAtHit(point,WindowFromPoint(point));}
#else
static HWND gestureWindowAtHit(POINT,HWND hit){return hit;}
static HWND gestureWindowAt(POINT point){return WindowFromPoint(point);}
#endif
