#pragma once
namespace mgtheme {
using Color=unsigned;
constexpr unsigned rgb(unsigned r,unsigned g,unsigned b){return r|(g<<8)|(b<<16);}
struct Palette{Color bg,field,ink,muted,frame,button,hover,pressed,selectedShape,category,accent,accentText;};
constexpr Color gestureBlue=rgb(0,120,212);
constexpr Palette legacyDark{rgb(32,32,32),rgb(42,42,42),rgb(242,242,242),rgb(180,185,191),rgb(83,83,83),rgb(48,48,48),rgb(63,63,63),rgb(75,75,75),rgb(80,80,80),rgb(53,53,53),gestureBlue,rgb(255,255,255)};
constexpr bool useDark(int mode,bool systemDark){return mode==2||(mode==0&&systemDark);}
constexpr Color mix(Color a,Color b,unsigned percent){Color c=0;for(unsigned i=0;i<24;i+=8)c|=((((a>>i)&255)*(100-percent)+((b>>i)&255)*percent+50)/100)<<i;return c;}
// WCAG sRGB relative luminance, fixed-point lookup avoids pow() in paint paths.
constexpr unsigned linearChannel[256]={
 0,304,607,911,1214,1518,1821,2125,2428,2732,3035,3347,3677,4025,4391,4777,
 5182,5605,6049,6512,6995,7499,8023,8568,9134,9721,10330,10960,11612,12286,12983,13702,
 14444,15209,15996,16807,17642,18500,19382,20289,21219,22174,23153,24158,25187,26241,27321,28426,
 29557,30713,31896,33105,34340,35601,36889,38204,39546,40915,42311,43735,45186,46665,48172,49707,
 51269,52861,54480,56128,57805,59511,61246,63010,64803,66626,68478,70360,72272,74214,76185,78187,
 80220,82283,84376,86500,88656,90842,93059,95307,97587,99899,102242,104616,107023,109462,111932,114435,
 116971,119538,122139,124772,127438,130136,132868,135633,138432,141263,144128,147027,149960,152926,155926,158961,
 162029,165132,168269,171441,174647,177888,181164,184475,187821,191202,194618,198069,201556,205079,208637,212231,
 215861,219526,223228,226966,230740,234551,238398,242281,246201,250158,254152,258183,262251,266356,270498,274677,
 278894,283149,287441,291771,296138,300544,304987,309469,313989,318547,323143,327778,332452,337164,341914,346704,
 351533,356400,361307,366253,371238,376262,381326,386429,391572,396755,401978,407240,412543,417885,423268,428690,
 434154,439657,445201,450786,456411,462077,467784,473531,479320,485150,491021,496933,502886,508881,514918,520996,
 527115,533276,539479,545724,552011,558340,564712,571125,577580,584078,590619,597202,603827,610496,617207,623960,
 630757,637597,644480,651406,658375,665387,672443,679542,686685,693872,701102,708376,715694,723055,730461,737910,
 745404,752942,760525,768151,775822,783538,791298,799103,806952,814847,822786,830770,838799,846873,854993,863157,
 871367,879622,887923,896269,904661,913099,921582,930111,938686,947307,955973,964686,973445,982251,991102,1000000
};
constexpr unsigned luminance(Color c){
 return (unsigned)((2126ull*linearChannel[c&255]+7152ull*linearChannel[(c>>8)&255]+722ull*linearChannel[(c>>16)&255]+5000)/10000);
}
constexpr bool contrastAtLeast(Color a,Color b,unsigned tenths){
 unsigned x=luminance(a)+50000,y=luminance(b)+50000;
 return (x>y?x:y)*10u>=(x>y?y:x)*tenths;
}
constexpr Color contrastInk(Color bg){
 constexpr Color black=rgb(0,0,0),white=rgb(255,255,255);
 unsigned l=luminance(bg)+50000;
 return 1ull*l*l>=50000ull*1050000ull?black:white;
}
// Preserve the preferred foreground whenever it is readable. For accent
// controls prefer white on saturated/dark colors; pale accents use black.
constexpr Color controlInk(Color bg,Color preferred,unsigned minimum=45){
 return contrastAtLeast(bg,preferred,minimum)?preferred:contrastInk(bg);
}
constexpr Color accentControlInk(Color bg){
 return controlInk(bg,rgb(255,255,255));
}
constexpr Color accentControlFill(Color bg,bool hover,bool pressed){
 // Keep the text polarity stable while the button is hovered or pressed.
 Color text=accentControlInk(bg),opposite=text==rgb(255,255,255)?rgb(0,0,0):rgb(255,255,255);
 return mix(bg,opposite,pressed?14:hover?7:0);
}
static_assert(accentControlInk(gestureBlue)==rgb(255,255,255));
static_assert(accentControlInk(rgb(255,255,0))==rgb(0,0,0));
static_assert(contrastAtLeast(accentControlInk(accentControlFill(gestureBlue,true,false)),accentControlFill(gestureBlue,true,false),45));
static_assert(contrastAtLeast(controlInk(rgb(42,42,42),rgb(80,80,80)),rgb(42,42,42),45));
constexpr Color selectionHighlight(Color accent,Color field,bool dark){
 Color base=mix(accent,rgb(255,255,255),50);
 Color shade=mix(base,dark?rgb(0,0,0):rgb(255,255,255),dark?40:20);
 // A very pale system accent on a white field (or the reverse in dark
 // mode) still needs a distinguishable selection background.
 Color selected=contrastAtLeast(shade,field,13)?shade:mix(shade,contrastInk(field),20);
 return dark?selected:mix(selected,rgb(255,255,255),20);
}
constexpr Color gestureInk(Color bg,Color preferred=gestureBlue,unsigned minimum=45){
 return contrastAtLeast(bg,preferred,minimum)?preferred:contrastInk(bg);
}
static_assert(gestureInk(gestureBlue)!=gestureBlue);
static_assert(contrastAtLeast(gestureInk(gestureBlue),gestureBlue,45));
static_assert(contrastAtLeast(contrastInk(rgb(255,255,0)),rgb(255,255,0),45));
static_assert(selectionHighlight(gestureBlue,rgb(255,255,255),false)==rgb(173,212,241));
static_assert(selectionHighlight(gestureBlue,rgb(42,42,42),true)==rgb(77,113,140));
constexpr unsigned brightness(Color c){return (299*(c&255)+587*((c>>8)&255)+114*((c>>16)&255))/1000;}
constexpr unsigned separation(Color a,Color b){return brightness(a)>brightness(b)?brightness(a)-brightness(b):brightness(b)-brightness(a);}
constexpr Color readable(Color bg,Color a,Color b){return separation(bg,a)>=separation(bg,b)?a:b;}
constexpr Color invert(Color c){return c^0xffffffu;}
constexpr bool same(const Palette&a,const Palette&b){return a.bg==b.bg&&a.field==b.field&&a.ink==b.ink&&a.muted==b.muted&&a.frame==b.frame&&a.button==b.button&&a.hover==b.hover&&a.pressed==b.pressed&&a.selectedShape==b.selectedShape&&a.category==b.category&&a.accent==b.accent&&a.accentText==b.accentText;}
// All inputs come from Windows. Derived states are used only when the visual
// style does not expose a suitable color. Normal dark mode uses legacyDark.
inline Palette systemPalette(Color face,Color window,Color text,Color gray,Color border,Color highlight,Color highlightText,bool dark,bool contrastMode){
 if(dark&&!contrastMode){Palette p=legacyDark;p.accent=highlight;p.accentText=accentControlInk(highlight);p.selectedShape=selectionHighlight(highlight,p.field,true);return p;}
 if(!contrastMode && (brightness(window)<128)!=dark){face=invert(face);window=invert(window);text=invert(text);gray=invert(gray);border=invert(border);}
 Palette p{face,window,text,gray,border,face,mix(face,text,7),mix(face,text,14),mix(window,highlight,12),mix(face,text,4),highlight,highlightText};
 if(!contrastMode){if(separation(p.bg,p.muted)<65)p.muted=mix(p.bg,p.ink,65);if(separation(p.field,p.frame)<35)p.frame=mix(p.field,p.ink,35);p.accent=highlight;p.accentText=accentControlInk(highlight);p.selectedShape=selectionHighlight(highlight,p.field,dark);}
 else {p.hover=p.pressed=p.category=face;p.selectedShape=window;}
 // Override only the regular light-mode window background. Preserve Windows
 // control colors, the legacy dark palette and accessibility palettes.
 if(!dark&&!contrastMode)p.bg=rgb(243,243,243);
 return p;
}
}
