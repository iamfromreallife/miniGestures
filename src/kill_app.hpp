#pragma once
// Terminate only the process owning the captured window, never all matching names.
static inline bool killTargetApp(const Target&t){
 if(t.pid<=4||t.pid==GetCurrentProcessId()||!valid(t))return false;
 using ShellWindow=HWND(*)();
 auto shell=(ShellWindow)GetProcAddress(GetModuleHandleW(L"user32.dll"),"GetShellWindow");
 if(!shell)return false;DWORD shellPid=0;HWND desktop=shell();
 if(desktop)GetWindowThreadProcessId(desktop,&shellPid);
 if(t.pid==shellPid)return false;
 H process=OpenProcess(0x0001|0x1000,0,t.pid);
 if(!process)return false;
 using Critical=B(*)(H,B*);
 auto critical=(Critical)GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"IsProcessCritical");
 B isCritical=1;bool ok=false;W path[520]{};DWORD size=520;
 if(critical&&critical(process,&isCritical)&&!isCritical&&valid(t)&&QueryFullProcessImageNameW(process,0,path,&size)&&(!*t.exe||eq(path,t.exe))){
  if(shellPid||!eq(basename(path),L"explorer.exe"))ok=TerminateProcess(process,1)!=0;
 }
 CloseHandle(process);return ok;
}
