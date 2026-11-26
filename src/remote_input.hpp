#pragma once
namespace mgremote {
// WH_MOUSE_LL: LLMHF_INJECTED=1, LLMHF_LOWER_IL_INJECTED=2.
// Reject our own tagged input unconditionally; opt in to external injected input.
constexpr bool accepts(unsigned flags,bool own,bool allowExternal){
 return !own&&(!(flags&3u)||allowExternal);
}
// The marker is the primary identity check. Some input bridges strip or
// truncate dwExtraInfo. A replay must not become a fresh gesture in that case.
// This state is owned exclusively by the hook thread, including callbacks
// reentered while SendInput runs. No sleeps, waits, or recursive injection.
struct ReplayGuard {
 static constexpr unsigned echoWindowMs=100;
 bool active=false,positionKnown=false;
 int button=0,phase=0,x=0,y=0;
 unsigned finished=0;
 constexpr void begin(int b,int px,int py,bool known){
  active=true;button=b;phase=1;x=px;y=py;positionKnown=known;
 }
 constexpr void end(unsigned now,bool success){
  active=false;finished=now;if(!success||!positionKnown)phase=0;
 }
 constexpr bool bypass(unsigned flags,bool own,int b,bool down,bool up,int px,int py,unsigned now){
  // Track the expected pair even when the ordinary marker is intact.
  bool injected=(flags&3u)!=0;
  if(!active&&phase&&now-finished>echoWindowMs)phase=0;
  if(!active&&!injected&&!own&&b){phase=0;return false;}
  bool expected=phase&&b==button&&((phase==1&&down)||(phase==2&&up));
  bool match=expected&&(own||injected)&&(active||(positionKnown&&px==x&&py==y));
  if(match){phase=phase==1?2:0;}
  // Every injected edge during our synchronous replay passes through.
  // A late, untagged expected edge is allowed once at the same position.
  // A new physical click never matches this fallback.
  return own||(active&&injected)||match;
 }
};
constexpr bool replayRegression(){
 ReplayGuard g;
 g.begin(2,100,200,true);
 if(!g.bypass(1,false,2,true,false,100,200,10))return false;
 if(!g.bypass(1,false,2,false,true,100,200,10))return false;
 g.end(10,true);
 if(g.bypass(1,false,2,true,false,100,200,11))return false;
 g.begin(2,100,200,true);g.end(20,true);
 if(g.bypass(1,false,2,true,false,101,200,21))return false;
 if(!g.bypass(1,false,2,true,false,100,200,21))return false;
 if(!g.bypass(1,false,2,false,true,100,200,22))return false;
 if(g.bypass(1,false,2,true,false,100,200,23))return false;
 g.begin(2,100,200,true);g.end(30,true);
 if(g.bypass(0,false,2,true,false,100,200,31))return false;
 if(g.bypass(1,false,2,true,false,100,200,32))return false;
 g.begin(2,100,200,true);g.end(40,true);
 if(g.bypass(1,false,2,true,false,100,200,141))return false;
 g.begin(2,100,200,true);
 if(!g.bypass(1,true,2,true,false,100,200,142))return false;
 if(!g.bypass(1,true,2,false,true,100,200,142))return false;
 g.end(142,true);
 if(g.bypass(1,false,2,true,false,100,200,143))return false;
 return true;
}
static_assert(replayRegression());
static_assert(accepts(0,false,false));
static_assert(!accepts(1,false,false));
static_assert(!accepts(3,false,false));
static_assert(accepts(1,false,true));
static_assert(accepts(3,false,true));
static_assert(!accepts(0,true,true));
static_assert(!accepts(1,true,true));
}
