#pragma once
// Reserve room for the author credit and version at all supported DPIs.
static constexpr int footerVersionWidth=190;
// Pure viewport geometry, independent of Win32 and scroll offset.
struct FooterGeometry {int footerY,statusX,statusW,versionX,versionW,textH,scrollX,scrollY;};
static inline FooterGeometry footerGeometry(int width,int height,int canvasW,int canvasH,int footerH,int margin,int versionW,int gap,int textH,int sx,int sy){
 auto max=[](int a,int b){return a>b?a:b;};auto min=[](int a,int b){return a<b?a:b;};
 int y=max(0,height-footerH),m=min(margin,max(0,(width-2)/2));
 int vw=max(1,min(versionW,max(1,width-2*m-gap-1)));int vx=max(m,width-m-vw);
 return {y,m,max(1,vx-m-gap),vx,vw,max(1,min(textH,height-y)),max(0,min(sx,canvasW-width)),max(0,min(sy,canvasH-footerH-y))};
}
