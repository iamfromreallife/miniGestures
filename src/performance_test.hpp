#pragma once
// Headless regression checks for the behavior-preserving performance work.
// Run with --performance-self-test; no hooks, tray icon, configuration writes
// or external-window actions are started.
namespace mgperf {
static inline bool referenceNormalize(const P*p,int n,P*out){
 if(n<2||n>maxDrawingPoints)return false;
 double total=0;for(int i=1;i<n;i++)total+=arcDistance(p[i-1],p[i]);if(total<8*minSegmentPx)return false;
 struct Precise{double x,y;};Precise points[N];
 int j=1;double run=0,d=arcDistance(p[0],p[1]);
 for(int k=0;k<N;k++){
  // Euclidean arc length, kept in double precision until storage.
  double target=total*k;
  while(j<n-1&&(run+d)*(N-1)<target){run+=d;++j;d=arcDistance(p[j-1],p[j]);}
  double t=d?(double)(target-run*(N-1))/((double)d*(N-1)):1.0;
  if(t<0)t=0;if(t>1)t=1;
  points[k]={p[j-1].x+((double)p[j].x-p[j-1].x)*t,p[j-1].y+((double)p[j].y-p[j-1].y)*t};
 }
 double minx=points[0].x,maxx=minx,miny=points[0].y,maxy=miny;
 for(int i=1;i<N;i++){auto q=points[i];if(q.x<minx)minx=q.x;if(q.x>maxx)maxx=q.x;if(q.y<miny)miny=q.y;if(q.y>maxy)maxy=q.y;}
 double w=maxx-minx,h=maxy-miny,scale=w>h?w:h;if(scale<=0)return false;
 for(int i=0;i<N;i++)out[i]={nearestInt(((points[i].x-minx)-w/2)*1024/scale),nearestInt(((points[i].y-miny)-h/2)*1024/scale)};
 return true;
}

static u32 random(u32& seed){seed=seed*1664525u+1013904223u;return seed;}
static bool normalization(){
 u32 seed=1;P input[maxDrawingPoints],a[N],b[N];
 for(int i=0;i<256;i++){int n=2+(int)(random(seed)%(maxDrawingPoints-1));input[0]={0,0};
  for(int j=1;j<n;j++){int x=(int)(random(seed)%41)-20,y=(int)(random(seed)%41)-20;input[j]={input[j-1].x+x,input[j-1].y+y};}
  bool x=referenceNormalize(input,n,a),y=normalize(input,n,b);if(x!=y||(x&&memcmp(a,b,sizeof(a))))return false;
 }return true;
}
static bool regex(){
 const W* patterns[]={L"re:^chrome[.]exe$",L"re:(foo|bar)+$",L"re:[^0-9]+",L"re:^\\w+\\s\\d+$",L"re:ab?c*",L"re:^(a|b)*$",L"re:[",L"re:("};
 const W* subjects[]={L"",L"chrome.exe",L"CHROME.EXE",L"foobar",L"123",L"Alpha 42",L"abcc",L"bbb",L"no match"};
 for(auto* pattern:patterns){auto* program=mgregex::prepare(pattern);for(auto* subject:subjects){bool want=mgregex::test(subject,pattern),got=program&&program->match(subject);if(want!=got){freep(program);return false;}}freep(program);}
 return true;
}
static bool runtime(){
 Config c;c.enabled=1;c.button=2;c.sensitivity=75;c.ng=32;c.na=2;c.ne=3;c.nb=9;
 if(!c.g.reserve(c.ng)||!c.a.reserve(c.na)||!c.e.reserve(c.ne)||!c.blocks.reserve(c.nb))return false;
 for(int i=0;i<c.ng;i++){auto&g=c.g[i];initgesture(g,i+1,i%3==0?1:i%5==0?2:0);g.button=1+i%5;g.mods=i%4;g.trigger=i%8;g.steps=1+i%MAXA;
  g.hotkey=i%4==0?'A'+i%26:0;g.hotmods=i%4;g.count=g.trigger==0?1+i%MAXS:0;
  for(int k=0;k<g.count;k++){P points[3]={{0,0},{80+k,10-i},{80+k+(i%2?40:-40),80+i}};if(!normalize(points,3,g.samples[k]))return false;}
  for(int k=0;k<g.steps;k++){g.actions[k].type=0;cp(g.actions[k].arg,L"Ctrl+VK65",256);}
 }
 c.a[0]={};c.a[0].id=1;c.a[0].enabled=c.a[0].inherit=1;cp(c.a[0].exe,L"re:^chrome[.]exe$",260);
 c.a[1]={};c.a[1].id=2;c.a[1].enabled=1;cp(c.a[1].exe,L"other.exe",260);
 for(int i=0;i<c.ne;i++){c.e[i]={};c.e[i].enabled=1;c.e[i].mode=i+1;cp(c.e[i].title,i==0?L"re:^blocked":i==1?L"hotkey blocked":L"rocker blocked",128);}
 for(int i=0;i<c.nb;i++)c.blocks[i]={1,1+i};
 auto* snap=buildSnapshot(c);if(!snap)return false;
 auto check=[&](){
  if(!sameRuntimeConfig(c,*snap)||!snap->index||!snap->c.prepared||!snap->c.sortedBlocks)return false;
  if(snap->c.g[0].actions[0].key!=65||snap->c.g[0].actions[0].mods!=2)return false;
  RuntimeAction prepared{};Act program{};program.type=22;cp(program.arg,L"test.exe|--argument",256);prepareAction(prepared,program);if(prepared.separator!=8)return false;
  cp(c.g[0].folder,L"editor folder",96);c.a[0].uiOrder=7;cp(c.a[0].folder,L"editor folder",96);if(!sameRuntimeConfig(c,*snap))return false;
  c.winx=200;c.winy=100;c.actionWindowWidth=720;c.actionWindowHeight=650;c.actionWindowX=-900;c.actionWindowY=120;c.actionWindowPositionSaved=1;if(!sameRuntimeConfig(c,*snap))return false;
  ++c.g[0].actions[0].type;bool changed=!sameRuntimeConfig(c,*snap);--c.g[0].actions[0].type;if(!changed)return false;
  for(int pr=0;pr<3;pr++)for(int id=0;id<40;id++)if(blockedGlobal(c,pr,id)!=blockedGlobal(snap->c,pr,id))return false;
  Target t{};P points[N];memcpy(points,c.g[0].samples[0],sizeof(points));
  Job j{};j.s=snap;j.p=points;j.n=N;
  auto equivalent=[&](){int a=0,b=0,ca=0,cb=0,oa=0,ob=0,xa=-1,xb=-1;
   int first=choose(j,a,&ca,&oa,&xa);auto* index=snap->index;snap->index=nullptr;
   int second=choose(j,b,&cb,&ob,&xb);snap->index=index;
   return first==second&&a==b&&ca==cb&&oa==ob&&xa==xb;
  };
  const W* exe[]={L"C:\\Program Files\\chrome.exe",L"other.exe",L"unmatched.exe"};
  const W* title[]={L"normal",L"blocked document",L"hotkey blocked",L"rocker blocked"};
  for(auto* path:exe)for(auto* name:title){cp(t.exe,path,520);cp(t.title,name,256);j.t=t;
   if(profile(c,t)!=profile(snap->c,t))return false;
   for(int type=1;type<=4;type++)if(excluded(c,t,type)!=excluded(snap->c,t,type))return false;
   for(int button=1;button<=5;button++)for(int mod=0;mod<4;mod++)for(int trigger=0;trigger<8;trigger++){j.button=button;j.mods=mod;j.trigger=trigger;j.hotkey=0;if(!equivalent())return false;}
   for(int key='A';key<='Z';key++){j.hotkey=key;j.mods=0;j.trigger=0;j.button=0;if(!equivalent())return false;}
  }
  auto* next=buildSnapshot(c,snap);if(!next)return false;bool ok=sameRuntimeConfig(c,*next);
  for(int i=0;i<c.ng&&ok;i++){const auto& g=snap->c.g[i];if(!g.enabled||g.trigger||!g.steps)continue;for(int k=0;k<g.count;k++){const auto&a=snap->angular[snap->angularOffsets[i]+k];const auto&b=next->angular[next->angularOffsets[i]+k];if(a.valid!=b.valid||memcmp(a.angle,b.angle,sizeof(a.angle)))ok=false;}}
  release(next);return ok;
 };
 bool ok=check();release(snap);return ok;
}
static bool raster(){
 constexpr int w=37,h=29,scale=8,sw=w*scale,sh=h*scale,band=5;
 u32 full[sw*sh],tile[sw*band*scale],lo1[w*h],lo2[w*h],seed=9;
 for(int trial=0;trial<32;trial++){DWORD fg=random(seed)&0xffffff,bg=random(seed)&0xffffff;POINT points[12];
  for(auto& p:points)p={(int)(random(seed)%sw),(int)(random(seed)%sh)};
  float radius=4.0f+(float)(random(seed)%12);std::fill_n(full,sw*sh,bg);
  aaPolylineToSurface(full,sw,sh,points,12,radius,fg);
  aaFillTri(full,sw,sh,(float)points[9].x,(float)points[9].y,(float)points[10].x,(float)points[10].y,(float)points[11].x,(float)points[11].y,fg);
  aaDownsample(lo1,w,h,full,scale);
  for(int y=0;y<h;y+=band){int rows=minn(band,h-y),height=rows*scale;std::fill_n(tile,sw*height,bg);
   aaPolylineToSurface(tile,sw,height,points,12,radius,fg,y*scale);
   aaFillTri(tile,sw,height,(float)points[9].x,(float)points[9].y,(float)points[10].x,(float)points[10].y,(float)points[11].x,(float)points[11].y,fg,y*scale);
   aaDownsample(lo2+y*w,w,rows,tile,scale);
  }if(memcmp(lo1,lo2,sizeof(lo1)))return false;
 }return true;
}
static bool trailGrowth(){
 constexpr int size=128;u32 before[64*64]{},expanded[size*size]{},reference[size*size]{};
 P points[]={{-20,-20},{10,12},{-3,22},{50,47},{-55,-50}};
 TrailBounds a{-32,-32,64,64},b{-64,-64,size,size};
 for(int width=1;width<=24;width++){memset(before,0,sizeof(before));memset(expanded,0,sizeof(expanded));memset(reference,0,sizeof(reference));
  TrailStamp stamp;rasterTrail(before,a,points,3,width,0x5388cc,&stamp);
  // Only test old bounds that actually contained the complete old stroke.
  if(!trailContains(a,trailBounds(points,3,width)))continue;
  if(!copyTrailPixels(expanded,b,before,a))return false;
  rasterTrail(expanded,b,points+2,3,width,0x5388cc,&stamp);
  rasterTrail(reference,b,points,5,width,0x5388cc,&stamp);
  if(memcmp(expanded,reference,sizeof(expanded)))return false;
 }return true;
}
static bool json(){
 char* b=(char*)allocRaw(configInitial);if(!b)return false;Writer w{b,0,configInitial,true};
 w.str(L"a\\b\"c\n\r\t\x0001");const char expected[]="\"a\\\\b\\\"c\\n\\r\\t \"";
 bool ok=w.ok&&w.n==(int)sizeof(expected)-1&&!memcmp(w.p,expected,sizeof(expected)-1);freep(w.p);return ok;
}
// Test registration and runtime packing without changing Explorer settings.
static bool hiddenFilesAction(){
 Gesture g{};
 for(int type: {TOGGLE_HIDDEN_FILES_ACTION,TOGGLE_ALL_HIDDEN_FILES_ACTION}){
  Act a{};a.type=type;int index=g.steps;
  if(commitAction(g,-1,a)!=index||g.steps!=index+1||actionGroup(type)!=1||retiredAction(type))return false;
  RuntimeAction packed{};prepareAction(packed,g.actions[index]);
  if(packed.type!=type||defs[type].vk!=0||*defs[type].hint)return false;
  for(int lang=0;lang<6;++lang)if(!*defs[type].name[lang])return false;
 }
 return g.steps==2;
}
static bool run(){return normalization()&&regex()&&runtime()&&raster()&&trailGrowth()&&json()&&hiddenFilesAction();}
}
