#pragma once
// Use the standard title-bar close command for normal windows. Modal owners
// may be disabled: prefer their visible, enabled, same-process active popup.
static inline bool closeTargetWindow(const Target&t){if(!valid(t))return false;HWND h=t.hwnd;if(!IsWindowEnabled(h)){HWND p=GetLastActivePopup(h);DWORD pid=0;if(p&&p!=h&&IsWindow(p)&&IsWindowVisible(p)&&IsWindowEnabled(p)&&GetWindowThreadProcessId(p,&pid)&&pid==t.pid)h=p;else return false;}DWORD pid=0;if(!IsWindow(h)||!GetWindowThreadProcessId(h,&pid)||pid!=t.pid)return false;auto style=GetWindowLongPtrW(h,-16);if(style&0x80000){if(PostMessageW(h,0x112,0xf060,0))return true;if(GetLastError()==5)return false;}return PostMessageW(h,0x10,0,0)!=0;}

