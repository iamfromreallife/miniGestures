#pragma once
namespace mgtheme {
using Color=unsigned;
constexpr unsigned rgb(unsigned r,unsigned g,unsigned b){return r|(g<<8)|(b<<16);}
struct Palette{Color bg,field,ink,muted,frame,button,hover,pressed,selectedShape,category,accent,accentText;};
constexpr Palette legacyDark{rgb(32,32,32),rgb(42,42,42),rgb(242,242,242),rgb(180,185,191),rgb(83,87,94),rgb(48,48,48),rgb(60,63,68),rgb(67,75,87),rgb(36,63,85),rgb(53,53,53),rgb(0,120,212),rgb(255,255,255)};
constexpr bool useDark(int mode,bool systemDark){return mode==2||(mode==0&&systemDark);}
constexpr Color mix(Color a,Color b,unsigned percent){Color c=0;for(unsigned i=0;i<24;i+=8)c|=((((a>>i)&255)*(100-percent)+((b>>i)&255)*percent+50)/100)<<i;return c;}
constexpr unsigned brightness(Color c){return (299*(c&255)+587*((c>>8)&255)+114*((c>>16)&255))/1000;}
constexpr unsigned separation(Color a,Color b){return brightness(a)>brightness(b)?brightness(a)-brightness(b):brightness(b)-brightness(a);}
constexpr Color readable(Color bg,Color a,Color b){return separation(bg,a)>=separation(bg,b)?a:b;}
constexpr Color invert(Color c){return c^0xffffffu;}
constexpr bool same(const Palette&a,const Palette&b){return a.bg==b.bg&&a.field==b.field&&a.ink==b.ink&&a.muted==b.muted&&a.frame==b.frame&&a.button==b.button&&a.hover==b.hover&&a.pressed==b.pressed&&a.selectedShape==b.selectedShape&&a.category==b.category&&a.accent==b.accent&&a.accentText==b.accentText;}
// All inputs come from Windows. Derived states are used only when the visual
// style does not expose a suitable color. Normal dark mode uses legacyDark.
inline Palette systemPalette(Color face,Color window,Color text,Color gray,Color border,Color highlight,Color highlightText,bool dark,bool contrastMode){
 if(dark&&!contrastMode)return legacyDark;
 if(!contrastMode && (brightness(window)<128)!=dark){face=invert(face);window=invert(window);text=invert(text);gray=invert(gray);border=invert(border);}
 Palette p{face,window,text,gray,border,face,mix(face,text,7),mix(face,text,14),mix(window,highlight,12),mix(face,text,4),highlight,highlightText};
 if(!contrastMode){if(separation(p.bg,p.muted)<65)p.muted=mix(p.bg,p.ink,65);if(separation(p.field,p.frame)<35)p.frame=mix(p.field,p.ink,35);p.accentText=readable(highlight,text,window);}
 else {p.hover=p.pressed=p.category=face;p.selectedShape=window;}
 return p;
}
}
