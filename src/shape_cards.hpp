#pragma once
// A compact, DPI-independent card grid. The same geometry drives paint and hit testing.
namespace mgcards {
struct Rect { int left,top,right,bottom; };
inline int smaller(int a,int b){return a<b?a:b;}
struct Grid {
 int columns=0,width=0,height=0,gap=0,pad=0,count=0;
 Rect card(int i) const {
  int x=pad+(i%columns)*(width+gap),y=pad+(i/columns)*(height+gap);
  return {x,y,x+width,y+height};
 }
 int hit(int x,int y) const {
  for(int i=0;i<count;i++){auto r=card(i);if(x>=r.left&&x<r.right&&y>=r.top&&y<r.bottom)return i;}
  return -1;
 }
};
inline Grid fit(int width,int height,int count,int pad,int gap,int maxW,int maxH){
 Grid result{};result.gap=gap;result.pad=pad;
 if(count<=0||width<=2*pad||height<=2*pad)return result;
 int best=-1;
 for(int columns=1;columns<=count;columns++){
  int rows=(count+columns-1)/columns;
  int w=smaller(maxW,(width-2*pad-(columns-1)*gap)/columns);
  int h=smaller(maxH,(height-2*pad-(rows-1)*gap)/rows);
  if(w<=0||h<=0)continue;
  int score=smaller(w,h);
  if(score>best||(score==best&&w*h>result.width*result.height)){
   result.columns=columns;result.width=w;result.height=h;result.count=count;best=score;
  }
 }
 return result;
}
}
