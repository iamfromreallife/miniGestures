#pragma once
// Center the visible frame in the monitor work area; SetWindowPos takes the
// outer (potentially invisible resize-border) coordinates instead.
static inline bool centeredOrigin(const RECT& work,const RECT& outer,const RECT& visible,int& x,int& y){
 const int w=visible.right-visible.left,h=visible.bottom-visible.top;
 if(w<=0||h<=0||work.right<=work.left||work.bottom<=work.top||outer.right<=outer.left||outer.bottom<=outer.top)return false;
 x=work.left+(work.right-work.left-w)/2-(visible.left-outer.left);
 y=work.top+(work.bottom-work.top-h)/2-(visible.top-outer.top);
 return true;
}
