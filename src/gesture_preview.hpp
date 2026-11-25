#pragma once
#include "recognition.hpp"
// UI-only thumbnail geometry. Fit the actual stored sample uniformly, with
// padding for the arrowhead; never modify the recognition data.
namespace mgpreview {
inline int smaller(int a,int b){return a<b?a:b;}
inline int larger(int a,int b){return a>b?a:b;}
inline int absolute(int a){return a<0?-a:a;}
inline int roundedDivide(long long n,int d){return (int)(n>=0?(n+d/2)/d:-((-n+d/2)/d));}
inline bool fit(const P* sample,P* points,P* head,int width,int height,int arrow,int pad){
 if(!sample||width<=2*pad||height<=2*pad||arrow<1||pad<arrow)return false;
 int l=sample[0].x,r=l,t=sample[0].y,b=t;
 for(int i=1;i<N;++i){l=smaller(l,sample[i].x);r=larger(r,sample[i].x);t=smaller(t,sample[i].y);b=larger(b,sample[i].y);}
 int w=r-l,h=b-t;if(!w&&!h)return false;
 int availW=width-1-2*pad,availH=height-1-2*pad;
 if(availW<1||availH<1)return false;
 int num,den;
 if(!h||(w&&(long long)availW*h<=(long long)availH*w)){num=availW;den=w;}else{num=availH;den=h;}
 int usedW=roundedDivide((long long)w*num,den),usedH=roundedDivide((long long)h*num,den);
 int ox=(width-1-usedW)/2,oy=(height-1-usedH)/2;
 for(int i=0;i<N;++i)points[i]={ox+roundedDivide((long long)(sample[i].x-l)*num,den),oy+roundedDivide((long long)(sample[i].y-t)*num,den)};
 P tip=points[N-1];int k=N-2;
 while(k>=0&&larger(absolute(tip.x-points[k].x),absolute(tip.y-points[k].y))<arrow)--k;
 if(k<0){k=N-2;while(k>=0&&tip.x==points[k].x&&tip.y==points[k].y)--k;}
 if(k<0)return false;
 int dx=tip.x-points[k].x,dy=tip.y-points[k].y,mag=larger(absolute(dx),absolute(dy));
 int vx=roundedDivide((long long)dx*arrow,mag),vy=roundedDivide((long long)dy*arrow,mag);
 // The diagonal wing may exceed the nominal arrow radius by a pixel.
 auto bound=[](int v,int n){return v<0?0:v>=n?n-1:v;};
 head[0]={bound(tip.x-vx-vy/2,width),bound(tip.y-vy+vx/2,height)};
 head[1]=tip;
 head[2]={bound(tip.x-vx+vy/2,width),bound(tip.y-vy-vx/2,height)};
 return true;
}
}
