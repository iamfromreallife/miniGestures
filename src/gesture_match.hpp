#pragma once
#include "model.hpp"
struct Job{Snapshot*s;Target t;int button,mods,trigger,hotkey,n,direct,deferred;P p[maxDrawingPoints];};
// Every consumer reads only the populated point prefix. Avoid clearing the
// 8 KiB stroke buffer for hotkeys, rocker/wheel events and direct actions.
static inline Job* allocateJob(){auto* j=(Job*)allocRaw(sizeof(Job));if(j)memset(j,0,offsetof(Job,p));return j;}
static inline bool eligible(const Config&c,const Gesture&g,const Target&t,int pr,int b,int m,int trig){return g.enabled&&scopeEligible(c,g,pr)&&activationButton(c,g)==b&&g.mods==m&&g.trigger==trig;}
// Drawn shapes compete across all eligible scopes. A local binding wins only
// an equal angular rank and similarity; discrete triggers keep local override.
static inline int chooseAtThreshold(const Job&j,int&quality,int threshold,int* conflictId,int* otherId,int* closestIndex){
 auto&c=j.s->c;quality=0;if(closestIndex)*closestIndex=-1;if(conflictId)*conflictId=0;if(otherId)*otherId=0;
 if(j.direct){if(c.ng&&c.g[0].steps){quality=100;return 0;}return -1;}
 if(!c.enabled)return -1;
 int pr=profile(c,j.t);
 if(excluded(c,j.t,j.hotkey?2:1)||(!j.hotkey&&j.trigger>=1&&j.trigger<=4&&excluded(c,j.t,4))||(!j.hotkey&&j.trigger>=5&&excluded(c,j.t,3))){quality=-4;return -1;}
 bool drawn=!j.hotkey&&!j.trigger;
 mgangular::Signature input{};
 if(drawn){if(j.n<2||j.n>maxDrawingPoints)return -1;input=mgangular::signature(j.p,j.n);if(!input.valid)return -1;}
 long long closestRank=-1;
 int best=-1,bscore=-1,second=-1,secondIndex=-1;long long bestRank=-1;
 for(int pass=0;pass<2;pass++){
  int wanted=pass==0?pr:0;if(pass==1&&(!pr||!inherits(c,pr)))break;
  BindingRange range(*j.s,j.button,j.mods,j.hotkey);
  for(int i=range.first;range.valid(i);i=range.advance(i)){
   auto&g=c.g[i];if(!g.enabled||g.profile!=wanted||!g.steps)continue;
   if(pass==1&&blockedGlobal(c,pr,g.id))continue;
   if(j.hotkey){if(g.hotkey!=j.hotkey||g.hotmods!=j.mods)continue;}
   else if(activationButton(c,g)!=j.button||g.mods!=j.mods||g.trigger!=j.trigger)continue;
   int s=100;long long rank=0;
   if(drawn){
    if(!g.count)continue;
    // Use the cached signature by reference. Compute a temporary only
    // when allocation of the immutable snapshot cache failed.
    const double similarity=j.s->angular?mgangular::similarity(input,j.s->angular[i]):
      mgangular::similarity(input,mgangular::signature(g.samples[0],N));
    rank=mgangular::rank(similarity);if(rank<0)continue;
    s=mgangular::percent(similarity);
    if(rank>closestRank){closestRank=rank;quality=s;if(closestIndex)*closestIndex=i;}
    if(similarity+1e-9<threshold)continue;
   }
   // Profiles are visited local-first. Preserve local preference only when
   // rank and score tie; within a scope retain the original stable ID tie-break.
   bool wins=best<0||(drawn?(rank>bestRank||(rank==bestRank&&(s>bscore||
    (s==bscore&&g.profile==c.g[best].profile&&g.id<c.g[best].id)))):s>bscore);
   if(wins){second=bscore;secondIndex=best;bscore=s;best=i;bestRank=rank;}
   else if(s>second){second=s;secondIndex=i;}
  }
  // Do not fall through to global hotkeys, rockers or wheel actions when a
  // local binding exists: their exact-trigger priority is intentionally unchanged.
  if(!drawn&&best>=0){quality=bscore;if(second>=0&&bscore-second<4){quality=-2;if(conflictId)*conflictId=c.g[best].id;if(otherId&&secondIndex>=0)*otherId=c.g[secondIndex].id;return -1;}return best;}
  // An exact local angular match cannot be beaten by a global shape.
  if(drawn&&bestRank==shapeRankMax&&bscore==100)break;
 }
 if(best>=0){quality=bscore;return best;}
 return -1;
}

static inline int choose(const Job&j,int&quality,int* conflictId=nullptr,int* otherId=nullptr,int* closestIndex=nullptr){
 return chooseAtThreshold(j,quality,j.s->c.sensitivity,conflictId,otherId,closestIndex);
}
