#pragma once
// Pixel reference extracted from the supplied folder screenshot. White matte
// removed at the antialiased outer edge, so the icon works on any row color.
// The 96-DPI white-background rendering reproduces the reference exactly.
namespace mgfolder {
struct Pixel { unsigned r,g,b,a; }; // premultiplied channels
constexpr int width=16,height=12;
constexpr Pixel palette[]={
 {216,169,83,255},
 {219,169,73,255},
 {221,172,81,255},
 {221,173,82,255},
 {222,173,79,255},
 {222,174,79,255},
 {223,176,86,255},
 {223,176,87,255},
 {131,95,31,162},
 {131,96,31,162},
 {136,99,31,166},
 {134,98,31,162},
 {230,176,67,255},
 {240,184,69,255},
 {245,189,72,255},
 {31,22,8,39},
 {24,17,6,30},
 {8,6,2,10},
 {0,0,0,0}
};
constexpr const char* rows[]={
 "IGGGGGHIIIIIIIII",
 "G05551BHIIIIIIII",
 "G6EEEECBFGGGGGGI",
 "G7EEEEEC4555550G",
 "G6EEEEEEEEEEEE6G",
 "G6EEEEEEEEEEEE6G",
 "G6EEEEEEEEEEEE6G",
 "G6EEEEEEEEEEEE6G",
 "G6EEEEEEEEEEEE6G",
 "G6EEEEEEEEEEEE6G",
 "G3DDDDDDDDDDDD2G",
 "G9AAAAAAAAAAAA8G"
};
constexpr Pixel at(int x,int y){
 if(x<0||y<0||x>=width||y>=height)return {0,0,0,0};
 char c=rows[y][x];return palette[c<='9'?c-'0':c-'A'+10];
}
inline int floor256(int v){return v>=0?v/256:-((-v+255)/256);}
inline unsigned render(int x,int y,int w,int h,unsigned bg){
 // Center-aligned interpolation in premultiplied space avoids white halos.
 int sx=(2*x+1)*width*128/w-128,sy=(2*y+1)*height*128/h-128;
 int ix=floor256(sx),iy=floor256(sy);unsigned fx=sx-ix*256,fy=sy-iy*256;
 Pixel p[]={at(ix,iy),at(ix+1,iy),at(ix,iy+1),at(ix+1,iy+1)};
 unsigned weights[]={(256-fx)*(256-fy),fx*(256-fy),(256-fx)*fy,fx*fy};
 unsigned rr=0,gg=0,bb=0,aa=0;
 for(int i=0;i<4;++i){rr+=p[i].r*weights[i];gg+=p[i].g*weights[i];bb+=p[i].b*weights[i];aa+=p[i].a*weights[i];}
 // Compose onto COLORREF background and output a BI_RGB DIB pixel.
 constexpr unsigned total=65536,den=255*total;unsigned trans=255*total-aa;
 auto channel=[&](unsigned premul,unsigned base){return (premul*255+base*trans+den/2)/den;};
 return (channel(rr,bg&255u)<<16)|(channel(gg,(bg>>8)&255u)<<8)|channel(bb,(bg>>16)&255u);
}
}
