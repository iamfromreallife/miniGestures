#pragma once
#include <stddef.h>
// Minimal declarations of the documented Win32 ABI for a freestanding cross-build.
using u8=unsigned char; using u16=unsigned short; using u32=unsigned int; using i32=int; using u64=unsigned long long; using up=unsigned long long; using ip=long long; using W=wchar_t; using B=int; using H=void*; using HWND=H; using HANDLE=H; using HDC=H; using HFONT=H; using HBRUSH=H; using HICON=H; using HINSTANCE=H; using HMENU=H; using WP=up; using LP=ip; using LR=ip; using DWORD=u32; using UINT=u32;
#define API extern "C" __declspec(dllimport)
#define CB __stdcall
#define PTR(x) ((void*)(up)(x))
#define LO(x) ((u16)(x))
#define HI(x) ((u16)((up)(x)>>16))
#define RGB(r,g,b) ((u32)(r)|((u32)(g)<<8)|((u32)(b)<<16))
struct LocalSystemTime{u16 year,month,dayOfWeek,day,hour,minute,second,milliseconds;};
static_assert(sizeof(LocalSystemTime)==16);
API void GetLocalTime(LocalSystemTime*);
API DWORD GetSysColor(int);
struct MGScrollInfo{UINT cbSize,fMask;int nMin,nMax;UINT nPage;int nPos,nTrackPos;};
API int SetScrollInfo(HWND,int,const MGScrollInfo*,B);
API B GetScrollInfo(HWND,int,MGScrollInfo*);
struct POINT{int x,y;}; struct RECT{int left,top,right,bottom;}; struct MSG{HWND hwnd;UINT message;WP wParam;LP lParam;DWORD time;POINT pt;DWORD lPrivate;};
using PROC=LR(CB*)(HWND,UINT,WP,LP); using HOOK=LR(CB*)(int,WP,LP); using THREAD=DWORD(CB*)(void*);
struct WC{UINT cbSize,style;PROC lpfnWndProc;int cbClsExtra,cbWndExtra;HINSTANCE hInstance;HICON hIcon;H hCursor;HBRUSH hbrBackground;const W* lpszMenuName;const W* lpszClassName;HICON hIconSm;};
struct PAINT{HDC hdc;B fErase;RECT rcPaint;B fRestore,fIncUpdate;u8 reserved[32];};
struct KEYHOOK{DWORD vkCode,scanCode,flags,time;up extra;};
struct MOUSEHOOK{POINT pt;DWORD mouseData,flags,time;up extra;};
struct MI{int dx,dy;DWORD mouseData,dwFlags,time;up extra;};struct KI{u16 vk,scan;DWORD flags,time;up extra;};struct INPUT{DWORD type;union{MI mi;KI ki;};};
// Win32 x64 SDK ABI: prove the marker is written/read at the correct offset.
static_assert(sizeof(MOUSEHOOK)==32&&offsetof(MOUSEHOOK,extra)==24);
static_assert(sizeof(KEYHOOK)==24&&offsetof(KEYHOOK,extra)==16);
static_assert(sizeof(MI)==32&&offsetof(MI,extra)==24);
static_assert(sizeof(KI)==24&&offsetof(KI,extra)==16);
static_assert(sizeof(INPUT)==40&&offsetof(INPUT,mi)==8&&offsetof(INPUT,ki)==8);
struct NID{DWORD cbSize;HWND hWnd;UINT uID,uFlags,uCallbackMessage;HICON hIcon;W szTip[128];DWORD dwState,dwStateMask;W szInfo[256];UINT version;W szInfoTitle[64];DWORD dwInfoFlags;u8 guid[16];HICON hBalloonIcon;};
struct MONITOR{DWORD cbSize;RECT rcMonitor,rcWork;DWORD dwFlags;};
struct MINMAX{POINT reserved,maxSize,maxPos,minTrack,maxTrack;};
struct OPENFILE{DWORD lStructSize;HWND hwndOwner;HINSTANCE hInstance;const W* lpstrFilter;W* lpstrCustomFilter;DWORD nMaxCustFilter,nFilterIndex;W* lpstrFile;DWORD nMaxFile;W* lpstrFileTitle;DWORD nMaxFileTitle;const W* lpstrInitialDir;const W* lpstrTitle;DWORD Flags;u16 nFileOffset,nFileExtension;const W* lpstrDefExt;LP lCustData;void* lpfnHook;const W* lpTemplateName;void* pvReserved;DWORD dwReserved,FlagsEx;};
API const W* GetCommandLineW(); API H GetModuleHandleW(const W*); API DWORD GetModuleFileNameW(H,W*,DWORD); API void ExitProcess(UINT); API B TerminateProcess(H,UINT); API H GetProcessHeap(); API void* HeapAlloc(H,DWORD,up); API B HeapFree(H,DWORD,void*); API H CreateEventW(void*,B,B,const W*); API B SetEvent(H); API H CreateThread(void*,up,THREAD,void*,DWORD,DWORD*); API DWORD GetCurrentThreadId(); API DWORD GetCurrentProcessId(); API DWORD WaitForSingleObject(H,DWORD); API B CloseHandle(H); API void Sleep(DWORD); API H CreateMutexW(void*,B,const W*); API DWORD GetLastError(); API H OpenProcess(DWORD,B,DWORD); API B QueryFullProcessImageNameW(H,DWORD,W*,DWORD*); API H CreateFileW(const W*,DWORD,DWORD,void*,DWORD,DWORD,H); API B ReadFile(H,void*,DWORD,DWORD*,void*); API B WriteFile(H,const void*,DWORD,DWORD*,void*); API B FlushFileBuffers(H); API B DeleteFileW(const W*); API B MoveFileExW(const W*,const W*,DWORD); API B CopyFileW(const W*,const W*,B); API DWORD GetFileAttributesW(const W*); API int MultiByteToWideChar(UINT,DWORD,const char*,int,W*,int); API int WideCharToMultiByte(UINT,DWORD,const W*,int,char*,int,const char*,B*); extern "C" long _InterlockedExchange(volatile long*,long); extern "C" long _InterlockedIncrement(volatile long*); extern "C" long _InterlockedExchangeAdd(volatile long*,long);
#pragma intrinsic(_InterlockedExchange, _InterlockedIncrement, _InterlockedExchangeAdd)
static inline long InterlockedExchange(volatile long*p,long v){return _InterlockedExchange(p,v);} static inline long InterlockedIncrement(volatile long*p){return _InterlockedIncrement(p);} static inline long InterlockedExchangeAdd(volatile long*p,long v){return _InterlockedExchangeAdd(p,v);} API DWORD GetTickCount(); API DWORD WaitForMultipleObjects(DWORD,const H*,B,DWORD); API DWORD MsgWaitForMultipleObjectsEx(DWORD,const H*,DWORD,DWORD,DWORD); API B QueryPerformanceCounter(long long*); API B QueryPerformanceFrequency(long long*);
static inline u16 (*RegisterClassExW)(const WC*) = nullptr; static inline HWND (*CreateWindowExW)(DWORD,const W*,const W*,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,void*) = nullptr; static inline LR (*DefWindowProcW)(HWND,UINT,WP,LP) = nullptr; static inline LR (*CallWindowProcW)(PROC,HWND,UINT,WP,LP) = nullptr; static inline ip (*SetWindowLongPtrW)(HWND,int,ip) = nullptr; static inline ip (*GetWindowLongPtrW)(HWND,int) = nullptr; static inline B (*DestroyWindow)(HWND) = nullptr; static inline B (*ShowWindow)(HWND,int) = nullptr; static inline B (*UpdateWindow)(HWND) = nullptr; static inline B (*IsWindow)(HWND) = nullptr; static inline B (*IsWindowVisible)(HWND) = nullptr; static inline B (*IsZoomed)(HWND) = nullptr; static inline B (*IsIconic)(HWND) = nullptr; static inline B (*SetForegroundWindow)(HWND) = nullptr; static inline HWND (*GetForegroundWindow)() = nullptr; static inline HWND (*GetAncestor)(HWND,UINT) = nullptr; static inline HWND (*GetLastActivePopup)(HWND) = nullptr; static inline HWND (*WindowFromPoint)(POINT) = nullptr; static inline DWORD (*GetWindowThreadProcessId)(HWND,DWORD*) = nullptr; static inline int (*GetWindowTextW)(HWND,W*,int) = nullptr; static inline int (*GetClassNameW)(HWND,W*,int) = nullptr; static inline B (*GetClientRect)(HWND,RECT*) = nullptr; static inline B (*GetWindowRect)(HWND,RECT*) = nullptr; API B MoveWindow(HWND,int,int,int,int,B); static inline B (*SetWindowPos)(HWND,HWND,int,int,int,int,UINT) = nullptr; static inline H (*BeginDeferWindowPos)(int) = nullptr; static inline H (*DeferWindowPos)(H,HWND,HWND,int,int,int,int,UINT) = nullptr; static inline B (*EndDeferWindowPos)(H) = nullptr; static inline B (*GetMessageW)(MSG*,HWND,UINT,UINT) = nullptr; static inline B (*PeekMessageW)(MSG*,HWND,UINT,UINT,UINT) = nullptr; static inline B (*TranslateMessage)(const MSG*) = nullptr; static inline LR (*DispatchMessageW)(const MSG*) = nullptr; static inline B (*IsDialogMessageW)(HWND,MSG*) = nullptr; static inline void (*PostQuitMessage)(int) = nullptr; static inline B (*PostMessageW)(HWND,UINT,WP,LP) = nullptr; static inline B (*PostThreadMessageW)(DWORD,UINT,WP,LP) = nullptr; static inline LR (*SendMessageW)(HWND,UINT,WP,LP) = nullptr; static inline B (*SetWindowTextW)(HWND,const W*) = nullptr; API B EnableWindow(HWND,B); static inline HWND (*SetFocus)(HWND) = nullptr; API HWND GetFocus(); static inline H (*LoadCursorW)(H,const W*) = nullptr; static inline H (*LoadIconW)(H,const W*) = nullptr; API H LoadImageW(H,const W*,UINT,int,int,UINT); static inline int (*MessageBoxW)(HWND,const W*,const W*,UINT) = nullptr; static inline B (*InvalidateRect)(HWND,const RECT*,B) = nullptr; static inline HDC (*BeginPaint)(HWND,PAINT*) = nullptr; static inline B (*EndPaint)(HWND,const PAINT*) = nullptr; static inline int (*FillRect)(HDC,const RECT*,HBRUSH) = nullptr; static inline int (*DrawTextW)(HDC,const W*,int,RECT*,UINT) = nullptr; API HDC GetDC(HWND); API int ReleaseDC(HWND,HDC); static inline HWND (*SetCapture)(HWND) = nullptr; static inline B (*ReleaseCapture)() = nullptr; static inline B (*GetCursorPos)(POINT*) = nullptr; static inline B (*SetCursorPos)(int,int) = nullptr; static inline short (*GetAsyncKeyState)(int) = nullptr; API short GetKeyState(int); static inline UINT (*MapVirtualKeyW)(UINT,UINT) = nullptr; static inline int (*GetKeyNameTextW)(i32,W*,int) = nullptr; static inline H (*SetWindowsHookExW)(int,HOOK,H,DWORD) = nullptr; static inline B (*UnhookWindowsHookEx)(H) = nullptr; static inline LR (*CallNextHookEx)(H,int,WP,LP) = nullptr; static inline UINT (*SendInput)(UINT,INPUT*,int) = nullptr; static inline B (*RegisterHotKey)(HWND,int,UINT,UINT) = nullptr; API B UnregisterHotKey(HWND,int); static inline UINT (*GetDpiForWindow)(HWND) = nullptr; static inline B (*SetProcessDpiAwarenessContext)(H) = nullptr; static inline int (*GetSystemMetrics)(int) = nullptr; static inline H (*MonitorFromWindow)(HWND,DWORD) = nullptr; static inline B (*GetMonitorInfoW)(H,MONITOR*) = nullptr; API H MonitorFromPoint(POINT,DWORD); static inline B (*SetLayeredWindowAttributes)(HWND,DWORD,u8,DWORD) = nullptr; static inline HWND (*FindWindowW)(const W*,const W*) = nullptr; static inline HMENU (*CreatePopupMenu)() = nullptr; static inline B (*AppendMenuW)(HMENU,UINT,up,const W*) = nullptr; static inline UINT (*TrackPopupMenu)(HMENU,UINT,int,int,int,HWND,const RECT*) = nullptr; static inline B (*DestroyMenu)(HMENU) = nullptr; static inline UINT (*RegisterWindowMessageW)(const W*) = nullptr; static inline B (*LockWorkStation)() = nullptr; static inline B (*OpenClipboard)(HWND) = nullptr; static inline B (*CloseClipboard)() = nullptr; static inline B (*EmptyClipboard)() = nullptr; static inline H (*SetClipboardData)(UINT,H) = nullptr; API H GlobalAlloc(UINT,up); API void* GlobalLock(H); API B GlobalUnlock(H); API H GlobalFree(H); static inline up (*SetTimer)(HWND,up,UINT,void*) = nullptr; static inline B (*KillTimer)(HWND,up) = nullptr; API B ClientToScreen(HWND,POINT*); API B ScreenToClient(HWND,POINT*); API B SetWindowPlacement(HWND,const void*);
static inline HBRUSH (*CreateSolidBrush)(DWORD) = nullptr; static inline H (*SelectObject)(HDC,H) = nullptr; static inline B (*DeleteObject)(H) = nullptr; static inline H (*CreateFontW)(int,int,int,int,int,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,const W*) = nullptr; static inline int (*SetBkMode)(HDC,int) = nullptr; static inline DWORD (*SetTextColor)(HDC,DWORD) = nullptr; static inline DWORD (*SetBkColor)(HDC,DWORD) = nullptr; static inline H (*CreatePen)(int,int,DWORD) = nullptr; static inline B (*MoveToEx)(HDC,int,int,POINT*) = nullptr; static inline B (*LineTo)(HDC,int,int) = nullptr; static inline HDC (*CreateCompatibleDC)(HDC) = nullptr; static inline H (*CreateCompatibleBitmap)(HDC,int,int) = nullptr; static inline B (*BitBlt)(HDC,int,int,int,int,HDC,int,int,DWORD) = nullptr; static inline B (*DeleteDC)(HDC) = nullptr; static inline H (*GetStockObject)(int) = nullptr; static inline B (*RoundRect)(HDC,int,int,int,int,int,int) = nullptr; API B Polyline(HDC,const POINT*,int);
static inline up (*ShellExecuteW)(HWND,const W*,const W*,const W*,const W*,int) = nullptr; static inline B (*Shell_NotifyIconW)(DWORD,NID*) = nullptr; static inline B (*GetOpenFileNameW)(OPENFILE*) = nullptr; static inline B (*GetSaveFileNameW)(OPENFILE*) = nullptr;

API B FreeLibrary(H); API H LoadLibraryExW(const W*,H,DWORD); API void* GetProcAddress(H,const char*);
// Optional standard GDI bitmap transfer used for antialiased corners.
static inline int (*StretchDIBits)(HDC,int,int,int,int,int,int,int,int,const void*,const void*,UINT,DWORD)=nullptr;
static inline bool resolve_gui(){
 H user32=LoadLibraryExW(L"user32.dll",nullptr,0x800); if(!user32){(void)0;return false;}
 RegisterClassExW=reinterpret_cast<decltype(RegisterClassExW)>(GetProcAddress(user32,"RegisterClassExW"));if(!RegisterClassExW){(void)0;return false;}
 CreateWindowExW=reinterpret_cast<decltype(CreateWindowExW)>(GetProcAddress(user32,"CreateWindowExW"));if(!CreateWindowExW){(void)0;return false;}
 DefWindowProcW=reinterpret_cast<decltype(DefWindowProcW)>(GetProcAddress(user32,"DefWindowProcW"));if(!DefWindowProcW){(void)0;return false;}
 CallWindowProcW=reinterpret_cast<decltype(CallWindowProcW)>(GetProcAddress(user32,"CallWindowProcW"));if(!CallWindowProcW){(void)0;return false;}
 SetWindowLongPtrW=reinterpret_cast<decltype(SetWindowLongPtrW)>(GetProcAddress(user32,"SetWindowLongPtrW"));if(!SetWindowLongPtrW){(void)0;return false;}
 GetWindowLongPtrW=reinterpret_cast<decltype(GetWindowLongPtrW)>(GetProcAddress(user32,"GetWindowLongPtrW"));if(!GetWindowLongPtrW){(void)0;return false;}
 DestroyWindow=reinterpret_cast<decltype(DestroyWindow)>(GetProcAddress(user32,"DestroyWindow"));if(!DestroyWindow){(void)0;return false;}
 ShowWindow=reinterpret_cast<decltype(ShowWindow)>(GetProcAddress(user32,"ShowWindow"));if(!ShowWindow){(void)0;return false;}
 UpdateWindow=reinterpret_cast<decltype(UpdateWindow)>(GetProcAddress(user32,"UpdateWindow"));if(!UpdateWindow){(void)0;return false;}
 IsWindow=reinterpret_cast<decltype(IsWindow)>(GetProcAddress(user32,"IsWindow"));if(!IsWindow){(void)0;return false;}
 IsWindowVisible=reinterpret_cast<decltype(IsWindowVisible)>(GetProcAddress(user32,"IsWindowVisible"));if(!IsWindowVisible){(void)0;return false;}
 IsZoomed=reinterpret_cast<decltype(IsZoomed)>(GetProcAddress(user32,"IsZoomed"));if(!IsZoomed){(void)0;return false;}
 IsIconic=reinterpret_cast<decltype(IsIconic)>(GetProcAddress(user32,"IsIconic"));if(!IsIconic){(void)0;return false;}
 SetForegroundWindow=reinterpret_cast<decltype(SetForegroundWindow)>(GetProcAddress(user32,"SetForegroundWindow"));if(!SetForegroundWindow){(void)0;return false;}
 GetForegroundWindow=reinterpret_cast<decltype(GetForegroundWindow)>(GetProcAddress(user32,"GetForegroundWindow"));if(!GetForegroundWindow){(void)0;return false;}
 GetAncestor=reinterpret_cast<decltype(GetAncestor)>(GetProcAddress(user32,"GetAncestor"));if(!GetAncestor){(void)0;return false;}
 GetLastActivePopup=reinterpret_cast<decltype(GetLastActivePopup)>(GetProcAddress(user32,"GetLastActivePopup"));if(!GetLastActivePopup)return false;
 WindowFromPoint=reinterpret_cast<decltype(WindowFromPoint)>(GetProcAddress(user32,"WindowFromPoint"));if(!WindowFromPoint){(void)0;return false;}
 GetWindowThreadProcessId=reinterpret_cast<decltype(GetWindowThreadProcessId)>(GetProcAddress(user32,"GetWindowThreadProcessId"));if(!GetWindowThreadProcessId){(void)0;return false;}
 GetWindowTextW=reinterpret_cast<decltype(GetWindowTextW)>(GetProcAddress(user32,"GetWindowTextW"));if(!GetWindowTextW){(void)0;return false;}
 GetClassNameW=reinterpret_cast<decltype(GetClassNameW)>(GetProcAddress(user32,"GetClassNameW"));if(!GetClassNameW){(void)0;return false;}
 GetClientRect=reinterpret_cast<decltype(GetClientRect)>(GetProcAddress(user32,"GetClientRect"));if(!GetClientRect){(void)0;return false;}
 GetWindowRect=reinterpret_cast<decltype(GetWindowRect)>(GetProcAddress(user32,"GetWindowRect"));if(!GetWindowRect){(void)0;return false;}
 SetWindowPos=reinterpret_cast<decltype(SetWindowPos)>(GetProcAddress(user32,"SetWindowPos"));if(!SetWindowPos){(void)0;return false;}
 BeginDeferWindowPos=reinterpret_cast<decltype(BeginDeferWindowPos)>(GetProcAddress(user32,"BeginDeferWindowPos"));if(!BeginDeferWindowPos){(void)0;return false;}
 DeferWindowPos=reinterpret_cast<decltype(DeferWindowPos)>(GetProcAddress(user32,"DeferWindowPos"));if(!DeferWindowPos){(void)0;return false;}
 EndDeferWindowPos=reinterpret_cast<decltype(EndDeferWindowPos)>(GetProcAddress(user32,"EndDeferWindowPos"));if(!EndDeferWindowPos){(void)0;return false;}
 GetMessageW=reinterpret_cast<decltype(GetMessageW)>(GetProcAddress(user32,"GetMessageW"));if(!GetMessageW){(void)0;return false;}
 PeekMessageW=reinterpret_cast<decltype(PeekMessageW)>(GetProcAddress(user32,"PeekMessageW"));if(!PeekMessageW){(void)0;return false;}
 TranslateMessage=reinterpret_cast<decltype(TranslateMessage)>(GetProcAddress(user32,"TranslateMessage"));if(!TranslateMessage){(void)0;return false;}
 DispatchMessageW=reinterpret_cast<decltype(DispatchMessageW)>(GetProcAddress(user32,"DispatchMessageW"));if(!DispatchMessageW){(void)0;return false;}
 IsDialogMessageW=reinterpret_cast<decltype(IsDialogMessageW)>(GetProcAddress(user32,"IsDialogMessageW"));if(!IsDialogMessageW){(void)0;return false;}
 PostQuitMessage=reinterpret_cast<decltype(PostQuitMessage)>(GetProcAddress(user32,"PostQuitMessage"));if(!PostQuitMessage){(void)0;return false;}
 PostMessageW=reinterpret_cast<decltype(PostMessageW)>(GetProcAddress(user32,"PostMessageW"));if(!PostMessageW){(void)0;return false;}
 PostThreadMessageW=reinterpret_cast<decltype(PostThreadMessageW)>(GetProcAddress(user32,"PostThreadMessageW"));if(!PostThreadMessageW){(void)0;return false;}
 SendMessageW=reinterpret_cast<decltype(SendMessageW)>(GetProcAddress(user32,"SendMessageW"));if(!SendMessageW){(void)0;return false;}
 SetWindowTextW=reinterpret_cast<decltype(SetWindowTextW)>(GetProcAddress(user32,"SetWindowTextW"));if(!SetWindowTextW){(void)0;return false;}
 SetFocus=reinterpret_cast<decltype(SetFocus)>(GetProcAddress(user32,"SetFocus"));if(!SetFocus){(void)0;return false;}
 LoadCursorW=reinterpret_cast<decltype(LoadCursorW)>(GetProcAddress(user32,"LoadCursorW"));if(!LoadCursorW){(void)0;return false;}
 LoadIconW=reinterpret_cast<decltype(LoadIconW)>(GetProcAddress(user32,"LoadIconW"));if(!LoadIconW){(void)0;return false;}
 MessageBoxW=reinterpret_cast<decltype(MessageBoxW)>(GetProcAddress(user32,"MessageBoxW"));if(!MessageBoxW){(void)0;return false;}
 InvalidateRect=reinterpret_cast<decltype(InvalidateRect)>(GetProcAddress(user32,"InvalidateRect"));if(!InvalidateRect){(void)0;return false;}
 BeginPaint=reinterpret_cast<decltype(BeginPaint)>(GetProcAddress(user32,"BeginPaint"));if(!BeginPaint){(void)0;return false;}
 EndPaint=reinterpret_cast<decltype(EndPaint)>(GetProcAddress(user32,"EndPaint"));if(!EndPaint){(void)0;return false;}
 FillRect=reinterpret_cast<decltype(FillRect)>(GetProcAddress(user32,"FillRect"));if(!FillRect){(void)0;return false;}
 DrawTextW=reinterpret_cast<decltype(DrawTextW)>(GetProcAddress(user32,"DrawTextW"));if(!DrawTextW){(void)0;return false;}
 SetCapture=reinterpret_cast<decltype(SetCapture)>(GetProcAddress(user32,"SetCapture"));if(!SetCapture){(void)0;return false;}
 ReleaseCapture=reinterpret_cast<decltype(ReleaseCapture)>(GetProcAddress(user32,"ReleaseCapture"));if(!ReleaseCapture){(void)0;return false;}
 GetCursorPos=reinterpret_cast<decltype(GetCursorPos)>(GetProcAddress(user32,"GetCursorPos"));if(!GetCursorPos){(void)0;return false;}
 SetCursorPos=reinterpret_cast<decltype(SetCursorPos)>(GetProcAddress(user32,"SetCursorPos"));if(!SetCursorPos){(void)0;return false;}
 GetAsyncKeyState=reinterpret_cast<decltype(GetAsyncKeyState)>(GetProcAddress(user32,"GetAsyncKeyState"));if(!GetAsyncKeyState){(void)0;return false;}
 MapVirtualKeyW=reinterpret_cast<decltype(MapVirtualKeyW)>(GetProcAddress(user32,"MapVirtualKeyW"));if(!MapVirtualKeyW){(void)0;return false;}
 GetKeyNameTextW=reinterpret_cast<decltype(GetKeyNameTextW)>(GetProcAddress(user32,"GetKeyNameTextW"));if(!GetKeyNameTextW){(void)0;return false;}
 SetWindowsHookExW=reinterpret_cast<decltype(SetWindowsHookExW)>(GetProcAddress(user32,"SetWindowsHookExW"));if(!SetWindowsHookExW){(void)0;return false;}
 UnhookWindowsHookEx=reinterpret_cast<decltype(UnhookWindowsHookEx)>(GetProcAddress(user32,"UnhookWindowsHookEx"));if(!UnhookWindowsHookEx){(void)0;return false;}
 CallNextHookEx=reinterpret_cast<decltype(CallNextHookEx)>(GetProcAddress(user32,"CallNextHookEx"));if(!CallNextHookEx){(void)0;return false;}
 SendInput=reinterpret_cast<decltype(SendInput)>(GetProcAddress(user32,"SendInput"));if(!SendInput){(void)0;return false;}
 RegisterHotKey=reinterpret_cast<decltype(RegisterHotKey)>(GetProcAddress(user32,"RegisterHotKey"));if(!RegisterHotKey){(void)0;return false;}
 GetDpiForWindow=reinterpret_cast<decltype(GetDpiForWindow)>(GetProcAddress(user32,"GetDpiForWindow"));if(!GetDpiForWindow){(void)0;return false;}
 SetProcessDpiAwarenessContext=reinterpret_cast<decltype(SetProcessDpiAwarenessContext)>(GetProcAddress(user32,"SetProcessDpiAwarenessContext"));if(!SetProcessDpiAwarenessContext){(void)0;return false;}
 GetSystemMetrics=reinterpret_cast<decltype(GetSystemMetrics)>(GetProcAddress(user32,"GetSystemMetrics"));if(!GetSystemMetrics){(void)0;return false;}
 MonitorFromWindow=reinterpret_cast<decltype(MonitorFromWindow)>(GetProcAddress(user32,"MonitorFromWindow"));if(!MonitorFromWindow){(void)0;return false;}
 GetMonitorInfoW=reinterpret_cast<decltype(GetMonitorInfoW)>(GetProcAddress(user32,"GetMonitorInfoW"));if(!GetMonitorInfoW){(void)0;return false;}
 SetLayeredWindowAttributes=reinterpret_cast<decltype(SetLayeredWindowAttributes)>(GetProcAddress(user32,"SetLayeredWindowAttributes"));if(!SetLayeredWindowAttributes){(void)0;return false;}
 FindWindowW=reinterpret_cast<decltype(FindWindowW)>(GetProcAddress(user32,"FindWindowW"));if(!FindWindowW){(void)0;return false;}
 CreatePopupMenu=reinterpret_cast<decltype(CreatePopupMenu)>(GetProcAddress(user32,"CreatePopupMenu"));if(!CreatePopupMenu){(void)0;return false;}
 AppendMenuW=reinterpret_cast<decltype(AppendMenuW)>(GetProcAddress(user32,"AppendMenuW"));if(!AppendMenuW){(void)0;return false;}
 TrackPopupMenu=reinterpret_cast<decltype(TrackPopupMenu)>(GetProcAddress(user32,"TrackPopupMenu"));if(!TrackPopupMenu){(void)0;return false;}
 DestroyMenu=reinterpret_cast<decltype(DestroyMenu)>(GetProcAddress(user32,"DestroyMenu"));if(!DestroyMenu){(void)0;return false;}
 RegisterWindowMessageW=reinterpret_cast<decltype(RegisterWindowMessageW)>(GetProcAddress(user32,"RegisterWindowMessageW"));if(!RegisterWindowMessageW){(void)0;return false;}
 LockWorkStation=reinterpret_cast<decltype(LockWorkStation)>(GetProcAddress(user32,"LockWorkStation"));if(!LockWorkStation){(void)0;return false;}
 OpenClipboard=reinterpret_cast<decltype(OpenClipboard)>(GetProcAddress(user32,"OpenClipboard"));if(!OpenClipboard){(void)0;return false;}
 CloseClipboard=reinterpret_cast<decltype(CloseClipboard)>(GetProcAddress(user32,"CloseClipboard"));if(!CloseClipboard){(void)0;return false;}
 EmptyClipboard=reinterpret_cast<decltype(EmptyClipboard)>(GetProcAddress(user32,"EmptyClipboard"));if(!EmptyClipboard){(void)0;return false;}
 SetClipboardData=reinterpret_cast<decltype(SetClipboardData)>(GetProcAddress(user32,"SetClipboardData"));if(!SetClipboardData){(void)0;return false;}
 SetTimer=reinterpret_cast<decltype(SetTimer)>(GetProcAddress(user32,"SetTimer"));if(!SetTimer){(void)0;return false;}
 KillTimer=reinterpret_cast<decltype(KillTimer)>(GetProcAddress(user32,"KillTimer"));if(!KillTimer){(void)0;return false;}
 H gdi32=LoadLibraryExW(L"gdi32.dll",nullptr,0x800); if(!gdi32){(void)0;return false;}
 CreateSolidBrush=reinterpret_cast<decltype(CreateSolidBrush)>(GetProcAddress(gdi32,"CreateSolidBrush"));if(!CreateSolidBrush){(void)0;return false;}
 SelectObject=reinterpret_cast<decltype(SelectObject)>(GetProcAddress(gdi32,"SelectObject"));if(!SelectObject){(void)0;return false;}
 DeleteObject=reinterpret_cast<decltype(DeleteObject)>(GetProcAddress(gdi32,"DeleteObject"));if(!DeleteObject){(void)0;return false;}
 CreateFontW=reinterpret_cast<decltype(CreateFontW)>(GetProcAddress(gdi32,"CreateFontW"));if(!CreateFontW){(void)0;return false;}
 SetBkMode=reinterpret_cast<decltype(SetBkMode)>(GetProcAddress(gdi32,"SetBkMode"));if(!SetBkMode){(void)0;return false;}
 SetTextColor=reinterpret_cast<decltype(SetTextColor)>(GetProcAddress(gdi32,"SetTextColor"));if(!SetTextColor){(void)0;return false;}
 SetBkColor=reinterpret_cast<decltype(SetBkColor)>(GetProcAddress(gdi32,"SetBkColor"));if(!SetBkColor){(void)0;return false;}
 CreatePen=reinterpret_cast<decltype(CreatePen)>(GetProcAddress(gdi32,"CreatePen"));if(!CreatePen){(void)0;return false;}
 MoveToEx=reinterpret_cast<decltype(MoveToEx)>(GetProcAddress(gdi32,"MoveToEx"));if(!MoveToEx){(void)0;return false;}
 LineTo=reinterpret_cast<decltype(LineTo)>(GetProcAddress(gdi32,"LineTo"));if(!LineTo){(void)0;return false;}
 CreateCompatibleDC=reinterpret_cast<decltype(CreateCompatibleDC)>(GetProcAddress(gdi32,"CreateCompatibleDC"));if(!CreateCompatibleDC){(void)0;return false;}
 CreateCompatibleBitmap=reinterpret_cast<decltype(CreateCompatibleBitmap)>(GetProcAddress(gdi32,"CreateCompatibleBitmap"));if(!CreateCompatibleBitmap){(void)0;return false;}
 BitBlt=reinterpret_cast<decltype(BitBlt)>(GetProcAddress(gdi32,"BitBlt"));if(!BitBlt){(void)0;return false;}
 DeleteDC=reinterpret_cast<decltype(DeleteDC)>(GetProcAddress(gdi32,"DeleteDC"));if(!DeleteDC){(void)0;return false;}
 GetStockObject=reinterpret_cast<decltype(GetStockObject)>(GetProcAddress(gdi32,"GetStockObject"));if(!GetStockObject){(void)0;return false;}
 StretchDIBits=reinterpret_cast<decltype(StretchDIBits)>(GetProcAddress(gdi32,"StretchDIBits"));
 RoundRect=reinterpret_cast<decltype(RoundRect)>(GetProcAddress(gdi32,"RoundRect"));if(!RoundRect){(void)0;return false;}
 H shell32=LoadLibraryExW(L"shell32.dll",nullptr,0x800); if(!shell32){(void)0;return false;}
 ShellExecuteW=reinterpret_cast<decltype(ShellExecuteW)>(GetProcAddress(shell32,"ShellExecuteW"));if(!ShellExecuteW){(void)0;return false;}
 Shell_NotifyIconW=reinterpret_cast<decltype(Shell_NotifyIconW)>(GetProcAddress(shell32,"Shell_NotifyIconW"));if(!Shell_NotifyIconW){(void)0;return false;}
 return true;
}

static inline bool resolve_dialogs(){if(GetOpenFileNameW&&GetSaveFileNameW)return true;
 H comdlg32=LoadLibraryExW(L"comdlg32.dll",nullptr,0x800); if(!comdlg32){(void)0;return false;}
 GetOpenFileNameW=reinterpret_cast<decltype(GetOpenFileNameW)>(GetProcAddress(comdlg32,"GetOpenFileNameW"));if(!GetOpenFileNameW){(void)0;return false;}
 GetSaveFileNameW=reinterpret_cast<decltype(GetSaveFileNameW)>(GetProcAddress(comdlg32,"GetSaveFileNameW"));if(!GetSaveFileNameW){(void)0;return false;}
return true;}
