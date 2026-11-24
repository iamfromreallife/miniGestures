#pragma once
// All measurements are physical pixels. Center the text line rather than the
// EDIT window: native single-line EDITs need extra room for their caret.
namespace mgtext {
struct Band { int top, height; };
constexpr int min(int a,int b){return a<b?a:b;}
constexpr int max(int a,int b){return a>b?a:b;}
constexpr int centeredTop(int outer,int text){return max(0,(outer-text+1)/2);}
constexpr Band editBand(int outer,int text,int spare,int nativeTop=0){
 int margin=outer>2?1:0;
 int room=max(1,outer-2*margin);
 int height=min(room,max(1,text+max(0,spare)));
 int top=max(margin,min(outer-margin-height,centeredTop(outer,text)-nativeTop));
 return {top,height};
}
}
