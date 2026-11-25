#pragma once
static inline void keyname(int vk,int m,W*out,int cap){out[0]=0;if(!vk)return;if(m&2)cat(out,L"Ctrl+",cap);if(m&1)cat(out,L"Alt+",cap);if(m&4)cat(out,L"Shift+",cap);if(m&8)cat(out,L"Win+",cap);W b[64]{};if(vk>=0x70&&vk<=0x87){cp(b,L"F",64);number(b+1,vk-0x6f);}else if(vk>='A'&&vk<='Z'||vk>='0'&&vk<='9'){b[0]=(W)vk;b[1]=0;}else{UINT scan=MapVirtualKeyW(vk,0);if(vk>=0x21&&vk<=0x2e||vk==0x6f)scan|=0x100;GetKeyNameTextW(scan<<16,b,64);if(!*b){cp(b,L"VK",64);number(b+2,vk);}}cat(out,b,cap);}
// Store VK codes rather than translated key names; display can follow keyboard layout.
static inline void canonicalHotkey(int vk,int m,W*out,int cap){out[0]=0;if(vk<=0||vk>255)return;if(m&2)cat(out,L"Ctrl+",cap);if(m&1)cat(out,L"Alt+",cap);if(m&4)cat(out,L"Shift+",cap);if(m&8)cat(out,L"Win+",cap);cat(out,L"VK",cap);W n[16];number(n,vk);cat(out,n,cap);}
static inline bool modifierKey(int vk){return vk==0x10||vk==0x11||vk==0x12||vk==0x5b||vk==0x5c||(vk>=0xa0&&vk<=0xa5);}
static inline bool parsekey(const W*s,int&vk,int&m){vk=0;m=0;W seg[64];while(*s){int n=0;while(*s&&*s!=L'+'&&n<63)seg[n++]=*s++;seg[n]=0;if(*s==L'+')s++;while(n&&seg[n-1]==L' ')seg[--n]=0;W*t=seg;while(*t==L' ')t++;if(eq(t,L"Ctrl")||eq(t,L"Control"))m|=2;else if(eq(t,L"Alt"))m|=1;else if(eq(t,L"Shift"))m|=4;else if(eq(t,L"Win"))m|=8;else{if(vk)return false;if(len(t)==1){W c=t[0];if(c>=L'a'&&c<=L'z')c-=32;vk=c;}else if(fold(t[0])==L'f'&&num(t+1)>=1&&num(t+1)<=24)vk=0x6f+num(t+1);else if(fold(t[0])==L'v'&&fold(t[1])==L'k')vk=num(t+2);else for(int i=1;i<256;i++){W name[64];keyname(i,0,name,64);if(eq(t,name)){vk=i;break;}}}}return vk>0&&vk<256;}

// Native Windows capture shortcuts must not be consumed by gesture bindings.
static inline bool screenshotShortcut(int vk,int modifiers){return vk==0x2c||(vk=='S'&&modifiers==12);}
struct ScreenshotKeyPass {
 bool snipHeld=false;
 bool pass(int vk,bool down,bool up,int modifiers){
  if(vk==0x2c)return true; // VK_SNAPSHOT: including Alt/Win variants.
  if(vk!='S')return false;
  if(snipHeld){if(up)snipHeld=false;return true;}
  if(down&&screenshotShortcut(vk,modifiers)){snipHeld=true;return true;}
  return false;
 }
};

// Own-window Print Screen routing: keep a complete physical key sequence
// together, even when focus or modifier state changes before release.
// A number of keyboard drivers deliver VK_SNAPSHOT as a release-only event.
struct OwnPrintScreen {
 enum Decision { Pass, Consume, Launch };
 enum State { Idle, Native, Routed } state=Idle;
 bool needsPolicy(bool down,bool up) const { return state==Idle&&(down||up); }
 Decision edge(bool down,bool up,bool route){
  if(down){if(state==Idle)state=route?Routed:Native;return state==Routed?Consume:Pass;}
  if(up){const bool launch=state==Routed||(state==Idle&&route);state=Idle;return launch?Launch:Pass;}
  return Pass;
 }
};
