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
// At most 16 cards. Selection bits are independent of card geometry.
using Mask=unsigned;
inline Mask bit(int i){return i>=0&&i<16?1u<<i:0u;}
inline Mask all(int count){return count<=0?0u:count>=16?0xffffu:(1u<<count)-1u;}
inline Mask range(int first,int last){
 if(first>last){int t=first;first=last;last=t;}
 Mask m=0;for(int i=first;i<=last;i++)m|=bit(i);return m;
}
inline Rect box(int x1,int y1,int x2,int y2){
 return {smaller(x1,x2),smaller(y1,y2),x1>x2?x1:x2,y1>y2?y1:y2};
}
inline Mask intersecting(const Grid& grid,Rect r){
 Mask result=0;for(int i=0;i<grid.count;i++){auto q=grid.card(i);
  if(q.left<r.right&&q.right>r.left&&q.top<r.bottom&&q.bottom>r.top)result|=bit(i);
 }return result;
}
inline Mask click(Mask base,int anchor,int hit,bool ctrl,bool shift){
 if(hit<0)return ctrl||shift?base:0u;
 if(shift){Mask r=range(anchor<0?hit:anchor,hit);return ctrl?base|r:r;}
 return ctrl?base^bit(hit):bit(hit);
}
inline Mask marquee(Mask base,Mask hits,bool ctrl,bool shift){
 return ctrl?(shift?base|hits:base^hits):shift?base|hits:hits;
}

}
