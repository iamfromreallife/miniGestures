#pragma once
#include "desktop_direct.hpp"
#ifdef _WIN32
// A direct virtual-desktop switch must hand foreground ownership to the shell
// when an application is active. Do this only on the action worker after a
// gesture matched; never synthesize a click or attach input from the hook.
// Match the actual shell PID before choosing its XAML host. Do not activate
// arbitrary windows, or minimize the taskbar/desktop as a cleanup shortcut.
static inline void prepareDesktopSwitch(void* proxy){
 H user=GetModuleHandleW(L"user32.dll"),ole=GetModuleHandleW(L"ole32.dll");
 using ShellWindow=HWND(*)();using Attach=B(*)(DWORD,DWORD,B);
 using Allow=B(*)(DWORD);using CoAllow=int(*)(void*,void*);
 auto shell=(ShellWindow)GetProcAddress(user,"GetShellWindow");
 auto attach=(Attach)GetProcAddress(user,"AttachThreadInput");
 auto allow=(Allow)GetProcAddress(user,"AllowSetForegroundWindow");
 auto coallow=ole?(CoAllow)GetProcAddress(ole,"CoAllowSetForegroundWindow"):nullptr;
 if(coallow&&proxy)coallow(proxy,nullptr);
 HWND desktop=shell?shell():nullptr;if(!desktop||!IsWindow(desktop))return;
 DWORD shellPid=0;DWORD shellTid=GetWindowThreadProcessId(desktop,&shellPid);
 if(!shellPid||!shellTid)return;
 if(allow)allow(shellPid);
 HWND fg=GetForegroundWindow();DWORD fgPid=0;DWORD fgTid=fg?GetWindowThreadProcessId(fg,&fgPid):0;
 // An Explorer file manager can share the shell PID, so only treat its actual
 // desktop/taskbar windows as already prepared.
 HWND host=FindWindowW(L"XamlExplorerHostIslandWindow",nullptr);DWORD hostPid=0;
 DWORD hostTid=host?GetWindowThreadProcessId(host,&hostPid):0;
 if(!hostTid||hostPid!=shellPid||!IsWindow(host)){host=desktop;hostTid=shellTid;}
 if(fg==desktop||fg==host)return;
 if(SetForegroundWindow(host))return;
 if(!attach||!fgTid||GetForegroundWindow()!=fg)return;
 using ThreadId=DWORD(*)();auto threadId=(ThreadId)GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"GetCurrentThreadId");
 if(!threadId)return;DWORD self=threadId();if(!self)return;
 // Ensure this worker has a message queue (also covers parallel macros).
 MSG msg{};PeekMessageW(&msg,nullptr,0,0,0);
 bool linkedFg=fgTid!=self&&attach(self,fgTid,1);
 if(fgTid!=self&&!linkedFg)return;
 bool linkedShell=hostTid!=self&&hostTid!=fgTid&&attach(self,hostTid,1);
 if(GetForegroundWindow()==fg&&IsWindow(host))SetForegroundWindow(host);
 if(linkedShell)attach(self,hostTid,0);
 if(linkedFg)attach(self,fgTid,0);
}
// Load documented COM entry points from System32. Keep the private shell
// objects local to the invoking worker/apartment and release before uninitializing.
static inline bool directEdgeDesktop(bool last){
 using namespace directDesktop;
 H ole=GetModuleHandleW(L"ole32.dll");if(!ole)ole=LoadLibraryExW(L"ole32.dll",nullptr,0x800);
 if(!ole)return false;
 auto init=(int(*)(void*,DWORD))GetProcAddress(ole,"CoInitializeEx");
 auto uninit=(void(*)())GetProcAddress(ole,"CoUninitialize");
 auto create=(int(*)(const Guid*,void*,DWORD,const Guid*,void**))GetProcAddress(ole,"CoCreateInstance");
 if(!init||!uninit||!create)return false;
 int hr=init(nullptr,2); // STA; tolerate a pre-existing MTA on the worker.
 if(hr<0&&hr!=(int)0x80010106u)return false;
 bool ok=false;
 { Ref sp; if(create(&shell,nullptr,4,&provider,&sp.p)>=0&&sp.p)
    ok=edge(sp.p,last,[](){return stopActionEvent&&WaitForSingleObject(stopActionEvent,0)==0;},prepareDesktopSwitch); }
 if(hr>=0)uninit();
 return ok;
}
static inline bool desktopEdge(bool last){
 // Serialize edge navigation even when actions are configured as simultaneous.
 static volatile long busy=0;if(InterlockedExchange(&busy,1))return false;
 bool ok=directEdgeDesktop(last);
 InterlockedExchange(&busy,0);return ok;
}
#else
static inline bool desktopEdge(bool){return false;}
#endif
