#pragma once
namespace mgremote {
// WH_MOUSE_LL: LLMHF_INJECTED=1, LLMHF_LOWER_IL_INJECTED=2.
// Reject our own tagged input unconditionally; opt in to external injected input.
constexpr bool accepts(unsigned flags,bool own,bool allowExternal){
 return !own&&(!(flags&3u)||allowExternal);
}
static_assert(accepts(0,false,false));
static_assert(!accepts(1,false,false));
static_assert(!accepts(3,false,false));
static_assert(accepts(1,false,true));
static_assert(accepts(3,false,true));
static_assert(!accepts(0,true,true));
static_assert(!accepts(1,true,true));
}
