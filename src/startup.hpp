#pragma once
// ShellExecuteEx launches only our own fixed helper command. No command shell is used here.
struct StartupShellInfo {DWORD size,mask; HWND hwnd; const W* verb; const W* file; const W* parameters; const W* directory; int show; H instance; void* idList; const W* className; H keyClass; DWORD hotKey; H icon; H process;};
static_assert(sizeof(StartupShellInfo)==112);
static DWORD startupEnv(const W*name,W*value,DWORD size){auto f=(DWORD(*)(const W*,W*,DWORD))GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"GetEnvironmentVariableW");return f?f(name,value,size):0;}
static bool isElevated(){W value[8]{};return startupEnv(L"MINIGESTURES_ELEVATED",value,8)&&eq(value,L"1");}
static int configureStartupTask(bool enabled,bool admin,bool elevate){
 W sid[192]{},exe[1024]{},args[256]{};DWORD n=startupEnv(L"MINIGESTURES_USER_SID",sid,192);if(!n||n>=192)return 12;n=GetModuleFileNameW(nullptr,exe,1024);if(!n||n>=1024)return 11;
 cp(args,L"--configure-startup ",256);cat(args,enabled?L"1 ":L"0 ",256);cat(args,admin?L"1 ":L"0 ",256);cat(args,sid,256);
 H shell=GetModuleHandleW(L"shell32.dll");auto run=(B(*)(StartupShellInfo*))GetProcAddress(shell,"ShellExecuteExW");auto code=(B(*)(H,DWORD*))GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"GetExitCodeProcess");if(!run||!code)return 11;
 StartupShellInfo i{};i.size=sizeof(i);i.mask=0x40|0x100|0x400;i.hwnd=mainwnd;i.verb=elevate?L"runas":L"open";i.file=exe;i.parameters=args;i.show=0;
 if(!run(&i)||!i.process)return 11;DWORD result=11;if(WaitForSingleObject(i.process,0xffffffff)==0)code(i.process,&result);CloseHandle(i.process);return (int)result;
}

// Permit only the benign "show existing editor" message from a non-elevated launch.
static void allowShowMessage(HWND h){if(!isElevated())return;auto f=(B(*)(HWND,UINT,DWORD,void*))GetProcAddress(GetModuleHandleW(L"user32.dll"),"ChangeWindowMessageFilterEx");if(f)f(h,0x8011,1,nullptr);}

// Use the native SDK host for elevation and verified process handoff.
extern "C" DWORD CB NativeRestartApplication(HWND,int,W*,DWORD);
static bool restartApplication(HWND owner,bool admin,W* detail,DWORD capacity){return NativeRestartApplication(owner,admin?1:0,detail,capacity)==0;}
