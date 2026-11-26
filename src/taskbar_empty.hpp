#pragma once
// Exact, recent empty-space proof only. No UI Automation or shell calls here.
namespace mgtaskbar {
constexpr unsigned hoverPollMs=35,refreshMs=150,maxAgeMs=350;
constexpr bool allows(bool empty,bool sameRoot,int cachedX,int cachedY,int x,int y,unsigned age){
 return empty&&sameRoot&&cachedX==x&&cachedY==y&&age<=maxAgeMs;
}
static_assert(allows(true,true,-50,700,-50,700,0));
static_assert(allows(true,true,20,30,20,30,maxAgeMs));
static_assert(!allows(true,true,20,30,20,30,maxAgeMs+1));
static_assert(!allows(false,true,20,30,20,30,0));
static_assert(!allows(true,false,20,30,20,30,0));
static_assert(!allows(true,true,20,30,21,30,0));
static_assert(!allows(true,true,20,30,20,31,0));
}
