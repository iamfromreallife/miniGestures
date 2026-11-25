#pragma once
// The editor supports at most 16 samples per gesture; no heap allocation.
namespace mgshapes {
struct Selection {
 unsigned mask=0; int anchor=-1;
 static unsigned bit(int i){return i>=0&&i<16?1u<<i:0;}
 bool has(int i)const{return (mask&bit(i))!=0;}
 void single(int i){mask=bit(i);anchor=i;}
 void click(int i,bool ctrl,bool shift){if(i<0){if(!ctrl&&!shift)single(-1);return;}if(shift&&anchor>=0){unsigned range=0;int a=anchor<i?anchor:i,b=anchor>i?anchor:i;for(int j=a;j<=b;j++)range|=bit(j);mask=ctrl?mask|range:range;}else if(ctrl){mask^=bit(i);anchor=i;}else single(i);}
};
struct Box {int l,t,r,b;};
inline Box bounds(int x,int y,int x2,int y2){return {x<x2?x:x2,y<y2?y:y2,(x>x2?x:x2)+1,(y>y2?y:y2)+1};}
inline bool intersects(Box a,Box b){return a.l<b.r&&b.l<a.r&&a.t<b.b&&b.t<a.b;}
inline unsigned dragged(unsigned initial,unsigned hits,bool ctrl,bool shift){return ctrl?initial^hits:shift?initial|hits:hits;}
template<class Copy> int compact(int count,unsigned mask,Copy copy){int out=0;for(int i=0;i<count;i++)if(!(mask&Selection::bit(i))){if(out!=i)copy(out,i);++out;}return out;}
}
