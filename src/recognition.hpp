#pragma once
#include "angular_recognition.hpp"
// Platform-independent integer recognition. Translation/scale normalized, order preserved.
struct P{int x,y;}; constexpr int legacyShapePoints=48,N=70,maxDrawingPoints=1000,minSegmentPx=6;
static constexpr int maxGestureTimeoutMs=60000;
// Unsigned subtraction also handles GetTickCount wrapping during a stroke.
static inline bool gestureTimeoutExpired(unsigned start,unsigned now,int limit){return limit>0&&now-start>=(unsigned)limit;}
// Fixed-size capture: retain both ends when compacting an exceptionally long stroke.
static inline void compactDrawing(P* p,int& n){int old=n,out=1;for(int i=2;i<old-1;i+=2)p[out++]=p[i];if(old>1)p[out++]=p[old-1];n=out;}
static inline bool appendDrawingPoint(P* p,int& n,P q,bool endpoint=false){
 if(n<1||n>maxDrawingPoints)return false;
 long long dx=(long long)q.x-p[n-1].x,dy=(long long)q.y-p[n-1].y;
 if(!dx&&!dy)return false;
 // Avoid overflow even for coordinates spanning the entire signed int range.
 if(!endpoint&&dx>-minSegmentPx&&dx<minSegmentPx&&dy>-minSegmentPx&&dy<minSegmentPx&&dx*dx+dy*dy<minSegmentPx*minSegmentPx)return false;
 if(n==maxDrawingPoints)compactDrawing(p,n);p[n++]=q;return true;
}
static inline int rabs(int x){return x<0?-x:x;} static inline int rmax(int a,int b){return a>b?a:b;}
static inline int dist(P a,P b){int x=rabs(a.x-b.x),y=rabs(a.y-b.y);return rmax(x,y)+(x<y?x:y)*3/8;}
static inline int pathlen(const P*p,int n){int d=0;for(int i=1;i<n;i++)d+=dist(p[i-1],p[i]);return d;}
// Preserve fractional arc length and coordinates until the final normalized
// integer representation. The previous integer-pixel interpolation could
// collapse adjacent points on small gestures and distort corners.
static inline double arcDistance(P a,P b){return 8*mgangular::distance(a,b);}
static inline int nearestInt(double x){return (int)(x<0?x-0.5:x+0.5);}
static inline bool normalize(const P*p,int n,P*out){
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
// Saved legacy points already represent equal arc-length intervals. Interpolate
// by index, preserving their normalized geometry and exact endpoints; do not
// renormalize or re-scale a sample that was already normalized by the old engine.
static inline void migrateLegacyShape(const P* old,P* out){
 for(int k=0;k<N;k++){int t=k*(legacyShapePoints-1),i=t/(N-1),f=t%(N-1);if(i==legacyShapePoints-1){out[k]=old[i];continue;}
  auto lerp=[&](int a,int b){int v=a*(N-1-f)+b*f;return v<0?(v-(N-1)/2)/(N-1):(v+(N-1)/2)/(N-1);};out[k]={lerp(old[i].x,old[i+1].x),lerp(old[i].y,old[i+1].y)};
 }
}
static inline int direction(int dx,int dy){int x=rabs(dx),y=rabs(dy);if(x*5<y*2)return dy<0?0:4;if(y*5<x*2)return dx>0?2:6;return dy<0?(dx>0?1:7):(dx>0?3:5);}
static inline int sequence(const P*p,int n,char*out){int z=0;P anchor=p[0];for(int i=1;i<n;i++){if(dist(anchor,p[i])<90)continue;int d=direction(p[i].x-anchor.x,p[i].y-anchor.y);if(!z||out[z-1]!=char('0'+d)){if(z<31)out[z++]=char('0'+d);}anchor=p[i];}out[z]=0;return z;}
static inline int score(const P*a,const P*b){int d=0;for(int i=0;i<N;i++)d+=dist(a[i],b[i]);int s=100-d*100/(N*1024);return s<0?0:s;}
static inline int directionSimilarity(const char*x,int nx,const char*y,int ny){if(nx==ny){int i=0;while(i<nx&&x[i]==y[i])++i;if(i==nx)return 100;}
 // Exact bit-parallel Levenshtein distance for longer compass strings.
 // Keep the small dynamic-programming path for short strings and any
 // non-compass input. All production strings contain at most 31 digits.
 if(nx>0&&nx<=31&&ny>0&&ny<=31&&nx*ny>=48){unsigned masks[8]{};bool compass=true;for(int i=0;i<nx;i++){unsigned k=(unsigned)(x[i]-'0');if(k>=8){compass=false;break;}masks[k]|=1u<<i;}
  if(compass){unsigned pv=~0u,mv=0,high=1u<<(nx-1);int edits=nx;for(int i=0;i<ny;i++){unsigned k=(unsigned)(y[i]-'0');unsigned eq=k<8?masks[k]:0,xv=eq|mv,xh=(((eq&pv)+pv)^pv)|eq;unsigned ph=mv|~(xh|pv),mh=pv&xh;edits+=(ph&high)!=0;edits-=(mh&high)!=0;ph=(ph<<1)|1u;mh<<=1;pv=mh|~(xv|ph);mv=ph&xv;}return 100-100*edits/rmax(nx,ny);}}
 int d[33];for(int j=0;j<=ny;j++)d[j]=j;for(int i=1;i<=nx;i++){int prev=d[0];d[0]=i;for(int j=1;j<=ny;j++){int old=d[j],v=prev+(x[i-1]!=y[j-1]);if(d[j]+1<v)v=d[j]+1;if(d[j-1]+1<v)v=d[j-1]+1;d[j]=v;prev=old;}}return 100-100*d[ny]/rmax(1,rmax(nx,ny));}
static inline int hybrid(const P*a,const P*b){char x[32],y[32];int nx=sequence(a,N,x),ny=sequence(b,N,y);return (score(a,b)*9+directionSimilarity(x,nx,y,ny))/10;}
// Exact upper bound: the direction term cannot exceed 100. Skip only comparisons
// that cannot improve the best score, preserving the original match semantics.
static inline int bestShapeScorePrepared(const P* normalized,const char* seq,int sn,const P (*samples)[N],int count){int best=0;for(int i=0;i<count;i++){int base=score(normalized,samples[i]);if((base*9+100)/10<=best)continue;char y[32];int ny=sequence(samples[i],N,y);best=rmax(best,(base*9+directionSimilarity(seq,sn,y,ny))/10);if(best==100)break;}return best;}
static inline int bestShapeScore(const P* normalized,const P (*samples)[N],int count){char seq[32];int sn=sequence(normalized,N,seq);return bestShapeScorePrepared(normalized,seq,sn,samples,count);}

// Keep the legacy sensitivity gate for compatibility with existing training data.
// Among accepted samples rank by continuous, ordered geometric evidence instead
// of rounded percentages or hard eight-direction bin boundaries.
// Compare point positions, local stroke vectors at two scales, and both ends.
// Rank the 70 normalized points using integer arithmetic. Scale chord spans
// to retain approximately the same relative stroke lengths as at 48 points.
struct RankedShape {int score; long long rank;};
static constexpr long long shapeRankMax=1000000000LL;
static constexpr int shortChordSpan=6,longChordSpan=18,endpointWeight=18;
static inline int chordError(const P*a,const P*b,int span){int d=0;for(int k=span;k<N;k++){P av{a[k].x-a[k-span].x,a[k].y-a[k-span].y},bv{b[k].x-b[k-span].x,b[k].y-b[k-span].y};d+=dist(av,bv);}return d;}
static inline long long shapeRank(const P*a,const P*b,int pointError){
 // Local vectors distinguish bends and hooks; longer chords tolerate small
 // corner-timing differences. Endpoints help distinguish open/closed strokes.
 int ends=dist(a[0],b[0])+dist(a[N-1],b[N-1]);
 long long loss=6LL*pointError+4LL*chordError(a,b,shortChordSpan)+2LL*chordError(a,b,longChordSpan)+(long long)endpointWeight*ends;
 return shapeRankMax-loss;
}
// Average similarity alone can hide a severely mismatched suffix. Independent
// geometric guards reject a wrong ending, sustained local deviation or substantial
// extra travel, while allowing normal variations and small terminal hooks.
static inline bool wholeStrokeCompatible(const P* a,const P* b){
 constexpr int window=N/5, endpointLimit=300, tailMeanLimit=220, localMeanLimit=300;
 if(dist(a[N-1],b[N-1])>endpointLimit||dist(a[0],b[0])>endpointLimit)return false;
 int errors[window]{};int sum=0;double travelA=0,travelB=0;
 for(int i=0;i<N;i++){
  int d=dist(a[i],b[i]),slot=i%window;sum+=d-errors[slot];errors[slot]=d;
  if(i>=window-1&&sum>window*localMeanLimit)return false;
  if(i){travelA+=arcDistance(a[i-1],a[i]);travelB+=arcDistance(b[i-1],b[i]);}
 }
 return sum<=window*tailMeanLimit&&travelB>0&&travelA*2<=travelB*3;
}
static inline RankedShape rankedShapePrepared(const P*norm,const char*seq,int sn,const P (*samples)[N],int count,int threshold=0,int* observed=nullptr,long long rankFloor=-1,bool requireComplete=false){
 RankedShape best{-1,-1};for(int i=0;i<count;i++){
  int d=0;for(int k=0;k<N;k++)d+=dist(norm[k],samples[i][k]);
  if(!d){if(observed)*observed=rmax(*observed,100);if(threshold<=100)return {100,shapeRankMax};}
  int base=rmax(0,100-d*100/(N*1024));int upper=(base*9+100)/10;
  if(upper<threshold&&(!observed||upper<=*observed))continue;
  char y[32];int ny=sequence(samples[i],N,y),dir=directionSimilarity(seq,sn,y,ny);int s=(base*9+dir)/10;
  if(observed)*observed=rmax(*observed,s);if(s<threshold||(requireComplete&&!wholeStrokeCompatible(norm,samples[i])))continue;
  // Position error alone is a lower bound on total loss.
  long long floor=best.rank>rankFloor?best.rank:rankFloor;
  // All loss terms are non-negative. Strict inequality preserves score/ID tie breaks.
  long long rank=shapeRankMax-6LL*d;
  if(rank<floor)continue;
  rank-=(long long)endpointWeight*(dist(norm[0],samples[i][0])+dist(norm[N-1],samples[i][N-1]));
  if(rank<floor)continue;
  rank-=4LL*chordError(norm,samples[i],shortChordSpan);
  if(rank<floor)continue;
  rank-=2LL*chordError(norm,samples[i],longChordSpan);
  if(rank<floor)continue;
  if(rank>best.rank||(rank==best.rank&&s>best.score))best={s,rank};
  if(best.rank==shapeRankMax)break;
 }return best;
}

// Choose the medoid of a legacy gesture library: the actual sample with the
// smallest sum of geometric losses to all other samples. Ties retain the first.
static inline int representativeShape(const P (*samples)[N],int count){
 if(count<=1)return 0;long long losses[64]{};
 for(int i=0;i<count;i++)for(int j=0;j<i;j++){int d=0;for(int k=0;k<N;k++)d+=dist(samples[i][k],samples[j][k]);long long loss=shapeRankMax-shapeRank(samples[i],samples[j],d);losses[i]+=loss;losses[j]+=loss;}
 int best=0;for(int i=1;i<count;i++)if(losses[i]<losses[best])best=i;return best;
}
