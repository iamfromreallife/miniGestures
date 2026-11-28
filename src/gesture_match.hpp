#pragma once
#include "model.hpp"
struct Job{Snapshot*s;Target t;int button,mods,trigger,hotkey,n,direct,deferred,selectedIndex,quality;P* p;};
// Discrete jobs carry no stroke storage; drawn jobs carry exactly n points.
static inline Job* allocateJob(int points=0){
 if(points<0||points>maxDrawingPoints)return nullptr;
 auto* j=(Job*)allocRaw(sizeof(Job)+(up)points*sizeof(P));
 if(j){memset(j,0,sizeof(Job));if(points)j->p=reinterpret_cast<P*>(j+1);}return j;
}
static inline bool eligible(const Config&c,const Gesture&g,const Target&t,int pr,int b,int m,int trig){return g.enabled&&scopeEligible(c,g,pr)&&activationButton(c,g)==b&&g.mods==m&&g.trigger==trig;}
// Local matches above threshold override inherited globals. Diagnostics may
// score both scopes, but cannot change the selected action.
struct MatchTrace{int profileId=0,localId=0,localScore=-1,globalId=0,globalScore=-1;};
static inline int chooseAtThreshold(const Job&j,int&quality,int threshold,int* conflictId,int* otherId,int* closestIndex,MatchTrace* trace=nullptr){
 if(trace)*trace=MatchTrace{};
 auto&c=j.s->c;quality=0;if(closestIndex)*closestIndex=-1;if(conflictId)*conflictId=0;if(otherId)*otherId=0;
 if(j.direct){if(c.ng&&c.g[0].steps){quality=100;return 0;}return -1;}
 if(!c.enabled)return -1;
 int pr=profile(c,j.t);if(trace)trace->profileId=pr;
 unsigned excludedTypes=1u<<(j.hotkey?2:1);if(!j.hotkey&&j.trigger>=1&&j.trigger<=4)excludedTypes|=1u<<4;if(!j.hotkey&&j.trigger>=5)excludedTypes|=1u<<3;
 if(exclusionMask(c,j.t,excludedTypes)){quality=-4;return -1;}
 bool drawn=!j.hotkey&&!j.trigger;
 mgangular::Signature input{};
 if(drawn){if(j.n<2||j.n>maxDrawingPoints)return -1;input=mgangular::signature(j.p,j.n);if(!input.valid)return -1;}
 long long closestRank=-1;
 int best=-1,bscore=-1,second=-1,secondIndex=-1;long long bestRank=-1;int localWinner=-1,localQuality=-1,localClosest=-1;
 for(int pass=0;pass<2;pass++){
  int wanted=pass==0?pr:0;if(pass==1&&(!pr||!inherits(c,pr)))break;
  long long traceRank=-1;
  BindingRange range(*j.s,j.button,j.mods,j.hotkey,j.hotkey?-1:j.trigger,wanted);
  for(int i=range.first;range.valid(i);i=range.advance(i)){
   auto&g=c.g[i];if(!g.enabled||g.profile!=wanted||!g.steps)continue;
   if(pass==1&&blockedGlobal(c,pr,g.id))continue;
   if(j.hotkey){if(g.hotkey!=j.hotkey||g.hotmods!=j.mods)continue;}
   else if(activationButton(c,g)!=j.button||g.mods!=j.mods||g.trigger!=j.trigger)continue;
   int s=100;long long rank=0;
   if(drawn){
    if(g.count<=0||g.count>MAXS)continue;
    // Every saved shape is an independent alternative for the same action.
    // Compare all populated samples, retaining the highest angular score.
    double similarity=-1;
    for(int k=0;k<g.count;k++){
     const double score=j.s->angular?mgangular::similarity(input,j.s->angular[j.s->angularOffsets[i]+k]):
       mgangular::similarity(input,mgangular::signature(g.samples[k],N));
     if(score>similarity)similarity=score;
     if(similarity==100)break;
    }
    rank=mgangular::rank(similarity);if(rank<0)continue;
    s=mgangular::percent(similarity);
    if(trace&&rank>traceRank){traceRank=rank;if(wanted){trace->localId=g.id;trace->localScore=s;}else{trace->globalId=g.id;trace->globalScore=s;}}
    if(rank>closestRank){closestRank=rank;quality=s;if(closestIndex)*closestIndex=i;}
    if(similarity+1e-9<threshold)continue;
   }
   // Compare ranks within a scope and retain the stable ID tie-break.
   bool wins=best<0||(drawn?(rank>bestRank||(rank==bestRank&&(s>bscore||
    (s==bscore&&g.profile==c.g[best].profile&&g.id<c.g[best].id)))):s>bscore);
   if(wins){second=bscore;secondIndex=best;bscore=s;best=i;bestRank=rank;}
   else if(s>second){second=s;secondIndex=i;}
  }
  // Do not fall through to global hotkeys, rockers or wheel actions when a
  // local binding exists: their exact-trigger priority is intentionally unchanged.
  if(!drawn&&best>=0){quality=bscore;if(second>=0&&bscore-second<4){quality=-2;if(conflictId)*conflictId=c.g[best].id;if(otherId&&secondIndex>=0)*otherId=c.g[secondIndex].id;return -1;}return best;}
  if(drawn&&best>=0&&wanted){
   if(!trace||!inherits(c,pr)){quality=bscore;return best;}
   localWinner=best;localQuality=bscore;localClosest=closestIndex?*closestIndex:-1;
   best=-1;bscore=-1;second=-1;secondIndex=-1;bestRank=-1;
  }
 }
 if(localWinner>=0){quality=localQuality;if(closestIndex)*closestIndex=localClosest;return localWinner;}
 if(best>=0){quality=bscore;return best;}
 return -1;
}

static inline int choose(const Job&j,int&quality,int* conflictId=nullptr,int* otherId=nullptr,int* closestIndex=nullptr,MatchTrace* trace=nullptr){
 return chooseAtThreshold(j,quality,j.s->c.sensitivity,conflictId,otherId,closestIndex,trace);
}
