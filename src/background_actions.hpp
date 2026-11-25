#pragma once
#include "capture_log.hpp"
// Pointer/window commands remain addressed to the captured HWND. Keyboard
// actions require real foreground input; posted modifier messages are not real keys.
static inline bool belongsToTarget(HWND h,const Target&t){DWORD pid=0;return h&&IsWindow(h)&&GetWindowThreadProcessId(h,&pid)&&pid==t.pid&&(h==t.hwnd||GetAncestor(h,2)==t.hwnd);}
static inline HWND pointerChild(const Target&t){return belongsToTarget(t.child,t)?t.child:t.hwnd;}
static inline bool targetIsForeground(const Target&t){HWND f=GetForegroundWindow();return f&&(f==t.hwnd||GetAncestor(f,2)==t.hwnd);}
// SW_MINIMIZE (6) transfers foreground activation to the next window.
// SW_SHOWMINNOACTIVE (7) is suitable only for an already background window.
static inline int minimizeShowCommand(const Target&t){return targetIsForeground(t)?6:7;}
static inline bool appCommand(const Target&t,int code){return valid(t)&&PostMessageW(t.hwnd,0x319,(WP)pointerChild(t),(LP)((u32)code<<16))!=0;}
static inline bool shellShortcut(int vk,int m){return (m==8&&(vk=='D'||vk=='E'||vk=='I'||vk==0x09))||(m==10&&(vk==0x25||vk==0x27||vk=='D'||vk==0x73))||(m==12&&vk=='S');}
static inline bool snapTarget(HWND h,bool right);
// Foreground ownership and keyboard focus are separate, particularly during
// minimize/restore and desktop transitions. Never wait on DWM animations.
struct TargetGuiInfo { DWORD size,flags; HWND active,focus,capture,menuOwner,moveSize,caret; RECT caretRect; };
API B GetGUIThreadInfo(DWORD,TargetGuiInfo*);
API B AttachThreadInput(DWORD,DWORD,B);
API LR SendMessageTimeoutW(HWND,UINT,WP,LP,UINT,UINT,WP*);
static inline bool actionCancelled(){return stopActionEvent&&WaitForSingleObject(stopActionEvent,0)==0;}
static inline bool targetWindowMember(HWND h,const Target&t){return h&&IsWindow(h)&&(h==t.hwnd||GetAncestor(h,2)==t.hwnd);}
static inline bool queryTargetGui(const Target&t,TargetGuiInfo&g){
 if(!valid(t)||IsIconic(t.hwnd)||!targetIsForeground(t))return false;
 g={};g.size=sizeof(g);
 if(GetGUIThreadInfo(0,&g)&&g.active==t.hwnd&&targetWindowMember(g.focus,t)&&targetIsForeground(t))return true;
 // During an activation transition, the foreground query can expose stale
 // handles. Query the actual target queue, never a previous foreground window.
 DWORD tid=GetWindowThreadProcessId(t.hwnd,nullptr);
 g={};g.size=sizeof(g);
 return tid&&GetGUIThreadInfo(tid,&g)&&g.active==t.hwnd&&targetWindowMember(g.focus,t)&&targetIsForeground(t);
}
static inline bool targetFocusReady(const Target&t){
 TargetGuiInfo g{};return queryTargetGui(t,g);
}
static inline bool activationBarrier(HWND h,UINT timeout){WP ignored=0;return h&&SendMessageTimeoutW(h,0,0,0,0x23,timeout,&ignored)!=0;}
static inline bool awaitTargetFocus(const Target&t,UINT budget,bool synchronize=false){
 if(!synchronize&&targetFocusReady(t))return true;
 if(!valid(t)||actionCancelled())return false;
 DWORD start=GetTickCount();
 activationBarrier(t.hwnd,budget);
 do {
  if(targetFocusReady(t))return true;
  if(!valid(t)||actionCancelled()||GetTickCount()-start>=budget)return false;
  // Only a pending/failed transition reaches this path. Do not sleep on a
  // successful foreground change or in any input-hook callback.
  if(stopActionEvent){if(WaitForSingleObject(stopActionEvent,2)==0)return false;}else Sleep(2);
 }while(true);
}
static inline bool requestTargetFocus(const Target&t,UINT budget){
 if(!valid(t)||!IsWindowEnabled(t.hwnd)||actionCancelled())return false;
 if(targetFocusReady(t))return true;
 // Even a FALSE return can race a foreground change. Always verify the state.
 B requested=SetForegroundWindow(t.hwnd);MG_LOG("SetForegroundWindow target result foreground",(up)t.hwnd,requested,(up)GetForegroundWindow());
 // Let the newly activated queue process its pending activation before input.
 // This is a message handshake, not an animation delay.
 return awaitTargetFocus(t,budget,true);
}
static inline bool unlockForegroundForGesture(){
 // Windows unlocks foreground activation on Alt. Only use a balanced pulse
 // after the ordinary activation was unsuccessful; never click the webpage,
 // move the mouse, or release a modifier held by the user.
 if(mods()!=0||actionCancelled())return false;
 INPUT a[2]{};for(int i=0;i<2;i++){a[i].type=1;a[i].ki.vk=0x12;a[i].ki.flags=i?2:0;a[i].ki.extra=MARK;}
 UINT sent=SendInput(2,a,sizeof(INPUT));MG_LOG("activation Alt pulse accepted",sent);
 if(sent==1)SendInput(1,a+1,sizeof(INPUT));
 return sent==2;
}
static inline bool recoverThroughInputQueue(const Target&t){
 // Last resort, not a hot-path operation. Joining input queues can block or
 // reset key state, so never join them from a hook or with held modifiers.
 if(!valid(t)||actionCancelled()||mods()!=0||!IsWindowEnabled(t.hwnd))return false;
 HWND fg=GetForegroundWindow();DWORD fgPid=0,fgTid=fg?GetWindowThreadProcessId(fg,&fgPid):0;
 DWORD targetPid=0,targetTid=GetWindowThreadProcessId(t.hwnd,&targetPid),self=GetCurrentThreadId();
 if(!fgTid||!targetTid||!self||targetPid!=t.pid)return false;
 // A worker in a parallel macro may not yet have a GUI queue.
 MSG message{};PeekMessageW(&message,nullptr,0,0,0);
 if(!activationBarrier(fg,25)||(fg!=t.hwnd&&!activationBarrier(t.hwnd,25)))return false;
 if(!valid(t)||GetForegroundWindow()!=fg||actionCancelled()||mods()!=0)return false;
 bool linked=fgTid!=self&&AttachThreadInput(self,fgTid,1);MG_LOG("attach foreground self target linked",self,fgTid,linked);
 if(fgTid!=self&&!linked)return false;
 if(valid(t)&&!actionCancelled()&&GetForegroundWindow()==fg){
  SetForegroundWindow(t.hwnd);
  // Repair only a missing/foreign focus in the already active target queue.
  // Never focus the hovered browser link or replace an existing valid child.
  if(fgTid==targetTid&&targetIsForeground(t)&&!targetFocusReady(t)){
   TargetGuiInfo g{};g.size=sizeof(g);
   if(GetGUIThreadInfo(0,&g)&&g.active==t.hwnd&&!(g.flags&0x1e)&&!targetWindowMember(g.focus,t))SetFocus(t.hwnd);
  }
 }
 if(linked)AttachThreadInput(self,fgTid,0);
 if(awaitTargetFocus(t,60,true))return true;
 if(!valid(t)||!targetIsForeground(t)||actionCancelled()||mods()!=0||!IsWindowEnabled(t.hwnd))return false;
 // The old foreground queue may differ from the restored target's queue.
 // Reattach directly only after the target really owns the foreground.
 bool targetLinked=targetTid!=self&&AttachThreadInput(self,targetTid,1);MG_LOG("attach target self target linked",self,targetTid,targetLinked);
 if(targetTid!=self&&!targetLinked)return false;
 TargetGuiInfo g{};g.size=sizeof(g);
 if(valid(t)&&targetIsForeground(t)&&!actionCancelled()&&GetGUIThreadInfo(targetTid,&g)&&g.active==t.hwnd&&!(g.flags&0x1e)&&!targetWindowMember(g.focus,t))SetFocus(t.hwnd);
 if(targetLinked)AttachThreadInput(self,targetTid,0);
 return awaitTargetFocus(t,60,true);
}
static inline bool ensureTargetForeground(const Target&t){
 if(!valid(t)||!IsWindowEnabled(t.hwnd)||actionCancelled())return false;
 if(targetFocusReady(t))return true;
 if(requestTargetFocus(t,24))return true;
 if(!valid(t)||actionCancelled())return false;
 if(!targetIsForeground(t)&&unlockForegroundForGesture()){
  if(requestTargetFocus(t,30))return true;
  // The injected Alt event may itself still be queued on the first request.
  if(requestTargetFocus(t,30))return true;
 }
 return recoverThroughInputQueue(t);
}
// Only cancel an abandoned capture in the target's own window tree, not a
// real drag, menu, resize loop or a different application's capture.
static inline bool mouseButtonsDown(){
 return ((GetAsyncKeyState(1)|GetAsyncKeyState(2)|GetAsyncKeyState(4)|GetAsyncKeyState(5)|GetAsyncKeyState(6))&0x8000)!=0;
}
#ifdef _WIN32
API DWORD GetEnvironmentVariableW(const W*,W*,DWORD);
API void OutputDebugStringW(const W*);
struct DiagnosticCursor {DWORD size,flags;H cursor;POINT point;};
API B GetCursorInfo(DiagnosticCursor*);
static inline void diagnosticState(const char* event,const Target&t,int vk=0,int m=0){
 if(!mgdiag::enabled())return;
 HWND fg=GetForegroundWindow();TargetGuiInfo g{};g.size=sizeof(g);B ok=GetGUIThreadInfo(0,&g);DWORD pid=0;DWORD tid=GetWindowThreadProcessId(t.hwnd,&pid);
 MG_LOG(event,(up)t.hwnd,(up)t.child,t.pid,tid,(up)fg,(up)g.active,(up)g.focus,(up)g.capture);
 DiagnosticCursor c{};c.size=sizeof(c);B cursorOk=GetCursorInfo(&c);int kind=0;if(cursorOk&&c.cursor){kind=c.cursor==LoadCursorW(nullptr,(const W*)PTR(32512))?1:c.cursor==LoadCursorW(nullptr,(const W*)PTR(32649))?2:c.cursor==LoadCursorW(nullptr,(const W*)PTR(32513))?3:4;}
 MG_LOG("state guiOK flags cursorKind mouseDown heldMods targetIconic vk mods",ok,g.flags,kind,mouseButtonsDown(),mods(),t.hwnd?IsIconic(t.hwnd):0,vk,m);
 MG_LOG("state menuOwner moveSize caret cursorHandle focusThread captureThread fgThread",(up)g.menuOwner,(up)g.moveSize,(up)g.caret,(up)c.cursor,GetWindowThreadProcessId(g.focus,nullptr),GetWindowThreadProcessId(g.capture,nullptr),GetWindowThreadProcessId(fg,nullptr));
 TargetGuiInfo local{};local.size=sizeof(local);B localOk=tid&&GetGUIThreadInfo(tid,&local);
 MG_LOG("target queue guiOK flags active focus capture menuOwner moveSize caret",localOk,local.flags,(up)local.active,(up)local.focus,(up)local.capture,(up)local.menuOwner,(up)local.moveSize,(up)local.caret);
 MG_LOG("window lifecycle foreground iconic visible target iconic visible focus visible",(up)fg,fg?IsIconic(fg):0,fg?IsWindowVisible(fg):0,(up)t.hwnd,t.hwnd?IsIconic(t.hwnd):0,t.hwnd?IsWindowVisible(t.hwnd):0,g.focus?IsWindowVisible(g.focus):0);
 const int keys[]={0xa2,0xa3,0xa0,0xa1,0xa4,0xa5,0x5b,0x5c};unsigned held=0;for(int i=0;i<8;i++)if(GetAsyncKeyState(keys[i])&0x8000)held|=1u<<i;
 MG_LOG("state leftRightModifiers mask LC RC LS RS LA RA LW RW (bits 0-7) cursorFlags",held,c.flags);

}
static inline void deliveryTrace(const W*stage,const Target&t,int vk=0,int m=0){
 if(mgdiag::enabled()){const char* label=eq(stage,L"before shortcut")?"before shortcut target child pid tid fg active focus capture":eq(stage,L"SendInput accepted")?"after accepted target child pid tid fg active focus capture":eq(stage,L"SendInput rejected")?"after rejected target child pid tid fg active focus capture":eq(stage,L"cancel stale capture")?"cancel stale capture target child pid tid fg active focus capture":"focus unavailable target child pid tid fg active focus capture";diagnosticState(label,t,vk,m);}

 W enabled[8]{};if(GetEnvironmentVariableW(L"MINIGESTURES_TRACE",enabled,8)!=1||enabled[0]!=L'1')return;
 TargetGuiInfo g{};g.size=sizeof(g);GetGUIThreadInfo(0,&g);
 W line[512]=L"miniGestures delivery: ";cat(line,stage,512);W n[24]{};
 auto field=[&](const W*name,int value){cat(line,name,512);number(n,value);cat(line,n,512);};
 field(L" targetPID=",(int)t.pid);field(L" foregroundMatch=",targetIsForeground(t));field(L" activeMatch=",g.active==t.hwnd);field(L" focusMatch=",targetWindowMember(g.focus,t));field(L" capture=",g.capture!=nullptr);field(L" captureMatch=",targetWindowMember(g.capture,t));field(L" flags=",g.flags);field(L" mouseDown=",mouseButtonsDown());field(L" vk=",vk);field(L" mods=",m);field(L" heldMods=",mods());cat(line,L"\n",512);OutputDebugStringW(line);
}
#else
static inline void deliveryTrace(const W*,const Target&,int=0,int=0){}
#endif
static inline bool prepareTargetInput(const Target&t){
 if(!ensureTargetForeground(t))return false;
 TargetGuiInfo g{};if(!queryTargetGui(t,g))return false;
 if(g.capture&&targetWindowMember(g.capture,t)&&!(g.flags&0x1e)&&!mouseButtonsDown()){
  // WM_CANCELMODE is a native cancellation notification, not a click.
  // Recheck ownership and state before sending it; no synthetic mouse input.
  TargetGuiInfo check{};if(queryTargetGui(t,check)&&check.capture==g.capture&&!(check.flags&0x1e)&&!mouseButtonsDown()){
   WP ignored=0;deliveryTrace(L"cancel stale capture",t);
   LR cancelled=SendMessageTimeoutW(g.capture,0x1f,0,0,0x23,25,&ignored);MG_LOG("cancel capture result",cancelled);
   if(!targetFocusReady(t)&&!ensureTargetForeground(t))return false;
  }
 }
 return targetFocusReady(t)&&!actionCancelled();
}
static inline bool targetKey(const Target&t,int vk,int m){
 if(shellShortcut(vk,m))return sendkey(vk,m);
 if(!valid(t))return false;
 if(m==8&&(vk==0x25||vk==0x27))return snapTarget(t.hwnd,vk==0x27);
 // Alt+F4 has an addressed equivalent and needs no foreground activation.
 if(m==1&&vk==0x73&&!targetIsForeground(t))return closeTargetWindow(t);
 deliveryTrace(L"before shortcut",t,vk,m);
 if(prepareTargetInput(t)){bool ok=sendkey(vk,m);deliveryTrace(ok?L"SendInput accepted":L"SendInput rejected",t,vk,m);return ok;}
 deliveryTrace(L"focus unavailable",t,vk,m);
 // Native semantic fallbacks only: never report a posted Ctrl+key chord as
 // successful input when Windows has refused activation.
 if(!valid(t))return false;
 if(m==1&&vk==0x25)return appCommand(t,1);
 if(m==1&&vk==0x27)return appCommand(t,2);
 if(m==0&&vk==0x74)return appCommand(t,3);
 return false;
}
static inline bool targetText(const Target&t,const W*text){
 if(!prepareTargetInput(t))return false;
 for(int i=0;text[i];i++){
  if(stopActionEvent&&WaitForSingleObject(stopActionEvent,0)==0)return false;
  if(!targetFocusReady(t))return false;
  INPUT in[2]{};for(int j=0;j<2;j++){in[j].type=1;in[j].ki.scan=text[i];in[j].ki.flags=4|(j?2:0);in[j].ki.extra=MARK;}
  if(SendInput(2,in,sizeof(INPUT))!=2)return false;
 }
 return true;
}
static inline LP mousePoint(int x,int y){return (LP)((u32)(u16)x|((u32)(u16)y<<16));}
static inline bool targetClick(const Target&t,int b){if(!valid(t))return false;HWND h=pointerChild(t);POINT p{};if(t.hasPoint)p={t.x,t.y};else if(!GetCursorPos(&p))return false;if(!ScreenToClient(h,&p))return false;const UINT down[]={0,0x201,0x204,0x207,0x20b,0x20b},up[]={0,0x202,0x205,0x208,0x20c,0x20c};const WP mask[]={0,1,2,16,32,64};b=clamp(b,1,5);WP extra=b>=4?(WP)(b-3)<<16:0;bool ok=PostMessageW(h,down[b],mask[b]|extra,mousePoint(p.x,p.y))!=0;return PostMessageW(h,up[b],extra,mousePoint(p.x,p.y))&&ok;}
static inline bool targetScroll(const Target&t,int delta,bool horizontal){if(!valid(t))return false;POINT p{};if(t.hasPoint)p={t.x,t.y};else if(!GetCursorPos(&p))return false;return PostMessageW(pointerChild(t),horizontal?0x20e:0x20a,(WP)(u16)delta<<16,mousePoint(p.x,p.y))!=0;}
static inline bool snapTarget(HWND h,bool right){MONITOR mi{sizeof(mi)};if(!GetMonitorInfoW(MonitorFromWindow(h,2),&mi))return false;if(IsZoomed(h)||IsIconic(h))ShowWindow(h,4);int width=mi.rcWork.right-mi.rcWork.left;int x=mi.rcWork.left+(right?width/2:0);return SetWindowPos(h,nullptr,x,mi.rcWork.top,right?width-width/2:width/2,mi.rcWork.bottom-mi.rcWork.top,4|16)!=0;}
