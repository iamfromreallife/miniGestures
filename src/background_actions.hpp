#pragma once
// All application-directed input is addressed to the captured HWND. Never
// activate an inactive application merely to deliver an action.
struct GuiThreadInfo {DWORD size,flags;HWND active,focus,capture,menuOwner,moveSize,caret;RECT caretRect;};
API B GetGUIThreadInfo(DWORD,GuiThreadInfo*);
static_assert(sizeof(GuiThreadInfo)==72,"GUITHREADINFO x64 ABI");
static inline bool belongsToTarget(HWND h,const Target&t){DWORD pid=0;return h&&IsWindow(h)&&GetWindowThreadProcessId(h,&pid)&&pid==t.pid&&(h==t.hwnd||GetAncestor(h,2)==t.hwnd);}
static inline HWND pointerChild(const Target&t){return belongsToTarget(t.child,t)?t.child:t.hwnd;}
static inline HWND keyboardChild(const Target&t){DWORD tid=GetWindowThreadProcessId(t.hwnd,nullptr);GuiThreadInfo info{};info.size=sizeof(info);if(tid&&GetGUIThreadInfo(tid,&info)&&belongsToTarget(info.focus,t))return info.focus;return pointerChild(t);}
static inline bool targetIsForeground(const Target&t){HWND f=GetForegroundWindow();return f&&(f==t.hwnd||GetAncestor(f,2)==t.hwnd);}
static inline bool postKey(HWND h,int vk,bool up,bool alt){UINT scan=MapVirtualKeyW(vk,0);unsigned int bits=1|((scan&255)<<16);if((vk>=0x21&&vk<=0x2e)||vk==0x5b||vk==0x5c||vk==0x6f||vk==0xa3||vk==0xa5)bits|=1u<<24;if(alt)bits|=1u<<29;if(up)bits|=3u<<30;return PostMessageW(h,alt?(up?0x105:0x104):(up?0x101:0x100),vk,(LP)(int)bits)!=0;}
static inline bool appCommand(const Target&t,int code){return valid(t)&&PostMessageW(t.hwnd,0x319,(WP)pointerChild(t),(LP)((u32)code<<16))!=0;}
static inline bool shellShortcut(int vk,int m){return (m==8&&(vk=='D'||vk=='E'||vk=='I'||vk==0x09))||(m==10&&(vk==0x25||vk==0x27||vk=='D'||vk==0x73))||(m==12&&vk=='S');}
static inline bool snapTarget(HWND h,bool right);
// Known semantic commands do not rely on the physical keyboard modifier state.
static inline bool targetKey(const Target&t,int vk,int m){if(shellShortcut(vk,m))return sendkey(vk,m);if(!valid(t))return false;if(m==8&&(vk==0x25||vk==0x27))return snapTarget(t.hwnd,vk==0x27);if(targetIsForeground(t))return sendkey(vk,m);if(m==1&&vk==0x73)return closeTargetWindow(t);if(m==1&&vk==0x25)return appCommand(t,1);if(m==1&&vk==0x27)return appCommand(t,2);if(m==0&&vk==0x74)return appCommand(t,3);HWND h=keyboardChild(t);if(m==2&&(vk=='C'||vk=='X'||vk=='V'))return PostMessageW(h,vk=='C'?0x301:vk=='X'?0x300:0x302,0,0)!=0;
 // Windows does not offer a universal background SendInput API. Post an
 // addressed key sequence as a compatibility path; the target may ignore it.
 // Win-key shortcuts are shell/global operations, not background window keys.
 if(m&8)return false;int keys[]={0x12,0x11,0x10};int down=0;bool ok=true;for(int i=0;i<3;i++)if(m&(1<<i)){if(!postKey(h,keys[i],false,i==0)){ok=false;break;}down|=1<<i;}if(ok){ok=postKey(h,vk,false,(m&1)!=0);if(ok)ok=postKey(h,vk,true,(m&1)!=0);}for(int i=2;i>=0;i--)if(down&(1<<i))ok=postKey(h,keys[i],true,(m&1)!=0)&&ok;return ok;}
static inline bool targetText(const Target&t,const W*text){if(!valid(t))return false;HWND h=keyboardChild(t);bool foreground=targetIsForeground(t);for(int i=0;text[i];i++){if(stopActionEvent&&WaitForSingleObject(stopActionEvent,0)==0)return false;if(foreground&&targetIsForeground(t)){INPUT in[2]{};for(int j=0;j<2;j++){in[j].type=1;in[j].ki.scan=text[i];in[j].ki.flags=4|(j?2:0);in[j].ki.extra=MARK;}if(SendInput(2,in,sizeof(INPUT))!=2)return false;}else if(!belongsToTarget(h,t)||!PostMessageW(h,0x102,text[i],1))return false;}return true;}
static inline LP mousePoint(int x,int y){return (LP)((u32)(u16)x|((u32)(u16)y<<16));}
static inline bool targetClick(const Target&t,int b){if(!valid(t))return false;HWND h=pointerChild(t);POINT p{};if(t.hasPoint)p={t.x,t.y};else if(!GetCursorPos(&p))return false;if(!ScreenToClient(h,&p))return false;const UINT down[]={0,0x201,0x204,0x207,0x20b,0x20b},up[]={0,0x202,0x205,0x208,0x20c,0x20c};const WP mask[]={0,1,2,16,32,64};b=clamp(b,1,5);WP extra=b>=4?(WP)(b-3)<<16:0;bool ok=PostMessageW(h,down[b],mask[b]|extra,mousePoint(p.x,p.y))!=0;return PostMessageW(h,up[b],extra,mousePoint(p.x,p.y))&&ok;}
static inline bool targetScroll(const Target&t,int delta,bool horizontal){if(!valid(t))return false;POINT p{};if(t.hasPoint)p={t.x,t.y};else if(!GetCursorPos(&p))return false;return PostMessageW(pointerChild(t),horizontal?0x20e:0x20a,(WP)(u16)delta<<16,mousePoint(p.x,p.y))!=0;}
static inline bool snapTarget(HWND h,bool right){MONITOR mi{sizeof(mi)};if(!GetMonitorInfoW(MonitorFromWindow(h,2),&mi))return false;if(IsZoomed(h)||IsIconic(h))ShowWindow(h,4);int width=mi.rcWork.right-mi.rcWork.left;int x=mi.rcWork.left+(right?width/2:0);return SetWindowPos(h,nullptr,x,mi.rcWork.top,right?width-width/2:width/2,mi.rcWork.bottom-mi.rcWork.top,4|16)!=0;}
