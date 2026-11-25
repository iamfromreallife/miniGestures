#pragma once
// Platform-independent integer recognition. Translation/scale normalized, order preserved.
struct P{int x,y;}; constexpr int N=48;
static inline int rabs(int x){return x<0?-x:x;} static inline int rmax(int a,int b){return a>b?a:b;}
static inline int dist(P a,P b){int x=rabs(a.x-b.x),y=rabs(a.y-b.y);return rmax(x,y)+(x<y?x:y)*3/8;}
static inline int pathlen(const P*p,int n){int d=0;for(int i=1;i<n;i++)d+=dist(p[i-1],p[i]);return d;}
static inline bool normalize(const P*p,int n,P*out){int total=pathlen(p,n);if(n<2||total<3)return false;int j=1,run=0,d=dist(p[0],p[1]);for(int k=0;k<N;k++){int target=(int)((long long)total*k/(N-1));while(j<n-1&&run+d<target){run+=d;j++;d=dist(p[j-1],p[j]);}int t=target-run;if(t<0)t=0;if(t>d)t=d;out[k]=d?P{p[j-1].x+(int)((long long)(p[j].x-p[j-1].x)*t/d),p[j-1].y+(int)((long long)(p[j].y-p[j-1].y)*t/d)}:p[j];}int minx=out[0].x,maxx=minx,miny=out[0].y,maxy=miny;for(int i=1;i<N;i++){auto q=out[i];if(q.x<minx)minx=q.x;if(q.x>maxx)maxx=q.x;if(q.y<miny)miny=q.y;if(q.y>maxy)maxy=q.y;}int s=rmax(maxx-minx,maxy-miny);if(!s)return false;for(int i=0;i<N;i++){out[i].x=(out[i].x-minx)*1024/s-(maxx-minx)*512/s;out[i].y=(out[i].y-miny)*1024/s-(maxy-miny)*512/s;}return true;}
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
// All coordinates are the existing normalized 48 points; no allocations or
// floating point, and translation/scale normalization is unchanged.
struct RankedShape {int score; long long rank;};
static constexpr long long shapeRankMax=1000000000LL;
static inline int chordError(const P*a,const P*b,int span){int d=0;for(int k=span;k<N;k++){P av{a[k].x-a[k-span].x,a[k].y-a[k-span].y},bv{b[k].x-b[k-span].x,b[k].y-b[k-span].y};d+=dist(av,bv);}return d;}
static inline long long shapeRank(const P*a,const P*b,int pointError){
 // Local vectors distinguish bends and hooks; longer chords tolerate small
 // corner-timing differences. Endpoints help distinguish open/closed strokes.
 int ends=dist(a[0],b[0])+dist(a[N-1],b[N-1]);
 long long loss=6LL*pointError+4LL*chordError(a,b,4)+2LL*chordError(a,b,12)+12LL*ends;
 return shapeRankMax-loss;
}
static inline RankedShape rankedShapePrepared(const P*norm,const char*seq,int sn,const P (*samples)[N],int count,int threshold=0,int* observed=nullptr){
 RankedShape best{-1,-1};for(int i=0;i<count;i++){
  int d=0;for(int k=0;k<N;k++)d+=dist(norm[k],samples[i][k]);
  if(!d){if(observed)*observed=rmax(*observed,100);if(threshold<=100)return {100,shapeRankMax};}
  int base=rmax(0,100-d*100/(N*1024));int upper=(base*9+100)/10;
  if(upper<threshold&&(!observed||upper<=*observed))continue;
  char y[32];int ny=sequence(samples[i],N,y),dir=directionSimilarity(seq,sn,y,ny);int s=(base*9+dir)/10;
  if(observed)*observed=rmax(*observed,s);if(s<threshold)continue;
  // Position error alone is a lower bound on total loss.
  if(shapeRankMax-6LL*d<best.rank)continue;
  long long rank=shapeRank(norm,samples[i],d);
  if(rank>best.rank||(rank==best.rank&&s>best.score))best={s,rank};
  if(best.rank==shapeRankMax)break;
 }return best;
}
