#pragma once
#include "actions.hpp"
#include "capture_log.hpp"
#include "remote_input.hpp"
// Hook thread owns capture state. A separate worker owns recognition/actions.
// controllerwnd is stable until process shutdown; mainwnd is UI-thread only.
static HWND controllerwnd,mainwnd,overlaywnd; static DWORD hooktid,worktid,actiontid;static HANDLE hookthread,workerthread,actionthread,hookready,workready,actionready;static H mousehook,keyhook,diagkeyhook;static Snapshot* live;static volatile long running=1,hookok=0;static constexpr UINT MSG_CONFIG=0x8001,MSG_JOB=0x8002,MSG_RESULT=0x8003,MSG_TRAIL=0x8004,MSG_CLEAR=0x8005,MSG_EXECUTE=0x8006,MSG_RECORD=0x8007,MSG_GESTURE_TIMEOUT=0x8012;static volatile long recording=0,shapeRecording=0,trayMenuActive=0;
static H engineStop,metadataWake;static HANDLE metathread;static DWORD metatid;
static volatile long rulesActive=0,metadataPending=0,pendingJobs=0,actionStarted=0;
static constexpr UINT MSG_METADATA=0x8008,MSG_ENGINE_READY=0x800a,MSG_REFRESH_HOOKS=0x800b;
static constexpr int MAX_PENDING_JOBS=64;static constexpr UINT MSG_REARM=0x800c;static volatile long mouseEvents=0,mouseExpected=0,keyboardExpected=0;
// A single coalesced metadata update from a separate thread. The hook only reads its own copy.
struct MetadataPair {Target cursor,foreground;DWORD tick;};
struct CachedTarget{Target t;DWORD tick;};static constexpr int META_CACHE=32;static CachedTarget metadata[META_CACHE]{};static int metaNext=0;static DWORD ownPid;static DWORD lastTrailTick;
// Hook-owned copy of process metadata proactively collected by the worker.
struct ProcessPath{DWORD pid,tick;int safety;W path[520];};
static constexpr int WARM_LIMIT=96;static constexpr UINT MSG_WARM_METADATA=0x800e;
struct WarmMetadata{int count;ProcessPath p[WARM_LIMIT];};
static WarmMetadata warm{};static volatile long warmPending=0;
static const ProcessPath* warmProcess(DWORD pid,DWORD now){for(int i=0;i<warm.count;i++)if(warm.p[i].pid==pid&&now-warm.p[i].tick<=5000)return &warm.p[i];return nullptr;}
static void acceptWarmMetadata(WarmMetadata* data){warm.count=data->count;memcpy(warm.p,data->p,sizeof(ProcessPath)*warm.count);freep(data);InterlockedExchange(&warmPending,0);}

static bool shouldStop(){return !running||WaitForSingleObject(engineStop,0)==0;}
static int waitMessage(MSG& m){if(shouldStop())return 0;int result=GetMessageW(&m,nullptr,0,0);if(result<0)(void)0;return result>0;}
static Target identity(HWND h){Target t{};if(!h)return t;t.child=h;t.hwnd=GetAncestor(h,2);if(!t.hwnd)t.hwnd=h;GetWindowThreadProcessId(t.hwnd,&t.pid);return t;}
#include "task_view_win.hpp"
// The taskbar option covers the entire taskbar, including its icons.
 // Actual popup menus and notification overflow remain native.
static bool shellTrayClick(HWND h,bool taskbarGestures,POINT){
 if(!h)return false;HWND root=GetAncestor(h,2);W name[96]{};
 if(!GetClassNameW(root?root:h,name,96))return false;
 return (!taskbarGestures&&(eq(name,L"Shell_TrayWnd")||eq(name,L"Shell_SecondaryTrayWnd")))||
  eq(name,L"#32768")||eq(name,L"NotifyIconOverflowWindow")||eq(name,L"TopLevelWindowForOverflowXamlIsland");
}
// Fullscreen detection is opt-in and only runs when starting a candidate gesture.
// Compare the hovered top-level window with its monitor, not the taskbar work area.
// Framed windows (including maximized windows with an auto-hidden taskbar) and
// desktop surfaces are not fullscreen applications. No process queries or waits.
static bool fullscreenTarget(HWND h){
 if(!h)return false;HWND root=GetAncestor(h,2);if(root)h=root;
 if(!IsWindowVisible(h)||IsIconic(h))return false;
 const auto style=(u32)GetWindowLongPtrW(h,-16);
 if((style&0x00c00000u)==0x00c00000u||(IsZoomed(h)&&(style&0x00040000u)))return false;
 W cls[96]{};if(!GetClassNameW(h,cls,96))return false;
 if(eq(cls,L"Progman")||eq(cls,L"WorkerW")||eq(cls,L"Shell_TrayWnd")||eq(cls,L"Shell_SecondaryTrayWnd")||eq(cls,L"#32768")||eq(cls,L"NotifyIconOverflowWindow")||eq(cls,L"TopLevelWindowForOverflowXamlIsland"))return false;
 RECT r{};MONITOR mi{sizeof(mi)};H monitor=MonitorFromWindow(h,0);
 if(!monitor||!GetWindowRect(h,&r)||!GetMonitorInfoW(monitor,&mi))return false;
 const auto& m=mi.rcMonitor;
 return r.right>r.left&&r.bottom>r.top&&m.right>m.left&&m.bottom>m.top&&r.left<=m.left&&r.top<=m.top&&r.right>=m.right&&r.bottom>=m.bottom;
}
// Warm metadata runs on the metadata thread. A cold candidate may query limited
// process/token information directly, never wait for the periodic refresh.
#ifdef _WIN32
static int integrityLevel(DWORD pid){
 using OpenToken=B(*)(H,DWORD,H*);using TokenInfo=B(*)(H,int,void*,DWORD,DWORD*);
 H adv=GetModuleHandleW(L"advapi32.dll");if(!adv)adv=LoadLibraryExW(L"advapi32.dll",nullptr,0x800);if(!adv)return -1;
 auto open=(OpenToken)GetProcAddress(adv,"OpenProcessToken");auto info=(TokenInfo)GetProcAddress(adv,"GetTokenInformation");if(!open||!info)return -1;
 H p=OpenProcess(0x1000,0,pid),token=nullptr;if(!p)return -1;
 int level=-1;alignas(8) u8 buf[256]{};DWORD n=0;
 if(open(p,8,&token)){if(info(token,25,buf,sizeof(buf),&n)&&n>=16){const u8* sid=*(const u8**)buf;up start=(up)buf,end=start+minn(n,sizeof(buf));if((up)sid>=start&&(up)sid+8<=end&&sid[1]&&sid[1]<=15&&(up)sid+8+4*sid[1]<=end){u32 v=0;memcpy(&v,sid+8+4*(sid[1]-1),4);level=(int)v;}}CloseHandle(token);}CloseHandle(p);return level;
}
#else
static int integrityLevel(DWORD){return 0x2000;}
#endif
static int ownIntegrity=-1;
// Small hook-owned cache for newly encountered processes; independent of the
// periodic warm scan and exact HWND cache. No allocations, waits or messages to
// a foreign window. Cache failures briefly, so inaccessible processes fail open.
static ProcessPath coldPaths[16]{};static bool coldPathQueried[16]{};static int coldNext=0;
static const ProcessPath* coldProcess(DWORD pid,bool needPath){
 DWORD now=GetTickCount();for(int i=0;i<16;i++){auto& c=coldPaths[i];if(c.pid==pid&&now-c.tick<=(c.safety==1&&(!needPath||*c.path)?2000u:100u)&&(!needPath||coldPathQueried[i]||c.safety!=1))return &c;}
 auto& c=coldPaths[coldNext];coldPathQueried[coldNext]=needPath;coldNext=(coldNext+1)%16;c={};c.pid=pid;c.tick=now;
#ifdef _WIN32
 int level=integrityLevel(pid);c.safety=ownIntegrity>=0&&level>=0&&level<=ownIntegrity?1:2;
 if(c.safety==1&&needPath){H p=OpenProcess(0x1000,0,pid);if(p){DWORD n=520;if(!QueryFullProcessImageNameW(p,0,c.path,&n))c.path[0]=0;CloseHandle(p);}}
#else
 ++fakeColdQueries;c.safety=fakeColdSafety;if(fakeColdPath&&needPath)cp(c.path,fakeColdPath,520);
#endif
 return &c;
}
static bool clickTargetSafe(const Target&t){if(t.pid==ownPid)return true;if(t.inputSafety)return t.inputSafety==1;
 DWORD now=GetTickCount();
#ifdef _WIN32
 for(auto& v:metadata)if(v.t.pid==t.pid&&now-v.tick<=5000&&v.t.inputSafety)return v.t.inputSafety==1;
#endif
 if(auto*p=warmProcess(t.pid,now))return p->safety==1;
#ifndef _WIN32
 if(!fakeColdEnabled)return fakeInputSafe;
#endif
 auto*p=coldProcess(t.pid,false);if(metadataWake)SetEvent(metadataWake);return p->safety==1;
}
static bool isOwnEditorFrame(HWND h){W cls[64]{};return h&&GetClassNameW(h,cls,64)&&(eq(cls,L"miniGesturesMain")||(eq(cls,L"miniGesturesController")&&IsWindowVisible(h)));}
static bool targetForHook(HWND h,Target& out,bool& deferred,const Snapshot* snap=nullptr){if(!snap)snap=live;deferred=false;out=identity(h);if(out.hwnd==overlaywnd&&overlaywnd)return false;if(!out.hwnd||!out.pid)return false;
 // Both modeless editors are legitimate targets. Keep modal dialogs and recording protected.
 if(out.pid==ownPid){
  if(recording||shapeRecording||!IsWindowEnabled(out.hwnd))return false;
  if(!isOwnEditorFrame(out.hwnd)){W cls[64]{};if(!GetClassNameW(out.hwnd,cls,64)||!eq(cls,L"miniGesturesSettings"))return false;}
 }
 if(!snap->rules)return true;
 DWORD now=GetTickCount();
 for(auto& c:metadata)if(c.t.hwnd==out.hwnd&&c.t.pid==out.pid&&now-c.tick<=1000){HWND child=out.child;out=c.t;out.child=child;if(!snap->pathRules||*out.exe){if(out.pid!=ownPid){GetClassNameW(out.hwnd,out.cls,128);out.title[0]=0;GetWindowTextW(out.hwnd,out.title,256);}return true;}break;}
 if(out.pid!=ownPid){const W* path=nullptr;int safety=0;
  for(auto& c:metadata)if(c.t.pid==out.pid&&now-c.tick<=5000&&(!snap->pathRules||*c.t.exe)){path=c.t.exe;safety=c.t.inputSafety;break;}
  if(!path)if(auto*p=warmProcess(out.pid,now)){path=p->path;safety=p->safety;}
  // The first gesture on a different desktop need not wait for the background
  // scanner. Only query an uncached candidate, not every mouse movement.
#ifdef _WIN32
  if(!path||(snap->pathRules&&!*path)){
#else
  if(fakeColdEnabled&&(!path||(snap->pathRules&&!*path))){
#endif
   auto*p=coldProcess(out.pid,snap->pathRules);path=p->path;safety=p->safety;
  }
  // Class/title are read from the actual hovered HWND, not the last foreground.
  if(path&&(!snap->pathRules||*path)&&GetClassNameW(out.hwnd,out.cls,128)){cp(out.exe,path,520);out.inputSafety=safety;out.title[0]=0;GetWindowTextW(out.hwnd,out.title,256);return true;}
 }
 // If identity cannot be verified, preserve exclusions and native click delivery.
 if(metadataWake)SetEvent(metadataWake);if(snap->exclusionRules)return false;deferred=true;return true;}
static void cacheTarget(const Target&t,DWORD tick){if(!t.hwnd||!t.pid)return;for(auto& c:metadata)if(c.t.hwnd==t.hwnd&&c.t.pid==t.pid){c.t=t;c.tick=tick;return;}metadata[metaNext]={t,tick};metaNext=(metaNext+1)%META_CACHE;}
// Warm scans need only process identity, not every window title/class. Keep the
// same freshness interval and refresh the exact hovered/foreground metadata.
static ProcessPath* metadataProcess(DWORD pid,ProcessPath* paths,int& next){DWORD now=GetTickCount();ProcessPath* found=nullptr;for(int i=0;i<WARM_LIMIT;i++)if(paths[i].pid==pid&&now-paths[i].tick<2000){found=&paths[i];break;}if(!found){found=&paths[next];next=(next+1)%WARM_LIMIT;found->pid=pid;found->tick=now;found->path[0]=0;int level=integrityLevel(pid);found->safety=(ownIntegrity>=0&&level>=0&&level<=ownIntegrity)?1:2;H p=OpenProcess(0x1000,0,pid);if(p){DWORD n=520;QueryFullProcessImageNameW(p,0,found->path,&n);CloseHandle(p);}}return found;}
static Target metadataTarget(HWND hwnd,ProcessPath* paths,int& next){auto t=identity(hwnd);if(!t.hwnd||!t.pid)return t;auto* found=metadataProcess(t.pid,paths,next);t.inputSafety=found->safety;cp(t.exe,found->path,520);GetClassNameW(t.hwnd,t.cls,128);GetWindowTextW(t.hwnd,t.title,256);return t;}
static bool sameTarget(const Target&a,const Target&b){return a.hwnd==b.hwnd&&a.pid==b.pid&&a.inputSafety==b.inputSafety&&eq(a.exe,b.exe)&&eq(a.cls,b.cls)&&eq(a.title,b.title);}
struct WarmScan{WarmMetadata* data;ProcessPath* paths;int* next;};
static void warmWindow(WarmScan& scan,HWND hwnd){
 if(!hwnd||scan.data->count>=WARM_LIMIT)return;auto t=identity(hwnd);if(!t.pid)return;
 for(int i=0;i<scan.data->count;i++)if(scan.data->p[i].pid==t.pid)return;
 auto* cached=metadataProcess(t.pid,scan.paths,*scan.next);scan.data->p[scan.data->count++]=*cached;
}
#ifdef _WIN32
static B CB warmEnum(HWND hwnd,LP arg){auto& scan=*(WarmScan*)arg;if(shouldStop()||scan.data->count>=WARM_LIMIT)return 0;if(IsWindowVisible(hwnd))warmWindow(scan,hwnd);return 1;}
static void publishWarm(ProcessPath* paths,int& next){
 if(warmPending)return;auto*d=(WarmMetadata*)allocRaw(sizeof(WarmMetadata));if(!d)return;d->count=0;
 WarmScan scan{d,paths,&next};H user=GetModuleHandleW(L"user32.dll");
 using ShellWindow=HWND(CB*)();using Enum=B(CB*)(B(CB*)(HWND,LP),LP);
 auto shell=(ShellWindow)GetProcAddress(user,"GetShellWindow");auto enumerate=(Enum)GetProcAddress(user,"EnumWindows");
 // Prioritize the Explorer desktop, regardless of foreground focus.
 if(shell)warmWindow(scan,shell());POINT p{};if(GetCursorPos(&p))warmWindow(scan,WindowFromPoint(p));warmWindow(scan,GetForegroundWindow());
 if(enumerate)enumerate(warmEnum,(LP)&scan);
 if(!shouldStop()&&!InterlockedExchange(&warmPending,1)){if(PostThreadMessageW(hooktid,MSG_WARM_METADATA,0,(LP)d))return;InterlockedExchange(&warmPending,0);}freep(d);
}
#endif
static DWORD CB metamain(void*){
 static ProcessPath paths[WARM_LIMIT]{};int next=0;MetadataPair previous{};H events[]={engineStop,metadataWake};DWORD lastWarm=0;bool warmed=false;
 while(!shouldStop()){
#ifdef _WIN32
  // Poll only while explicitly capturing diagnostics; independent of metadata rules.
  if(mgdiag::enabled()){POINT p{};GetCursorPos(&p);Target hovered=identity(WindowFromPoint(p));diagnosticState("sample hover child pid tid fg active focus capture",hovered);MG_LOG("sample engine running mouseExpected keyboardExpected pendingJobs hookOK",running,mouseExpected,keyboardExpected,pendingJobs,hookok);}
#endif
  if((rulesActive||mouseExpected)&&!metadataPending){POINT p{};GetCursorPos(&p);MetadataPair current{};
current.cursor=metadataTarget(WindowFromPoint(p),paths,next);auto fg=identity(GetForegroundWindow());current.foreground=fg.hwnd==current.cursor.hwnd&&fg.pid==current.cursor.pid?current.cursor:metadataTarget(fg.hwnd,paths,next);current.tick=GetTickCount();if(current.tick-previous.tick>=400||!sameTarget(current.cursor,previous.cursor)||!sameTarget(current.foreground,previous.foreground)){auto*d=(MetadataPair*)allocRaw(sizeof(MetadataPair));if(d){*d=current;if(!shouldStop()&&!InterlockedExchange(&metadataPending,1)){if(PostThreadMessageW(hooktid,MSG_METADATA,0,(LP)d))previous=current;else{InterlockedExchange(&metadataPending,0);freep(d);}}else freep(d);}}}
#ifdef _WIN32
  DWORD now=GetTickCount();if((rulesActive||mouseExpected)&&(!warmed||now-lastWarm>=1000)&&!warmPending){publishWarm(paths,next);lastWarm=now;warmed=true;}
#endif
  DWORD r=WaitForMultipleObjects(2,events,0,(rulesActive||mouseExpected
#ifdef _WIN32
  ||mgdiag::enabled()
#endif
  )?100:0xffffffff);if(r==0||r==0xffffffff)break;
 }return 0;
}
// A reserved ordinary click is returned immediately when its physical up
// arrives. The capture is already cleared before SendInput; our marker
// makes the injected edges pass through without entering the matcher.
// No queue, timer, sleep, gesture worker, or foreground activation.
static constexpr UINT MSG_RELAY_JOB=0x800f;
static bool replay(int button){
 MG_LOG("replay click button",button);
 return click(button);
}

#include "gesture_match.hpp"
struct Capture{Snapshot*s;Target t;int button,mods,n,used,moved,swallow,mask,deferred;DWORD started;bool expired;P p[maxDrawingPoints];};static Capture cap;static up captureTimer=0;static int blocked=0;static unsigned injectedBlocked=0;
static inline bool cantrigger(const Config&c,const Target&t,int b,int m,int trig){int pr=profile(c,t);for(int i=0;i<c.ng;i++)if(eligible(c,c.g[i],t,pr,b,m,trig)&&(trig||c.g[i].count))return true;return false;}
static inline void finishJob(Job*j){release(j->s);freep(j);InterlockedExchangeAdd(&pendingJobs,-1);}
static inline bool postjob(Job*j,bool fromHook=false){if(!j)return false;MG_LOG("queue job target child pid trigger samples deferred hook",(up)j,(up)j->t.hwnd,(up)j->t.child,j->t.pid,j->trigger,j->n,j->deferred,fromHook);if(InterlockedIncrement(&pendingJobs)>MAX_PENDING_JOBS||!running||!PostThreadMessageW(fromHook?hooktid:worktid,fromHook?MSG_RELAY_JOB:MSG_JOB,0,(LP)j)){MG_LOG("queue rejected job",(up)j);finishJob(j);return false;}return true;}
// Do not race the consumed physical mouse/key edge with activation on another
// worker. This relay runs in the hook message loop after its callback returns.
static void relayJob(Job*j){MG_LOG("relay job",(up)j);if(!running||!PostThreadMessageW(worktid,MSG_JOB,0,(LP)j)){MG_LOG("relay rejected job",(up)j);finishJob(j);}}
static inline bool submit(int trig,const Target* current=nullptr,bool deferred=false){Job*j=allocateJob();if(!j)return false;j->s=cap.s;retain(j->s);j->t=current?*current:cap.t;j->button=cap.button;j->mods=cap.mods;j->trigger=trig;j->deferred=current?deferred:cap.deferred;j->n=cap.n;if(!trig)memcpy(j->p,cap.p,cap.n*sizeof(P));return postjob(j,true);}
static inline void stopCaptureTimer(){if(captureTimer){KillTimer(nullptr,captureTimer);captureTimer=0;}}
static inline void expireCapture(DWORD now){
 if(!cap.s||cap.used||cap.expired||!(cap.mask&1)||!gestureTimeoutExpired(cap.started,now,cap.s->c.gestureTimeoutMs))return;
 cap.expired=true;stopCaptureTimer();
 if(cap.moved&&cap.s->c.overlay)PostMessageW(controllerwnd,MSG_CLEAR,0,0);
 MG_LOG("drawn gesture timeout ms",now-cap.started);
}
static inline void resetcap(){stopCaptureTimer();cap.expired=false;bool clearTrail=cap.moved&&cap.s&&cap.s->c.overlay;if(cap.s)release(cap.s);cap.s=nullptr;cap.button=0;cap.n=0;cap.used=0;cap.moved=0;cap.swallow=0;if(running&&clearTrail)PostMessageW(controllerwnd,MSG_CLEAR,0,0);}
static inline int mb(UINT msg,DWORD data){switch(msg){case 0x201:case 0x202:return 1;case 0x204:case 0x205:return 2;case 0x207:case 0x208:return 3;case 0x20b:case 0x20c:return HI(data)==1?4:5;}return 0;}
static inline bool isdown(UINT m){return m==0x201||m==0x204||m==0x207||m==0x20b;}
static inline bool isup(UINT m){return m==0x202||m==0x205||m==0x208||m==0x20c;}
// The immutable snapshot contains the complete trigger bitmask for each button
// and modifier combination. The common global-only path never scans gestures.
static int captureMask(const Snapshot& snap,const Target&t,bool deferred,int b,int mod){
 const auto&c=snap.c;if(!snap.rules||deferred)return snap.buttons[b][mod];
 int pr=profile(c,t),mask=0;bool inherited=inherits(c,pr),wheelExcluded=(snap.buttons[b][mod]&0x1e)&&excluded(c,t,4),rockerExcluded=(snap.buttons[b][mod]&0xe0)&&excluded(c,t,3);
 BindingRange range(snap,b,mod);for(int i=range.first;range.valid(i);i=range.advance(i)){const auto&g=c.g[i];if(!g.enabled||!g.steps||!(g.profile==pr||(g.profile==0&&inherited&&!blockedGlobal(c,pr,g.id)))||activationButton(c,g)!=b||g.mods!=mod||g.trigger>=8||(!g.trigger&&!g.count))continue;if((g.trigger>=1&&g.trigger<=4&&wheelExcluded)||(g.trigger>=5&&rockerExcluded))continue;mask|=1<<g.trigger;if(mask==snap.buttons[b][mod])break;}return mask;
}
// Wheel and rocker triggers are discrete events. While the activation button
// remains held across desktop switches, resolve each trigger at its own point.
// Drawn shapes re-resolve at their original press coordinate on recognition,
// so a window that restores during the stroke is picked up without redirecting
// ordinary strokes that cross into another window.
static bool submitAtPointer(int trig,POINT point){
 if(!(cap.s->buttons[cap.button][cap.mods]&(1<<trig)))return false;
 HWND h=gestureWindowAt(point);Target t{};bool deferred=false;const auto& c=cap.s->c;MG_LOG("submit rehit trigger original hit",trig,(up)cap.t.hwnd,(up)h);
 if(shellTrayClick(h,c.taskbarGestures,point)||(c.disableFullscreenGestures&&fullscreenTarget(h))||!targetForHook(h,t,deferred,cap.s)||(!deferred&&cap.s->rules&&excluded(c,t,1)))return false;
 if(trig==0&&cap.button==1&&t.pid==ownPid&&isOwnEditorFrame(t.hwnd)){W cls[64]{};if(GetClassNameW(h,cls,64)&&eq(cls,L"miniGesturesCanvas"))return false;}
 if(!(captureMask(*cap.s,t,deferred,cap.button,cap.mods)&(1<<trig))||!clickTargetSafe(t))return false;
 t.x=point.x;t.y=point.y;t.hasPoint=1;return submit(trig,&t,deferred);
}
static LR CB hookproc(int n,WP msg,LP lp){
 if(n<0||!live)return CallNextHookEx(mousehook,n,msg,lp);
 InterlockedIncrement(&mouseEvents);auto&m=*(MOUSEHOOK*)lp;auto pass=[&](){return CallNextHookEx(mousehook,n,msg,lp);};
 if(msg!=0x200)MG_LOG("mouse edge message flags own buttonData capButton used moved blocked",msg,m.flags,m.extra==MARK,m.mouseData,cap.button,cap.used,cap.moved,blocked);
 int b=mb((UINT)msg,m.mouseData);
 // Preserve the active stroke's input policy until its matching release.
 // Also drain any already-consumed injected rocker release after a settings change.
 bool allowExternal=cap.s?cap.s->c.allowRemoteInput!=0:live->c.allowRemoteInput!=0;
 if(b&&isup((UINT)msg)&&(injectedBlocked&(1u<<b)))allowExternal=true;
 if(!mgremote::accepts(m.flags,m.extra==MARK,allowExternal))return pass();
 if(b&&isup((UINT)msg)&&(blocked&(1<<b))){blocked&=~(1<<b);injectedBlocked&=~(1u<<b);if(!blocked&&!cap.button&&(!live->c.enabled||!live->mouseWanted))PostThreadMessageW(hooktid,MSG_REFRESH_HOOKS,0,0);return 1;}
 if(shapeRecording||!running||!live->c.enabled){if(cap.button&&b==cap.button&&isup((UINT)msg)){int old=cap.button;bool used=cap.used||(cap.moved&&(cap.mask&1)),exclusive=cap.swallow;resetcap();PostThreadMessageW(hooktid,MSG_REFRESH_HOOKS,0,0);if(exclusive&&(used||replay(old)))return 1;}return pass();}
 // A native tray menu is modal. Do not start a new capture while it is
 // being opened/tracked; already captured releases still drain normally.
 if(!cap.button&&trayMenuActive)return pass();
 if(!cap.button&&b&&isdown((UINT)msg)){
  bool possible=false;for(int i=0;i<16;i++)if(live->buttons[b][i]){possible=true;break;}if(!possible){MG_LOG("capture skipped no button binding",b);return pass();}int mod=mods(),mask=live->buttons[b][mod];if(!mask){MG_LOG("capture skipped no modifier binding button mods",b,mod);return pass();}
  // Reserve both edges only when the target is eligible and safe to replay.
  HWND nativeHit=WindowFromPoint(m.pt);if(shellTrayClick(nativeHit,live->c.taskbarGestures,m.pt))return pass();
  HWND hovered=gestureWindowAtHit(m.pt,nativeHit);Target t{};bool deferred=false;
  if(shellTrayClick(hovered,live->c.taskbarGestures,m.pt)||(live->c.disableFullscreenGestures&&fullscreenTarget(hovered))||!targetForHook(hovered,t,deferred)||(!deferred&&live->rules&&excluded(live->c,t,1))){MG_LOG("capture skipped target filter hovered target pid",(up)hovered,(up)t.hwnd,t.pid);return pass();}
  if(b==1&&t.pid==ownPid&&isOwnEditorFrame(t.hwnd)){W cls[64]{};if(GetClassNameW(hovered,cls,64)&&eq(cls,L"miniGesturesCanvas"))return pass();}mask=captureMask(*live,t,deferred,b,mod);if(!mask||!clickTargetSafe(t)){MG_LOG("capture skipped mask or safety target pid mask",(up)t.hwnd,t.pid,mask);return pass();}
  cap.mask=mask;cap.deferred=deferred;lastTrailTick=0;cap.s=live;retain(cap.s);t.x=m.pt.x;t.y=m.pt.y;t.hasPoint=1;cap.t=t;cap.button=b;cap.mods=mod;cap.n=1;cap.p[0]={m.pt.x,m.pt.y};cap.used=cap.moved=0;cap.expired=false;cap.started=m.time;if((mask&1)&&cap.s->c.gestureTimeoutMs>0)captureTimer=SetTimer(nullptr,0,(UINT)cap.s->c.gestureTimeoutMs,nullptr);// A rocker-right-only left press stays native: no click replay or drag interference.
  cap.swallow=!(b==1&&mask==(1<<6));MG_LOG("capture begin target child pid button mods mask swallow deferred",(up)t.hwnd,(up)t.child,t.pid,b,mod,mask,cap.swallow,deferred);return cap.swallow?1:pass();
 }
 if(!cap.button)return pass();if(b==cap.button&&isdown((UINT)msg))return cap.swallow?1:pass();const auto&c=cap.s->c;
 // Plain right-up: do not run timeout/recognition logic, allocate a job,
 // resolve the target again, or wait for a queued replay. Preserve the
 // threshold test even if the final movement did not produce WM_MOUSEMOVE.
 if(b==2&&cap.button==2&&isup((UINT)msg)&&!cap.used&&!cap.moved&&
    !mgangular::reached(P{m.pt.x,m.pt.y},cap.p[0],c.threshold)){
  bool exclusive=cap.swallow;resetcap();
  if(!live->mouseWanted)PostThreadMessageW(hooktid,MSG_REFRESH_HOOKS,0,0);
  if(exclusive&&replay(2))return 1;
  return pass();
 }
 expireCapture(m.time);
 if(msg==0x200&&!cap.used&&cap.expired){if(!cap.moved&&mgangular::reached(P{m.pt.x,m.pt.y},cap.p[0],c.threshold))cap.moved=1;}
 if(msg==0x200&&!cap.used&&!cap.expired){P q{m.pt.x,m.pt.y};if(appendDrawingPoint(cap.p,cap.n,q)){if(!cap.moved&&mgangular::reached(q,cap.p[0],c.threshold)){cap.moved=1;if((cap.mask&1)&&c.overlay)PostMessageW(controllerwnd,MSG_TRAIL,1,(LP)((u64)(u32)cap.p[0].x|((u64)(u32)cap.p[0].y<<32)));}if(cap.moved&&(cap.mask&1)&&c.overlay){DWORD now=GetTickCount();if(!lastTrailTick||now-lastTrailTick>=16){lastTrailTick=now;PostMessageW(controllerwnd,MSG_TRAIL,0,(LP)((u64)(u32)q.x|((u64)(u32)q.y<<32)));}}}}
 if((msg==0x20a||msg==0x20e)&&!cap.moved){int d=(short)HI(m.mouseData);int trig=msg==0x20a?(d>0?1:2):(d>0?4:3);if(submitAtPointer(trig,m.pt)){cap.used=1;return cap.swallow?1:pass();}}
 if(b&&b!=cap.button&&b<=3&&isdown((UINT)msg)&&!cap.moved){int trig=4+b;if(submitAtPointer(trig,m.pt)){cap.used=1;// Even with a native left press, consume both edges of the matched right click.
 if(cap.swallow||trig==6){blocked|=1<<b;if(m.flags&3u)injectedBlocked|=1u<<b;return 1;}return pass();}}
 if(b==cap.button&&isup((UINT)msg)){int old=b;bool used=cap.used,exclusive=cap.swallow;bool moved=cap.moved||mgangular::reached(P{m.pt.x,m.pt.y},cap.p[0],c.threshold);MG_LOG("capture release target child pid button used moved swallow points",(up)cap.t.hwnd,(up)cap.t.child,cap.t.pid,b,used,moved,exclusive,cap.n);if(!used&&moved&&(cap.mask&1)){if(cap.expired){PostMessageW(controllerwnd,MSG_GESTURE_TIMEOUT,0,0);used=true;}else{P q{m.pt.x,m.pt.y};appendDrawingPoint(cap.p,cap.n,q,true);bool submitted=submitAtPointer(0,{cap.p[0].x,cap.p[0].y});MG_LOG("shape submit result",submitted);used=true;}}resetcap();if(!live->mouseWanted)PostThreadMessageW(hooktid,MSG_REFRESH_HOOKS,0,0);if(exclusive&&(used||replay(old)))return 1;return pass();}
 return pass();
}
static int heldkeys[256];
static ScreenshotKeyPass screenshotKeys;
static OwnPrintScreen ownPrintScreen;
extern "C" int CB NativeScreenClipEnabled();
extern "C" int CB NativeQueueScreenClip();
static bool routeOwnPrintScreen(){
 DWORD pid=0;HWND fg=GetForegroundWindow();
 return fg&&GetWindowThreadProcessId(fg,&pid)&&pid==ownPid&&mods()==0&&NativeScreenClipEnabled();
}
static int actionRecordCandidate=0;
static int recordModifierMask(int vk){switch(vk){case 0x12:case 0xa4:case 0xa5:return 1;case 0x11:case 0xa2:case 0xa3:return 2;case 0x10:case 0xa0:case 0xa1:return 4;case 0x5b:case 0x5c:return 8;default:return 0;}}
static int capturedModifiers(){int mask=0;for(int i=0;i<256;i++)if(heldkeys[i]==2)mask|=recordModifierMask(i);return mask;}
// Passive observation of our own injected events only. Never log physical keys
// or scan codes for Unicode text; never consume an observed event.
static LR observeOwnKey(H hook,int n,WP msg,LP lp){
 if(n<0)return CallNextHookEx(hook,n,msg,lp);
 const auto&k=*(KEYHOOK*)lp;
 if(k.extra!=MARK||!mgdiag::enabled())return CallNextHookEx(hook,n,msg,lp);
 const DWORD vk=k.vkCode,flags=k.flags,scan=vk==0xe7?0:k.scanCode;
 const LR result=CallNextHookEx(hook,n,msg,lp);
 MG_LOG("own injected key edge message flags vk scan downstreamResult",msg,flags,vk,scan,(up)result);
 return result;
}
static LR CB diagnosticKeyboardProc(int n,WP msg,LP lp){return observeOwnKey(diagkeyhook,n,msg,lp);}
static LR CB keyboardproc(int n,WP msg,LP lp){if(n<0)return CallNextHookEx(keyhook,n,msg,lp);auto&k=*(KEYHOOK*)lp;if(k.extra==MARK)return observeOwnKey(keyhook,n,msg,lp);if(k.vkCode>255)return CallNextHookEx(keyhook,n,msg,lp);int vk=k.vkCode;bool down=msg==0x100||msg==0x104;bool up=msg==0x101||msg==0x105;
 // Plain physical Print Screen on our own windows can fail in the Windows
 // shell, especially at elevated integrity. Route it to the normal-user
 // Snipping Tool instead. Never intercept other apps or modified variants.
 if(vk==0x2c&&!(k.flags&0x10)){
  bool route=ownPrintScreen.needsPolicy(down,up)&&!(k.flags&0x20)&&routeOwnPrintScreen();
  auto decision=ownPrintScreen.edge(down,up,route);heldkeys[vk]=0;
  if(decision==OwnPrintScreen::Consume)return 1;
  if(decision==OwnPrintScreen::Launch&&NativeQueueScreenClip())return 1;
  return CallNextHookEx(keyhook,n,msg,lp);
 }
 // Pass both edges to Windows before recording, target lookup or dispatch.
 if(screenshotKeys.pass(vk,down,up,vk=='S'?mods():0)){heldkeys[vk]=0;return CallNextHookEx(keyhook,n,msg,lp);}
 if(up&&heldkeys[vk]){int kind=heldkeys[vk];heldkeys[vk]=0;if(kind==2&&actionRecordCandidate==vk){actionRecordCandidate=0;DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);if(recording==2&&pid==GetCurrentProcessId())PostMessageW(controllerwnd,MSG_RECORD,vk,0);}if(!recording&&live&&(!live->c.enabled||!live->keyWanted))PostThreadMessageW(hooktid,MSG_REFRESH_HOOKS,0,0);return 1;}if(!down||!live||!running||shapeRecording||trayMenuActive)return CallNextHookEx(keyhook,n,msg,lp);if(heldkeys[vk])return 1;bool keyPossible=false;for(int i=0;i<16;i++)if(live->keys[vk][i]){keyPossible=true;break;}if(!recording&&!keyPossible)return CallNextHookEx(keyhook,n,msg,lp);int m=mods();if(recording){DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);if(pid==GetCurrentProcessId()&&recording==2){int bit=recordModifierMask(vk),held=capturedModifiers();if(bit){actionRecordCandidate=(held||m)?0:vk;heldkeys[vk]=2;return 1;}actionRecordCandidate=0;heldkeys[vk]=1;PostMessageW(controllerwnd,MSG_RECORD,vk,m|held);return 1;}if(pid==GetCurrentProcessId()&&vk!=0x10&&vk!=0x11&&vk!=0x12&&vk!=0x5b&&vk!=0x5c&&!(vk>=0xa0&&vk<=0xa5)){heldkeys[vk]=1;PostMessageW(controllerwnd,MSG_RECORD,vk,m);return 1;}}if(!running||!live->c.enabled)return CallNextHookEx(keyhook,n,msg,lp);const auto&c=live->c;if(!live->keys[vk][m])return CallNextHookEx(keyhook,n,msg,lp);POINT cursor{};if(!GetCursorPos(&cursor))return CallNextHookEx(keyhook,n,msg,lp);Target t{};bool deferred=false;HWND hovered=gestureWindowAt(cursor);if((c.disableFullscreenGestures&&fullscreenTarget(hovered))||!targetForHook(hovered,t,deferred)||(!deferred&&excluded(c,t,2)))return CallNextHookEx(keyhook,n,msg,lp);int pr=profile(c,t);bool found=false;BindingRange range(*live,0,m,vk);for(int i=range.first;range.valid(i);i=range.advance(i)){auto&g=c.g[i];if(g.enabled&&g.hotkey==vk&&g.hotmods==m&&(deferred||scopeEligible(c,g,pr))){found=true;break;}}if(!found)return CallNextHookEx(keyhook,n,msg,lp);auto*j=allocateJob();if(!j)return CallNextHookEx(keyhook,n,msg,lp);j->s=live;retain(j->s);t.x=cursor.x;t.y=cursor.y;t.hasPoint=1;j->t=t;j->hotkey=vk;j->mods=m;j->deferred=deferred;if(postjob(j,true)){heldkeys[vk]=1;return 1;}return CallNextHookEx(keyhook,n,msg,lp);}
static Snapshot* initialSnapshot;
static void configureHooks(){if(recording!=2)actionRecordCandidate=0;if(!live||shouldStop())return;bool wantMouse=live->c.enabled&&live->mouseWanted,wantKey=(live->c.enabled&&live->keyWanted)||recording;
 // Keep the screenshot compatibility hook available even when gestures are
 // paused or no keyboard bindings exist. Failure of this optional hook must
 // not prevent an otherwise functional mouse-only engine from starting.
 if(!wantMouse&&mousehook&&!cap.button&&!blocked){UnhookWindowsHookEx(mousehook);mousehook=nullptr;}
 if(wantMouse&&!mousehook&&!shouldStop())mousehook=SetWindowsHookExW(14,hookproc,GetModuleHandleW(nullptr),0);
 if(!keyhook&&!shouldStop())keyhook=SetWindowsHookExW(13,keyboardproc,GetModuleHandleW(nullptr),0);
 bool ok=(!wantMouse||mousehook)&&(!wantKey||keyhook)&&!shouldStop();if(!ok){if(mousehook)UnhookWindowsHookEx(mousehook);if(keyhook)UnhookWindowsHookEx(keyhook);mousehook=keyhook=nullptr;screenshotKeys={};ownPrintScreen={};}
 // The diagnostic-only hook is optional and must never disable real gestures.
 if(diagkeyhook&&(!mgdiag::enabled()||keyhook)){UnhookWindowsHookEx(diagkeyhook);diagkeyhook=nullptr;}
 if(mgdiag::enabled()&&!keyhook&&!diagkeyhook&&!shouldStop()){
  diagkeyhook=SetWindowsHookExW(13,diagnosticKeyboardProc,GetModuleHandleW(nullptr),0);
  MG_LOG("diagnostic keyboard observer installed",diagkeyhook!=nullptr);
 }
 InterlockedExchange(&mouseExpected,ok&&wantMouse?1:0);InterlockedExchange(&keyboardExpected,ok&&wantKey?1:0);InterlockedExchange(&hookok,ok?1:0);InterlockedExchange(&rulesActive,ok&&live->rules&&(wantMouse||wantKey)?1:0);if(metadataWake)SetEvent(metadataWake);
}
static DWORD CB hookmain(void*){MSG m{};PeekMessageW(&m,nullptr,0,0,0);live=initialSnapshot;initialSnapshot=nullptr;ownPid=GetCurrentProcessId();configureHooks();SetEvent(hookready);PostMessageW(controllerwnd,MSG_ENGINE_READY,0,0);
 while(waitMessage(m)){if(m.message==MSG_CONFIG){Snapshot* next=(Snapshot*)m.lParam;Snapshot* old=live;live=next;release(old);long was=hookok;configureHooks();if(was!=hookok)PostMessageW(controllerwnd,MSG_ENGINE_READY,1,0);}
 else if(m.message==0x113&&!m.hwnd&&captureTimer&&m.wParam==captureTimer){if(!cap.s||cap.used)stopCaptureTimer();else expireCapture(GetTickCount());}
 else if(m.message==MSG_RELAY_JOB){relayJob((Job*)m.lParam);}
 else if(m.message==MSG_REFRESH_HOOKS){configureHooks();}else if(m.message==MSG_REARM){if(!cap.button&&!blocked&&mousehook){UnhookWindowsHookEx(mousehook);mousehook=nullptr;configureHooks();(void)0;PostMessageW(controllerwnd,MSG_ENGINE_READY,1,0);}}
 else if(m.message==MSG_WARM_METADATA){acceptWarmMetadata((WarmMetadata*)m.lParam);}
 else if(m.message==MSG_METADATA){auto*d=(MetadataPair*)m.lParam;cacheTarget(d->cursor,d->tick);cacheTarget(d->foreground,d->tick);freep(d);InterlockedExchange(&metadataPending,0);}
 else{TranslateMessage(&m);DispatchMessageW(&m);}}
 if(mousehook)UnhookWindowsHookEx(mousehook);if(keyhook)UnhookWindowsHookEx(keyhook);if(diagkeyhook)UnhookWindowsHookEx(diagkeyhook);diagkeyhook=nullptr;InterlockedExchange(&hookok,0);InterlockedExchange(&rulesActive,0);resetcap();release(live);live=nullptr;if(running){InterlockedExchange(&mouseExpected,0);InterlockedExchange(&keyboardExpected,0);(void)0;PostMessageW(controllerwnd,MSG_ENGINE_READY,1,0);}while(PeekMessageW(&m,nullptr,0,0,1)){if(m.message==MSG_CONFIG)release((Snapshot*)m.lParam);else if(m.message==MSG_METADATA||m.message==MSG_WARM_METADATA)freep((void*)m.lParam);else if(m.message==MSG_RELAY_JOB)finishJob((Job*)m.lParam);}return 0;}
struct Result{int ok,score,conflictId,otherId;W name[196];Result* next;};
static void resultGestureName(W* out,int cap,const RuntimeConfig&c,const Gesture&g){
 out[0]=0;if(g.profile)for(int i=0;i<c.na;i++)if(c.a[i].id==g.profile){if(*c.a[i].name){cp(out,c.a[i].name,cap);cat(out,L" - ",cap);}break;}cat(out,g.name,cap);
}
static Result* recognitionNotice(const RuntimeConfig&c,int idx,int score){if(!c.showGestureNames||idx<0||idx>=c.ng)return nullptr;auto*r=(Result*)alloc(sizeof(Result));if(r){r->ok=2;r->score=score;resultGestureName(r->name,196,c,c.g[idx]);}return r;}
struct Execution{Job*j;int idx,q;};
// Only accept result pointers allocated and registered by this process. Lock is never used by a low-level hook.
static volatile long resultLock=0;static Result* pendingResults=nullptr;
static void lockResults(){while(InterlockedExchange(&resultLock,1))Sleep(0);}
static void unlockResults(){InterlockedExchange(&resultLock,0);}
static Result* takeResult(Result* requested){lockResults();Result** link=&pendingResults;while(*link&&*link!=requested)link=&(*link)->next;Result* result=*link;if(result)*link=result->next;unlockResults();return result;}
static inline void deliver(Result*r){if(!r)return;lockResults();if(running){r->next=pendingResults;pendingResults=r;if(PostMessageW(controllerwnd,MSG_RESULT,0,(LP)r)){unlockResults();return;}pendingResults=r->next;}unlockResults();freep(r);}
static void clearResults(){lockResults();Result* r=pendingResults;pendingResults=nullptr;unlockResults();while(r){Result* next=r->next;freep(r);r=next;}}
// Parallel macros own a heap-backed batch, so cancellation never leaves
// running threads referencing the dispatcher stack or a freed configuration.
struct ParallelBatch;
struct ParallelStep{ParallelBatch* batch;Act a;bool ok;};
struct ParallelBatch{volatile long refs;H gate;Target target;ParallelStep steps[MAXA];};
static void releaseBatch(ParallelBatch*b){if(InterlockedExchangeAdd(&b->refs,-1)==1){CloseHandle(b->gate);freep(b);}}
static DWORD CB parallelStep(void*p){auto&s=*(ParallelStep*)p;auto*b=s.batch;WaitForSingleObject(b->gate,0xffffffff);MG_LOG("parallel action type target pid",s.a.type,(up)b->target.hwnd,b->target.pid);s.ok=running&&execute(s.a,b->target);MG_LOG("parallel action result type success",s.a.type,s.ok);releaseBatch(b);return 0;}
static bool executeSteps(const Gesture&g,const Target&t){if(!g.parallel){for(int i=0;i<g.steps;i++){MG_LOG("action step index type target pid",i,g.actions[i].type,(up)t.hwnd,t.pid);bool ok=running&&execute(g.actions[i],t,g.steps>1);MG_LOG("action step result index type success",i,g.actions[i].type,ok);if(!ok)return false;}return true;}
 auto*b=(ParallelBatch*)alloc(sizeof(ParallelBatch));if(!b)return false;b->refs=1;b->gate=CreateEventW(nullptr,1,0,nullptr);if(!b->gate){freep(b);return false;}b->target=t;H threads[MAXA]{};bool ok=true;
 for(int i=0;i<g.steps;i++){b->steps[i].batch=b;b->steps[i].a=g.actions[i];InterlockedIncrement(&b->refs);threads[i]=CreateThread(nullptr,0,parallelStep,&b->steps[i],0,nullptr);if(!threads[i]){releaseBatch(b);ok=false;}}
 SetEvent(b->gate);for(int i=0;i<g.steps;i++)if(threads[i]){H wait[]={engineStop,threads[i]};DWORD r=WaitForMultipleObjects(2,wait,0,0xffffffff);if(r!=1||!b->steps[i].ok)ok=false;CloseHandle(threads[i]);}releaseBatch(b);return ok;}
static DWORD CB actionmain(void*){MSG m{};PeekMessageW(&m,nullptr,0,0,0);SetEvent(actionready);while(waitMessage(m)){if(m.message!=MSG_EXECUTE)continue;Execution*e=(Execution*)m.lParam;Job*j=e->j;auto&g=j->s->c.g[e->idx];InterlockedExchange(&actionStarted,1);MG_LOG("dispatch begin job target child pid gestureID score steps parallel",(up)j,(up)j->t.hwnd,(up)j->t.child,j->t.pid,g.id,e->q,g.steps,g.parallel);bool ok=running&&executeSteps(g,j->t);MG_LOG("dispatch end job success",(up)j,ok);Result*r=(Result*)alloc(sizeof(Result));if(r){r->ok=ok?1:-1;r->score=e->q;resultGestureName(r->name,196,j->s->c,g);deliver(r);}finishJob(j);freep(e);}while(PeekMessageW(&m,nullptr,0,0,1))if(m.message==MSG_EXECUTE){auto* e=(Execution*)m.lParam;finishJob(e->j);freep(e);}return 0;}
static DWORD CB workmain(void*){MSG m{};PeekMessageW(&m,nullptr,0,0,0);SetEvent(workready);while(waitMessage(m)){if(m.message!=MSG_JOB)continue;Job*j=(Job*)m.lParam;if(!running){finishJob(j);continue;}int q=0,idx=-1,conflictId=0,otherId=0,closest=-1;if(j->deferred){auto t=target(j->t.hwnd);if(t.hwnd==j->t.hwnd&&t.pid==j->t.pid){t.child=j->t.child;t.x=j->t.x;t.y=j->t.y;t.hasPoint=j->t.hasPoint;j->t=t;idx=choose(*j,q,&conflictId,&otherId,&closest);}}else idx=choose(*j,q,&conflictId,&otherId,&closest);MG_LOG("recognition job index score conflict other",(up)j,idx,q,conflictId,otherId);if(idx>=0){auto*e=(Execution*)alloc(sizeof(Execution));if(e){e->j=j;e->idx=idx;e->q=j->direct?-1:q;Result* notice=recognitionNotice(j->s->c,idx,e->q);if(running&&PostThreadMessageW(actiontid,MSG_EXECUTE,0,(LP)e)){deliver(notice);continue;}freep(notice);freep(e);}q=-3;}Result*r=(Result*)alloc(sizeof(Result));if(r){r->ok=0;r->score=q;r->conflictId=conflictId;r->otherId=otherId;if(q>=0&&closest>=0&&closest<j->s->c.ng)resultGestureName(r->name,196,j->s->c,j->s->c.g[closest]);deliver(r);}finishJob(j);}while(PeekMessageW(&m,nullptr,0,0,1))if(m.message==MSG_JOB)finishJob((Job*)m.lParam);return 0;}
static inline Snapshot* snapshot(const Config&c){return buildSnapshot(c);}
static inline void publish(const Config&c){MG_LOG("config published enabled gestures apps exclusions button threshold sensitivity",c.enabled,c.ng,c.na,c.ne,c.button,c.threshold,c.sensitivity);if(!running||!hooktid||!hookready||WaitForSingleObject(hookready,0)!=0||shouldStop())return;auto*s=snapshot(c);if(!s){(void)0;return;}if(!PostThreadMessageW(hooktid,MSG_CONFIG,0,(LP)s)){release(s);(void)0;}}
static inline void hotjob(const Config&c,int vk,int m){if(screenshotShortcut(vk,m)||!c.enabled||!running)return;auto*s=snapshot(c);if(!s)return;Job*j=allocateJob();if(!j){release(s);return;}j->s=s;POINT p{};if(!GetCursorPos(&p)){release(s);freep(j);return;}j->t=target(gestureWindowAt(p));j->t.x=p.x;j->t.y=p.y;j->t.hasPoint=1;j->hotkey=vk;j->mods=m;postjob(j);}
