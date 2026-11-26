#include "config.hpp"
#include "program_parameter.hpp"
#include "gesture_order.hpp"
#include "engine.hpp"
#include "locale.hpp"
#include "templates.hpp"
#include "window_tray.hpp"
static TrayClickFilter trayClicks;
#include "trail_raster.hpp"
static Config cfg;static Gesture edit;static int selectedShape=-1,shapeAnchor=-1;static unsigned shapeSelection=0;static Rule rule;static int page=0,appId=0,selected=-1,ruleIndex=-1,actIndex=0,ruleEdit=0,settings=0,record=0;static int hkregistered[256][16];static HWND controls[164],canvas;static int dpi=96;static HFONT font,tabBold;static HBRUSH white,side,blue;static HINSTANCE inst;static HICON appicon,pausedIcon,updateIcon,pausedUpdateIcon;static bool updateAvailable=false;static bool geometryReady=false,trayAdded=false;static HICON trayCurrentIcon=nullptr;static NID nid;static UINT taskbarmsg;static bool loading=false,dirty=false,exiting=false,configValid=true;static P drawn[maxDrawingPoints];static int drawnN=0,drawdrag=0;static P trail[maxDrawingPoints];static int trailN=0;static Target testTarget;static W last[256];static int actionVk=0,actionMods=0;static PROC recordOld;static bool lazyUI=false,noHooks=false,geometryDirty=false,firstSave=false,testFailed=false,shutdownTestMode=false,busyTestMode=false,parallelTestMode=false;static DWORD testStart;static H shutdownSignal,startupReady,watchdogThread;static DWORD startupDeadline;static int editLineHeight,editTextHeight;
static const W* tr(int id){return strings[id][cfg.lang];}
enum C{TAB0=10,TAB1,TAB2,SETBTN,TOGGLE,IMPORTBTN,EXPORTBTN,SEARCHBOX,LIST,NEWBTN,CATBTN,DELBTN,DUPBTN,APPGEST,NAMEBOX,FOLDERBOX,ENABLEBOX,BUTTONBOX,MODBOX,TRIGGERBOX,KEYBOX,RECBTN,KEYCLEAR,SENSEBOX,TRAIN,ADDSAMPLE,RESETTRAIN,ACTLIST,ACTCOMBO,ARGBOX,ACTADD,ACTDEL,ACTUP,ACTDOWN,ACTREC,TESTBTN,SAVEBTN,EXEBOX,CLASSBOX,TITLEBOX,PRIORITYBOX,INHERITBOX,MODEBOX,BROWSEBTN,LANGBOX,THRESHBOX,OVERBOX,CLOSEBOX,STARTBOX,UNUSED_CONTROL,STATUSBOX,HEAD,LABSEARCH,LABNAME,LABFOLDER,LABBUTTON,LABMOD,LABTRIGGER,LABKEY,LABSENSE,LABTRAIN,LABACTION,LABPARAM,LABEXE,LABCLASS,LABTITLE,LABPRIORITY,LABMODE,LABLANG,LABTHRESH,LABSELECT,ACTHINT,APPBACK,PRESETBTN,PICKBTN,EXECBOX,LABEXEC,LABSENSHELP,CONFIRMBOX,VERSIONBOX,STARTWINBOX,ADMINBOX,ADMINHELP,ADMINSTATUS,ADDITIONALHEADER,THEMEBOX,LABTHEME,TOPBOX, APPLIST=105,APPNEW,APPCAT,APPEDIT,APPDEL,APPHEAD,GESTHEAD,EDITORHEAD,APPSEARCH,TRAILCOLOR,TRAILWIDTH,LABTRAILCOLOR,LABTRAILWIDTH,TRAILPICK,TASKBARBOX,TRAILRESET,EXPATHBOX,LABEXPATH,CANCELCREATE=148,SELECTOR_ENABLED=150,SELECTOR_INHERIT=151,FULLSCREENBOX=152,INDICATORBOX=153,CAPTUREBTN=154,UPDATEBOX=155,LABUPDATE=156,UPDATEBTN=157,UPDATEVERSIONS=158,INDICATORSECONDS=159,LABINDICATORSECONDS=149,LABGESTURETIMEOUT=160,GESTURETIMEOUT=161,REMOTEINPUTBOX=162};
static HWND ctrl(int id){return controls[id];}static int px(int x){return x*dpi/96;}static void txt(int id,const W*s){if(ctrl(id))SetWindowTextW(ctrl(id),s);}static void get(int id,W*b,int cap){GetWindowTextW(ctrl(id),b,cap);}static int selection(int id){return (int)SendMessageW(ctrl(id),0x147,0,0);}static void select(int id,int n){SendMessageW(ctrl(id),0x14e,n,0);}static int ls(int id){return (int)SendMessageW(ctrl(id),0x188,0,0);}static void sl(int id,int n){SendMessageW(ctrl(id),0x186,n,0);}static int checked(int id){return (int)SendMessageW(ctrl(id),0xf0,0,0)==1;}static void check(int id,int on){SendMessageW(ctrl(id),0xf1,on?1:0,0);}static void combo(int id,const W*s){SendMessageW(ctrl(id),0x143,0,(LP)s);}static void listadd(int id,const W*s,int data){int x=(int)SendMessageW(ctrl(id),0x180,0,(LP)s);if(x>=0)SendMessageW(ctrl(id),0x19a,x,data);}static int listdata(int id){int n=ls(id);return n<0?-1:(int)SendMessageW(ctrl(id),0x199,n,0);}static void listreset(int id){SendMessageW(ctrl(id),0x184,0,0);}static void comboclear(int id){SendMessageW(ctrl(id),0x14b,0,0);}static void inttext(int id,int n){W b[24];number(b,n);txt(id,b);}static int getnum(int id){W b[24];get(id,b,24);return num(b);}
#include "editor_strings.hpp"
static bool gstview();
#include "flat_ui.hpp"
#include "shape_cards.hpp"
#include "focus_cues.hpp"
static void hideNativeFocus(HWND h){if(h)SendMessageW(h,mgfocus::change,mgfocus::hideRequest,0);}
static bool nativeFocusState(HWND h,UINT m,WP w,LP l,LR& result){
 if(!mgfocus::isState(m))return false;
 unsigned request=(unsigned)w;
 if(mgfocus::blocked(request)){result=0;return true;}
 result=DefWindowProcW(h,m,mgfocus::filter(request),l);
 // Windows initializes UI state according to the last input device.
 // Reassert our local focus policy without changing mnemonic visibility.
 if(LO(w)==mgfocus::initialize)hideNativeFocus(h);
 return true;
}

#include "app_order.hpp"
#include "startup.hpp"
static int modalCreateButton=0;static bool dialogOwnerLimited=false;static bool gestureDraft=false;static int draftPreviousId=0;static W draftPreviousCategory[96]{};
static int appCategory=-1;static int categorySelection=-1;static HWND popup=nullptr,drawCanvas=nullptr;static HWND settingsWnd=nullptr;static HWND settingsControls[164]{};static bool settingsLoading=false;static void openSettingsView();static void closeSettingsView();static void updateSettingsTheme();
static void updateTrayIcon();
static void indicatorThemeChanged();
static constexpr W versionCaption[]=L"by iamfromreallife  v2.0.10";
static void failedGestureLabel(W* b,int cap,const W* name,int score){
 static const W* const failed[]={L"Gesture recognition failed",L"Falha no reconhecimento do gesto",L"Error de reconocimiento del gesto",L"Échec de la reconnaissance du geste",L"Gestenerkennung fehlgeschlagen",L"Riconoscimento del gesto non riuscito"};
 static const W* const similar[]={L" — Similar to ",L" — Semelhante a ",L" — Similar a ",L" — Similaire à ",L" — Ähnlich wie ",L" — Simile a "};
 cp(b,failed[cfg.lang],cap);if(name&&*name&&score>=0){cat(b,similar[cfg.lang],cap);int prefix=len(b);gestureScoreLabel(b+prefix,cap-prefix,name,score);}
}
static void status(const W*s){updateTrayIcon();cp(last,s,256);W b[416];cp(b,noHooks?L"Safe mode — hooks off":hookok?(cfg.enabled?(mouseExpected||keyboardExpected?tr(ACTIVE):et(NO_BINDINGS_E)):tr(PAUSED)):tr(STATUS_ERROR),416);cat(b,L"   |   ",416);cat(b,tr(GESTURES),416);cat(b,L": ",416);W n[24];number(n,cfg.ng);cat(b,n,416);if(*s){cat(b,L"   |   ",416);cat(b,s,416);}txt(STATUSBOX,b);txt(VERSIONBOX,versionCaption);}
static void error(){MessageBoxW(mainwnd,tr(INVALID),L"miniGestures",0x10);}
static HWND footerWnd=nullptr;
// Resolve optional Windows APIs from system DLLs, without new distributed dependencies.
static bool windowsDark(){using Fn=long(*)(H,const W*,const W*,DWORD,DWORD*,void*,DWORD*);static H adv=nullptr;if(!adv)adv=LoadLibraryExW(L"advapi32.dll",nullptr,0x800);auto f=adv?(Fn)GetProcAddress(adv,"RegGetValueW"):nullptr;DWORD value=1,n=4;return f&&f(PTR((ip)(i32)0x80000001),L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",L"AppsUseLightTheme",0x10,nullptr,&value,&n)==0&&value==0;}
// AccentColor is ABGR; DWM fallback is ARGB.
static int systemTrailAccent(){using Reg=long(*)(H,const W*,const W*,DWORD,DWORD*,void*,DWORD*);static H adv=nullptr;if(!adv)adv=LoadLibraryExW(L"advapi32.dll",nullptr,0x800);auto reg=adv?(Reg)GetProcAddress(adv,"RegGetValueW"):nullptr;DWORD value=0,n=4;if(reg&&reg(PTR((ip)(i32)0x80000001),L"Software\\Microsoft\\Windows\\DWM",L"AccentColor",0x10,nullptr,&value,&n)==0)return value&0xffffff;using Dwm=long(*)(DWORD*,B*);static H dwm=nullptr;if(!dwm)dwm=LoadLibraryExW(L"dwmapi.dll",nullptr,0x800);auto getColor=dwm?(Dwm)GetProcAddress(dwm,"DwmGetColorizationColor"):nullptr;B opaque=0;if(getColor&&getColor(&value,&opaque)==0)return trailArgbToColorRef(value);return (int)GetSysColor(13);}
static int currentTrailAccent=-1;static bool draftTrailAuto=false;
static int effectiveTrailColor(int saved){if(saved>=0)return saved;if(currentTrailAccent<0)currentTrailAccent=systemTrailAccent();return currentTrailAccent;}
static void showAutoTrailColor(){W hex[8];trailHex(effectiveTrailColor(-1),hex);if(settingsControls[TRAILCOLOR]){settingsLoading=true;SetWindowTextW(settingsControls[TRAILCOLOR],hex);settingsLoading=false;draftTrailAuto=true;}}
static void refreshTrailAccent(){currentTrailAccent=systemTrailAccent();if(settingsWnd&&settingsControls[TRAILCOLOR]&&draftTrailAuto)showAutoTrailColor();}
static void refreshSearchCue(){if(ctrl(SEARCHBOX))SendMessageW(ctrl(SEARCHBOX),0x1501,0,(LP)tr(SEARCH));if(ctrl(APPSEARCH))SendMessageW(ctrl(APPSEARCH),0x1501,0,(LP)tr(SEARCH));}
static void refresh();
static void choosefirst();
static bool highContrast();
using DwmWindowAttributeFn=long(*)(HWND,DWORD,const void*,DWORD);
static DwmWindowAttributeFn dwmWindowAttribute(){
 static H dwm=LoadLibraryExW(L"dwmapi.dll",nullptr,0x800);
 static auto f=dwm?(DwmWindowAttributeFn)GetProcAddress(dwm,"DwmSetWindowAttribute"):nullptr;
 return f;
}
static void enableNativeWindowTransitions(HWND h){
 // DWMWA_TRANSITIONS_FORCEDISABLED = 3; FALSE allows the system compositor
 // to use its normal transitions, while honoring system animation policy.
 if(auto f=dwmWindowAttribute()){const B disabled=0;f(h,3,&disabled,sizeof(disabled));}
}
static void themeCaption(HWND h){
 if(!h)return;auto f=dwmWindowAttribute();if(!f)return;
 int dark=darkTheme;if(f(h,20,&dark,4)!=0)f(h,19,&dark,4);
 // Windows 11 DWMWA_CAPTION_COLOR: native solid caption, no custom frame.
 // Reset to DWMWA_COLOR_DEFAULT for dark mode and accessibility themes.
 // Other dialogs retain their original native captions.
 if(h==mainwnd||h==settingsWnd){const DWORD color=(!darkTheme&&!highContrast())?RGB(243,243,243):0xffffffffu;f(h,35,&color,sizeof(color));}
}
static void themeNative(HWND h){if(!h)return;using Fn=long(*)(HWND,const W*,const W*);static H ux=nullptr;if(!ux)ux=LoadLibraryExW(L"uxtheme.dll",nullptr,0x800);auto f=ux?(Fn)GetProcAddress(ux,"SetWindowTheme"):nullptr;if(f)f(h,darkTheme?L"DarkMode_Explorer":nullptr,nullptr);}
// System colors are the authoritative light/high-contrast palette. Windows
// visual style classes can supply light control-state colors;
// their availability and contents vary with Windows versions.
static bool highContrast(){struct HC{UINT size,flags;W* scheme;};HC hc{sizeof(HC),0,nullptr};using Fn=B(*)(UINT,UINT,void*,UINT);auto f=(Fn)GetProcAddress(GetModuleHandleW(L"user32.dll"),"SystemParametersInfoW");return f&&f(0x42,sizeof(hc),&hc,0)&&(hc.flags&1);}
static bool styleColor(const W* cls,int part,int state,int prop,DWORD& out){using Open=H(*)(HWND,const W*);using Get=long(*)(H,int,int,int,DWORD*);using Close=long(*)(H);static H ux=LoadLibraryExW(L"uxtheme.dll",nullptr,0x800);static auto open=ux?(Open)GetProcAddress(ux,"OpenThemeData"):nullptr;static auto get=ux?(Get)GetProcAddress(ux,"GetThemeColor"):nullptr;static auto close=ux?(Close)GetProcAddress(ux,"CloseThemeData"):nullptr;if(!open||!get||!close)return false;H theme=open(nullptr,cls);if(!theme)return false;DWORD c=0;long hr=get(theme,part,state,prop,&c);close(theme);if(hr<0)return false;out=c&0xffffff;return true;}
static mgtheme::Palette readWindowsPalette(bool dark,bool hc){
 DWORD highlight=hc?GetSysColor(13):(DWORD)systemTrailAccent();auto p=mgtheme::systemPalette(GetSysColor(15),GetSysColor(5),GetSysColor(8),GetSysColor(17),GetSysColor(16),highlight,GetSysColor(14),dark,hc);
 if(hc||dark)return p;
 auto surface=[&](const W* cls,int part,int state,DWORD& target){DWORD v;if(styleColor(cls,part,state,3802,v)&&(mgtheme::brightness(v)<128)==dark&&mgtheme::separation(v,p.ink)>=100)target=v;};
 const W* buttonClass=L"Button";surface(buttonClass,1,1,p.button);surface(buttonClass,1,2,p.hover);surface(buttonClass,1,3,p.pressed);DWORD v;if(styleColor(L"Edit",1,1,3801,v)&&mgtheme::separation(v,p.field)>=25)p.frame=v;
 // Keep accent labels legible even when the chosen Windows accent is pale.
 p.accentText=mgtheme::readable(p.accent,GetSysColor(8),GetSysColor(5));return p;
}
static void applyTheme(){bool hc=highContrast();highContrastTheme=hc;bool dark=hc?mgtheme::brightness(GetSysColor(5))<128:mgtheme::useDark(cfg.theme,windowsDark());auto p=readWindowsPalette(dark,hc);static mgtheme::Palette previous{};if(white&&fieldBrush&&dark==darkTheme&&legacyDarkTheme==(dark&&!hc)&&mgtheme::same(p,previous))return;HBRUSH a=CreateSolidBrush(p.bg),b=CreateSolidBrush(p.field);if(!a||!b){if(a)DeleteObject(a);if(b)DeleteObject(b);return;}previous=p;HBRUSH old=white,oldField=fieldBrush;white=a;fieldBrush=b;darkTheme=dark;legacyDarkTheme=dark&&!hc;appFill=p.bg;fieldFill=p.field;ink=p.ink;muted=p.muted;frame=p.frame;buttonFill=p.button;hoverFill=p.hover;pressedFill=p.pressed;shapeFill=p.selectedShape;categoryFill=p.category;accent=p.accent;accentText=p.accentText;if(old)DeleteObject(old);if(oldField)DeleteObject(oldField);themeCaption(mainwnd);if(mainwnd){themeNative(mainwnd);InvalidateRect(mainwnd,nullptr,0);}for(int i=0;i<164;i++)if(ctrl(i)){if(flatList(i)||flatCombo(i)||flatEdit(i))themeNative(ctrl(i));InvalidateRect(ctrl(i),nullptr,1);}if(footerWnd)InvalidateRect(footerWnd,nullptr,1);updateSettingsTheme();indicatorThemeChanged();}
static const W* themeText(int n){static const W* values[4][6]={{L"Theme",L"Tema",L"Tema",L"Thème",L"Design",L"Tema"},{L"Follow System",L"Seguir o sistema",L"Seguir el sistema",L"Suivre le système",L"Systemeinstellung",L"Segui il sistema"},{L"Light",L"Claro",L"Claro",L"Clair",L"Hell",L"Chiaro"},{L"Dark",L"Escuro",L"Oscuro",L"Sombre",L"Dunkel",L"Scuro"}};return values[n][cfg.lang];}
static HBRUSH controlColor(HDC dc,UINT m,HWND child){bool field=m==0x133||m==0x134||m==0x135||(child&&(flatEdit(GetDlgCtrlID(child))||GetDlgCtrlID(child)==130||GetDlgCtrlID(child)==145));SetTextColor(dc,child==ctrl(ADMINHELP)?muted:ink);SetBkColor(dc,field?fieldFill:appFill);return field?fieldBrush:white;}

static LR CB footerProc(HWND h,UINT m,WP w,LP l){LR uiState;if(nativeFocusState(h,m,w,l,uiState))return uiState;if(m==0x14)return 1;if(m==0xf){PAINT p{};HDC dc=BeginPaint(h,&p);FillRect(dc,&p.rcPaint,white);EndPaint(h,&p);return 0;}if(m==0x138){return (LR)controlColor((HDC)w,m,(HWND)l);}return DefWindowProcW(h,m,w,l);}
static HWND make(int id,const W*cls,DWORD style,DWORD ex=0){HWND h=CreateWindowExW(ex,cls,L"",0x44000000|style,0,0,1,1,((id==STATUSBOX||id==VERSIONBOX)&&footerWnd)?footerWnd:mainwnd,PTR(id),inst,nullptr);controls[id]=h;SendMessageW(h,0x30,(WP)font,1);return h;}
static void button(int id){make(id,L"BUTTON",0x10000|0xb);}static void label(int id){bool wrapped=id==LABSELECT||id==ACTHINT||id==LABSENSHELP||id==ADMINHELP||id==ADMINSTATUS;make(id,L"STATIC",wrapped?0:0x200);}static void field(int id,int cap=255){make(id,L"EDIT",0x10000|0x80);SendMessageW(ctrl(id),0xc5,cap,0);}static void cb(int id){make(id,L"COMBOBOX",0x10000|0x200000|3|0x10|0x200|0x400);}static void box(int id){make(id,L"BUTTON",0x10000|3);}static void list(int id){make(id,L"LISTBOX",0x10000|0x200000|1|0x100|0x10|0x40);}
struct Placement{int x,y,w,h;bool show;};static Placement logicalPlace[164],place[164],previous[164];static bool laidout=false;
static int virtualW=0,virtualH=0;static bool layoutBusy=false;
#include "fixed_footer.hpp"
static void pos(int id,int x,int y,int w,int h,bool show=true){if(!ctrl(id))return;logicalPlace[id]={px(x),px(y),px(maxn(1,w)),px(maxn(1,h)),show};}
static void hide(int id){if(!ctrl(id))return;logicalPlace[id].show=false;place[id].show=false;}
static void inset(int id,Placement& p){if(!flatFrame(id))return;int l=px((id==SEARCHBOX||id==APPSEARCH)?23:4),t=px(3),r=px(4),b=px(3);if(flatEdit(id)){auto band=mgtext::editBand(p.h,editTextHeight,maxn(1,px(2)));t=band.top;b=p.h-t-band.height;}p.x+=l;p.y+=t;p.w=maxn(1,p.w-l-r);p.h=maxn(1,p.h-t-b);}

static void syncFooterLayout(){
 RECT rr{};GetClientRect(mainwnd,&rr);
 auto v=footerGeometry(rr.right,rr.bottom,rr.right,rr.bottom,px(21),px(12),px(footerVersionWidth),px(6),px(18),0,0);
 memcpy(place,logicalPlace,sizeof(place));
 place[STATUSBOX]={v.statusX,v.footerY,v.statusW,v.textH,true};place[VERSIONBOX]={v.versionX,v.footerY,v.versionW,v.textH,true};
 if(footerWnd){SetWindowPos(footerWnd,nullptr,0,v.footerY,maxn(1,rr.right),maxn(1,rr.bottom-v.footerY),0x40|16);SetWindowPos(ctrl(STATUSBOX),nullptr,v.statusX,0,v.statusW,v.textH,0x40|4|16);SetWindowPos(ctrl(VERSIONBOX),nullptr,v.versionX,0,v.versionW,v.textH,0x40|4|16);}
}
static void applylayout(){int ids[164],n=0;Placement inner[164];UINT flags[164];for(int i=0;i<164;i++){if(!ctrl(i)||i==STATUSBOX||i==VERSIONBOX)continue;auto&p=place[i];auto&q=previous[i];if(laidout&&p.show==q.show&&(!p.show||(p.x==q.x&&p.y==q.y&&p.w==q.w&&p.h==q.h)))continue;ids[n]=i;inner[n]=p;inset(i,inner[n]);flags[n]=4|16;if(!laidout||p.show!=q.show)flags[n]|=p.show?0x40:0x80;if(!p.show)flags[n]|=1|2;n++;}if(!n)return;H d=BeginDeferWindowPos(n);for(int j=0;j<n&&d;j++){auto&p=inner[j];d=DeferWindowPos(d,ctrl(ids[j]),nullptr,p.x,p.y,p.w,p.h,flags[j]);}bool ok=d&&EndDeferWindowPos(d);if(!ok)for(int j=0;j<n;j++){auto&p=inner[j];SetWindowPos(ctrl(ids[j]),nullptr,p.x,p.y,p.w,p.h,flags[j]);}for(int j=0;j<n;j++){int id=ids[j];auto&p=place[id];if(p.show&&flatEdit(id))alignNativeEdit(ctrl(id),mainwnd,{p.x,p.y,p.x+p.w,p.y+p.h},editTextHeight,maxn(1,px(2)));previous[id]=p;}laidout=true;}static void setfont(){font=CreateFontW(-px(12),0,0,0,400,0,0,0,1,0,0,5,0,L"Segoe UI");tabBold=CreateFontW(-px(12),0,0,0,700,0,0,0,1,0,0,5,0,L"Segoe UI");HDC dc=GetDC(mainwnd);if(dc){H old=SelectObject(dc,font);POINT ext{};GetTextExtentPoint32W(dc,L"Ag",2,&ext);SelectObject(dc,old);ReleaseDC(mainwnd,dc);editTextHeight=ext.y;editLineHeight=ext.y+px(2);}else {editTextHeight=px(16);editLineHeight=px(18);}}
// Each view uses the widest visible localized label, plus a 30 DIP gap.
static constexpr int labelGap=30;
// Cache by DPI and language; resize does not measure text repeatedly.
static const int gestureLabels[]={LABNAME,LABBUTTON,LABMOD,LABTRIGGER,LABKEY,LABTRAIN,LABEXEC,LABACTION,LABPARAM};
static const int appGestureLabels[]={LABNAME,LABBUTTON,LABMOD,LABTRIGGER,LABKEY,LABTRAIN,LABEXEC,LABACTION,LABPARAM};
static const int settingsLabels[]={LABTHEME,LABLANG,LABBUTTON,LABTHRESH,LABSENSE,LABTRAILCOLOR,LABTRAILWIDTH};
static const int applicationLabels[]={LABNAME,LABEXE,LABCLASS,LABTITLE};
static const int exclusionLabels[]={LABNAME,LABEXE,LABEXPATH,LABCLASS,LABTITLE,LABMODE};
struct LabelSet{const int* ids;int count;};
static LabelSet labelSet(int view){switch(view){case 0:return {gestureLabels,sizeof(gestureLabels)/sizeof(int)};case 4:return {appGestureLabels,sizeof(appGestureLabels)/sizeof(int)};case 1:return {settingsLabels,sizeof(settingsLabels)/sizeof(int)};case 2:return {applicationLabels,sizeof(applicationLabels)/sizeof(int)};default:return {exclusionLabels,sizeof(exclusionLabels)/sizeof(int)};}}
static int labelView(){return settings?1:page==0?0:page==1?(!ruleEdit&&appId?4:2):3;}
static int labelColumn(int view){
 static int cachedDpi=0,cachedLang=-1,widths[5]{};
 if(cachedDpi!=dpi||cachedLang!=cfg.lang){cachedDpi=dpi;cachedLang=cfg.lang;memset(widths,0,sizeof(widths));}
 if(widths[view])return widths[view];
 HDC dc=GetDC(mainwnd);if(!dc)return 174;H old=SelectObject(dc,font);int widest=0;auto set=labelSet(view);
 for(int i=0;i<set.count;i++){W text[256]{};get(set.ids[i],text,256);POINT extent{};if(GetTextExtentPoint32W(dc,text,len(text),&extent))widest=maxn(widest,extent.x);}
 SelectObject(dc,old);ReleaseDC(mainwnd,dc);
 // Round upward, not down, so no label loses its final glyph at fractional DPI.
 return widths[view]=maxn(76,(widest*96+dpi-1)/dpi+2+labelGap);
}
// Compact layout: keep native text metrics, stable control HWNDs and a narrow sidebar.
static int captionWidth(int id){
 static int widths[164]{},oldDpi=0,oldLang=-1;
 if(oldDpi!=dpi||oldLang!=cfg.lang){oldDpi=dpi;oldLang=cfg.lang;memset(widths,0,sizeof(widths));}
 if(id<0||id>=164)return 50;if(widths[id])return widths[id];
 W b[256]{};get(id,b,256);POINT z{};HDC dc=GetDC(mainwnd);
 if(dc){H old=SelectObject(dc,id>=TAB0&&id<=TAB2&&tabBold?tabBold:font);GetTextExtentPoint32W(dc,b,len(b),&z);if(id>=TAB0&&id<=TAB2&&tabBold){POINT normal{};SelectObject(dc,font);GetTextExtentPoint32W(dc,b,len(b),&normal);z.x=maxn(z.x,normal.x);}if(id==ACTREC){int keys[]={RECORD,BROWSE};for(int key:keys){POINT other{};const W* label=tr(key);GetTextExtentPoint32W(dc,label,len(label),&other);z.x=maxn(z.x,other.x);}}SelectObject(dc,old);ReleaseDC(mainwnd,dc);}
 return widths[id]=maxn(36,(z.x*96+dpi-1)/dpi+12);
}
// Reserve the wider of the two font weights, independent of the selected page.
static int tabCaptionWidth(int id){return captionWidth(id);}
static int groupWidth(const int* ids,int n){int w=(n-1)*3;for(int i=0;i<n;i++)w+=captionWidth(ids[i]);return w;}
static const int shapeButtons[]={ADDSAMPLE,RESETTRAIN,PRESETBTN};
static const int actionButtons[]={ACTADD,ACTUP,ACTDOWN,ACTDEL};
static constexpr int sidebarWidth=176, statusOffset=21, statusVersionWidth=footerVersionWidth, contentGap=22, tabContentGap=contentGap-6, footerButtonHeight=23, footerOffset=statusOffset+contentGap+footerButtonHeight, tabTop=8, tabHeight=26, contentTop=tabTop+tabHeight+tabContentGap, paneTitleHeight=20, paneTitlePitch=26;
// Pane headings are fixed-position labels. Change only their font weight.
static int activePaneHeading=GESTHEAD;
static void applyPaneHeading(){
 if(page==2)activePaneHeading=GESTHEAD;
 if(page!=1&&activePaneHeading==APPHEAD)activePaneHeading=GESTHEAD;
 const int headings[]={APPHEAD,GESTHEAD,EDITORHEAD};for(int id:headings)if(ctrl(id)){
  HFONT wanted=id==activePaneHeading&&tabBold?tabBold:font;
  if((HFONT)SendMessageW(ctrl(id),0x31,0,0)!=wanted)SendMessageW(ctrl(id),0x30,(WP)wanted,1);
 }
}
static int paneAtX(int x){
 if(page==2)return GESTHEAD;
 if(place[EDITORHEAD].show&&x>=place[EDITORHEAD].x)return EDITORHEAD;
 if(page==1&&place[GESTHEAD].show&&x<place[GESTHEAD].x)return APPHEAD;
 return GESTHEAD;
}
static void selectPaneHeading(int id){if(id&&id!=activePaneHeading){activePaneHeading=id;applyPaneHeading();}}
static int paneForWindow(HWND h){
 if(!h||h==mainwnd||GetAncestor(h,2)!=mainwnd)return 0;
 while(h&&GetAncestor(h,1)!=mainwnd)h=GetAncestor(h,1);
 if(!h)return 0;int id=GetDlgCtrlID(h);
 if(id<0||id>=164||ctrl(id)!=h||!place[id].show||id==STATUSBOX||id==VERSIONBOX)return 0;
 if(place[id].y<px(contentTop))return 0;
 return paneAtX(place[id].x);
}
// Observe UI input only; never modify the mouse hook or send synthetic input.
// Remember the last pane when the toolbar, a dialog or another window has focus.
struct PaneInteraction{
 HWND before;
 explicit PaneInteraction(const MSG& m):before(GetFocus()){
  if(m.message==0x201||m.message==0x204||m.message==0x207||m.message==0x203||m.message==0x206||m.message==0x209){
   int target=paneForWindow(m.hwnd);
   if(!target&&m.hwnd==mainwnd){int y=(short)HI(m.lParam);if(y>=px(contentTop)&&y<place[STATUSBOX].y)target=paneAtX((short)LO(m.lParam));}
   selectPaneHeading(target);
  }
 }
 ~PaneInteraction(){HWND after=GetFocus();if(after!=before)selectPaneHeading(paneForWindow(after));}
};
static int compactClientWidth(){if(!ctrl(TAB0))return 720;int tabs[]={TAB0,TAB1,TAB2};
 int editor=maxn(250,maxn(groupWidth(shapeButtons,3),groupWidth(actionButtons,4)));editor=maxn(editor,captionWidth(RECBTN)+captionWidth(KEYCLEAR)+96);editor=maxn(editor,captionWidth(BROWSEBTN)+140);
 int labels=0;for(int i=0;i<4;i++)labels=maxn(labels,labelColumn(i));if(!settings&&page==1)labels=maxn(labels,labelColumn(4));
 int footer=maxn(114,captionWidth(SAVEBTN))+captionWidth(IMPORTBTN)+captionWidth(EXPORTBTN)+36;return maxn(610,maxn(footer,maxn(groupWidth(tabs,3)+captionWidth(SETBTN)+36,sidebarWidth+50+labels+editor+(!settings&&page==1?196:0))));}
static constexpr int minimumClientHeight=704;
static constexpr DWORD mainStyle=0x00cf0000|0x02000000;
static POINT minimumOuter(HWND h){
 RECT r{0,0,(compactClientWidth()*dpi+95)/96,(minimumClientHeight*dpi+95)/96};
 using Adjust= B(*)(RECT*,DWORD,B,DWORD,UINT);
 auto adjust=(Adjust)GetProcAddress(GetModuleHandleW(L"user32.dll"),"AdjustWindowRectExForDpi");
 if(adjust&&adjust(&r,mainStyle,0,0,dpi))return {r.right-r.left,r.bottom-r.top};
 RECT client{},outer{};if(h&&GetClientRect(h,&client)&&GetWindowRect(h,&outer)&&client.right>0&&client.bottom>0)return {r.right+maxn(0,outer.right-outer.left-client.right),r.bottom+maxn(0,outer.bottom-outer.top-client.bottom)};
 return {r.right+px(16),r.bottom+px(40)};
}
static void buttonGroup(const int* ids,int n,int x,int y,int width,int h){int remaining=width-(n-1)*3;int total=0;for(int i=0;i<n;i++)total+=captionWidth(ids[i]);int offset=0;for(int i=0;i<n;i++){int bw=i==n-1?width-offset:total<=remaining?captionWidth(ids[i])+(remaining-total)/n:maxn(1,remaining*captionWidth(ids[i])/maxn(1,total));pos(ids[i],x+offset,y,bw,h);offset+=bw+3;}}
static void relayout(){
 if(layoutBusy)return;layoutBusy=true;
 RECT rr{};GetClientRect(mainwnd,&rr);
 // Old saved sizes, language switches and DPI changes must not clip the editor.
 if(!IsZoomed(mainwnd)&&!IsIconic(mainwnd)&&(rr.right<(compactClientWidth()*dpi+95)/96||rr.bottom<(minimumClientHeight*dpi+95)/96)){
  RECT outer{};if(GetWindowRect(mainwnd,&outer)){auto min=minimumOuter(mainwnd);SetWindowPos(mainwnd,nullptr,0,0,maxn(min.x,outer.right-outer.left),maxn(min.y,outer.bottom-outer.top),2|4|16);GetClientRect(mainwnd,&rr);}
 }
 memset(logicalPlace,0,sizeof(logicalPlace));memset(place,0,sizeof(place));int w=maxn(1,rr.right*96/dpi),h=maxn(1,rr.bottom*96/dpi);virtualW=px(w);virtualH=px(h);
 const int margin=12,gap=3,vgap=5,fieldH=22,pitch=26,buttonH=footerButtonHeight;
 bool apps=!settings&&page==1;int appWidth=apps?176:0,offset=apps?appWidth+12:0;int left=sidebarWidth+clamp((w-compactClientWidth())/5,0,180),x=offset+left+margin+18,lab=labelColumn(labelView()),fx=x+lab;int fw=w-x-margin,ed=fw-lab;
 int base=contentTop+paneTitlePitch;
 hide(TOGGLE);
 int tabids[]={TAB0,TAB1,TAB2};int tx=margin,setw=captionWidth(SETBTN)+14;
 for(int id:tabids){int tw=tabCaptionWidth(id);pos(id,tx,tabTop,tw,tabHeight);tx+=tw+gap;}pos(SETBTN,w-margin-setw,tabTop,setw,tabHeight);
 hide(EXPORTBTN);hide(IMPORTBTN);
 pos(STATUSBOX,margin,h-statusOffset,w-2*margin-statusVersionWidth-6,18);pos(VERSIONBOX,w-margin-statusVersionWidth,h-statusOffset,statusVersionWidth,18);bool gst=!settings&&(page==0||(page==1&&!ruleEdit&&appId));bool rul=!settings&&!gst&&!apps;bool sideOn=!settings;EnableWindow(ctrl(LIST),!apps||appId);EnableWindow(ctrl(SEARCHBOX),!apps||appId);
 const int listTop=base+fieldH+vgap;int sideButtons=rul?3:gst?2:1;int sideFooterTop=h-footerOffset-(sideButtons-1)*(buttonH+vgap);
 int cancelFor=modalCreateButton?modalCreateButton:gestureDraft&&gst?NEWBTN:0;bool sideCancel=cancelFor==NEWBTN||cancelFor==CATBTN;bool appCancel=cancelFor==APPNEW||cancelFor==APPCAT;int step=buttonH+vgap;
 int sideStart=sideFooterTop-(sideCancel?step:0),sideY=sideStart;
 pos(SEARCHBOX,margin+offset,base,left,fieldH,sideOn);pos(LIST,margin+offset,listTop,left,sideStart-vgap-listTop,sideOn);txt(NEWBTN,rul?tr(NEW_EXCL):et(ADD_GESTURE_E));
 if(cancelFor==NEWBTN){pos(CANCELCREATE,margin+offset,sideY,left,buttonH);sideY+=step;}
 pos(NEWBTN,margin+offset,sideY,left,buttonH,gst||rul);sideY+=step;
 if(cancelFor==CATBTN){pos(CANCELCREATE,margin+offset,sideY,left,buttonH);sideY+=step;}
 pos(CATBTN,margin+offset,sideY,left,buttonH,gst||rul);sideY+=step;pos(DELBTN,margin+offset,sideY,left,buttonH,rul);
 pos(GESTHEAD,margin+offset,contentTop,left,paneTitleHeight);pos(EDITORHEAD,x,contentTop,fw,paneTitleHeight,page!=2);
 if(apps){pos(APPHEAD,margin,contentTop,appWidth,paneTitleHeight);int by=h-footerOffset-step-(appCancel?step:0);pos(APPSEARCH,margin,base,appWidth,fieldH);pos(APPLIST,margin,listTop,appWidth,by-vgap-listTop);hide(APPEDIT);hide(APPDEL);
 if(cancelFor==APPNEW){pos(CANCELCREATE,margin,by,appWidth,buttonH);by+=step;}pos(APPNEW,margin,by,appWidth,buttonH);by+=step;
 if(cancelFor==APPCAT){pos(CANCELCREATE,margin,by,appWidth,buttonH);by+=step;}pos(APPCAT,margin,by,appWidth,buttonH);}
 txt(CANCELCREATE,et(CANCEL_E));

 pos(APPGEST,x,contentTop,fw,buttonH,false);pos(APPBACK,x,contentTop,100,buttonH,false);
 for(int id=NAMEBOX;id<=STARTBOX;id++)if(id!=SAVEBTN)hide(id);for(int id=LABNAME;id<=ACTHINT;id++)hide(id);hide(TRAIN);hide(LABSELECT);hide(EXECBOX);hide(LABEXEC);hide(LABSENSHELP);hide(CONFIRMBOX);hide(INDICATORBOX);hide(TOPBOX);hide(STARTWINBOX);hide(ADMINBOX);hide(ADMINHELP);hide(ADMINSTATUS);hide(ADDITIONALHEADER);hide(THEMEBOX);hide(LABTHEME);hide(TRAILCOLOR);hide(TRAILWIDTH);hide(LABTRAILCOLOR);hide(LABTRAILWIDTH);hide(TRAILPICK);hide(TRAILRESET);hide(TASKBARBOX);hide(FULLSCREENBOX);hide(EXPATHBOX);hide(LABEXPATH);
 int y=base;auto row=[&](int l,int f){pos(l,x,y,lab-labelGap,fieldH);pos(f,fx,y,ed,flatCombo(f)?190:fieldH);y+=pitch;};
 if((gst||rul)&&categorySelection>=0){pos(LABSELECT,x,y+4,fw,60);txt(LABSELECT,rul?(cfg.lang==1?L"Selecione ou adicione uma exclusão nesta categoria.":L"Select or add an exclusion in this category."):et(CATEGORY_HELP));}
 else if(gst){row(LABNAME,NAMEBOX);pos(ENABLEBOX,fx,y,ed,20);y+=24;row(LABTRIGGER,TRIGGERBOX);
  int shapeH=128;pos(LABTRAIN,x,y+2,lab-labelGap,22);pos(TRAIN,fx,y,ed,shapeH);y+=shapeH+vgap;buttonGroup(shapeButtons,3,fx,y,ed,buttonH);y+=buttonH+vgap;
  row(LABEXEC,EXECBOX);pos(LABACTION,x,y+2,lab-labelGap,22);int al=clamp(h-y-footerOffset-(vgap+buttonH+vgap+pitch+28+8+24+3*pitch+2),38,64);pos(ACTLIST,fx,y,ed,al);y+=al+vgap;buttonGroup(actionButtons,4,fx,y,ed,buttonH);y+=buttonH+vgap;
  int ar=captionWidth(ACTREC);pos(LABPARAM,x,y,lab-labelGap,fieldH);pos(ARGBOX,fx,y,ed-ar-gap,fieldH);pos(ACTREC,fx+ed-ar,y,ar,fieldH);y+=pitch;pos(ACTHINT,fx,y,ed,28);y+=36;pos(ADDITIONALHEADER,x,y,fw,20);y+=24;row(LABBUTTON,BUTTONBOX);row(LABMOD,MODBOX);
  int rec=captionWidth(RECBTN),clear=captionWidth(KEYCLEAR),input=ed-rec-clear-2*gap;pos(LABKEY,x,y,lab-labelGap,fieldH);pos(KEYBOX,fx,y,input,fieldH);pos(RECBTN,fx+input+gap,y,rec,fieldH);pos(KEYCLEAR,fx+input+rec+2*gap,y,clear,fieldH);y+=pitch;pos(TESTBTN,x,h-footerOffset,maxn(76,captionWidth(TESTBTN)),buttonH);
 }else if(apps){pos(LABSELECT,x,y+4,fw,60);txt(LABSELECT,cfg.lang==1?L"Selecione ou adicione uma aplicação.":L"Select or add an application to edit its gestures.");}else if(rul&&page==2&&!validExclusionIndex(cfg,ruleIndex)){pos(LABSELECT,x,y+4,fw,60);txt(LABSELECT,tr(NEW_EXCL));}else if(rul){row(LABNAME,NAMEBOX);pos(ENABLEBOX,fx,y,ed,20);y+=24;int browse=captionWidth(BROWSEBTN);pos(LABEXE,x,y,lab-labelGap,fieldH);pos(EXEBOX,fx,y,ed-browse-gap,fieldH);pos(BROWSEBTN,fx+ed-browse,y,browse,fieldH);y+=pitch;if(page==2)row(LABEXPATH,EXPATHBOX);row(LABCLASS,CLASSBOX);row(LABTITLE,TITLEBOX);if(page==1){pos(INHERITBOX,fx,y,ed,22);}else row(LABMODE,MODEBOX);}
 int save=maxn(114,captionWidth(SAVEBTN));pos(SAVEBTN,w-margin-save,h-footerOffset,save,buttonH,!((gst||rul)&&categorySelection>=0)&&(!apps||appId)&&(!rul||page!=2||validExclusionIndex(cfg,ruleIndex)));if(rul&&page==2)EnableWindow(ctrl(DELBTN),listdata(LIST)!=-1);syncFooterLayout();applylayout();if(dialogOwnerLimited)for(int i=0;i<164;i++)if(ctrl(i)&&i!=CANCELCREATE&&IsWindowEnabled(ctrl(i)))EnableWindow(ctrl(i),0);applyPaneHeading();InvalidateRect(mainwnd,nullptr,0);layoutBusy=false;
}
static void chooseTrailColor(){struct ColorDialog{DWORD size;HWND owner;H instance;DWORD result;DWORD* custom;DWORD flags;LP data;void* hook;const W* templ;};static DWORD custom[16]{};W b[16]{};GetWindowTextW(settingsControls[TRAILCOLOR],b,16);int color=effectiveTrailColor(cfg.trailColor);parseTrailHex(b,color);H dll=LoadLibraryExW(L"comdlg32.dll",nullptr,0x800);if(!dll)return;auto choose=(B(CB*)(ColorDialog*))GetProcAddress(dll,"ChooseColorW");ColorDialog d{sizeof(d),settingsWnd,nullptr,(DWORD)color,custom,3,0,nullptr,nullptr};if(choose&&choose(&d)){W hex[8];trailHex(d.result,hex);SetWindowTextW(settingsControls[TRAILCOLOR],hex);draftTrailAuto=false;}FreeLibrary(dll);}
static void labels(){const W* indicator[]={L"Show detected gestures on the desktop",L"Mostrar gestos detetados no ambiente de trabalho",L"Mostrar gestos detectados en el escritorio",L"Afficher les gestes détectés sur le bureau",L"Erkannte Gesten auf dem Desktop anzeigen",L"Mostra i gesti rilevati sul desktop"};txt(INDICATORBOX,indicator[cfg.lang]);const W* pathLabels[]={L"Full file path",L"Caminho completo",L"Ruta completa",L"Chemin complet",L"Vollständiger Pfad",L"Percorso completo"};txt(LABEXPATH,pathLabels[cfg.lang]);const W* taskbar[]={L"Enable gestures on the taskbar",L"Ativar gestos na barra de tarefas",L"Activar gestos en la barra de tareas",L"Activer les gestes sur la barre des tâches",L"Gesten auf der Taskleiste aktivieren",L"Abilita gesti sulla barra delle applicazioni"};txt(TASKBARBOX,taskbar[cfg.lang]);const W* fullscreen[]={L"Disable gestures in fullscreen apps (includes full-screen videos)",L"Desativar gestos em aplicações em ecrã inteiro (inclui vídeos em ecrã inteiro)",L"Desactivar gestos en aplicaciones a pantalla completa (incluye vídeos a pantalla completa)",L"Désactiver les gestes dans les applications en plein écran (vidéos en plein écran incluses)",L"Gesten in Vollbildanwendungen deaktivieren (einschließlich Vollbildvideos)",L"Disabilita gesti nelle app a schermo intero (inclusi i video a schermo intero)"};txt(FULLSCREENBOX,fullscreen[cfg.lang]);const W* colors[]={L"Trail color",L"Cor do traço",L"Color del trazo",L"Couleur du tracé",L"Spurfarbe",L"Colore traccia"};const W* widths[]={L"Trail size (px)",L"Tamanho do traço (px)",L"Tamaño del trazo (px)",L"Taille du tracé (px)",L"Spurgröße (px)",L"Dimensione traccia (px)"};txt(LABTRAILCOLOR,colors[cfg.lang]);txt(LABTRAILWIDTH,widths[cfg.lang]);const W* picks[]={L"Select color",L"Selecionar cor",L"Elegir color",L"Choisir couleur",L"Farbe wählen",L"Scegli colore"};const W* resets[]={L"Reset",L"Repor",L"Restablecer",L"Réinitialiser",L"Zurücksetzen",L"Ripristina"};txt(TRAILPICK,picks[cfg.lang]);txt(TRAILRESET,resets[cfg.lang]);const W* excludedTitles[]={L"Excluded applications",L"Aplicações excluídas",L"Aplicaciones excluidas",L"Applications exclues",L"Ausgeschlossene Apps",L"Applicazioni escluse"};const W* editorTitles[]={L"Gesture editor",L"Editor de gestos",L"Editor de gestos",L"Éditeur de gestes",L"Gesteneditor",L"Editor dei gesti"};txt(APPHEAD,tr(APPS));txt(GESTHEAD,page==2?excludedTitles[cfg.lang]:tr(GESTURES));txt(EDITORHEAD,editorTitles[cfg.lang]);txt(APPNEW,cfg.lang==1?L"Adicionar aplicação":L"Add app");txt(APPCAT,et(ADD_CATEGORY_E));txt(APPEDIT,cfg.lang==1?L"Editar":L"Edit");txt(APPDEL,tr(REMOVE));static int lang=-1;txt(APPGEST,ruleEdit?tr(GESTURES):tr(APPS));if(lang==cfg.lang)return;lang=cfg.lang;int ids[]={TAB0,TAB1,TAB2,SETBTN,TOGGLE,IMPORTBTN,EXPORTBTN,NEWBTN,DELBTN,DUPBTN,RECBTN,KEYCLEAR,ADDSAMPLE,RESETTRAIN,ACTADD,ACTDEL,ACTUP,ACTDOWN,ACTREC,TESTBTN,SAVEBTN,BROWSEBTN,ENABLEBOX,INHERITBOX,OVERBOX,CLOSEBOX,STARTBOX,LABSEARCH,LABNAME,LABFOLDER,LABBUTTON,LABMOD,LABTRIGGER,LABKEY,LABSENSE,LABTRAIN,LABACTION,LABPARAM,LABEXE,LABCLASS,LABTITLE,LABMODE,LABLANG,LABTHRESH,APPBACK,PRESETBTN};int keys[]={GLOBAL,APPS,EXCL,SETTINGS,STOP,IMPORT,EXPORT,NEW,REMOVE,DUP,RECORD,CLEAR,SAMPLE,RESET_SAMPLE,ADD,REMOVE,UP,DOWN,RECORD,TEST,SAVE,BROWSE,ENABLED,INHERIT,OVERLAY,CLOSETRAY,STARTTRAY,SEARCH,NAME,FOLDER,BUTTON,MODIFIERS,TRIGGER,HOTKEY,HELP,TRAIN_HINT,ACTIONS_L,PARAM,EXE,CLASS,TITLE,MODE,LANGUAGE,THRESHOLD,OPEN,TEMPLATES};static_assert(sizeof(ids)==sizeof(keys));for(int i=0;i<(int)(sizeof(ids)/sizeof(ids[0]));i++)txt(ids[i],tr(keys[i]));SendMessageW(ctrl(SEARCHBOX),0x1501,0,(LP)tr(SEARCH));SendMessageW(ctrl(APPSEARCH),0x1501,0,(LP)tr(SEARCH));txt(APPBACK,et(BACK_E));txt(SETBTN,tr(SETTINGS));txt(IMPORTBTN,et(IMPORT_SETTINGS_E));txt(EXPORTBTN,et(EXPORT_SETTINGS_E));txt(CONFIRMBOX,et(CONFIRM_SETTING_E));txt(TOPBOX,et(ALWAYS_TOP_E));txt(STARTWINBOX,et(START_WINDOWS_E));txt(ADMINBOX,et(ADMIN_WINDOWS_E));txt(ADMINHELP,et(ADMIN_HELP_E));txt(ADMINSTATUS,et(isElevated()?ADMIN_ACTIVE_E:ADMIN_INACTIVE_E));txt(ADDSAMPLE,et(ADD_SHAPE));txt(RESETTRAIN,et(REMOVE_SHAPE));txt(LABTRAIN,et(SHAPES_LABEL));txt(LABACTION,et(ACTIONS_E));txt(LABEXEC,et(EXEC_MODE));txt(ADDITIONALHEADER,et(ADDITIONAL_OPTIONS_E));txt(LABKEY,et(OPTIONAL_HOTKEY_E));txt(LABSENSHELP,et(SENS_HELP));txt(ACTADD,et(ADD_ACTION_E));txt(NEWBTN,et(ADD_GESTURE_E));txt(CATBTN,et(ADD_CATEGORY_E));comboclear(EXECBOX);combo(EXECBOX,et(SEQ_MODE));combo(EXECBOX,et(PAR_MODE));txt(APPGEST,ruleEdit?tr(GESTURES):tr(APPS));comboclear(BUTTONBOX);for(int i=0;i<6;i++)combo(BUTTONBOX,tr(DEFAULTB+i));comboclear(MODBOX);for(int m=0;m<16;m++){W b[64]{};if(!m)cp(b,tr(NO_MOD),64);else{keyname('A',m,b,64);int n=len(b);if(n>=2)b[n-2]=0;}combo(MODBOX,b);}comboclear(TRIGGERBOX);for(int i=0;i<9;i++)combo(TRIGGERBOX,tr(SHAPE+i));comboclear(MODEBOX);for(int i=0;i<5;i++)combo(MODEBOX,tr(EXCLUSION_ALL+i));comboclear(LANGBOX);const W*langs[]={L"English",L"Português (Portugal)",L"Español",L"Français",L"Deutsch",L"Italiano"};for(auto s:langs)combo(LANGBOX,s);txt(LABTHEME,themeText(0));comboclear(THEMEBOX);for(int i=1;i<=3;i++)combo(THEMEBOX,themeText(i));}
static bool gstview(){return !settings&&(page==0||(page==1&&!ruleEdit&&appId));}
static int currentProfile(){return page==0?0:appId;}
static void gestureRow(int i,bool indented){auto&g=cfg.g[i];W b[256];cp(b,indented?L"    ":L"",256);cat(b,g.enabled?L"●  ":L"○  ",256);cat(b,g.name,256);listadd(LIST,b,i);if(i==selected&&categorySelection<0)sl(LIST,(int)SendMessageW(ctrl(LIST),0x18b,0,0)-1);}
static void categoryRow(int c,const W*filter){auto&catg=cfg.categories[c];const bool categoryMatches=contains(catg.name,filter);bool show=categoryMatches;for(int i=0;i<cfg.ng&&!show;i++)if(cfg.g[i].profile==currentProfile()&&eq(cfg.g[i].folder,catg.name)&&contains(cfg.g[i].name,filter))show=true;if(!show)return;listadd(LIST,catg.name,-2-c);if(c==categorySelection)sl(LIST,(int)SendMessageW(ctrl(LIST),0x18b,0,0)-1);if(catg.collapsed&&!*filter)return;for(int i=0;i<cfg.ng;i++)if(cfg.g[i].profile==currentProfile()&&eq(cfg.g[i].folder,catg.name)&&(categoryMatches||contains(cfg.g[i].name,filter)))gestureRow(i,true);}
static bool appMatches(int i,const W* q){auto& a=cfg.a[i];return contains(a.name,q)||contains(a.exe,q)||contains(a.path,q)||contains(a.cls,q)||contains(a.title,q)||contains(a.folder,q);}
static void rebuildApps(){if(settings||page!=1||!ctrl(APPLIST))return;if(appId){bool found=false;for(int i=0;i<cfg.na;i++)if(cfg.a[i].id==appId){appCategory=categoryIndex(cfg,-1,cfg.a[i].folder);found=true;break;}if(!found){appId=0;appCategory=-1;selected=-1;}}HWND h=ctrl(APPLIST);int top=(int)SendMessageW(h,0x18e,0,0);SendMessageW(h,0x0b,0,0);W q[128]{};get(APPSEARCH,q,128);listreset(APPLIST);auto selectLast=[&](){sl(APPLIST,(int)SendMessageW(h,0x18b,0,0)-1);};auto add=[&](int i,bool ind){W b[256];cp(b,ind?L"    ":L"",256);cat(b,cfg.a[i].enabled?L"●  ":L"○  ",256);cat(b,cfg.a[i].name,256);listadd(APPLIST,b,i);if(cfg.a[i].id==appId)selectLast();};Records<int> roots,children;int total;if(!addCount(cfg.na,cfg.nc,total)||!roots.reserve(total)||!children.reserve(cfg.na)){SendMessageW(h,0x0b,1,0);return;}int n=appSiblings(cfg,-1,roots);for(int j=0;j<n;j++){int d=roots[j];if(d>=0){if(appMatches(d,q))add(d,false);continue;}int c=-2-d,nc=appSiblings(cfg,c,children);bool match=contains(cfg.categories[c].name,q);for(int i=0;i<nc&&!match;i++)match=appMatches(children[i],q);if(!match)continue;listadd(APPLIST,cfg.categories[c].name,d);if(appCategory==c&&!appId)selectLast();if(*q||!cfg.categories[c].collapsed)for(int i=0;i<nc;i++)if(appMatches(children[i],q))add(children[i],true);}SendMessageW(h,0x197,clamp(top,0,maxn(0,(int)SendMessageW(h,0x18b,0,0)-1)),0);SendMessageW(h,0x0b,1,0);InvalidateRect(h,nullptr,0);}
static void rebuildlist(){loading=true;rebuildApps();bool redraw=IsWindowVisible(ctrl(LIST));if(redraw)SendMessageW(ctrl(LIST),0x0b,0,0);W filter[128];get(SEARCHBOX,filter,128);listreset(LIST);if(gstview()){Records<bool> seen;if(!seen.reserve(cfg.nc)){if(redraw)SendMessageW(ctrl(LIST),0x0b,1,0);loading=false;return;}for(int i=0;i<cfg.ng;i++){auto&g=cfg.g[i];if(g.profile!=currentProfile())continue;int c=categoryIndex(cfg,g.profile,g.folder);if(c>=0){if(!seen[c]){seen[c]=true;categoryRow(c,filter);}}else if(contains(g.name,filter))gestureRow(i,false);}for(int c=0;c<cfg.nc;c++)if(!seen[c]&&cfg.categories[c].profile==currentProfile())categoryRow(c,filter);}else if(page==2){auto match=[&](int i){auto&r=cfg.e[i];return contains(r.name,filter)||contains(r.exe,filter)||contains(r.path,filter)||contains(r.cls,filter)||contains(r.title,filter)||contains(r.folder,filter);};auto selectLast=[&](){sl(LIST,(int)SendMessageW(ctrl(LIST),0x18b,0,0)-1);};auto add=[&](int i){listadd(LIST,cfg.e[i].name,i);if(i==ruleIndex&&categorySelection<0)selectLast();};Records<int> roots,children;int total;if(!addCount(cfg.ne,cfg.nc,total)||!roots.reserve(total)||!children.reserve(cfg.ne)){if(redraw)SendMessageW(ctrl(LIST),0x0b,1,0);loading=false;return;}int n=ruleSiblings(cfg,-1,roots,true);for(int j=0;j<n;j++){int d=roots[j];if(d>=0){if(match(d))add(d);continue;}int c=-2-d,nc=ruleSiblings(cfg,c,children,true);bool visible=contains(cfg.categories[c].name,filter);for(int i=0;i<nc&&!visible;i++)visible=match(children[i]);if(!visible)continue;listadd(LIST,cfg.categories[c].name,d);if(categorySelection==c)selectLast();if(*filter||!cfg.categories[c].collapsed)for(int i=0;i<nc;i++)if(match(children[i]))add(children[i]);}}if(redraw){SendMessageW(ctrl(LIST),0x0b,1,0);InvalidateRect(ctrl(LIST),nullptr,0);}loading=false;}
static void actionfields(){loading=true;listreset(ACTLIST);for(int i=0;i<edit.steps;i++){W b[320];number(b,i+1);cat(b,L". ",320);cat(b,defs[clamp(edit.actions[i].type,0,ACTIONS-1)].name[cfg.lang],320);if(*edit.actions[i].arg){cat(b,L" — ",320);if(edit.actions[i].type==0){int vk,m;W friendly[128];if(parsekey(edit.actions[i].arg,vk,m)){keyname(vk,m,friendly,128);cat(b,friendly,320);}else cat(b,edit.actions[i].arg,320);}else cat(b,edit.actions[i].arg,320);}listadd(ACTLIST,b,i);}actIndex=clamp(actIndex,0,maxn(0,edit.steps-1));if(edit.steps){sl(ACTLIST,actIndex);int a=clamp(edit.actions[actIndex].type,0,ACTIONS-1);EnableWindow(ctrl(ACTREC),a==0||program_parameter::isProgram(a));txt(ACTREC,program_parameter::isProgram(a)?tr(BROWSE):tr(RECORD));SendMessageW(ctrl(ARGBOX),0xcf,a==0,0);if(a==0){int vk,m;W friendly[128];if(parsekey(edit.actions[actIndex].arg,vk,m)){keyname(vk,m,friendly,128);txt(ARGBOX,friendly);}else txt(ARGBOX,edit.actions[actIndex].arg);txt(ACTHINT,L"");}else{txt(ARGBOX,edit.actions[actIndex].arg);txt(ACTHINT,defs[a].hint);}}else{EnableWindow(ctrl(ACTREC),0);SendMessageW(ctrl(ARGBOX),0xcf,0,0);txt(ARGBOX,L"");txt(ACTHINT,L"");}loading=false;}
static void saveactionfields(){if(actIndex>=0&&actIndex<edit.steps&&edit.actions[actIndex].type!=0){get(ARGBOX,edit.actions[actIndex].arg,256);}}
static void refreshShapeCards(){
 selectedShape=clamp(selectedShape,-1,edit.count-1);
 shapeAnchor=clamp(shapeAnchor,-1,edit.count-1);
 shapeSelection&=mgcards::all(edit.count);
 if(canvas)InvalidateRect(canvas,nullptr,0);
}
static void clearShapeSelection(){
 selectedShape=shapeAnchor=-1;shapeSelection=0;refreshShapeCards();
}
static void selectSingleShape(int index){
 if(!edit.count){clearShapeSelection();return;}
 selectedShape=shapeAnchor=clamp(index,0,maxn(0,edit.count-1));
 shapeSelection=edit.count?mgcards::bit(selectedShape):0;
 refreshShapeCards();
}
static void removeSelectedShapes(){
 if(!shapeSelection||!edit.count)return;
 int keep=0,firstRemoved=edit.count;
 for(int i=0;i<edit.count;i++){
  if(shapeSelection&mgcards::bit(i)){firstRemoved=minn(firstRemoved,i);continue;}
  if(keep!=i)memcpy(edit.samples[keep],edit.samples[i],sizeof(edit.samples[0]));
  ++keep;
 }
 edit.count=keep;selectSingleShape(firstRemoved);
}
static void editfields(){loading=true;drawnN=0;if(gstview()){clearShapeSelection();txt(NAMEBOX,edit.name);check(ENABLEBOX,edit.enabled);select(BUTTONBOX,edit.trigger==6?1:edit.button);EnableWindow(ctrl(BUTTONBOX),edit.trigger!=6);select(MODBOX,edit.mods);select(TRIGGERBOX,edit.trigger);W b[128];keyname(edit.hotkey,edit.hotmods,b,128);txt(KEYBOX,b);select(EXECBOX,edit.parallel);actIndex=0;actionfields();InvalidateRect(canvas,nullptr,0);}else{txt(NAMEBOX,rule.name);check(ENABLEBOX,rule.enabled);txt(EXEBOX,rule.exe);txt(EXPATHBOX,rule.path);txt(CLASSBOX,rule.cls);txt(TITLEBOX,rule.title);check(INHERITBOX,rule.inherit);select(MODEBOX,rule.mode);}loading=false;}
static void refresh(){applyTheme();labels();rebuildlist();if(!settings&&page==2&&listdata(LIST)==-1){ruleIndex=-1;categorySelection=-1;}editfields();relayout();refreshSearchCue();for(int id=TAB0;id<=TAB2;id++)InvalidateRect(ctrl(id),nullptr,0);InvalidateRect(ctrl(SETBTN),nullptr,0);status(L"");}
static void hotkeys(){/* Keyboard hook resolves application/exclusion context before consuming a shortcut. */}
static bool persist(){if(!saveconfig(cfg,configpath)){MessageBoxW(mainwnd,tr(STATUS_ERROR),L"miniGestures",0x10);return false;}configValid=true;geometryDirty=false;firstSave=false;publish(cfg);hotkeys();status(tr(STATUS_SAVED));return true;}
static void choosefirst(){gestureDraft=false;categorySelection=-1;if(!settings&&page==1){ruleEdit=0;if(appId){bool found=false;for(int i=0;i<cfg.na;i++)if(cfg.a[i].id==appId){found=true;break;}if(!found){appId=0;appCategory=-1;}}if(!appId){selected=-1;return;}}if(gstview()){selected=-1;for(int i=0;i<cfg.ng;i++)if(cfg.g[i].profile==(page==0?0:appId)){selected=i;break;}if(selected>=0)edit=cfg.g[selected];else{initgesture(edit,cfg.nextid,page==0?0:appId);cp(edit.name,tr(NEW_GESTURE),96);}}else{ruleIndex=0;int n=page==1?cfg.na:cfg.ne;if(n){rule=(page==1?cfg.a:cfg.e)[0];if(page==2){int c=ruleParent(cfg,0,true);if(c>=0&&cfg.categories[c].collapsed){categorySelection=c;ruleIndex=-1;}}}else{ruleIndex=-1;memset(&rule,0,sizeof(rule));rule.enabled=rule.inherit=1;cp(rule.name,tr(page==1?NEW_APP:NEW_EXCL),96);}}}
static bool resolveAndSaveGesture();
static bool validateGestureEditor(bool testing=false);
static bool showEditor(int mode,const W*title,const W*initial,bool actionKeys=false);
static bool recordActionHotkey(W*out,int capacity);
static void recordstart(int mode);
static void recordOptionalHotkey();
static void scheduleConflictCheck();
static void remember();
static void saveGeometry();
static void toggleEngine(){if(exiting||!configValid)return;int old=cfg.enabled;cfg.enabled=!old;if(!persist()){cfg.enabled=old;status(L"");}else status(L"");}
static void beginShutdown();
// Exclusions are created only through the selector, never by saving an empty editor.
static bool confirmExclusionOverlap(const Rule&next,const Rule*previous=nullptr){
 if(!exclusionNeedsWarning(next,previous))return true;
 int count=0;W names[512]{};for(int i=0;i<cfg.na;i++)if(possibleRuleOverlap(cfg.a[i],next)){if(count<4){cat(names,L"\n  • ",512);cat(names,cfg.a[i].name,512);}count++;}
 if(!count)return true;
 const W* headings[]={L"This exclusion may overlap the following application profiles:",L"Esta exclusão pode coincidir com os seguintes perfis de aplicações:",L"Esta exclusión puede coincidir con los siguientes perfiles:",L"Cette exclusion peut recouper les profils suivants :",L"Dieser Ausschluss kann sich mit folgenden Anwendungsprofilen überschneiden:",L"Questa esclusione può sovrapporsi ai seguenti profili:"};
 const W* tails[]={L"The exclusion takes precedence for the selected input types. Application gestures are kept. Continue with the exclusion?",L"A exclusão tem prioridade para os tipos de entrada selecionados. Os gestos das aplicações são mantidos. Continuar com a exclusão?",L"La exclusión tiene prioridad para los tipos de entrada seleccionados. Se conservan los gestos. ¿Continuar?",L"L’exclusion est prioritaire pour les types d’entrée sélectionnés. Les gestes sont conservés. Continuer ?",L"Der Ausschluss hat für die ausgewählten Eingabearten Vorrang. Die Gesten bleiben erhalten. Fortfahren?",L"L’esclusione ha la precedenza per i tipi di input selezionati. I gesti restano salvati. Continuare?"};
 W message[1100]{};cp(message,headings[cfg.lang],1100);cat(message,names,1100);if(count>4)cat(message,L"\n  …",1100);cat(message,L"\n\n",1100);cat(message,tails[cfg.lang],1100);
 return MessageBoxW(mainwnd,message,tr(EXCL),0x1|0x30|0x100)==1;
}
static bool saveedit(){if(!settings&&page==2&&(categorySelection>=0||!validExclusionIndex(cfg,ruleIndex)||listdata(LIST)!=ruleIndex))return false;if(gstview()){saveactionfields();get(NAMEBOX,edit.name,96);edit.enabled=checked(ENABLEBOX);if(selection(TRIGGERBOX)!=6)edit.button=maxn(0,selection(BUTTONBOX));edit.mods=maxn(0,selection(MODBOX));edit.trigger=maxn(0,selection(TRIGGERBOX));edit.parallel=clamp(selection(EXECBOX),0,1);if(!validateGestureEditor())return false;return resolveAndSaveGesture();}else{get(NAMEBOX,rule.name,96);get(EXEBOX,rule.exe,260);if(page==2)get(EXPATHBOX,rule.path,260);get(CLASSBOX,rule.cls,128);get(TITLEBOX,rule.title,128);rule.enabled=checked(ENABLEBOX);rule.inherit=checked(INHERITBOX);rule.mode=clamp(selection(MODEBOX),0,4);if(!*rule.name||(!*rule.exe&&!*rule.path&&!*rule.cls&&!*rule.title)||!mgregex::valid(rule.exe)||!mgregex::valid(rule.path)||!mgregex::valid(rule.cls)||!mgregex::valid(rule.title)){error();return false;}if(page==2&&!confirmExclusionOverlap(rule,&cfg.e[ruleIndex])){rule=cfg.e[ruleIndex];editfields();return false;}int&n=page==1?cfg.na:cfg.ne;auto&a=page==1?cfg.a:cfg.e;if(ruleIndex<0){if(!reserveMore(a,n)||cfg.nextid>=0x7fffffff){error();return false;}ruleIndex=n++;rule.id=cfg.nextid++;}a[ruleIndex]=rule;}if(!persist())return false;rebuildlist();return true;}
static void setpage(int p){if(page!=p)activePaneHeading=p==1?APPHEAD:GESTHEAD;categorySelection=-1;appCategory=-1;settings=0;page=p;appId=p==1&&cfg.na?cfg.a[0].id:0;ruleEdit=0;if(appId){int c=categoryIndex(cfg,-1,cfg.a[0].folder);if(c>=0)cfg.categories[c].collapsed=0;}choosefirst();txt(SEARCHBOX,L"");refresh();}
static bool newAtRoot=false;
static bool dialogEnabled[164]{},dialogSettingsEnabled=false;
static void beginDialogOwner(){
 if(!modalCreateButton){EnableWindow(mainwnd,0);return;}
 dialogOwnerLimited=true;
 for(int i=0;i<164;i++)if(ctrl(i)){dialogEnabled[i]=IsWindowEnabled(ctrl(i));if(i!=CANCELCREATE)EnableWindow(ctrl(i),0);}
 EnableWindow(ctrl(CANCELCREATE),1);dialogSettingsEnabled=settingsWnd&&IsWindowEnabled(settingsWnd);if(dialogSettingsEnabled)EnableWindow(settingsWnd,0);
}
static void endDialogOwner(){
 if(!dialogOwnerLimited){EnableWindow(mainwnd,1);return;}
 for(int i=0;i<164;i++)if(ctrl(i))EnableWindow(ctrl(i),dialogEnabled[i]);
 if(dialogSettingsEnabled&&settingsWnd)EnableWindow(settingsWnd,1);dialogOwnerLimited=false;
}
struct CreationModalScope{CreationModalScope(int id){modalCreateButton=id;relayout();}~CreationModalScope(){modalCreateButton=0;relayout();}};
static void startGestureDraft(){if(gestureDraft)return;draftPreviousId=selected>=0&&selected<cfg.ng?cfg.g[selected].id:0;draftPreviousCategory[0]=0;if(categorySelection>=0&&categorySelection<cfg.nc)cp(draftPreviousCategory,cfg.categories[categorySelection].name,96);gestureDraft=true;}
static void cancelGestureDraft(){if(!gestureDraft)return;int oldId=draftPreviousId;W oldCategory[96];cp(oldCategory,draftPreviousCategory,96);choosefirst();if(*oldCategory){int c=categoryIndex(cfg,currentProfile(),oldCategory);if(c>=0){categorySelection=c;selected=-1;}}else if(oldId){for(int i=0;i<cfg.ng;i++)if(cfg.g[i].id==oldId&&cfg.g[i].profile==currentProfile()){selected=i;edit=cfg.g[i];break;}}refresh();SetFocus(ctrl(LIST));}
static void cancelCreation(){if(modalCreateButton){if(popup)SendMessageW(popup,0x10,0,0);return;}cancelGestureDraft();}
static bool createCategoryDialog(int buttonId){CreationModalScope scope(buttonId);return showEditor(3,et(NEW_CATEGORY),L"");}

static bool appSelector(Rule& r,bool exclusion);
static bool createRuleDialog(Rule& r,bool exclusion){CreationModalScope scope(exclusion?NEWBTN:APPNEW);return appSelector(r,exclusion);}
static void newitem(){if(!settings&&page==2){if(!reserveMore(cfg.e,cfg.ne)||cfg.nextid>=0x7fffffff){error();return;}Rule v{};v.enabled=1;int parent=newAtRoot?-1:categorySelection>=0?categorySelection:ruleIndex>=0?ruleParent(cfg,ruleIndex,true):-1;newAtRoot=false;if(parent>=0&&parent<cfg.nc&&cfg.categories[parent].profile==-2)cp(v.folder,cfg.categories[parent].name,96);if(!createRuleDialog(v,true)||!confirmExclusionOverlap(v))return;categorySelection=-1;if(parent>=0&&parent<cfg.nc)cfg.categories[parent].collapsed=0;v.id=cfg.nextid++;ruleIndex=cfg.ne;cfg.e[cfg.ne++]=v;if(!persist()){cfg.ne--;cfg.nextid--;choosefirst();refresh();return;}rule=v;txt(SEARCHBOX,L"");refresh();return;}if(gstview()){startGestureDraft();W folder[96]{};if(!newAtRoot&&categorySelection>=0&&categorySelection<cfg.nc)cp(folder,cfg.categories[categorySelection].name,96);else if(!newAtRoot&&selected>=0)cp(folder,cfg.g[selected].folder,96);newAtRoot=false;categorySelection=-1;selected=-1;initgesture(edit,cfg.nextid,page==0?0:appId);cp(edit.name,tr(NEW_GESTURE),96);cp(edit.folder,folder,96);}else{ruleIndex=-1;memset(&rule,0,sizeof(rule));rule.enabled=rule.inherit=1;cp(rule.name,tr(page==1?NEW_APP:NEW_EXCL),96);}editfields();relayout();sl(LIST,-1);SetFocus(ctrl(NAMEBOX));SendMessageW(ctrl(NAMEBOX),0xb1,0,-1);}
static bool filechoose(W*path,bool save,bool exe=false,HWND owner=nullptr){if(!resolve_dialogs())return false;OPENFILE of{};of.lStructSize=sizeof(of);of.hwndOwner=owner?owner:mainwnd;of.lpstrFilter=exe?L"Windows executable\0*.exe\0All files\0*.*\0\0":L"miniGestures JSON\0*.json\0\0";of.lpstrFile=path;of.nMaxFile=1024;of.Flags=0x80000|0x800|0x1000|8|(save?2:0);of.lpstrDefExt=exe?L"exe":L"json";return save?GetSaveFileNameW(&of):GetOpenFileNameW(&of);}
static void ensureUI();

// Leave all animation to the desktop compositor. Pair the nonactivating
// tray hide with a native visibility transition; never layer or move the UI.
static constexpr UINT hideToTrayFlags=1|2|4|16|0x200|0x80;
static constexpr UINT showFromTrayFlags=1|2|4|0x200|0x40;
static bool mainShownOnce=false;
static void showNativeMain(bool maximized){
 if(IsWindowVisible(mainwnd)&&!IsIconic(mainwnd))return;
 enableNativeWindowTransitions(mainwnd);
 if(IsIconic(mainwnd))ShowWindow(mainwnd,9); // SW_RESTORE
 else if(!mainShownOnce)ShowWindow(mainwnd,maximized?3:1);
 else if(maximized&&!IsZoomed(mainwnd))ShowWindow(mainwnd,3);
 else if(!maximized&&IsZoomed(mainwnd))ShowWindow(mainwnd,9);
 else SetWindowPos(mainwnd,nullptr,0,0,0,0,showFromTrayFlags); // SWP_SHOWWINDOW
 // A launcher's STARTUPINFO can override the first ShowWindow command.
 if(!IsWindowVisible(mainwnd)){
  if(maximized&&!IsZoomed(mainwnd))ShowWindow(mainwnd,3);
  else SetWindowPos(mainwnd,nullptr,0,0,0,0,showFromTrayFlags);
 }
 if(IsWindowVisible(mainwnd))mainShownOnce=true;
}
static void openmain(){testTarget=identity(GetForegroundWindow());ensureUI();showNativeMain(cfg.maximized!=0);SetForegroundWindow(mainwnd);remember();scheduleConflictCheck();}
static void hideEditor(bool fromTray=false){
 remember();
 enableNativeWindowTransitions(mainwnd);
 // Hide without requesting a second foreground activation from Explorer.
 SetWindowPos(mainwnd,nullptr,0,0,0,0,hideToTrayFlags);
 if(IsWindowVisible(mainwnd))ShowWindow(mainwnd,0);
 // A tray click never waits for a synchronous config write or cache rebuild.
 if(fromTray){if(geometryDirty&&configValid)SetTimer(mainwnd,95,750,nullptr);}
 else {saveGeometry();previewCache.clear();}
}
static void toggleEditor(){if(popup||record){openmain();return;}if(IsWindowVisible(mainwnd)&&!IsIconic(mainwnd))hideEditor(true);else openmain();}
static Snapshot* pendingTest=nullptr;
static bool usableTestTarget(const Target&t){return t.hwnd&&t.pid&&t.pid!=GetCurrentProcessId()&&valid(t)&&IsWindowVisible(t.hwnd);}
static void finishTest(){KillTimer(mainwnd,99);auto*s=pendingTest;pendingTest=nullptr;if(!s)return;Target t=identity(GetForegroundWindow());if(!usableTestTarget(t)){release(s);ShowWindow(mainwnd,9);status(et(TEST_TARGET_E));MessageBoxW(mainwnd,et(TEST_TARGET_E),L"miniGestures",0x30);return;}auto*j=allocateJob();if(!j){release(s);status(et(QUEUE_ERROR_E));return;}j->s=s;j->t=t;j->direct=1; if(!postjob(j)){status(et(QUEUE_ERROR_E));ShowWindow(mainwnd,9);}}
static void doTest(){if(pendingTest)return;if(!running||noHooks||!workready||!actionready||WaitForSingleObject(workready,0)!=0||WaitForSingleObject(actionready,0)!=0||shouldStop()){MessageBoxW(mainwnd,et(TEST_ENGINE_E),L"miniGestures",0x30);return;}saveactionfields();edit.parallel=clamp(selection(EXECBOX),0,1);if(!validateGestureEditor(true))return;auto*s=buildTestSnapshot(cfg,edit);if(!s){status(et(QUEUE_ERROR_E));return;}pendingTest=s;status(et(TEST_WAIT_E));ShowWindow(mainwnd,6);if(!SetTimer(mainwnd,99,300,nullptr)){pendingTest=nullptr;release(s);ShowWindow(mainwnd,9);status(et(QUEUE_ERROR_E));}}
static void recordstart(int mode){if(mode==1){recordOptionalHotkey();return;}record=mode;InterlockedExchange(&recording,1);if(hooktid&&hookready&&WaitForSingleObject(hookready,0)==0)PostThreadMessageW(hooktid,MSG_REFRESH_HOOKS,0,0);txt(mode==1?KEYBOX:ARGBOX,tr(RECORD_HINT));SetFocus(ctrl(mode==1?KEYBOX:ARGBOX));}
static void recordend(int key,int m,bool cancel){if(!record)return;if(record==1){if(!cancel){edit.hotkey=key;edit.hotmods=m;}W b[128];keyname(edit.hotkey,edit.hotmods,b,128);txt(KEYBOX,b);}else{if(!cancel){W b[128];keyname('A',m,b,128);int l=len(b);if(l)b[l-1]=0;cat(b,L"VK",128);W n[16];number(n,key);cat(b,n,128);txt(ARGBOX,b);saveactionfields();}else if(edit.steps)txt(ARGBOX,edit.actions[actIndex].arg);}int was=record;record=0;InterlockedExchange(&recording,0);if(hooktid&&hookready&&WaitForSingleObject(hookready,0)==0)PostThreadMessageW(hooktid,MSG_REFRESH_HOOKS,0,0);if(was==2)actionfields();}
static LR CB editproc(HWND h,UINT m,WP w,LP l){if(h==ctrl(KEYBOX)&&gstview()&&!popup&&!record){if(m==0x201){SetFocus(h);return 0;}if(m==0x202||(m==0x100&&(w==13||w==32))){recordOptionalHotkey();return 0;}}if(record&&(m==0x100||m==0x104)){int k=(int)w;if(screenshotShortcut(k,mods()))return CallWindowProcW(recordOld,h,m,w,l);if(k==0x1b){recordend(0,0,true);return 0;}if(k==0x10||k==0x11||k==0x12||k==0x5b||k==0x5c)return 0;recordend(k,mods(),false);return 0;}if(record&&(m==0x102||m==0x106))return 0;if(m==8&&record)recordend(0,0,true);return CallWindowProcW(recordOld,h,m,w,l);}
static void paintline(HDC dc,const P*p,int n,int dx,int dy,int scale=1){if(n<2)return;POINT pts[maxDrawingPoints];n=minn(n,maxDrawingPoints);for(int i=0;i<n;i++)pts[i]={p[i].x/scale+dx,p[i].y/scale+dy};H pen=CreatePen(0,px(3),accent);H old=SelectObject(dc,pen);Polyline(dc,pts,n);SelectObject(dc,old);DeleteObject(pen);}
static void shapeArrow(HDC dc,RECT area,const POINT* p,int n,int thickness,DWORD bg){if(n<2)return;POINT tip=p[n-1];int k=n-2;while(k>=0&&maxn(absn(tip.x-p[k].x),absn(tip.y-p[k].y))<px(5))k--;POINT head[3]{};const POINT* headPtr=nullptr;int headCount=0;if(k>=0){int dx=tip.x-p[k].x,dy=tip.y-p[k].y;int mag=maxn(absn(dx),absn(dy));if(mag>0){int size=minn(px(9),maxn(px(4),mag));int vx=dx*size/mag,vy=dy*size/mag;head[0]={tip.x-vx-vy/2,tip.y-vy+vx/2};head[1]=tip;head[2]={tip.x-vx+vy/2,tip.y-vy-vx/2};headPtr=head;headCount=3;}}if(!drawPolylineAA(dc,area,p,n,headPtr,headCount,accent,bg,thickness)){flatLine(dc,p,n,accent,thickness);if(headCount)flatLine(dc,head,3,accent,thickness);}}
// The main editor shows clickable cards for all saved shapes. Only the
// modal freehand canvas captures strokes; new drawings append to the gesture.
static mgcards::Grid shapeCardGrid(const RECT& r){
 return mgcards::fit(r.right-r.left,r.bottom-r.top,clamp(edit.count,0,MAXS),maxn(1,px(3)),maxn(1,px(4)),px(82),px(72));
}
// Selection gestures belong only to the main editor, never the freehand canvas.
static struct ShapeDrag {bool active,marquee,ctrl,shift;int x,y,hit,oldFocus,oldAnchor;unsigned base;mgcards::Rect rect;} shapeDrag{};
static void updateShapeDrag(HWND h,int x,int y){
 if(!shapeDrag.marquee&&maxn(absn(x-shapeDrag.x),absn(y-shapeDrag.y))<maxn(2,px(4)))return;
 shapeDrag.marquee=true;
 RECT r{};GetClientRect(h,&r);
 shapeDrag.rect=mgcards::box(shapeDrag.x,shapeDrag.y,x,y);
 // Clamp the visible rectangle to the panel; intersection uses identical card geometry.
 auto& q=shapeDrag.rect;q.left=clamp(q.left,0,r.right);q.right=clamp(q.right,0,r.right);q.top=clamp(q.top,0,r.bottom);q.bottom=clamp(q.bottom,0,r.bottom);
 auto hits=mgcards::intersecting(shapeCardGrid(r),q);
 shapeSelection=mgcards::marquee(shapeDrag.base,hits,shapeDrag.ctrl,shapeDrag.shift)&mgcards::all(edit.count);
 InvalidateRect(h,nullptr,0);
}
static LR CB canvasproc(HWND h,UINT m,WP w,LP l){
 bool drawing=h==drawCanvas;
 if(h==canvas&&!drawing){
  if(m==0x201){
   if(shapeDrag.active)return 0;
   RECT r{};GetClientRect(h,&r);int x=(short)LO(l),y=(short)HI(l),hit=shapeCardGrid(r).hit(x,y);
   SetFocus(h);
   shapeDrag={true,false,(w&8)!=0,(w&4)!=0,x,y,hit,selectedShape,shapeAnchor,shapeSelection,{}};
   SetCapture(h);return 0;
  }
  if(m==0x200&&shapeDrag.active){updateShapeDrag(h,(short)LO(l),(short)HI(l));return 0;}
  if(m==0x202&&shapeDrag.active){
   updateShapeDrag(h,(short)LO(l),(short)HI(l));
   if(!shapeDrag.marquee){
    int hit=shapeDrag.hit;
    shapeSelection=mgcards::click(shapeDrag.base,shapeAnchor,hit,shapeDrag.ctrl,shapeDrag.shift)&mgcards::all(edit.count);
    if(hit>=0){selectedShape=hit;if(!shapeDrag.shift||shapeAnchor<0)shapeAnchor=hit;}else if(!shapeDrag.ctrl&&!shapeDrag.shift){selectedShape=shapeAnchor=-1;}
   }else if(shapeSelection){
    // Prefer the first selected card as the anchor for a subsequent Shift range.
    for(int i=0;i<edit.count;i++)if(shapeSelection&mgcards::bit(i)){selectedShape=shapeAnchor=i;break;}
   }
   shapeDrag.active=false;ReleaseCapture();refreshShapeCards();return 0;
  }
  if((m==0x215||m==0x1f)&&shapeDrag.active){
   shapeSelection=shapeDrag.base;selectedShape=shapeDrag.oldFocus;shapeAnchor=shapeDrag.oldAnchor;
   shapeDrag.active=false;if(m==0x1f)ReleaseCapture();refreshShapeCards();return 0;
  }
  if(m==0x87)return 1; // DLGC_WANTARROWS; Tab navigation remains native.
  if(m==0x100&&edit.count>0){
   RECT r{};GetClientRect(h,&r);auto grid=shapeCardGrid(r);int next=selectedShape;
   switch(w){case 0x25:next=next<0?0:next-1;break;case 0x27:next=next<0?0:next+1;break;case 0x26:next=next<0?0:next-grid.columns;break;case 0x28:next=next<0?0:next+grid.columns;break;case 0x24:next=0;break;case 0x23:next=edit.count-1;break;default:return DefWindowProcW(h,m,w,l);}
   next=clamp(next,0,edit.count-1);
   if((GetAsyncKeyState(0x10)&0x8000)&&shapeAnchor>=0){selectedShape=next;shapeSelection=mgcards::range(shapeAnchor,next)&mgcards::all(edit.count);refreshShapeCards();}
   else selectSingleShape(next);
   return 0;
  }
 }
 if(drawing&&(m==0x201||m==0x204)){if(drawdrag)return 0;drawnN=1;drawn[0]={(short)LO(l),(short)HI(l)};drawdrag=m==0x201?1:2;SetCapture(h);InvalidateRect(h,nullptr,0);return 0;}
 if(drawing&&m==0x200&&drawdrag){P p{(short)LO(l),(short)HI(l)};appendDrawingPoint(drawn,drawnN,p);InvalidateRect(h,nullptr,0);return 0;}
 if(drawing&&(m==0x202||m==0x205)){if(drawdrag==(m==0x202?1:2)){P q{(short)LO(l),(short)HI(l)};appendDrawingPoint(drawn,drawnN,q,true);drawdrag=0;ReleaseCapture();InvalidateRect(h,nullptr,0);}return 0;}
 if(drawing&&m==0x7b)return 0;
 if(drawing&&(m==0x215||m==0x1f)){bool had=drawdrag!=0;drawdrag=0;if(m==0x1f&&had)ReleaseCapture();return 0;}
 if(m==0x14)return 1;
 if(m==0xf){PAINT p;HDC dc=BeginPaint(h,&p);RECT r{};GetClientRect(h,&r);if(r.right<=0||r.bottom<=0){EndPaint(h,&p);return 0;}HDC b=CreateCompatibleDC(dc);H bm=b?CreateCompatibleBitmap(dc,r.right,r.bottom):nullptr;if(!b||!bm){if(b)DeleteDC(b);FillRect(dc,&r,fieldBrush);EndPaint(h,&p);return 0;}H old=SelectObject(b,bm);FillRect(b,&r,fieldBrush);
  if(drawing){if(drawnN>1){POINT points[maxDrawingPoints];int n=minn(drawnN,maxDrawingPoints);for(int i=0;i<n;i++)points[i]={drawn[i].x,drawn[i].y};shapeArrow(b,r,points,n,px(3),fieldFill);}else textFlat(b,et(DRAW_HELP),r,muted,1|0x20|4|0x8000);}
  else if(edit.count){
   auto grid=shapeCardGrid(r);
   for(int i=0;i<grid.count;i++){
    auto cell=grid.card(i);RECT q{cell.left,cell.top,cell.right,cell.bottom};
    bool chosen=(shapeSelection&mgcards::bit(i))!=0;
    DWORD bg=chosen?rowSelectionFill(highContrastTheme):fieldFill;
    DWORD fg=chosen&&highContrastTheme?accentText:accent;
    rounded(b,q,maxn(1,px(4)),bg,chosen?accent:frame,true,fieldFill);
    int pad=maxn(2,px(5));RECT inner{q.left+pad,q.top+pad,q.right-pad,q.bottom-pad};
    if(grid.height>=px(42)&&grid.width>=px(30)){
     W index[16];number(index,i+1);RECT label{inner.left,q.bottom-px(16),inner.right,q.bottom-px(2)};
     textFlat(b,index,label,chosen?rowSelectionText(highContrastTheme):muted,1|4|0x20);
     inner.bottom=label.top;
    }
    if(inner.right-inner.left>=px(10)&&inner.bottom-inner.top>=px(10))
     paintShapePreview(b,edit.samples[i],inner,fg,bg,maxn(2,px(5)),maxn(1,px(2)));
   }
  }
  else textFlat(b,et(NO_SHAPES),r,muted,1|4|0x20);
  if(!drawing&&h==canvas&&shapeDrag.active&&shapeDrag.marquee){
   const auto q=shapeDrag.rect;int t=maxn(1,px(1));
   if(q.right>q.left&&q.bottom>q.top){
    fillSolid(b,{q.left,q.top,q.right,minn(q.bottom,q.top+t)},accent);
    fillSolid(b,{q.left,maxn(q.top,q.bottom-t),q.right,q.bottom},accent);
    fillSolid(b,{q.left,q.top,minn(q.right,q.left+t),q.bottom},accent);
    fillSolid(b,{maxn(q.left,q.right-t),q.top,q.right,q.bottom},accent);
   }
  }
  BitBlt(dc,0,0,r.right,r.bottom,b,0,0,0xcc0020);SelectObject(b,old);DeleteObject(bm);DeleteDC(b);EndPaint(h,&p);return 0;
 }return DefWindowProcW(h,m,w,l);
}
static LR CB overlayproc(HWND h,UINT m,WP w,LP l){if(m==0x84)return -1;if(m==0x14)return 1;if(m==0xf){PAINT p;BeginPaint(h,&p);EndPaint(h,&p);return 0;}return DefWindowProcW(h,m,w,l);}
static HICON desiredTrayIcon(){if(updateAvailable){HICON icon=cfg.enabled?updateIcon:pausedUpdateIcon;if(icon)return icon;}return !cfg.enabled&&pausedIcon?pausedIcon:appicon;}
static void updateTrayIcon(){if(!trayAdded||exiting)return;HICON icon=desiredTrayIcon();if(icon==trayCurrentIcon)return;NID change{};change.cbSize=sizeof(change);change.hWnd=mainwnd;change.uID=1;change.uFlags=2;change.hIcon=icon;if(Shell_NotifyIconW(1,&change))trayCurrentIcon=icon;}
static void tray(){trayClicks={};nid={};nid.cbSize=sizeof(nid);nid.hWnd=mainwnd;nid.uID=1;nid.uFlags=1|2|4;nid.uCallbackMessage=0x8010;nid.hIcon=desiredTrayIcon();cp(nid.szTip,L"miniGestures",128);trayAdded=Shell_NotifyIconW(0,&nid)!=0;trayCurrentIcon=trayAdded?nid.hIcon:nullptr;}
static void appendEditorShape(const P*points);
static void templates(){if(edit.count>=MAXS){MessageBoxW(mainwnd,et(SHAPE_LIMIT),L"miniGestures",0x30);return;}HMENU m=CreatePopupMenu(),pairs=CreatePopupMenu(),letters=CreatePopupMenu(),reverse=CreatePopupMenu(),symbols=CreatePopupMenu();const W* arrows[]={L"↑",L"↗",L"→",L"↘",L"↓",L"↙",L"←",L"↖"};for(int a=0;a<8;a++){AppendMenuW(m,0,2000+a,arrows[a]);HMENU sub=CreatePopupMenu();for(int b=0;b<8;b++){W t[12];cp(t,arrows[a],12);cat(t,L"  ",12);cat(t,arrows[b],12);AppendMenuW(sub,0,2100+a*8+b,t);}AppendMenuW(pairs,0x10,(up)sub,arrows[a]);}AppendMenuW(m,0x10,(up)pairs,tr(DIRECTION_PAIRS));for(int i=0;i<SHAPES;i++){AppendMenuW(i<36?letters:symbols,0,2200+i,shapes[i].name);AppendMenuW(reverse,0,2300+i,shapes[i].name);}AppendMenuW(m,0x10,(up)letters,tr(LETTERS_NUMBERS));AppendMenuW(m,0x10,(up)symbols,tr(SYMBOLS));AppendMenuW(m,0x10,(up)reverse,tr(REVERSED));RECT r;GetWindowRect(ctrl(PRESETBTN),&r);UINT a=TrackPopupMenu(m,0x102,r.left,r.bottom,0,mainwnd,nullptr);DestroyMenu(m);P p[N];if(templatePoints(a,p))appendEditorShape(p);}

#include "editor_popup.hpp"
static void recordOptionalHotkey(){if(popup||!gstview()||categorySelection>=0)return;if(showEditor(5,et(HOTKEY_TITLE_E),L"")){edit.hotkey=popupVk;edit.hotmods=popupMods;W b[128];keyname(edit.hotkey,edit.hotmods,b,128);txt(KEYBOX,b);}}
#include "categories_ui.hpp"
#include "apps_ui.hpp"
// One actionable validation dialog, shared by Save and Test.
static bool fixPrompt(int message){KillTimer(mainwnd,101);confirmText=et(message);return showEditor(7,et(VALIDATION_TITLE_E),L"");}
static void specificError(int message){MessageBoxW(mainwnd,et(message),L"miniGestures",0x10);}
static bool validateGestureEditor(bool testing){
 if(!testing){if(!hasTrigger(edit)){if(fixPrompt(NO_TRIGGER_E))PostMessageW(mainwnd,0x111,edit.trigger==0?ADDSAMPLE:RECBTN,0);return false;}
 if(edit.hotkey==0x7b&&edit.hotmods==7){if(fixPrompt(RESERVED_HOTKEY_E))recordstart(1);return false;}
 if(!*edit.name){if(fixPrompt(MISSING_NAME_E)){SetFocus(ctrl(NAMEBOX));SendMessageW(ctrl(NAMEBOX),0xb1,0,-1);}return false;}}
 if(!edit.steps){if(fixPrompt(MISSING_ACTION_E))PostMessageW(mainwnd,0x111,ACTADD,0);return false;}
 for(int i=0;i<edit.steps;i++)if(edit.actions[i].type==0){int k,m;if(!parsekey(edit.actions[i].arg,k,m)||(modifierKey(k)&&m!=0)){actIndex=i;actionfields();if(!fixPrompt(MISSING_ACTION_KEY_E))return false;W arg[256]{};if(!recordActionHotkey(arg,256))return false;cp(edit.actions[i].arg,arg,256);actionfields();}}
 for(int i=0;i<edit.steps;i++)if(program_parameter::isProgram(edit.actions[i].type)){W path[256]{},args[256]{},combined[256]{};program_parameter::split(edit.actions[i].arg,path,args);if(!program_parameter::combine(path,args,combined)){actIndex=i;actionfields();if(!editProgramAction(edit.actions[i].type,edit.actions[i].arg))return false;actionfields();}}
 return true;
}
static int conflictReason(int kind){return kind==1?HOTKEY_CONFLICT_E:kind==2?MOUSE_CONFLICT_E:kind==3?SHAPE_CONFLICT_E:RUNTIME_CONFLICT_E;}
static void appendShapeInfo(W*msg,int cap,int index){if(index<0)return;W n[16];cat(msg,L" (",cap);cat(msg,et(SHAPE_INDEX_E),cap);cat(msg,L" ",cap);number(n,index+1);cat(msg,n,cap);cat(msg,L")",cap);}
static int conflictNoticeA=0,conflictNoticeB=0;
static int gestureById(int id){for(int i=0;i<cfg.ng;i++)if(cfg.g[i].id==id)return i;return -1;}
static void scheduleConflictCheck(){if(mainwnd&&!exiting)SetTimer(mainwnd,101,200,nullptr);}
static void gotoGesture(int i){if(i<0||i>=cfg.ng)return;openmain();settings=0;page=cfg.g[i].profile?1:0;appId=cfg.g[i].profile;appCategory=-1;for(int a=0;a<cfg.na;a++)if(cfg.a[a].id==appId){int c=categoryIndex(cfg,-1,cfg.a[a].folder);if(c>=0)cfg.categories[c].collapsed=0;}ruleEdit=0;categorySelection=-1;selected=i;edit=cfg.g[i];int cat=categoryIndex(cfg,appId,edit.folder);if(cat>=0)cfg.categories[cat].collapsed=0;txt(SEARCHBOX,L"");refresh();SetFocus(ctrl(NAMEBOX));}
static void warnConflict(int a,int b){if(popup||exiting||record||a<=0||b<=0)return;int i=gestureById(a),j=gestureById(b);if(i<0||j<0)return;if((a==conflictNoticeA&&b==conflictNoticeB)||(b==conflictNoticeA&&a==conflictNoticeB))return;conflictNoticeA=a;conflictNoticeB=b;auto f=pairConflict(cfg,cfg.g[i],cfg.g[j],j);if(f.other<0)return;W msg[640];cp(msg,et(conflictReason(f.kind)),640);cat(msg,L"\n\n",640);cat(msg,cfg.g[i].name,640);appendShapeInfo(msg,640,f.ownShape);cat(msg,L"\n",640);cat(msg,cfg.g[j].name,640);appendShapeInfo(msg,640,f.otherShape);openmain();KillTimer(mainwnd,101);confirmText=msg;if(showEditor(7,et(CONFLICT_TITLE_E),L"")){gotoGesture(j);KillTimer(mainwnd,101);if(f.kind==3){InvalidateRect(canvas,nullptr,0);SetFocus(ctrl(RESETTRAIN));}else SetFocus(ctrl(f.kind==1?RECBTN:f.kind==2?TRIGGERBOX:ADDSAMPLE));}}
static void checkExistingConflicts(){if(popup||exiting||record||!IsWindowVisible(mainwnd))return;for(int i=0;i<cfg.ng;i++){auto f=bindingConflict(cfg,cfg.g[i],i);if(f.other>=0){int a=cfg.g[i].id,b=cfg.g[f.other].id;if((a==conflictNoticeA&&b==conflictNoticeB)||(b==conflictNoticeA&&a==conflictNoticeB))continue;warnConflict(a,b);return;}}}
static bool drawGestureShape(P*out){
 if(!showEditor(1,et(DRAW_SHAPE),L"")||!normalize(drawn,drawnN,out))return false;
 P canonical[N];auto match=closestTemplate(out,canonical);if(!match.id||sameShape(out,canonical))return true;
 const W* arrows[]={L"↑",L"↗",L"→",L"↘",L"↓",L"↙",L"←",L"↖"};W name[48]{};
 if(match.id<2008)cp(name,arrows[match.id-2000],48);
 else if(match.id<2164){cp(name,arrows[(match.id-2100)/8],48);cat(name,L" ",48);cat(name,arrows[(match.id-2100)%8],48);}
 else if(match.id<2300)cp(name,shapes[match.id-2200].name,48);
 else{cp(name,shapes[match.id-2300].name,48);cat(name,L" (",48);cat(name,tr(REVERSED),48);cat(name,L")",48);}
 W msg[512],n[16];cp(msg,et(SNAP_TEMPLATE_E),512);cat(msg,L"\n\n",512);cat(msg,name,512);cat(msg,L" (",512);number(n,match.score);cat(msg,n,512);cat(msg,L"%)\n\n",512);cat(msg,et(SNAP_KEEP_E),512);
 if(MessageBoxW(mainwnd,msg,L"miniGestures",0x24)==6)memcpy(out,canonical,sizeof(canonical));return true;
}
// Shapes belong to the gesture, even if other gestures have similar strokes.
static void appendEditorShape(const P*points){if(edit.count>=MAXS){specificError(SHAPE_LIMIT);return;}int index=edit.count;memcpy(edit.samples[edit.count++],points,sizeof(edit.samples[0]));drawnN=0;selectSingleShape(index);}
static bool resolveAndSaveGesture(){
 Config transaction;Config*next=&transaction;if(!next->copyFrom(cfg)||(selected<0&&(!reserveMore(next->g,next->ng)||next->nextid>=0x7fffffff))){specificError(MEMORY_FAILURE_E);return false;}
 for(;;){auto f=bindingConflict(*next,edit,selected);if(f.other<0)break;W msg[768];cp(msg,et(conflictReason(f.kind)),768);cat(msg,L"\n\n",768);cat(msg,next->g[f.other].name,768);appendShapeInfo(msg,768,f.otherShape);if(f.kind==1){W key[128];keyname(edit.hotkey,edit.hotmods,key,128);cat(msg,L" — ",768);cat(msg,key,768);}else if(f.kind==2){cat(msg,L" — ",768);cat(msg,tr(SHAPE+edit.trigger),768);}cat(msg,L"\n\n",768);cat(msg,et(TRANSFER_HELP_E),768);shapeConflict=f.kind==3;confirmText=msg;KillTimer(mainwnd,101);
 if(!showEditor(6,et(CONFLICT_TITLE_E),L"")){return false;}
 if(conflictChoice==2){if(f.kind==3){P sample[N];if(drawGestureShape(sample)){memcpy(edit.samples[f.ownShape],sample,sizeof(sample));InvalidateRect(canvas,nullptr,0);}}else if(f.kind==1){recordstart(1);}else SetFocus(ctrl(TRIGGERBOX));return false;}
 removeBinding(next->g[f.other],f);
 }
 int ix=selected;Gesture committed=edit;if(ix<0){ix=next->ng++;committed.id=next->nextid++;}if(*committed.folder){int c=ensureCategory(*next,committed.profile,committed.folder);if(c<0){specificError(MEMORY_FAILURE_E);return false;}next->categories[c].collapsed=0;}next->g[ix]=committed;if(!saveconfig(*next,configpath)){specificError(SAVE_FAILURE_E);return false;}cfg.swap(*next);edit=committed;selected=ix;gestureDraft=false;configValid=true;geometryDirty=firstSave=false;publish(cfg);hotkeys();status(tr(STATUS_SAVED));rebuildlist();relayout();conflictNoticeA=conflictNoticeB=0;scheduleConflictCheck();return true;
}

// Shared append/replace picker; commit only after all dialogs succeed.
static void pickEditorAction(int replace){
 saveactionfields();if(replace>=edit.steps||replace< -1)return;
 if(replace<0&&edit.steps>=MAXA){specificError(ACTION_LIMIT_E);return;}
 if(!showEditor(2,et(SELECT_ACTION),L""))return;
 int chosen=popupAction;if(chosen<0||chosen>=ACTIONS)return;
 Act next{};next.type=chosen;
 if(replace>=0&&edit.actions[replace].type==chosen)next=edit.actions[replace];
 if(chosen==0&&!recordActionHotkey(next.arg,256))return;
 if(program_parameter::isProgram(chosen)&&!editProgramAction(chosen,next.arg))return;
 int index=commitAction(edit,replace,next);if(index>=0){actIndex=index;actionfields();}
}
static void command(int id,int code){if(loading)return;if(id==CANCELCREATE){if(code==0)cancelCreation();return;}if(dialogOwnerLimited)return;if(page==1&&appCommand(id,code))return;if(id>=TAB0&&id<=TAB2){setpage(id-TAB0);return;}if(id==SETBTN){if(code==0)openSettingsView();return;}if(id==APPBACK){settings=0;choosefirst();refresh();return;}if(id==TOGGLE){toggleEngine();return;}if(id==SEARCHBOX&&code==0x300){rebuildlist();if(!settings&&page==2){if(listdata(LIST)==-1){ruleIndex=-1;categorySelection=-1;}relayout();}return;}if(id==LIST&&code==1){int i=listdata(LIST);bool endedDraft=gestureDraft&&i!=-1;if(endedDraft)gestureDraft=false;if(i<=-2&&(gstview()||(!settings&&page==2))){categorySelection=-2-i;if(page==2)ruleIndex=-1;relayout();return;}if(i<0){if(!settings&&page==2){ruleIndex=-1;categorySelection=-1;relayout();}return;}if(!settings&&page==2&&!validExclusionIndex(cfg,i))return;bool wasCategory=categorySelection>=0||(!settings&&page==2&&!validExclusionIndex(cfg,ruleIndex));categorySelection=-1;if(gstview()){if(selected!=i){selected=i;edit=cfg.g[i];editfields();}}else if(ruleIndex!=i){ruleIndex=i;rule=(page==1?cfg.a:cfg.e)[i];editfields();}if(wasCategory||endedDraft)relayout();return;}if(id==LIST&&code==2&&!settings&&page==2){int i=listdata(LIST);if(i<=-2){toggleCategory(-2-i);return;}if(i>=0&&i<cfg.ne){Rule old=cfg.e[i],v=old;if(appSelector(v,true)&&confirmExclusionOverlap(v,&old)){cfg.e[i]=v;if(!persist())cfg.e[i]=old;ruleIndex=i;rule=cfg.e[i];refresh();}}return;}if(id==LIST&&code==2&&gstview()){int i=listdata(LIST);if(i<=-2)toggleCategory(-2-i);return;}if(id==APPGEST){if(ruleEdit){if(ruleIndex<0){error();return;}appId=cfg.a[ruleIndex].id;ruleEdit=0;}else{ruleEdit=1;appId=0;}choosefirst();refresh();return;}if(id==NEWBTN){newitem();return;}if(id==CATBTN){newCategory();return;}if(id==SAVEBTN){if(code!=0)return;if((page!=1||settings||appId)&&(settings||categorySelection<0))saveedit();return;}if(id==DUPBTN){if(gstview()&&selected>=0){startGestureDraft();saveactionfields();selected=-1;cat(edit.name,L" (2)",96);editfields();sl(LIST,-1);relayout();}else if(!gstview()&&ruleIndex>=0){ruleIndex=-1;cat(rule.name,L" (2)",96);editfields();sl(LIST,-1);}return;}if(id==DELBTN){if(code!=0||settings)return;int d=listdata(LIST);if(page==2&&d>=0){if(!validExclusionIndex(cfg,d)||!confirmDeletion(tr(CONFIRM_DELETE)))return;Rule removed=cfg.e[d];if(!removeExclusionAt(cfg,d))return;if(!persist()){for(int j=cfg.ne;j>d;j--)cfg.e[j]=cfg.e[j-1];cfg.e[d]=removed;cfg.ne++;}choosefirst();refresh();SetFocus(ctrl(LIST));return;}if((gstview()||(!settings&&page==2))&&d<=-2){deleteCategory(catFromData(d));return;}if(d<0)return;if(gstview())selected=d;else ruleIndex=d;if(!confirmDeletion(tr(CONFIRM_DELETE)))return;if(gstview()){int removedGlobal=cfg.g[selected].profile?0:cfg.g[selected].id;for(int i=selected;i<cfg.ng-1;i++)cfg.g[i]=cfg.g[i+1];cfg.ng--;if(removedGlobal)removeGlobalBlocks(cfg,0,removedGlobal);}else{int&n=page==1?cfg.na:cfg.ne;Rule*a=page==1?cfg.a:cfg.e;if(page==1){int key=a[ruleIndex].id;removeGlobalBlocks(cfg,key,0);for(int i=0;i<cfg.nc;)if(cfg.categories[i].profile==key){for(int j=i;j<cfg.nc-1;j++)cfg.categories[j]=cfg.categories[j+1];cfg.nc--;}else i++;for(int i=0;i<cfg.ng;)if(cfg.g[i].profile==key){for(int j=i;j<cfg.ng-1;j++)cfg.g[j]=cfg.g[j+1];cfg.ng--;}else i++;}for(int i=ruleIndex;i<n-1;i++)a[i]=a[i+1];n--;}persist();choosefirst();refresh();return;}if(id==BROWSEBTN){W path[1024]{};if(filechoose(path,false,true))txt(EXEBOX,path);return;}
 if(!gstview())return;if(id==TRIGGERBOX&&code==1){bool rocker=selection(TRIGGERBOX)==6;if(rocker){select(BUTTONBOX,1);}else if(!IsWindowEnabled(ctrl(BUTTONBOX))){select(BUTTONBOX,edit.button);}EnableWindow(ctrl(BUTTONBOX),!rocker);return;}if(id==RECBTN){recordstart(1);return;}if(id==KEYCLEAR){edit.hotkey=edit.hotmods=0;txt(KEYBOX,L"");return;}if(id==PRESETBTN){templates();return;}if(id==ADDSAMPLE){if(edit.count>=MAXS){specificError(SHAPE_LIMIT);return;}P sample[N];if(drawGestureShape(sample))appendEditorShape(sample);return;}if(id==RESETTRAIN){removeSelectedShapes();return;}if(id==ACTLIST&&code==1){int i=ls(ACTLIST);if(i>=0){saveactionfields();actIndex=i;actionfields();}return;}if(id==ACTLIST&&code==2){int i=ls(ACTLIST);if(i>=0&&i<edit.steps){if(edit.actions[i].type==0){if(actIndex!=i){saveactionfields();actIndex=i;actionfields();}command(ACTREC,0);}else pickEditorAction(i);}return;}if(id==ACTADD){pickEditorAction(-1);return;}if(id==ACTREC){if(!edit.steps)return;int type=edit.actions[actIndex].type;if(program_parameter::isProgram(type)){saveactionfields();if(editProgramAction(type,edit.actions[actIndex].arg))actionfields();return;}if(type!=0)return;W arg[256]{};if(recordActionHotkey(arg,256)){cp(edit.actions[actIndex].arg,arg,256);actionfields();}return;}if(id==ACTDEL){if(edit.steps){for(int i=actIndex;i<edit.steps-1;i++)edit.actions[i]=edit.actions[i+1];edit.steps--;actionfields();}return;}if(id==ACTUP||id==ACTDOWN){saveactionfields();int i=actIndex+(id==ACTUP?-1:1);if(i>=0&&i<edit.steps){Act a=edit.actions[i];edit.actions[i]=edit.actions[actIndex];edit.actions[actIndex]=a;actIndex=i;actionfields();}return;}if(id==TESTBTN){doTest();return;}}
// Single-click selects; double-click records Send hotkey or opens the shared picker for other actions.
static PROC actionListOld=nullptr;
static LR CB actionListProc(HWND h,UINT m,WP w,LP l){
 if((m==0x201||m==0x203)&&gstview()&&categorySelection<0&&!popup){
  int row=rowAt(h,l);if(row>=0&&row<edit.steps){
   if(actIndex!=row){saveactionfields();actIndex=row;actionfields();}
   sl(ACTLIST,row);SetFocus(h);
   if(m==0x203)PostMessageW(mainwnd,0x111,(WP)ACTLIST|(2u<<16),(LP)h);
  }return 0;
 }
 if(m==0x202)return 0;
 if(m==0x100&&w==0x71&&gstview()&&actIndex>=0&&actIndex<edit.steps&&(edit.actions[actIndex].type==0||program_parameter::isProgram(edit.actions[actIndex].type))){command(ACTREC,0);return 0;}
 return CallWindowProcW(actionListOld,h,m,w,l);
}
static void createcontrols(){for(int i=TAB0;i<=DUPBTN;i++){if(i==SEARCHBOX)field(i,127);else if(i==LIST)list(i);else if((i<=CATBTN||i==DELBTN||i==DUPBTN)&&i!=TOGGLE)button(i);}list(APPLIST);field(APPSEARCH,127);button(APPNEW);button(APPCAT);button(CANCELCREATE);button(APPEDIT);button(APPDEL);label(APPHEAD);label(GESTHEAD);label(EDITORHEAD);button(APPGEST);field(NAMEBOX,95);box(ENABLEBOX);cb(BUTTONBOX);cb(MODBOX);cb(TRIGGERBOX);field(KEYBOX,127);SendMessageW(ctrl(KEYBOX),0xcf,1,0);button(RECBTN);button(KEYCLEAR);field(SENSEBOX,3);canvas=make(TRAIN,L"miniGesturesCanvas",0x10000);button(ADDSAMPLE);button(RESETTRAIN);list(ACTLIST);cb(EXECBOX);label(LABEXEC);label(LABSENSHELP);field(ARGBOX,255);button(ACTADD);button(ACTDEL);button(ACTUP);button(ACTDOWN);button(ACTREC);button(TESTBTN);button(SAVEBTN);field(EXEBOX,259);field(EXPATHBOX,259);label(LABEXPATH);field(CLASSBOX,127);field(TITLEBOX,127);box(INHERITBOX);cb(MODEBOX);button(BROWSEBTN);cb(LANGBOX);cb(THEMEBOX);label(LABTHEME);field(THRESHBOX,3);box(OVERBOX);box(TASKBARBOX);box(FULLSCREENBOX);field(TRAILCOLOR,7);field(TRAILWIDTH,2);label(LABTRAILCOLOR);label(LABTRAILWIDTH);button(TRAILPICK);button(TRAILRESET);box(CLOSEBOX);box(STARTBOX);box(CONFIRMBOX);box(INDICATORBOX);box(TOPBOX);box(STARTWINBOX);box(ADMINBOX);label(ADMINHELP);label(ADMINSTATUS);label(ADDITIONALHEADER);footerWnd=CreateWindowExW(0,L"miniGesturesFooter",L"",0x46000000,0,0,1,1,mainwnd,nullptr,inst,nullptr);make(STATUSBOX,L"STATIC",0x4200);make(VERSIONBOX,L"STATIC",0x202);txt(VERSIONBOX,versionCaption);for(int i=LABSEARCH;i<=ACTHINT;i++)if(i!=LABFOLDER&&i!=LABSEARCH&&i!=LABPRIORITY)label(i);button(APPBACK);button(PRESETBTN);recordOld=(PROC)SetWindowLongPtrW(ctrl(KEYBOX),-4,(ip)editproc);SetWindowLongPtrW(ctrl(ARGBOX),-4,(ip)editproc);for(int i=0;i<164;i++)if(ctrl(i)&&!flatFrame(i)&&(flatCombo(i)||flatCheck(i)||(GetWindowLongPtrW(ctrl(i),-16)&15)==11))flatSubclass(i);flatMetrics();for(int i=0;i<164;i++)if(ctrl(i)&&(flatList(i)||flatCombo(i)||flatEdit(i)))themeNative(ctrl(i));installListSubclass();actionListOld=(PROC)SetWindowLongPtrW(ctrl(ACTLIST),-4,(ip)actionListProc);}
static void ensureUI(){if(ctrl(TAB0))return;lazyUI=false;setfont();createcontrols();hideNativeFocus(mainwnd);choosefirst();refresh();(void)0;}
static void remember(){if(!geometryReady||!mainwnd||!configValid||!IsWindowVisible(mainwnd)||IsIconic(mainwnd))return;RECT r{};int mx=IsZoomed(mainwnd);bool changed=mx!=cfg.maximized;cfg.maximized=mx;if(!mx&&GetWindowRect(mainwnd,&r)&&r.right-r.left>=300&&r.bottom-r.top>=300){int w=r.right-r.left,h=r.bottom-r.top;changed|=cfg.winx!=r.left||cfg.winy!=r.top||cfg.winw!=w||cfg.winh!=h;cfg.winx=r.left;cfg.winy=r.top;cfg.winw=w;cfg.winh=h;}if(changed){geometryDirty=true;if(!exiting)SetTimer(mainwnd,95,750,nullptr);}}
static void saveGeometry(){if(geometryDirty&&configValid&&!shutdownTestMode){if(saveconfig(cfg,configpath)){geometryDirty=false;firstSave=false;}else if(firstSave){firstSave=false;configValid=false;cfg.enabled=0;publish(cfg);status(tr(STATUS_ERROR));(void)0;}}}

#include "gesture_indicator.hpp"
static void indicatorThemeChanged(){if(indicatorWnd)InvalidateRect(indicatorWnd,nullptr,0);}
#include "update_ui.hpp"
#include "settings_window.hpp"
// A separate modal, scrollable, read-only release-notes window. GitHub text
// is displayed literally; it cannot run HTML, Markdown links, or commands.
static bool updateDialogDone=false;
static int updateDialogChoice=0; // 0 dismiss, 1 release page, 3 ignore version
static HWND updateNotesEdit=nullptr;
static LR CB updateDialogProc(HWND h,UINT m,WP w,LP l){
 if(m==0x10){updateDialogDone=true;return 0;}
 if(m==0x14)return 1;
 if(m==0xf){PAINT p{};HDC dc=BeginPaint(h,&p);FillRect(dc,&p.rcPaint,white);EndPaint(h,&p);return 0;}
 if(m==0x133||m==0x138){HDC dc=(HDC)w;bool field=(HWND)l==updateNotesEdit;SetTextColor(dc,ink);SetBkColor(dc,field?fieldFill:appFill);return (LR)(field?fieldBrush:white);}
 if(m==0x111&&HI(w)==0&&(LO(w)==1||LO(w)==2||LO(w)==3)){updateDialogChoice=LO(w)==2?0:LO(w);updateDialogDone=true;return 0;}
 return DefWindowProcW(h,m,w,l);
}
static int showUpdateNotes(){
 HWND owner=settingsWnd&&IsWindowVisible(settingsWnd)&&!IsIconic(settingsWnd)?settingsWnd:IsWindowVisible(mainwnd)&&!IsIconic(mainwnd)?mainwnd:nullptr;RECT r{};if(owner)GetWindowRect(owner,&r);else{POINT pt{};GetCursorPos(&pt);r={pt.x,pt.y,pt.x+1,pt.y+1};}
 MONITOR mon{sizeof(mon)};auto fromRect=reinterpret_cast<H(*)(const RECT*,DWORD)>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"MonitorFromRect"));H monitor=fromRect?fromRect(&r,2):MonitorFromWindow(owner?owner:mainwnd,2);bool hasMon=GetMonitorInfoW(monitor,&mon);
 int scale=(int)GetDpiForWindow(owner?owner:mainwnd);if(scale<=0)scale=96;auto u=[scale](int x){return (x*scale+48)/96;};
 int width=u(620),height=u(500);if(hasMon){width=minn(width,mon.rcWork.right-mon.rcWork.left);height=minn(height,mon.rcWork.bottom-mon.rcWork.top);}
 int x=r.left+(r.right-r.left-width)/2,y=r.top+(r.bottom-r.top-height)/2;
 if(hasMon){x=clamp(x,mon.rcWork.left,maxn(mon.rcWork.left,mon.rcWork.right-width));y=clamp(y,mon.rcWork.top,maxn(mon.rcWork.top,mon.rcWork.bottom-height));}
 WC wc{};wc.cbSize=sizeof(wc);wc.hInstance=inst;wc.lpfnWndProc=updateDialogProc;wc.hCursor=LoadCursorW(nullptr,(const W*)PTR(32512));wc.hIcon=appicon;wc.hIconSm=appicon;wc.lpszClassName=L"miniGesturesUpdateNotes";RegisterClassExW(&wc);
 HWND h=CreateWindowExW(1,wc.lpszClassName,updateText(8),0x00c80000|0x02000000,x,y,width,height,owner,nullptr,inst,nullptr);if(!h)return false;
 themeCaption(h);themeNative(h);RECT area{};GetClientRect(h,&area);int w=area.right,ht=area.bottom;
 HFONT f=CreateFontW(-u(13),0,0,0,400,0,0,0,1,0,0,5,0,L"Segoe UI");
 auto make=[&](int id,const W* cls,const W* text,DWORD style,DWORD ex,int x,int y,int width,int height){HWND c=CreateWindowExW(ex,cls,text,0x50000000|style,x,y,width,height,h,PTR(id),inst,nullptr);if(c){SendMessageW(c,0x30,(WP)(f?f:font),1);themeNative(c);}return c;};
 W versions[256]{};versionStatusText(versions,256,latestAvailable);
 HWND header=make(10,L"STATIC",versions,0,0,u(16),u(12),w-u(32),u(35));
 HWND label=make(11,L"STATIC",updateText(15),0,0,u(16),u(50),w-u(32),u(20));
 updateNotesEdit=make(12,L"EDIT",L"",0x10000|0x200000|0x4|0x40|0x800|0x1000,0x200,u(16),u(74),w-u(32),maxn(u(35),ht-u(179)));
 static W changelog[120064]{};if(!NativeUpdateNotes(changelog,120064))cp(changelog,updateText(17),120064);
 if(updateNotesEdit){SendMessageW(updateNotesEdit,0xc5,120063,0);SetWindowTextW(updateNotesEdit,changelog);SendMessageW(updateNotesEdit,0xb1,0,0);}
 HWND warning=make(13,L"STATIC",updateText(7),0,0,u(16),ht-u(96),w-u(32),u(52));
 int cancelWidth=minn(u(110),maxn(1,(w-u(44))/4)),linkWidth=minn(u(230),maxn(1,(w-u(44))/2));
 int ignoreWidth=minn(u(160),maxn(1,w-u(44)-cancelWidth-linkWidth));
 HWND ignore=make(3,L"BUTTON",updateText(14),0x10000,0,u(16),ht-u(36),ignoreWidth,u(26));
 HWND yes=make(1,L"BUTTON",updateText(16),0x10000,0,w-u(22)-cancelWidth-linkWidth,ht-u(36),linkWidth,u(26));
 HWND no=make(2,L"BUTTON",et(CANCEL_E),0x10000|1,0,w-u(16)-cancelWidth,ht-u(36),cancelWidth,u(26));
 if(!header||!label||!updateNotesEdit||!warning||!yes||!no||!ignore){DestroyWindow(h);if(f)DeleteObject(f);updateNotesEdit=nullptr;return false;}
 bool mainWasEnabled=IsWindowEnabled(mainwnd)!=0,settingsWasEnabled=settingsWnd&&IsWindowEnabled(settingsWnd);HWND savedSettings=settingsWnd;
 if(mainWasEnabled)EnableWindow(mainwnd,0);if(settingsWasEnabled)EnableWindow(savedSettings,0);
 updateDialogDone=false;updateDialogChoice=0;ShowWindow(h,5);SetForegroundWindow(h);SetFocus(no);
 MSG msg{};while(!updateDialogDone&&!exiting){int result=GetMessageW(&msg,nullptr,0,0);if(result<=0){if(!result)PostQuitMessage((int)msg.wParam);break;}if(msg.message==0x100&&msg.wParam==0x1b&&(msg.hwnd==h||GetAncestor(msg.hwnd,2)==h)){updateDialogDone=true;break;}if(!IsDialogMessageW(h,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
 int accepted=exiting?0:updateDialogChoice;DestroyWindow(h);updateNotesEdit=nullptr;if(f)DeleteObject(f);if(mainWasEnabled&&IsWindow(mainwnd))EnableWindow(mainwnd,1);if(settingsWasEnabled&&IsWindow(savedSettings))EnableWindow(savedSettings,1);if(owner&&IsWindow(owner)&&IsWindowVisible(owner))SetForegroundWindow(owner);return accepted;
}
static void presentUpdate(){
 if(!updateAvailable){updateShowPending=false;return;}if(updatePrompt||exiting||settingsBusy||popup||record||dialogOwnerLimited||!IsWindowEnabled(mainwnd))return;
 updateShowPending=false;updatePrompt=true;
 int choice=showUpdateNotes();
 if(choice==3&&!exiting){
  mgupdate::Version v{};
  if(mgupdate::parse(latestAvailable,v)){
   int a=cfg.ignoredUpdateMajor,b=cfg.ignoredUpdateMinor,c=cfg.ignoredUpdatePatch;
   cfg.ignoredUpdateMajor=v.major;cfg.ignoredUpdateMinor=v.minor;cfg.ignoredUpdatePatch=v.patch;
   if(saveconfig(cfg,configpath)){updateAvailable=false;updateShowPending=false;updateTrayIcon();updateVersionStatus();}
   else{cfg.ignoredUpdateMajor=a;cfg.ignoredUpdateMinor=b;cfg.ignoredUpdatePatch=c;MessageBoxW(settingsWnd?settingsWnd:mainwnd,tr(STATUS_ERROR),L"miniGestures",0x30);}
  }
 }else if(choice==1&&!exiting){
  // Fixed public repository URL; never execute a URL or command from release metadata.
  if(ShellExecuteW(settingsWnd?settingsWnd:mainwnd,L"open",L"https://github.com/iamfromreallife/miniGestures/releases",nullptr,nullptr,1)<=32)MessageBoxW(settingsWnd?settingsWnd:mainwnd,updateText(10),L"miniGestures",0x30);
 }
 updatePrompt=false;
}
static void tickUpdate(){
 if(exiting||settingsBusy||updatePrompt||!configValid||noHooks||shutdownTestMode)return;
 if(updateState&&!updateManual&&cfg.updateMode==0){NativeUpdateCancel();updateState=0;updateButton();return;}
 if(updateState){
  int result=NativeUpdatePoll();if(!result)return;
  bool manual=updateManual;updateState=0;updateManual=false;
  if(result==1||result==2){
   if(NativeUpdateLatest(latestAvailable,18)){updateAvailable=result==1&&!mgupdate::ignored(latestAvailable,cfg.ignoredUpdateMajor,cfg.ignoredUpdateMinor,cfg.ignoredUpdatePatch);updateShowPending=updateAvailable&&(manual||updateShowPending);}
   else result=3;
  }
  NativeUpdateCancel();updateButton();updateTrayIcon();
  if(manual&&result!=1){updatePrompt=true;if(result==2)MessageBoxW(settingsWnd?settingsWnd:mainwnd,updateText(5),L"miniGestures",0x40);else showUpdateError(settingsWnd?settingsWnd:mainwnd);updatePrompt=false;}
 }
 if(updateShowPending){presentUpdate();return;}
 if(!updateState&&!popup&&!record&&!dialogOwnerLimited)startUpdate(false);
}

// 8x8 coverage of centered search glyph; no fixed-size DPI buffers.
static void drawSearchIconAA(HDC dc,int left,int top,int size,DWORD fg,DWORD bg){
 if(!StretchDIBits||size<2)return;auto* pixels=(u32*)allocRaw((up)size*size*4);if(!pixels)return;
 for(int y=0;y<size;y++)for(int x=0;x<size;x++){int hit=0;for(int sy=0;sy<8;sy++)for(int sx=0;sx<8;sx++){
  float xx=(x+(sx+0.5f)/8)/size,yy=(y+(sy+0.5f)/8)/size;
  float dx=xx-0.38f,dy=yy-0.38f,r2=dx*dx+dy*dy;
  float t=((xx-0.59f)+(yy-0.59f))/0.56f;if(t<0)t=0;if(t>1)t=1;
  float hx=xx-(0.59f+0.28f*t),hy=yy-(0.59f+0.28f*t);
  if((r2>=0.235f*0.235f&&r2<=0.325f*0.325f)||hx*hx+hy*hy<=0.045f*0.045f)hit++;
 }pixels[y*size+x]=rounded_aa::pixel(bg,fg,fg,hit,0);}
 CornerBitmapInfo bi{40,size,-size,1,32,0,(DWORD)(size*size*4),0,0,0,0};StretchDIBits(dc,left,top,size,size,0,0,size,size,pixels,&bi,0,0xcc0020);freep(pixels);
}
static void paintframes(HDC dc){for(int i=0;i<164;i++){if(!ctrl(i)||!place[i].show||!flatFrame(i))continue;auto&p=place[i];RECT r{p.x,p.y,p.x+p.w,p.y+p.h};rounded(dc,r,px(5),fieldFill,GetFocus()==ctrl(i)?accent:frame);if(i==SEARCHBOX||i==APPSEARCH){int size=px(13);int x=r.left+px(7),y=(r.top+r.bottom-size)/2;drawSearchIconAA(dc,x,y,size,muted,fieldFill);}}}
// Exit is explicit, bounded, and only terminates our own process if shutdown stalls.
static volatile long shutdownStarted=0;
// Watchdog is already waiting before global hooks can ever be installed.
static DWORD CB shutdownWatchdog(void*){if(WaitForSingleObject(startupReady,15000)!=0){TerminateProcess(PTR(-1),8);return 8;}if(WaitForSingleObject(shutdownSignal,0xffffffff)!=0)return 1;Sleep(2500);TerminateProcess(PTR(-1),5);return 5;}
static void beginShutdown(){if(InterlockedExchange(&shutdownStarted,1))return;exiting=true;if(indicatorDragging)indicatorRemember();hideIndicator();InterlockedExchange(&running,0);InterlockedExchange(&recording,0);if(shutdownSignal)SetEvent(shutdownSignal);if(engineStop)SetEvent(engineStop);if(hooktid)PostThreadMessageW(hooktid,0x12,0,0);if(worktid)PostThreadMessageW(worktid,0x12,0,0);if(actiontid)PostThreadMessageW(actiontid,0x12,0,0);if(stopActionEvent)SetEvent(stopActionEvent);(void)0;}
static bool joinWorkers(){H threads[]={hookthread,workerthread,actionthread,metathread};DWORD start=GetTickCount();for(H t:threads)if(t){DWORD elapsed=GetTickCount()-start;DWORD wait=elapsed>=1500?0:1500-elapsed;if(WaitForSingleObject(t,wait)!=0){(void)0;return false;}}(void)0;return true;}
static void startEngine(){if(exiting||noHooks)return;(void)0;engineStop=CreateEventW(nullptr,1,0,nullptr);stopActionEvent=engineStop;metadataWake=CreateEventW(nullptr,0,0,nullptr);hookready=CreateEventW(nullptr,1,0,nullptr);workready=CreateEventW(nullptr,1,0,nullptr);actionready=CreateEventW(nullptr,1,0,nullptr);if(!engineStop||!metadataWake||!hookready||!workready||!actionready){(void)0;status(tr(STATUS_ERROR));return;}actionthread=CreateThread(nullptr,0,actionmain,nullptr,0,&actiontid);workerthread=CreateThread(nullptr,0,workmain,nullptr,0,&worktid);startupDeadline=GetTickCount();SetTimer(mainwnd,94,30,nullptr);}
static void advanceEngine(){if(exiting)return;if(!actionthread||!workerthread||GetTickCount()-startupDeadline>=3000){KillTimer(mainwnd,94);if(engineStop)SetEvent(engineStop);if(worktid)PostThreadMessageW(worktid,0x12,0,0);if(actiontid)PostThreadMessageW(actiontid,0x12,0,0);(void)0;status(tr(STATUS_ERROR));return;}if(WaitForSingleObject(actionready,0)!=0||WaitForSingleObject(workready,0)!=0)return;KillTimer(mainwnd,94);initialSnapshot=snapshot(cfg);if(!initialSnapshot){(void)0;status(tr(STATUS_ERROR));return;}ownIntegrity=integrityLevel(GetCurrentProcessId());hookthread=CreateThread(nullptr,0,hookmain,nullptr,0,&hooktid);if(!hookthread){release(initialSnapshot);initialSnapshot=nullptr;(void)0;status(tr(STATUS_ERROR));return;}metathread=CreateThread(nullptr,0,metamain,nullptr,0,&metatid);startupDeadline=GetTickCount();SetTimer(mainwnd,97,100,nullptr);(void)0;}
static void hookHealth(){static POINT prev{};static long events=0;static int misses=0;static DWORD lastRearm=0;POINT pt{};if(!mouseExpected||!hookok||!running||!GetCursorPos(&pt)){misses=0;events=mouseEvents;prev=pt;return;}long count=mouseEvents;bool changed=pt.x!=prev.x||pt.y!=prev.y;if(count!=events)misses=0;else if(changed)misses++;prev=pt;events=count;if(misses>=2&&GetTickCount()-lastRearm>=10000){misses=0;lastRearm=GetTickCount();PostThreadMessageW(hooktid,MSG_REARM,0,0);}}
#include "trail_overlay.hpp"
static LR CB mainproc(HWND h,UINT m,WP w,LP l){if(dialogOwnerLimited){if(m==0x10){cancelCreation();return 0;}if(m==0x8010||m==0x8011)return 0;}LR uiState;if(nativeFocusState(h,m,w,l,uiState))return uiState;if(m==0x1){mainwnd=h;themeCaption(h);themeNative(h);dpi=GetDpiForWindow(h);if(!dpi)dpi=96;if(!lazyUI)ensureUI();return 0;}if(m==0xa&&settingsWnd&&!settingsBusy){EnableWindow(settingsWnd,w!=0);}if(m==0x1a||m==0x31a||m==0x320||m==0x15){refreshTrailAccent();applyTheme();return DefWindowProcW(h,m,w,l);}if(m==0x5){if(w!=1&&ctrl(TAB0))relayout();if(w!=1)remember();return 0;}if(m==0x3){remember();return 0;}if(m==0x232){remember();saveGeometry();return 0;}if(m==0x2e0){dpi=LO(w);HFONT f=font,tb=tabBold;if(ctrl(TAB0))setfont();for(int i=0;i<164;i++)if(controls[i])SendMessageW(controls[i],0x30,(WP)font,1);if(ctrl(TAB0)){if(f)DeleteObject(f);if(tb)DeleteObject(tb);}if(ctrl(TAB0))flatMetrics();laidout=false;auto*r=(RECT*)l;SetWindowPos(h,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,4|16);if(ctrl(TAB0))relayout();return 0;}if(m==0x24){auto*p=(MINMAX*)l;p->minTrack=minimumOuter(h);return 0;}if(m==0x14)return 1;if(m==0xf){PAINT p;HDC dc=BeginPaint(h,&p);RECT r;GetClientRect(h,&r);FillRect(dc,&p.rcPaint,white);paintframes(dc);if(footerWnd&&place[STATUSBOX].show){int y=place[STATUSBOX].y-px(5);fillSolid(dc,{0,y,r.right,y+maxn(1,px(1))},frame);}EndPaint(h,&p);return 0;}if(m==0x2c){auto*d=(MEASUREITEM*)l;if(d->type==2||d->type==3){d->height=px(21);return 1;}}if(m==0x2b){auto*d=(DRAWITEM*)l;if(d->type==2||d->type==3)paintRow(d);else if(d->type==4)paintButton(d);else return 0;return 1;}if(m==0x138||m==0x133||m==0x135||m==0x134||m==0x136){return (LR)controlColor((HDC)w,m,(HWND)l);}if(m==0x113&&w==102){tickUpdate();return 0;}if(m==0x113&&w==100){hookHealth();return 0;}if(m==0x113&&w==99){finishTest();return 0;}if(m==0x113&&w==93){KillTimer(h,93);startEngine();return 0;}if(m==0x113&&w==94){advanceEngine();return 0;}if(m==0x113&&w==95){KillTimer(h,95);if(!exiting)saveGeometry();return 0;}if(m==0x113&&w==97){if(WaitForSingleObject(hookready,0)==0){KillTimer(h,97);}else if(GetTickCount()-startupDeadline>3000){KillTimer(h,97);SetEvent(engineStop);if(hooktid)PostThreadMessageW(hooktid,0x12,0,0);if(worktid)PostThreadMessageW(worktid,0x12,0,0);if(actiontid)PostThreadMessageW(actiontid,0x12,0,0);(void)0;status(tr(STATUS_ERROR));}return 0;}if(m==MSG_ENGINE_READY){KillTimer(h,97);(void)0;if(!exiting){if(!w)publish(cfg);if(hookok)SetTimer(h,100,2000,nullptr);else KillTimer(h,100);status(L"");}return 0;}if(m==0x113&&w==96){if(!hookok){if(GetTickCount()-testStart>5000){testFailed=true;KillTimer(h,96);beginShutdown();DestroyWindow(h);}return 0;}KillTimer(h,96);if(busyTestMode){auto* j=allocateJob();auto* e=(Execution*)alloc(sizeof(Execution));auto* snap=snapshot(cfg);if(j&&e&&snap){snap->c.g[0].steps=parallelTestMode?2:1;snap->c.g[0].parallel=parallelTestMode?1:0;snap->c.g[0].actions[0].type=46;cp(snap->c.g[0].actions[0].arg,L"10000",256);if(parallelTestMode)snap->c.g[0].actions[1]=snap->c.g[0].actions[0];InterlockedExchange(&actionStarted,0);j->s=snap;e->j=j;e->idx=0;e->q=100;InterlockedIncrement(&pendingJobs);if(!PostThreadMessageW(actiontid,MSG_EXECUTE,0,(LP)e)){testFailed=true;finishJob(j);freep(e);}}else{testFailed=true;freep(j);freep(e);release(snap);}}if(busyTestMode&&!testFailed){testStart=GetTickCount();SetTimer(h,98,50,nullptr);}else SetTimer(h,92,300,nullptr);return 0;}if(m==0x113&&w==98){if(actionStarted||GetTickCount()-testStart>3000){KillTimer(h,98);if(!actionStarted)testFailed=true;SetTimer(h,92,300,nullptr);}return 0;}if(m==0x113&&w==92){KillTimer(h,92);beginShutdown();DestroyWindow(h);return 0;}if(m==0x111){if(exiting)return 0;int id=LO(w),code=HI(w);if(id<164&&flatFrame(id)&&(code==0x100||code==0x200||code==4||code==5)){auto&p=place[id];RECT q{p.x,p.y,p.x+p.w,p.y+p.h};InvalidateRect(h,&q,0);}command(id,code);return 0;}if(m==0x312){if(popup&&popMode==5)return 0;if(w==1){toggleEngine();return 0;}if(!record)hotjob(cfg,HI(l),LO(l)&15);return 0;}if(m==MSG_RECORD){if(record)recordend((int)w,(int)l,w==0x1b);return 0;}if(m==0x113&&w==101){KillTimer(h,101);checkExistingConflicts();return 0;}if(m==MSG_GESTURE_TIMEOUT){if(!exiting){const W* const labels[]={L"Gesture recognition discarded — timeout",L"Reconhecimento do gesto cancelado — tempo limite",L"Reconocimiento del gesto descartado — tiempo agotado",L"Reconnaissance du geste annulée — délai dépassé",L"Gestenerkennung verworfen — Zeitlimit",L"Riconoscimento del gesto annullato — tempo scaduto"};status(labels[cfg.lang]);showIndicator(labels[cfg.lang]);}return 0;}if(m==MSG_RESULT){Result*r=takeResult((Result*)l);if(r){if(r->ok==2){showIndicator(r->name,r->score);freep(r);return 0;}if(r->ok){W b[256];gestureScoreLabel(b,256,r->name,r->score);if(r->ok<0){cat(b,L" — ",256);cat(b,et(ACTION_FAILED_E),256);showIndicator(b);}status(b);}else{W b[256];if(r->score>=0){failedGestureLabel(b,256,r->name,r->score);showIndicator(b);}else{cp(b,r->score==-2?et(AMBIGUOUS_E):r->score==-3?et(QUEUE_ERROR_E):et(EXCLUDED_E),256);if(r->score==-2)showIndicator(b);}status(b);}int ca=r->conflictId,cb=r->otherId;freep(r);if(ca&&cb)warnConflict(ca,cb);}return 0;}if(m==MSG_TRAIL){if(exiting)return 0;P q{(int)(u32)l,(int)(u32)((u64)l>>32)};if(w){clearTrailOverlay();trailN=0;}if(trailN==maxDrawingPoints)compactDrawing(trail,trailN);trail[trailN++]=q;if(trailN>=2)renderTrailOverlay(trail,trailN,effectiveTrailColor(cfg.trailColor),cfg.trailWidth);return 0;}if(m==MSG_CLEAR){trailN=0;clearTrailOverlay();return 0;}if(m==0x8010){if(exiting||settingsBusy||updatePrompt)return 0;using MessageTime=long(*)();static auto messageTime=(MessageTime)GetProcAddress(GetModuleHandleW(L"user32.dll"),"GetMessageTime");auto cmd=trayClicks.command(LO(l),messageTime?(unsigned)messageTime():GetTickCount());if(cmd==TrayToggleEditor){toggleEditor();return 0;}if(cmd==TrayToggleEngine){toggleEngine();return 0;}if(cmd==TrayContext){POINT p;GetCursorPos(&p);H menu=CreatePopupMenu();AppendMenuW(menu,0,1001,tr(OPEN));AppendMenuW(menu,0,1004,tr(SETTINGS));if(updateAvailable)AppendMenuW(menu,0,1005,updateText(8));AppendMenuW(menu,0,1002,et(cfg.enabled?PAUSE_E:RESUME_E));AppendMenuW(menu,0,1003,tr(EXIT));SetForegroundWindow(h);UINT a=TrackPopupMenu(menu,0x100|2,p.x,p.y,0,h,nullptr);DestroyMenu(menu);if(a==1001)openmain();if(a==1004)openSettingsView();if(a==1005&&updateAvailable){updateShowPending=true;presentUpdate();}if(a==1002){toggleEngine();}if(a==1003){remember();beginShutdown();saveGeometry();DestroyWindow(h);}return 0;}}if(m==taskbarmsg&&taskbarmsg){if(!exiting)tray();return 0;}if(m==0x8011){if(!exiting){openmain();}return 0;}if(m==0x10){if(cfg.closeTray&&!exiting){hideEditor();return 0;}remember();beginShutdown();saveGeometry();DestroyWindow(h);return 0;}if(m==0x2){closeSettingsView();beginShutdown();KillTimer(h,93);KillTimer(h,94);KillTimer(h,95);KillTimer(h,96);KillTimer(h,97);KillTimer(h,98);KillTimer(h,99);KillTimer(h,100);KillTimer(h,101);KillTimer(h,102);NativeUpdateCancel();if(pendingTest){release(pendingTest);pendingTest=nullptr;}UnregisterHotKey(h,1);Shell_NotifyIconW(2,&nid);destroyTrailOverlay();previewCache.clear();clearResults();PostQuitMessage(0);return 0;}return DefWindowProcW(h,m,w,l);}
// Native build smoke: show without activating, paint all pages and verify geometry and native control styles.
static bool uiSelfTest(){
 
 if(!ctrl(NAMEBOX)||!ctrl(LIST)||!ctrl(BUTTONBOX)||!ctrl(TRAIN)||!ctrl(ENABLEBOX))return false;
 if((GetWindowLongPtrW(ctrl(NAMEBOX),-20)&0x200)!=0||(GetWindowLongPtrW(ctrl(LIST),-20)&0x200)!=0)return false;
 if((GetWindowLongPtrW(ctrl(LIST),-16)&0x10)==0||(GetWindowLongPtrW(ctrl(BUTTONBOX),-16)&0x10)==0)return false;
 if((GetWindowLongPtrW(ctrl(SAVEBTN),-16)&15)!=11||!flatOld[BUTTONBOX]||!flatOld[ENABLEBOX])return false;
 ShowWindow(mainwnd,4); // SW_SHOWNOACTIVATE
 hideNativeFocus(mainwnd);SendMessageW(ctrl(ENABLEBOX),mgfocus::change,mgfocus::clear|mgfocus::hide|0x20000,0);
 if(!(SendMessageW(mainwnd,mgfocus::query,0,0)&1)||!(SendMessageW(ctrl(ENABLEBOX),mgfocus::query,0,0)&1))return false;

 // Creation cancellation: no write to configuration, restore the original selection.
 {setpage(0);int before=cfg.ng,prior=selected;newitem();if(!gestureDraft||!logicalPlace[CANCELCREATE].show||logicalPlace[CANCELCREATE].y+logicalPlace[CANCELCREATE].h>=logicalPlace[NEWBTN].y)return false;txt(NAMEBOX,L"Discard me");command(CANCELCREATE,0);if(gestureDraft||logicalPlace[CANCELCREATE].show||selected!=prior||cfg.ng!=before)return false;}
 {for(int pg=0;pg<3;pg++){setpage(pg);int buttons[]={pg==1?APPNEW:NEWBTN,pg==1?APPCAT:CATBTN};for(int id:buttons){bool previous=IsWindowEnabled(ctrl(id));{CreationModalScope scope(id);if(!logicalPlace[CANCELCREATE].show||logicalPlace[CANCELCREATE].x!=logicalPlace[id].x||logicalPlace[CANCELCREATE].y+logicalPlace[CANCELCREATE].h>=logicalPlace[id].y)return false;beginDialogOwner();if(!IsWindowEnabled(ctrl(CANCELCREATE))||IsWindowEnabled(ctrl(id))||!IsWindowEnabled(mainwnd))return false;endDialogOwner();}if(logicalPlace[CANCELCREATE].show||(IsWindowEnabled(ctrl(id))!=0)!=previous)return false;}}setpage(0);}
 int cat=ensureCategory(cfg,0,L"Smoke category");if(cat<0)return false;cp(cfg.g[0].folder,cfg.categories[cat].name,96);cfg.g[0].count=1;if(!compass(2,-1,cfg.g[0].samples[0]))return false;cfg.g[0].parallel=1;choosefirst();refresh();if(edit.count!=1||selection(EXECBOX)!=1||logicalPlace[FOLDERBOX].show||logicalPlace[SENSEBOX].show||!logicalPlace[NEWBTN].show)return false;
 // Verify click, Ctrl-toggle, Shift-range, marquee, and stable multi-delete.
 {P sample[N],third[N],fourth[N];if(!compass(0,1,sample)||!compass(1,2,third)||!compass(3,4,fourth))return false;
  appendEditorShape(sample);appendEditorShape(third);appendEditorShape(fourth);
  RECT r{};GetClientRect(canvas,&r);auto grid=shapeCardGrid(r);if(grid.count!=4||shapeSelection!=8)return false;
  editfields();if(shapeSelection||selectedShape!=-1||shapeAnchor!=-1)return false;
  auto center=[&](int i){auto q=grid.card(i);return (LP)((u16)((q.left+q.right)/2)|((u32)(u16)((q.top+q.bottom)/2)<<16));};
  auto click=[&](int i,WP mods){SendMessageW(canvas,0x201,mods|1,center(i));SendMessageW(canvas,0x202,mods,center(i));};
  click(0,0);if(shapeSelection!=1)return false;
  click(2,8);if(shapeSelection!=5)return false;
  click(2,8);if(shapeSelection!=1)return false;
  click(0,0);click(2,4);if(shapeSelection!=7)return false;
  SendMessageW(canvas,0x201,1,0);SendMessageW(canvas,0x200,1,(LP)((u16)r.right|((u32)(u16)r.bottom<<16)));SendMessageW(canvas,0x202,0,(LP)((u16)r.right|((u32)(u16)r.bottom<<16)));
  if(shapeSelection!=15)return false;
  click(0,0);click(2,8);command(RESETTRAIN,0);
  if(edit.count!=2||!sameShape(edit.samples[0],sample)||!sameShape(edit.samples[1],fourth))return false;
  shapeSelection=mgcards::all(edit.count);command(RESETTRAIN,0);if(edit.count||shapeSelection)return false;
  edit=cfg.g[selected];editfields();if(shapeSelection||selectedShape!=-1||shapeAnchor!=-1)return false;
 }
 cfg.categories[cat].collapsed=1;rebuildlist();if(SendMessageW(ctrl(LIST),0x18b,0,0)!=cfg.ng)return false;cfg.categories[cat].collapsed=0;rebuildlist();if(SendMessageW(ctrl(LIST),0x18b,0,0)!=cfg.ng+1)return false;
 for(int lang=0;lang<6;lang++){cfg.lang=lang;Placement fixedTabs[3]{};for(int pg=0;pg<3;pg++){
  setpage(pg);
  for(int i=0;i<3;i++){auto q=logicalPlace[TAB0+i];if(pg==0)fixedTabs[i]=q;else if(q.x!=fixedTabs[i].x||q.w!=fixedTabs[i].w)return false;}
  for(int pass=0;pass<2;pass++){
   SetWindowPos(mainwnd,nullptr,0,0,px(pass?860:compactClientWidth()+20),px(pass?740:690),2|4|16);
   relayout();UpdateWindow(mainwnd);
   for(int i=0;i<164;i++)if(ctrl(i)&&logicalPlace[i].show){RECT r{};GetClientRect(ctrl(i),&r);if(r.right<=0||r.bottom<=0)return false;UpdateWindow(ctrl(i));}
   // Runtime font measurement verifies the actual localized label fits its allocated rectangle.
   {HDC dc=GetDC(mainwnd);if(!dc)return false;H old=SelectObject(dc,font);auto set=labelSet(labelView());bool fits=true;for(int i=0;i<set.count;i++){int id=set.ids[i];if(!logicalPlace[id].show)continue;W t[256]{};get(id,t,256);POINT ext{};if(!GetTextExtentPoint32W(dc,t,len(t),&ext)||ext.x>logicalPlace[id].w||ext.y>logicalPlace[id].h)fits=false;int target=pg==3?LANGBOX:NAMEBOX;if(logicalPlace[target].show&&logicalPlace[target].x-(logicalPlace[id].x+logicalPlace[id].w)<px(labelGap)-1)fits=false;}SelectObject(dc,old);ReleaseDC(mainwnd,dc);if(!fits)return false;}
   if(logicalPlace[ADDITIONALHEADER].show!=(gstview()&&categorySelection<0))return false;
   for(int id=TAB0;id<=TAB2;id++){if(logicalPlace[id].w!=px(tabCaptionWidth(id))||logicalPlace[id].y!=logicalPlace[SETBTN].y||logicalPlace[id].h!=logicalPlace[SETBTN].h)return false;if(id>TAB0&&logicalPlace[id].x-logicalPlace[id-1].x-logicalPlace[id-1].w>px(3)+1)return false;}
   if(logicalPlace[SETBTN].x+logicalPlace[SETBTN].w>virtualW-px(12)+1||logicalPlace[SETBTN].x<logicalPlace[TAB2].x+logicalPlace[TAB2].w)return false;
   if(!logicalPlace[SETBTN].show||logicalPlace[APPBACK].show||logicalPlace[TOGGLE].show||logicalPlace[STARTWINBOX].show!=(pg==3)||logicalPlace[ADMINBOX].show!=(pg==3))return false;
   // All pages share one heading row and use identical pane-title metrics.
   if(!logicalPlace[GESTHEAD].show||logicalPlace[APPHEAD].show!=(pg==1)||logicalPlace[EDITORHEAD].show!=(pg!=2))return false;
   int paneHeadings[]={APPHEAD,GESTHEAD,EDITORHEAD};for(int id:paneHeadings)if(logicalPlace[id].show&&(logicalPlace[id].y!=px(contentTop)||logicalPlace[id].h!=px(paneTitleHeight)))return false;
   if(logicalPlace[GESTHEAD].x!=logicalPlace[SEARCHBOX].x||logicalPlace[GESTHEAD].w!=logicalPlace[SEARCHBOX].w||logicalPlace[SEARCHBOX].y!=px(contentTop+paneTitlePitch))return false;
   if(pg==1&&(logicalPlace[APPHEAD].x!=logicalPlace[APPSEARCH].x||logicalPlace[APPSEARCH].y!=logicalPlace[SEARCHBOX].y))return false;
   if(logicalPlace[GESTHEAD].y-(logicalPlace[TAB0].y+logicalPlace[TAB0].h)!=px(contentTop)-px(tabTop)-px(tabHeight))return false;
   if(paneForWindow(ctrl(SEARCHBOX))!=GESTHEAD||paneForWindow(ctrl(SETBTN))!=0)return false;
   if(pg==1&&paneForWindow(ctrl(APPSEARCH))!=APPHEAD)return false;
   if(logicalPlace[NAMEBOX].show&&paneForWindow(ctrl(NAMEBOX))!=(pg==2?GESTHEAD:EDITORHEAD))return false;
   for(int id:paneHeadings)if(logicalPlace[id].show){selectPaneHeading(id);for(int other:paneHeadings)if(logicalPlace[other].show&&(HFONT)SendMessageW(ctrl(other),0x31,0,0)!=(other==id?tabBold:font))return false;}
   if(logicalPlace[NAMEBOX].show&&logicalPlace[NAMEBOX].y!=logicalPlace[SEARCHBOX].y)return false;
   if(logicalPlace[EDITORHEAD].show&&logicalPlace[LABNAME].show&&logicalPlace[EDITORHEAD].x!=logicalPlace[LABNAME].x)return false;
   {HDC dc=GetDC(mainwnd);if(!dc)return false;H old=SelectObject(dc,tabBold);bool fits=true;int headings[]={APPHEAD,GESTHEAD,EDITORHEAD};for(int id:headings)if(logicalPlace[id].show){W b[256]{};get(id,b,256);POINT z{};if(!GetTextExtentPoint32W(dc,b,len(b),&z)||z.x>logicalPlace[id].w||z.y>logicalPlace[id].h)fits=false;}SelectObject(dc,old);ReleaseDC(mainwnd,dc);if(!fits)return false;}
   // Footer and sidebar regression checks, exercised by the Windows UI self-test.
   if(!logicalPlace[STATUSBOX].show||!logicalPlace[VERSIONBOX].show||logicalPlace[STATUSBOX].y!=logicalPlace[VERSIONBOX].y||logicalPlace[STATUSBOX].x+logicalPlace[STATUSBOX].w>logicalPlace[VERSIONBOX].x)return false;
   if(logicalPlace[NEWBTN].show!=(gstview()||pg==2))return false;
   if(logicalPlace[LIST].y-(logicalPlace[SEARCHBOX].y+logicalPlace[SEARCHBOX].h)<px(5)-1)return false;
   if(pg==1&&logicalPlace[APPLIST].y-(logicalPlace[APPSEARCH].y+logicalPlace[APPSEARCH].h)<px(5)-1)return false;
   if(ctrl(PICKBTN)||logicalPlace[PICKBTN].show)return false;
   if(pg==2&&!logicalPlace[CATBTN].show)return false;if(pg==2&&!validExclusionIndex(cfg,ruleIndex)&&categorySelection<0&&(logicalPlace[SAVEBTN].show||logicalPlace[NAMEBOX].show))return false;
   if(pg==2&&categorySelection<0&&validExclusionIndex(cfg,ruleIndex)&&(!logicalPlace[EXPATHBOX].show||!logicalPlace[CLASSBOX].show||!logicalPlace[TITLEBOX].show||!logicalPlace[MODEBOX].show||absn((logicalPlace[EXPATHBOX].y-logicalPlace[EXEBOX].y)-(logicalPlace[CLASSBOX].y-logicalPlace[EXPATHBOX].y))>1))return false;
   // Content starts below the compact tab gap; the footer gap remains unchanged.
   int topChecks[]={SEARCHBOX,NAMEBOX,LANGBOX,APPGEST};for(int id:topChecks)if(logicalPlace[id].show&&logicalPlace[id].y-(logicalPlace[TAB0].y+logicalPlace[TAB0].h)<px(tabContentGap)-1)return false;
   int bottomChecks[]={LIST,NEWBTN,SAVEBTN,TESTBTN,IMPORTBTN,EXPORTBTN};for(int id:bottomChecks)if(logicalPlace[id].show&&logicalPlace[STATUSBOX].y-(logicalPlace[id].y+logicalPlace[id].h)<px(contentGap)-1)return false;
   if(logicalPlace[NEWBTN].show&&(logicalPlace[NEWBTN].x!=logicalPlace[LIST].x||logicalPlace[NEWBTN].w!=logicalPlace[LIST].w||logicalPlace[LIST].y+logicalPlace[LIST].h>logicalPlace[NEWBTN].y||logicalPlace[STATUSBOX].y-(logicalPlace[NEWBTN].y+logicalPlace[NEWBTN].h)<px(contentGap)-1))return false;
   if(logicalPlace[SAVEBTN].show&&logicalPlace[STATUSBOX].y-(logicalPlace[SAVEBTN].y+logicalPlace[SAVEBTN].h)<px(contentGap)-1)return false;
   {RECT client{};GetClientRect(mainwnd,&client);if(logicalPlace[IMPORTBTN].show!=(pg==3)||logicalPlace[EXPORTBTN].show!=(pg==3))return false;if(pg==3&&(logicalPlace[EXPORTBTN].x+logicalPlace[EXPORTBTN].w>logicalPlace[IMPORTBTN].x||logicalPlace[SAVEBTN].x+logicalPlace[SAVEBTN].w>logicalPlace[EXPORTBTN].x))return false;if(pg!=3&&(logicalPlace[LIST].w<px(sidebarWidth)||logicalPlace[LIST].x+logicalPlace[LIST].w>logicalPlace[LABNAME].x&&logicalPlace[LABNAME].show))return false;if(pg==0&&logicalPlace[NAMEBOX].show){if(absn(logicalPlace[NAMEBOX].x+logicalPlace[NAMEBOX].w-(virtualW-px(12)))>2)return false;if(logicalPlace[ACTADD].x+logicalPlace[ACTADD].w>logicalPlace[ACTUP].x||logicalPlace[ACTUP].x+logicalPlace[ACTUP].w>logicalPlace[ACTDOWN].x||logicalPlace[ACTDOWN].x+logicalPlace[ACTDOWN].w>logicalPlace[ACTDEL].x||logicalPlace[ACTDEL].x+logicalPlace[ACTDEL].w>virtualW)return false;if(!logicalPlace[ADDITIONALHEADER].show||logicalPlace[ADDITIONALHEADER].y<logicalPlace[ACTHINT].y+logicalPlace[ACTHINT].h||logicalPlace[BUTTONBOX].y<logicalPlace[ADDITIONALHEADER].y+logicalPlace[ADDITIONALHEADER].h||logicalPlace[MODBOX].y<=logicalPlace[BUTTONBOX].y||logicalPlace[KEYBOX].y<=logicalPlace[MODBOX].y||logicalPlace[KEYBOX].y+logicalPlace[KEYBOX].h>logicalPlace[SAVEBTN].y)return false;}}
   if(pg==3&&(logicalPlace[LABLANG].x!=px(12)||logicalPlace[OVERBOX].x!=px(12)||logicalPlace[SAVEBTN].x!=px(12)))return false;
   if(pg==0){if(ctrl(ACTCOMBO)||logicalPlace[ACTCOMBO].show||!(logicalPlace[ACTADD].x<logicalPlace[ACTUP].x&&logicalPlace[ACTUP].x<logicalPlace[ACTDOWN].x&&logicalPlace[ACTDOWN].x<logicalPlace[ACTDEL].x))return false;if(SendMessageW(ctrl(BUTTONBOX),0x146,0,0)!=6)return false;if(SendMessageW(ctrl(LIST),0x1a1,0,0)!=px(21))return false;}
  }
 }
  RECT before{};GetWindowRect(mainwnd,&before);int oldPage=page;HWND oldButton=ctrl(BUTTONBOX);openSettingsView();RECT after{};GetWindowRect(mainwnd,&after);
  if(!settingsWnd||!IsWindowVisible(settingsWnd)||!IsWindowEnabled(mainwnd)||!IsWindowVisible(mainwnd)||GetAncestor(settingsControls[BUTTONBOX],2)!=settingsWnd||settingsControls[BUTTONBOX]==oldButton||page!=oldPage||before.left!=after.left||before.top!=after.top||before.right!=after.right||before.bottom!=after.bottom||!settingsLayoutFits())return false;
  if((GetWindowLongPtrW(settingsWnd,-16)&(0x40000|0x10000))!=0)return false;RECT updateRow{},themeRow{},checkRow{};GetWindowRect(sc(UPDATEBOX),&updateRow);GetWindowRect(sc(THEMEBOX),&themeRow);GetWindowRect(sc(UPDATEBTN),&checkRow);if(updateRow.top<=themeRow.top||checkRow.top!=updateRow.top||checkRow.left<=updateRow.right)return false;
  RECT fsRow{},taskRow{};if(!GetWindowRect(sc(FULLSCREENBOX),&fsRow)||!GetWindowRect(sc(TASKBARBOX),&taskRow)||fsRow.left!=taskRow.left||absn(taskRow.top-fsRow.top-spx(25))>1||schecked(FULLSCREENBOX)!=bool(cfg.disableFullscreenGestures))return false;
  // The main window can switch tabs without closing or changing the settings form.
  W draft[8]{};GetWindowTextW(settingsControls[TRAILCOLOR],draft,8);setpage(0);W retained[8]{};GetWindowTextW(settingsControls[TRAILCOLOR],retained,8);if(!settingsWnd||!eq(draft,retained))return false;closeSettingsView();if(settingsWnd)return false;
 }
 // A programmatic undersize is corrected by relayout, just as a legacy saved size is.
 for(int pg=0;pg<3;pg++){setpage(pg);SetWindowPos(mainwnd,nullptr,0,0,300,300,4|16);relayout();RECT r{};GetClientRect(mainwnd,&r);if(r.right<px(compactClientWidth())||r.bottom<px(minimumClientHeight)||ctrl(HEAD)||logicalPlace[HEAD].show||(GetWindowLongPtrW(mainwnd,-16)&0x00300000))return false;if(!IsWindowVisible(footerWnd)||place[STATUSBOX].y!=r.bottom-px(21)||place[VERSIONBOX].x+place[VERSIONBOX].w!=r.right-px(12))return false;for(int i=0;i<164;i++)if(place[i].show&&ctrl(i)&&i!=STATUSBOX&&i!=VERSIONBOX){auto q=place[i];if(q.x<0||q.y<0||q.x+q.w>r.right||q.y+(flatCombo(i)?px(22):q.h)>r.bottom-px(21))return false;}}
 ShowWindow(mainwnd,0);(void)0;return true;
}
// Explicit diagnostic mode: exits before installing hooks or touching configuration.
static int runApplication(){shutdownSignal=CreateEventW(nullptr,1,0,nullptr);startupReady=CreateEventW(nullptr,1,0,nullptr);if(shutdownSignal&&startupReady)watchdogThread=CreateThread(nullptr,0,shutdownWatchdog,nullptr,0,nullptr);if(!resolve_gui())return 3;if(contains(GetCommandLineW(),L"--loader-self-test"))return 0;bool windowTest=contains(GetCommandLineW(),L"--window-self-test");bool parallelTest=contains(GetCommandLineW(),L"--shutdown-parallel-self-test");bool busyTest=parallelTest||contains(GetCommandLineW(),L"--shutdown-busy-self-test");bool shutdownTest=busyTest||contains(GetCommandLineW(),L"--shutdown-self-test");noHooks=contains(GetCommandLineW(),L"--no-hooks");shutdownTestMode=shutdownTest;busyTestMode=busyTest;parallelTestMode=parallelTest;testStart=GetTickCount();inst=GetModuleHandleW(nullptr);H mutex=windowTest?nullptr:CreateMutexW(nullptr,0,L"Local\\miniGestures.v1");if(!windowTest&&!mutex){MessageBoxW(nullptr,L"Cannot establish single-instance protection.",L"miniGestures",0x10);return 2;}if(mutex&&GetLastError()==183){if(shutdownTest)return 6;HWND h=FindWindowW(L"miniGesturesMain",nullptr);if(h)PostMessageW(h,0x8011,0,0);return 0;}if(!windowTest){H legacyGuard=CreateMutexW(nullptr,0,L"Local\\StrokeliteClean.v1");if(!legacyGuard)return 2;if(GetLastError()==183){HWND oldWindow=FindWindowW(L"StrokeliteCleanMain",nullptr);if(oldWindow&&!shutdownTest)PostMessageW(oldWindow,0x8011,0,0);return shutdownTest?6:0;}}(void)0;SetProcessDpiAwarenessContext(PTR(-4));DWORD pathSize=GetModuleFileNameW(nullptr,configpath,1024);if(!pathSize||pathSize>=1024)return 3;int n=(int)pathSize;while(n&&configpath[n-1]!=L'\\')n--;if(!n||n+20>=1024)return 3;configpath[n]=0;W previousConfig[1024],originalConfig[1024];cp(previousConfig,configpath,1024);cat(previousConfig,L"Strokesmini.json",1024);cp(originalConfig,configpath,1024);cat(originalConfig,L"Strokelite.json",1024);cat(configpath,L"miniGestures.json",1024);bool missing=GetFileAttributesW(configpath)==0xffffffff;const W* loadPath=configpath;if(missing&&!windowTest&&!shutdownTest){if(GetFileAttributesW(previousConfig)!=0xffffffff)loadPath=previousConfig;else if(GetFileAttributesW(originalConfig)!=0xffffffff)loadPath=originalConfig;}bool migrating=loadPath!=configpath;bool invalid=false,legacyShapes=false,shapeBackupFailed=false;if(windowTest||shutdownTest){defaults(cfg);}else if(!loadconfig(cfg,loadPath,&legacyShapes)){defaults(cfg);if(missing&&!migrating){firstSave=true;geometryDirty=true;}else{cfg.enabled=0;invalid=true;}}else if(legacyShapes&&!migrating){if(!backupMultiShapeConfig(configpath)){cfg.enabled=0;invalid=shapeBackupFailed=true;}else geometryDirty=true;}else if(migrating){firstSave=true;geometryDirty=true;}if(!invalid&&!cfg.layoutRevision){if(cfg.winw>=300)cfg.winw=maxn(640,cfg.winw-px(100));cfg.layoutRevision=1;geometryDirty=true;}configValid=!invalid;lazyUI=cfg.startTray&&!contains(GetCommandLineW(),L"--restart-wait")&&!windowTest&&!shutdownTest&&!noHooks&&!invalid;(void)0;applyTheme();appicon=LoadIconW(inst,(const W*)PTR(1));if(!appicon)appicon=LoadIconW(nullptr,(const W*)PTR(32512));pausedIcon=LoadIconW(inst,(const W*)PTR(2));updateIcon=LoadIconW(inst,(const W*)PTR(3));pausedUpdateIcon=LoadIconW(inst,(const W*)PTR(4));WC wc{};wc.cbSize=sizeof(wc);wc.hInstance=inst;wc.hCursor=LoadCursorW(nullptr,(const W*)PTR(32512));wc.hIcon=appicon;wc.hIconSm=appicon;wc.hbrBackground=nullptr;wc.lpfnWndProc=mainproc;wc.lpszClassName=L"miniGesturesMain";RegisterClassExW(&wc);wc.hbrBackground=nullptr;wc.lpfnWndProc=footerProc;wc.lpszClassName=L"miniGesturesFooter";RegisterClassExW(&wc);wc.lpfnWndProc=canvasproc;wc.lpszClassName=L"miniGesturesCanvas";RegisterClassExW(&wc);wc.hbrBackground=nullptr;wc.lpfnWndProc=editorProc;wc.lpszClassName=L"miniGesturesEditor";RegisterClassExW(&wc);wc.lpfnWndProc=overlayproc;wc.lpszClassName=L"miniGesturesTrail";wc.hbrBackground=nullptr;RegisterClassExW(&wc);testTarget=identity(GetForegroundWindow());mainwnd=CreateWindowExW(0,L"miniGesturesMain",L"miniGestures",mainStyle,0x80000000,0x80000000,710,660,nullptr,nullptr,inst,nullptr);if(!mainwnd){(void)0;return 2;}if(cfg.alwaysOnTop)SetWindowPos(mainwnd,PTR(-1),0,0,0,0,1|2|16);allowShowMessage(mainwnd);if(windowTest){if(startupReady)SetEvent(startupReady);(void)0;bool ok=uiSelfTest();if(!ok)(void)0;DestroyWindow(mainwnd);return ok?0:4;}MONITOR mon{sizeof(mon)};H monitor=MonitorFromWindow(mainwnd,2);bool savedBox=cfg.winw>=300&&cfg.winh>=300&&cfg.winw<=100000&&cfg.winh<=100000&&cfg.winx>=-1000000&&cfg.winx<=1000000&&cfg.winy>=-1000000&&cfg.winy<=1000000;if(savedBox){auto fromRect=reinterpret_cast<H(*)(const RECT*,DWORD)>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"MonitorFromRect"));RECT saved{cfg.winx,cfg.winy,cfg.winx+cfg.winw,cfg.winy+cfg.winh};if(fromRect){H m=fromRect(&saved,2);if(m)monitor=m;}}if(GetMonitorInfoW(monitor,&mon)){auto minimum=minimumOuter(mainwnd);auto r=restoreBox({cfg.winx,cfg.winy,savedBox?cfg.winw:0,savedBox?cfg.winh:0},{mon.rcWork.left,mon.rcWork.top,mon.rcWork.right-mon.rcWork.left,mon.rcWork.bottom-mon.rcWork.top},minimum.x,minimum.y,px(710),px(660));SetWindowPos(mainwnd,nullptr,r.x,r.y,r.w,r.h,4|16);}geometryReady=true;if(!lazyUI){showNativeMain(cfg.maximized&&!shutdownTest);remember();(void)0;}if(!noHooks)RegisterHotKey(mainwnd,1,7|0x4000,0x7B);taskbarmsg=RegisterWindowMessageW(L"TaskbarCreated");tray();if(startupReady)SetEvent(startupReady);(void)0;status(invalid?tr(INVALID):L"");if(!noHooks){SetTimer(mainwnd,93,50,nullptr);if(!windowTest&&!shutdownTest)SetTimer(mainwnd,102,1000,nullptr);}else (void)0;if(shutdownTest)SetTimer(mainwnd,noHooks?92:96,noHooks?300:50,nullptr);else if(firstSave||geometryDirty)SetTimer(mainwnd,95,750,nullptr);if(invalid)MessageBoxW(mainwnd,shapeBackupFailed?et(SHAPE_BACKUP_ERROR_E):tr(INVALID),L"miniGestures",0x10);if(!shutdownTest&&!lazyUI)scheduleConflictCheck();MSG m{};while(GetMessageW(&m,nullptr,0,0)>0){PaneInteraction paneInput(m);if(settingsWnd&&m.hwnd&&GetAncestor(m.hwnd,2)==settingsWnd){if(m.message==0x100&&m.wParam==0x1b&&!settingsBusy){closeSettingsView();continue;}if(!IsDialogMessageW(settingsWnd,&m)){TranslateMessage(&m);DispatchMessageW(&m);}continue;}if(record&&(m.message==0x100||m.message==0x104)&&!screenshotShortcut((int)m.wParam,mods())){if(m.wParam==0x1b)recordend(0,0,true);else if(m.wParam!=0x10&&m.wParam!=0x11&&m.wParam!=0x12&&m.wParam!=0x5b&&m.wParam!=0x5c)recordend((int)m.wParam,mods(),false);continue;}if(m.message==0x100&&m.wParam==0x1b&&dragSource!=-1){cancelListDrag(dragOwner?dragOwner:ctrl(LIST));continue;}if(m.message==0x100&&m.wParam==0x71&&!settings&&GetForegroundWindow()==mainwnd&&!inlineRename&&m.hwnd!=ctrl(ACTLIST)){if(page==1&&(m.hwnd==ctrl(APPLIST)||m.hwnd==ctrl(APPSEARCH)))renameApp();else renameSelected();continue;}if(inlineRename&&m.hwnd==inlineRename&&m.message==0x100&&(m.wParam==13||m.wParam==0x1b)){finishInlineRename(m.wParam==13,true);continue;}if(m.message==0x100&&m.wParam==13&&GetForegroundWindow()==mainwnd&&!popup&&!record&&!inlineRename){HWND f=GetFocus();int fid=f?GetDlgCtrlID(f):-1;if(fid!=SEARCHBOX&&fid!=LIST&&fid!=ACTLIST&&fid!=TRAIN&&fid!=NEWBTN&&fid!=CATBTN&&fid!=DELBTN&&fid!=APPDEL&&fid!=TESTBTN&&fid!=SAVEBTN&&fid!=IMPORTBTN&&fid!=EXPORTBTN&&fid!=RECBTN&&fid!=KEYCLEAR&&fid!=ACTADD&&fid!=ACTUP&&fid!=ACTDOWN&&fid!=ACTDEL&&fid!=ACTREC&&fid!=BROWSEBTN&&fid!=PRESETBTN&&fid!=APPGEST&&fid!=APPBACK&&fid!=TAB0&&fid!=TAB1&&fid!=TAB2&&fid!=SETBTN){saveedit();continue;}}if(!IsDialogMessageW(mainwnd,&m)){TranslateMessage(&m);DispatchMessageW(&m);}}beginShutdown();bool stopped=joinWorkers();if(stopped&&!windowTest&&!shutdownTest)saveGeometry();if(stopped){H threads[]={hookthread,workerthread,actionthread,metathread};for(H t:threads)if(t)CloseHandle(t);H events[]={hookready,workready,actionready,metadataWake};for(H t:events)if(t)CloseHandle(t);}(void)0;return !stopped?5:testFailed?7:0;}

// Native Windows host owns preflight and process lifetime.
extern "C" DWORD __stdcall NativeStart(void*) {return (DWORD)runApplication();}
// Read the same validated portable configuration as the UI, without a Go parser.
extern "C" int __stdcall NativeRequestedElevation(const W* path) {
 Config local;Config* c=&local;
 bool requested=loadconfig(*c,path)&&c->adminWindows==1;
 return requested?1:0;
}
