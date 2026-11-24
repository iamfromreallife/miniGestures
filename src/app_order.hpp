#pragma once
#include "model.hpp"
// UI-only sibling order: positive ranks precede legacy/unranked entries.
static inline int ruleParent(const Config& c,int d,bool ex){return d>=0?categoryIndex(c,ex?-2:-1,(ex?c.e:c.a)[d].folder):-1;}
static inline int& ruleRank(Config& c,int d,bool ex){return d>=0?(ex?c.e:c.a)[d].uiOrder:c.categories[-2-d].uiOrder;}
static inline int ruleRank(const Config& c,int d,bool ex){return d>=0?(ex?c.e:c.a)[d].uiOrder:c.categories[-2-d].uiOrder;}
static inline int ruleSiblings(const Config& c,int parent,int*out,bool ex){int n=0;for(int i=0;i<(ex?c.ne:c.na);i++)if(ruleParent(c,i,ex)==parent)out[n++]=i;if(parent<0)for(int i=0;i<c.nc;i++)if(c.categories[i].profile==(ex?-2:-1))out[n++]=-2-i;for(int i=1;i<n;i++){int d=out[i],j=i,rank=ruleRank(c,d,ex);while(j>0){int prev=ruleRank(c,out[j-1],ex);if(rank<=0||(prev>0&&prev<=rank))break;out[j]=out[j-1];j--;}out[j]=d;}return n;}
static inline bool moveRuleNode(Config& c,int source,int target,int category,bool inside,bool before,bool ex){
 if(source==-1||source>=(ex?c.ne:c.na)||(source<=-2&&(-2-source>=c.nc||c.categories[-2-source].profile!=(ex?-2:-1))))return false;
 if(target>=(ex?c.ne:c.na)||category>=c.nc||(category>=0&&c.categories[category].profile!=(ex?-2:-1)))return false;
 if(source<=-2)inside=false; // categories remain top-level
 int parent=inside?category:-1;if(inside&&parent<0)return false;
 int anchor=target;if(!inside&&category>=0)anchor=-2-category;
 if(inside&&target<0)anchor=-1; if(anchor==source)return true;
 int list[MAXR+MAXC],n=ruleSiblings(c,parent,list,ex),at=0;for(int i=0;i<n;i++)if(list[i]!=source)list[at++]=list[i];n=at;at=n;
 if(anchor!=-1){for(int i=0;i<n;i++)if(list[i]==anchor){at=i+(before?0:1);break;}}
 for(int i=n;i>at;i--)list[i]=list[i-1];list[at]=source;n++;
 if(source>=0)cp((ex?c.e:c.a)[source].folder,parent>=0?c.categories[parent].name:L"",96);
 for(int i=0;i<n;i++)ruleRank(c,list[i],ex)=i+1;if(parent>=0)c.categories[parent].collapsed=0;return true;
}
static inline int appParent(const Config& c,int d){return ruleParent(c,d,false);}
static inline int appSiblings(const Config& c,int parent,int*out){return ruleSiblings(c,parent,out,false);}
static inline bool moveAppNode(Config& c,int source,int target,int category,bool inside,bool before){return moveRuleNode(c,source,target,category,inside,before,false);}
static inline int cloneExclusion(Config& c,const Rule& r,int parent){if(c.ne>=MAXR||parent<-1||parent>=c.nc||(parent>=0&&c.categories[parent].profile!=-2))return -1;int i=c.ne++;auto& v=c.e[i];v=r;v.id=c.nextid++;v.enabled=0;v.uiOrder=0;cp(v.folder,parent>=0?c.categories[parent].name:L"",96);cat(v.name,L" (2)",96);if(parent>=0)c.categories[parent].collapsed=0;return i;}
static inline bool renameExclusionCategory(Config& c,int catIndex,const W* name){if(catIndex<0||catIndex>=c.nc||c.categories[catIndex].profile!=-2||!*name)return false;int other=categoryIndex(c,-2,name);if(other>=0&&other!=catIndex)return false;auto& v=c.categories[catIndex];for(int i=0;i<c.ne;i++)if(eq(c.e[i].folder,v.name))cp(c.e[i].folder,name,96);cp(v.name,name,96);return true;}
static inline bool removeExclusionCategory(Config& c,int ci){if(ci<0||ci>=c.nc||c.categories[ci].profile!=-2)return false;for(int i=0;i<c.ne;i++)if(eq(c.e[i].folder,c.categories[ci].name)){c.e[i].folder[0]=0;c.e[i].uiOrder=0;}for(int i=ci;i<c.nc-1;i++)c.categories[i]=c.categories[i+1];c.nc--;return true;}
// Clone all saved app data with fresh IDs. Disable the duplicate until its selector is adjusted.
static inline int cloneApp(Config& c,const Rule& r,const Gesture* g,int ng,const Category* cats,int nc,int parent){
 if(c.na>=MAXR||c.ng+ng>MAXG||c.nc+nc>MAXC)return -1;int i=c.na++;auto& a=c.a[i];a=r;a.id=c.nextid++;a.enabled=0;a.uiOrder=0;cp(a.folder,parent>=0?c.categories[parent].name:L"",96);cat(a.name,L" (2)",96);
 for(int j=0;j<ng;j++){auto& v=c.g[c.ng++];v=g[j];v.id=c.nextid++;v.profile=a.id;}
 for(int j=0;j<nc;j++){auto& v=c.categories[c.nc++];v=cats[j];v.profile=a.id;}
 if(parent>=0)c.categories[parent].collapsed=0;return i;
}
