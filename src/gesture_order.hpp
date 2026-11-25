#pragma once
#include "model.hpp"
// Rebuild the active profile in its visible, grouped order. Other profiles keep
// their existing slots and all gesture IDs and action/shape data are preserved.
// A category header is a destination even when empty or collapsed.
static inline int reorderGesture(Config& c,int source,int target,int category,bool inside,bool before){
 if(source<0||source>=c.ng)return -1;
 int profile=c.g[source].profile;
 if(target>=0&&(target>=c.ng||c.g[target].profile!=profile))return -1;
 if(category>=0&&(category>=c.nc||c.categories[category].profile!=profile))return -1;
 if(inside&&category<0)return -1;
 Records<int> slots,order;Records<bool> seen;if(!slots.reserve(c.ng)||!order.reserve(c.ng)||!seen.reserve(c.nc))return -1;int count=0,n=0;
 for(int i=0;i<c.ng;i++)if(c.g[i].profile==profile){
  slots[count++]=i;int k=categoryIndex(c,profile,c.g[i].folder);
  if(k<0)order[n++]=i;
  else if(!seen[k]){seen[k]=true;for(int j=0;j<c.ng;j++)if(c.g[j].profile==profile&&eq(c.g[j].folder,c.categories[k].name))order[n++]=j;}
 }
 if(n!=count)return -1;
 int old=-1,at=n,first=-1,last=-1;
 for(int i=0;i<n;i++){
  int g=order[i];if(g==source)old=i;
  if(category>=0&&eq(c.g[g].folder,c.categories[category].name)){if(first<0)first=i;last=i;}
  if(g==target)at=i+(before?0:1);
 }
 if(old<0)return -1;
 if(category>=0){
  if(!inside)at=first<0?n:(before?first:last+1);
  else if(target<0)at=last<0?n:last+1;
 }
 const W* name=inside?c.categories[category].name:L"";
 if(at>old)at--;at=clamp(at,0,n-1);
 if(at==old&&eq(c.g[source].folder,name)){if(inside)c.categories[category].collapsed=0;return source;}
 // Allocate on the heap, not the input/UI thread's stack: Gesture includes templates.
 auto* reordered=(Gesture*)alloc(sizeof(Gesture)*n);if(!reordered)return -1;
 for(int i=old;i<n-1;i++)order[i]=order[i+1];
 for(int i=n-1;i>at;i--)order[i]=order[i-1];order[at]=source;
 for(int i=0;i<n;i++)reordered[i]=c.g[order[i]];
 cp(reordered[at].folder,name,96);
 for(int i=0;i<n;i++)c.g[slots[i]]=reordered[i];
 freep(reordered);if(inside)c.categories[category].collapsed=0;
 return slots[at];
}
