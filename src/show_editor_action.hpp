#pragma once
// The existing UI-thread open message also supports lazy tray-only startup.
// Never send the command to another process with a matching class name.
static constexpr unsigned SHOW_EDITOR_ABOVE=0;
static inline bool showMiniGesturesWindow(){
 HWND h=FindWindowW(L"miniGesturesController",nullptr);if(!h)h=FindWindowW(L"miniGesturesMain",nullptr);DWORD pid=0;
 return h&&GetWindowThreadProcessId(h,&pid)&&pid==GetCurrentProcessId()&&PostMessageW(h,0x8011,SHOW_EDITOR_ABOVE,0);
}
