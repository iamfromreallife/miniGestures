#pragma once
// Private shell COM ABI. Query an exact interface IID before calling any slot.
// Windows 11 24H2+ layout cross-checked with VirtualDesktopAccessor and
// MScholtes/VirtualDesktop; older layout IDs from Virtual Desktop Helper.
// No keyboard or animated fallback: failure leaves the current desktop alone.
namespace directDesktop {
struct Guid { unsigned int a; unsigned short b,c; unsigned char d[8]; };
static_assert(sizeof(Guid)==16,"COM GUID ABI");
constexpr Guid shell={0xc2f03a33,0x21f5,0x47fa,{0xb4,0xbb,0x15,0x63,0x62,0xa2,0xf2,0x39}};
constexpr Guid provider={0x6d5140c1,0x7436,0x11ce,{0x80,0x34,0x00,0xaa,0x00,0x60,0x09,0xfa}};
constexpr Guid service={0xc5e0cdca,0x7b6e,0x41b2,{0x9f,0xc4,0xd9,0x39,0x75,0xcc,0x46,0x7b}};
constexpr Guid desktop11={0x3f07f4be,0xb107,0x441a,{0xaf,0x0f,0x39,0xd8,0x25,0x29,0x07,0x2c}};
struct Layout { Guid manager,desktop; bool monitor; };
constexpr Layout layouts[]={
 {{0x53f5ca0b,0x158f,0x4124,{0x90,0x0c,0x05,0x71,0x58,0x06,0x0b,0x27}},desktop11,false},
 {{0xa3175f2d,0x239c,0x4bd2,{0x8a,0xa0,0xee,0xba,0x8b,0x0b,0x13,0x8e}},desktop11,false},
 {{0xb2f925b9,0x5a0f,0x4d2e,{0x9f,0x4d,0x2b,0x15,0x07,0x59,0x3c,0x10}},{0x536d3495,0xb208,0x4cc9,{0xae,0x26,0xde,0x81,0x11,0x27,0x5b,0xf8}},true},
 {{0xf31574d6,0xb682,0x4cdc,{0xbd,0x56,0x18,0x27,0x86,0x0a,0xbe,0xc6}},{0xff72ffdd,0xbe7e,0x43fc,{0x9c,0x03,0xad,0x81,0x68,0x1e,0x88,0xe4}},false}
};
static inline bool same(const Guid&a,const Guid&b){if(a.a!=b.a||a.b!=b.b||a.c!=b.c)return false;for(int i=0;i<8;i++)if(a.d[i]!=b.d[i])return false;return true;}
// x64 Windows uses the same calling convention for all COM methods.
template<class F> static F method(void*obj,int slot){return reinterpret_cast<F>((*reinterpret_cast<void***>(obj))[slot]);}
struct Ref {void*p=nullptr;~Ref(){if(p)method<unsigned int(*)(void*)>(p,2)(p);}Ref()=default;Ref(const Ref&)=delete;Ref&operator=(const Ref&)=delete;};
using Out=int(*)(void*,void**); using MonitorOut=int(*)(void*,void*,void**);
using GetId=int(*)(void*,Guid*);
// Called on an initialized COM worker thread, not the input hook.
template<class Stop,class Prepare> static bool edge(void*sp,bool last,Stop stop,Prepare prepare){
 if(!sp||stop())return false;
 for(const auto&layout:layouts){
  Ref manager;int hr=method<int(*)(void*,const Guid*,const Guid*,void**)>(sp,3)(sp,&service,&layout.manager,&manager.p);
  if(hr<0||!manager.p)continue;
  Ref desktops,current,target;
  hr=layout.monitor?method<MonitorOut>(manager.p,7)(manager.p,nullptr,&desktops.p):method<Out>(manager.p,7)(manager.p,&desktops.p);
  if(hr<0||!desktops.p)return false;
  unsigned int count=0;
  if(method<int(*)(void*,unsigned int*)>(desktops.p,3)(desktops.p,&count)<0||!count)return false;
  if(count==1)return true;
  if(method<int(*)(void*,unsigned int,const Guid*,void**)>(desktops.p,4)(desktops.p,last?count-1:0,&layout.desktop,&target.p)<0||!target.p)return false;
  hr=layout.monitor?method<MonitorOut>(manager.p,6)(manager.p,nullptr,&current.p):method<Out>(manager.p,6)(manager.p,&current.p);
  if(hr<0||!current.p)return false;
  Guid a{},b{};
  if(method<GetId>(target.p,4)(target.p,&a)<0||method<GetId>(current.p,4)(current.p,&b)<0)return false;
  if(stop())return false;
  if(same(a,b))return true;
  // Allow shell foreground handoff only for an actual switch, after matching.
  prepare(sp);
  if(stop())return false;
  // SwitchDesktop (slot 9), not SwitchDesktopWithAnimation. Never visits intermediate desktops.
  return (layout.monitor?method<int(*)(void*,void*,void*)>(manager.p,9)(manager.p,nullptr,target.p):method<int(*)(void*,void*)>(manager.p,9)(manager.p,target.p))>=0;
 }
 return false;
}
template<class Stop> static bool edge(void*sp,bool last,Stop stop){return edge(sp,last,stop,[](void*){});}
}
