#pragma once
// Keep native focus cues hidden without disabling keyboard navigation or
// changing accelerator visibility. Only applies to our own window trees.
namespace mgfocus {
constexpr unsigned change=0x127, update=0x128, query=0x129;
constexpr unsigned set=1, clear=2, initialize=3, hide=0x10000;
constexpr unsigned hideRequest=set|hide;
constexpr bool isState(unsigned msg){return msg==change||msg==update;}
constexpr unsigned filter(unsigned request){return (request&0xffff)==clear?request&~hide:request;}
constexpr bool blocked(unsigned request){return (request&0xffff)==clear&&(filter(request)>>16)==0&&(request&hide)!=0;}
}
