#pragma once
#include <cmath>
// Independent continuous-angle recognizer. Storage format remains unchanged.
namespace mgangular {
constexpr int points=100, segments=points-1;
constexpr double pi=3.1415926535897932384626433832795;
struct Signature { double angle[segments]; bool valid; };
template<class P> inline double distance(P a,P b){
 const double x=(double)b.x-a.x,y=(double)b.y-a.y;
 return std::sqrt(x*x+y*y);
}
template<class P> inline bool reached(P a,P b,int limit){
 const double x=(double)b.x-a.x,y=(double)b.y-a.y;
 return x*x+y*y>=(double)limit*limit;
}
template<class P> inline Signature signature(const P* p,int n){
 Signature result{};if(!p||n<2)return result;
 double total=0;for(int i=1;i<n;i++)total+=distance(p[i-1],p[i]);
 if(!(total>0))return result;
 int j=1;double run=0,d=distance(p[0],p[1]),px=p[0].x,py=p[0].y;
 for(int k=1;k<points;k++){
  const double target=total*k/(points-1);
  while(j<n-1&&(d==0||run+d<target)){run+=d;++j;d=distance(p[j-1],p[j]);}
  double t=d?(target-run)/d:1;if(t<0)t=0;if(t>1)t=1;
  double x=p[j-1].x+((double)p[j].x-p[j-1].x)*t;
  double y=p[j-1].y+((double)p[j].y-p[j-1].y)*t;
  if(k==points-1){x=p[n-1].x;y=p[n-1].y;}
  const double dx=x-px,dy=y-py;
  // If equal-distance samples land on exactly the same coordinate during
  // a reversal, use the actual local stroke direction, not atan2(0,0).
  result.angle[k-1]=(dx||dy)?std::atan2(dy,dx):
   (d?std::atan2((double)p[j].y-p[j-1].y,(double)p[j].x-p[j-1].x):
    (k>1?result.angle[k-2]:0.0));
  px=x;py=y;
 }
 result.valid=true;return result;
}
inline double similarity(const Signature& a,const Signature& b){
 if(!a.valid||!b.valid)return -1;
 double error=0;for(int i=0;i<segments;i++){
  double delta=std::fabs(a.angle[i]-b.angle[i]);
  if(delta>pi)delta=2*pi-delta;
  error+=delta;
 }
 double score=100*(1-error/(segments*pi));
 return score<0?0:score>100?100:score;
}
inline long long rank(double score){return score<0?-1:(long long)(score*10000000+0.5);}
inline int percent(double score){return score<0?-1:(int)(score+0.5);}
}
