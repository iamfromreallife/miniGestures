#pragma once
#include "base.hpp"
#include <stddef.h>
#include "recognition.hpp"
constexpr int MAXS=16,MAXA=8;
constexpr int TASK_VIEW_ACTION=34, RETIRED_DELAY_ACTION=46, SHOW_MINIGESTURES_ACTION=62, FIRST_DESKTOP_ACTION=63, LAST_DESKTOP_ACTION=64, RETIRED_ACTION_ID=65, KILL_APP_ACTION=66, BROWSER_HOME_ACTION=67, ACTION_COUNT=68;
static inline bool retiredAction(int type){return type==RETIRED_ACTION_ID||type==RETIRED_DELAY_ACTION;}
static inline bool elevationAction(int type){return type==23||type==KILL_APP_ACTION;}
struct Act{int type;W arg[256];};
struct Gesture{int id,profile,enabled,button,mods,trigger,hotkey,hotmods,sensitivity,count,steps,parallel;W name[96],folder[96];P samples[MAXS][N];Act actions[MAXA];};
// Replace one action without touching neighbors; append uses index -1.
static inline int commitAction(Gesture& g,int index,const Act& next){if(next.type<0||next.type>=ACTION_COUNT||retiredAction(next.type)||index< -1||index>=g.steps||(index==-1&&g.steps>=MAXA))return -1;if(index==-1)index=g.steps++;g.actions[index]=next;return index;}
static inline bool hasTrigger(const Gesture&g){return g.hotkey>0||(g.trigger==0?g.count>0:g.trigger>=1&&g.trigger<=7);}
struct Rule{int id,enabled,mode,inherit,uiOrder;W name[96],exe[260],cls[128],title[128],path[260],folder[96];};
struct Category{int profile,collapsed,uiOrder;W name[96];};
struct ConfigScalars{int version,lang,enabled,button,threshold,overlay,closeTray,startTray,nextid,ng,na,ne,winx,winy,winw,winh,maximized,sensitivity,nc,skipDeleteConfirm,layoutRevision,startWithWindows,adminWindows,theme,alwaysOnTop,trailColor,trailWidth,taskbarGestures,disableFullscreenGestures,settingsX,settingsY,settingsPositionSaved,showGestureNames,indicatorX,indicatorY,indicatorPositionSaved,updateMode,updateLastCheck,indicatorSeconds,ignoredUpdateMajor,ignoredUpdateMinor,ignoredUpdatePatch;};
// Heap-backed trivial records. Capacity grows geometrically; callers reserve
// before changing counts. No allocations are made by the input hook.
template<class T> struct Records{
 T* p=nullptr;int capacity=0;
 Records()=default;Records(const Records&)=delete;Records& operator=(const Records&)=delete;
 ~Records(){freep(p);}
 void clear(){freep(p);p=nullptr;capacity=0;}
 bool reserve(int n){if(n<0)return false;if(n<=capacity)return true;int k=capacity?capacity:1;while(k<n){if(k>0x7fffffff/2){k=n;break;}k*=2;}if((up)k>((up)-1)/sizeof(T))return false;T*q=(T*)alloc(sizeof(T)*(up)k);if(!q)return false;if(capacity)memcpy(q,p,sizeof(T)*(up)capacity);freep(p);p=q;capacity=k;return true;}
 T& operator[](int i){return p[i];}const T& operator[](int i)const{return p[i];}
 operator T*(){return p;}operator const T*()const{return p;}
 void swap(Records& o){T*t=p;p=o.p;o.p=t;int n=capacity;capacity=o.capacity;o.capacity=n;}
};
static inline bool addCount(int a,int b,int& n){if(a<0||b<0||a>0x7ffffffe - b)return false;n=a+b;return true;}
template<class T> static inline bool reserveMore(Records<T>& r,int count,int add=1){int n;return addCount(count,add,n)&&r.reserve(n);}
struct Config:ConfigScalars{
 Records<Category> categories;Records<Gesture> g;Records<Rule> a,e;
 Config():ConfigScalars{}{} Config(const Config&)=delete;Config&operator=(const Config&)=delete;
 void clear(){categories.clear();g.clear();a.clear();e.clear();static_cast<ConfigScalars&>(*this)={};}
 void swap(Config& o){ConfigScalars v=*this;static_cast<ConfigScalars&>(*this)=o;static_cast<ConfigScalars&>(o)=v;categories.swap(o.categories);g.swap(o.g);a.swap(o.a);e.swap(o.e);}
 bool validCounts()const{return ng>=0&&ng<=g.capacity&&na>=0&&na<=a.capacity&&ne>=0&&ne<=e.capacity&&nc>=0&&nc<=categories.capacity;}
 bool copyFrom(const Config& o){if(!o.validCounts())return false;Config t;if(!t.g.reserve(o.ng)||!t.a.reserve(o.na)||!t.e.reserve(o.ne)||!t.categories.reserve(o.nc))return false;static_cast<ConfigScalars&>(t)=o;if(o.ng)memcpy(t.g.p,o.g.p,sizeof(Gesture)*(up)o.ng);if(o.na)memcpy(t.a.p,o.a.p,sizeof(Rule)*(up)o.na);if(o.ne)memcpy(t.e.p,o.e.p,sizeof(Rule)*(up)o.ne);if(o.nc)memcpy(t.categories.p,o.categories.p,sizeof(Category)*(up)o.nc);swap(t);return true;}
};
struct RuntimeConfig:ConfigScalars{Gesture* g;Rule* a;Rule* e;};
// Rocker right always starts with the left button, independently of the default.
static inline int activationButton(const ConfigScalars&c,const Gesture&g){return g.trigger==6?1:(g.button?g.button:c.button);}
struct Snapshot{volatile long refs;unsigned char buttons[6][16],keys[256][16];bool rules,pathRules,exclusionRules,mouseWanted,keyWanted;RuntimeConfig c;};
static_assert(alignof(Snapshot)>=alignof(Gesture)&&alignof(Gesture)>=alignof(Rule),"Runtime snapshot alignment");
static inline void retain(Snapshot*s){InterlockedIncrement(&s->refs);} static inline void release(Snapshot*s){if(s&&InterlockedExchangeAdd(&s->refs,-1)==1)freep(s);}
// Immutable runtime snapshots contain only existing gestures and rules, not the
// editor's unused fixed capacity or category records. Keep one slot for Test.
static inline up snapshotBytes(const Config& c){return sizeof(Snapshot)+sizeof(Gesture)*maxn(1,c.ng)+sizeof(Rule)*((up)c.na+c.ne);}
static inline Snapshot* buildSnapshot(const Config&c){if(!c.validCounts())return nullptr;auto*s=(Snapshot*)allocRaw(snapshotBytes(c));if(s){memset(s,0,sizeof(Snapshot));s->refs=1;static_cast<ConfigScalars&>(s->c)=c;s->c.g=(Gesture*)(s+1);s->c.a=(Rule*)(s->c.g+maxn(1,c.ng));s->c.e=s->c.a+c.na;if(c.ng)memcpy(s->c.g,c.g.p,sizeof(Gesture)*c.ng);if(c.na)memcpy(s->c.a,c.a.p,sizeof(Rule)*c.na);if(c.ne)memcpy(s->c.e,c.e.p,sizeof(Rule)*c.ne);for(int i=0;i<c.na;i++)if(c.a[i].enabled&&(*c.a[i].exe||*c.a[i].cls||*c.a[i].title||*c.a[i].path)){s->rules=true;s->pathRules|=*c.a[i].exe!=0||*c.a[i].path!=0;}for(int i=0;i<c.ne;i++)if(c.e[i].enabled&&(*c.e[i].exe||*c.e[i].cls||*c.e[i].title||*c.e[i].path)){s->rules=true;s->exclusionRules=true;s->pathRules|=*c.e[i].exe!=0||*c.e[i].path!=0;}for(int i=0;i<c.ng;i++){auto&g=c.g[i];if(!g.enabled)continue;if(g.hotkey>0&&g.hotkey<256&&g.hotmods>=0&&g.hotmods<16)s->keys[g.hotkey][g.hotmods]=1,s->keyWanted=true;int b=activationButton(c,g);if(b>=1&&b<=5&&g.mods>=0&&g.mods<16&&g.trigger>=0&&g.trigger<8&&(g.trigger||g.count))s->buttons[b][g.mods]|=1<<g.trigger,s->mouseWanted=true;}}return s;}
struct Target{HWND hwnd;DWORD pid;W exe[520],cls[128],title[256];HWND child;int x,y,hasPoint,inputSafety;};
static inline Target target(HWND h){Target t{};t.child=h;t.hwnd=GetAncestor(h,2);if(!t.hwnd)t.hwnd=h;if(!t.hwnd)return t;GetWindowThreadProcessId(t.hwnd,&t.pid);H p=OpenProcess(0x1000,0,t.pid);if(p){DWORD n=520;QueryFullProcessImageNameW(p,0,t.exe,&n);CloseHandle(p);}GetClassNameW(t.hwnd,t.cls,128);GetWindowTextW(t.hwnd,t.title,256);return t;}
static inline bool matchfield(const W*s,const W*p){return !*p||mgregex::test(s,p);}
static inline bool matches(const Rule&r,const Target&t){return r.enabled&&(*r.exe||*r.cls||*r.title||*r.path)&&(!*r.exe||mgregex::test(t.exe,r.exe)||mgregex::test(basename(t.exe),r.exe))&&matchfield(t.exe,r.path)&&matchfield(t.cls,r.cls)&&matchfield(t.title,r.title);}
// Editor-only conservative overlap check. Exact criteria can prove disjointness;
// two nonliteral criteria may overlap. Runtime matching is always exact.
static inline bool literalRulePattern(const W*p){return *p&&!mgregex::is(p)&&!contains(p,L"*")&&!contains(p,L"?");}
static inline bool pathLikeRule(const W*p){return contains(p,L"\\")||contains(p,L"/");}
static inline bool possibleFieldOverlap(const W*a,const W*b){if(!*a||!*b)return true;if(literalRulePattern(a))return mgregex::test(a,b);if(literalRulePattern(b))return mgregex::test(b,a);return true;}
static inline bool executableMatchesRule(const Rule&r,const W*path){return (!*r.exe||mgregex::test(path,r.exe)||mgregex::test(basename(path),r.exe))&&matchfield(path,r.path);}
static inline const W* exactRulePath(const Rule&r){if(literalRulePattern(r.path))return r.path;if(literalRulePattern(r.exe)&&pathLikeRule(r.exe))return r.exe;return nullptr;}
static inline bool possibleRuleOverlap(const Rule&a,const Rule&b){
 if(!possibleFieldOverlap(a.cls,b.cls)||!possibleFieldOverlap(a.title,b.title))return false;
 const W*pa=exactRulePath(a),*pb=exactRulePath(b);
 if(pa)return executableMatchesRule(a,pa)&&executableMatchesRule(b,pa);
 if(pb)return executableMatchesRule(a,pb)&&executableMatchesRule(b,pb);
 if(!possibleFieldOverlap(a.path,b.path))return false;
 // Basename-only glob rules can be compared with an exact executable name.
 // Path-aware and regex patterns may match the full path instead: warn conservatively.
 if(*a.exe&&*b.exe&&!mgregex::is(a.exe)&&!mgregex::is(b.exe)&&!pathLikeRule(a.exe)&&!pathLikeRule(b.exe))return possibleFieldOverlap(a.exe,b.exe);
 return true;
}
static inline bool exclusionNeedsWarning(const Rule&next,const Rule*previous){return next.enabled&&(!previous||!previous->enabled||previous->mode!=next.mode||!eq(previous->exe,next.exe)||!eq(previous->path,next.path)||!eq(previous->cls,next.cls)||!eq(previous->title,next.title));}
static inline bool validExclusionIndex(const Config&c,int i){return i>=0&&i<c.ne;}
static inline bool removeExclusionAt(Config&c,int i){if(!validExclusionIndex(c,i))return false;for(int j=i;j+1<c.ne;j++)c.e[j]=c.e[j+1];--c.ne;return true;}
template<class C> static inline int profile(const C&c,const Target&t){for(int i=0;i<c.na;i++)if(matches(c.a[i],t))return c.a[i].id;return 0;}
template<class C> static inline bool inherits(const C&c,int id){for(int i=0;i<c.na;i++)if(c.a[i].id==id)return c.a[i].inherit;return true;}
template<class C> static inline bool excluded(const C&c,const Target&t,int type){for(int i=0;i<c.ne;i++)if((c.e[i].mode==0||c.e[i].mode==type)&&matches(c.e[i],t))return true;return false;}
static inline void initgesture(Gesture&g,int id,int p){memset(&g,0,sizeof(g));g.id=id;g.profile=p;g.enabled=1;g.button=0;g.sensitivity=70;g.steps=1;cp(g.name,L"New gesture",96);}
static inline void preset(Gesture&g,const P*p,int n){g.count=normalize(p,n,g.samples[0])?1:0;}
static inline void defaults(Config&c){c.clear();c.indicatorSeconds=4;c.version=1;c.layoutRevision=1;c.sensitivity=70;c.enabled=1;c.button=2;c.threshold=20;c.overlay=1;c.trailColor=-1;c.trailWidth=3;c.taskbarGestures=1;c.closeTray=1;c.nextid=1;const W* names[]={L"Maximize window",L"Minimize window",L"Browser back",L"Browser forward",L"Close window",L"Show desktop"};P p[6][3]={{{0,100},{0,0},{0,0}},{{0,0},{0,100},{0,100}},{{100,0},{0,0},{0,0}},{{0,0},{100,0},{100,0}},{{0,0},{100,100},{100,100}},{{0,0},{0,100},{0,0}}};int a[]={1,2,8,9,3,13};if(!c.g.reserve(6))return;for(int i=0;i<6;i++){auto&g=c.g[c.ng++];initgesture(g,c.nextid++,0);cp(g.name,names[i],96);preset(g,p[i],i==5?3:2);g.actions[0].type=a[i];}}

static inline int categoryIndex(const Config&c,int profile,const W*name){for(int i=0;i<c.nc;i++)if(c.categories[i].profile==profile&&eq(c.categories[i].name,name))return i;return -1;}
static inline int ensureCategory(Config&c,int profile,const W*name){if(!*name)return -1;int i=categoryIndex(c,profile,name);if(i>=0)return i;if(!reserveMore(c.categories,c.nc))return -1;i=c.nc++;c.categories[i]={};c.categories[i].profile=profile;cp(c.categories[i].name,name,96);return i;}

static inline bool sameShape(const P*a,const P*b){for(int i=0;i<N;i++)if(a[i].x!=b[i].x||a[i].y!=b[i].y)return false;return true;}
// Only competing bindings at the same resolution priority conflict. Global/application overrides remain legal.
struct Conflict{int other=-1,ownShape=-1,otherShape=-1,kind=0;};
// Drawn shapes are ranked at runtime, never treated as duplicate bindings.
static inline Conflict pairConflict(const Config&c,const Gesture&g,const Gesture&o,int i){if(!g.enabled||!g.steps||!o.enabled||!o.steps||o.profile!=g.profile)return {};if(g.hotkey&&g.hotkey==o.hotkey&&g.hotmods==o.hotmods)return {i,-1,-1,1};if(g.trigger!=o.trigger||g.trigger==8||g.mods!=o.mods||activationButton(c,g)!=activationButton(c,o))return {};if(g.trigger)return {i,-1,-1,2};return {};}
static inline Conflict bindingConflict(const Config&c,const Gesture&g,int ignore){for(int i=0;i<c.ng;i++){if(i==ignore)continue;auto f=pairConflict(c,g,c.g[i],i);if(f.other>=0)return f;}return {};}
static inline void removeBinding(Gesture&g,Conflict f){if(f.kind==1){g.hotkey=g.hotmods=0;if(g.trigger==8||(!g.trigger&&!g.count))g.enabled=0;}else if(f.kind==2){g.trigger=8;if(!g.hotkey)g.enabled=0;}else if(f.kind==3&&f.otherShape>=0&&f.otherShape<g.count){for(int i=f.otherShape;i<g.count-1;i++)memcpy(g.samples[i],g.samples[i+1],sizeof(g.samples[i]));g.count--;if(!g.count&&!g.hotkey)g.enabled=0;}}

// Copy all action and binding data, but never introduce an active conflict.
static inline int pasteGesture(Config& c,const Gesture& source,int profile,int category,bool& disabled){
 disabled=false;if(category<-1||category>=c.nc||(category>=0&&c.categories[category].profile!=profile))return -1;
 Gesture g=source;g.id=c.nextid;g.profile=profile;cp(g.folder,category>=0?c.categories[category].name:L"",96);
 if(g.enabled&&bindingConflict(c,g,-1).other>=0){g.enabled=0;disabled=true;}
 if(!reserveMore(c.g,c.ng)||c.nextid>=0x7fffffff)return -1;int i=c.ng++;c.nextid++;c.g[i]=g;return i;
}
