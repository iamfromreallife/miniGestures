#pragma once
#include "win.hpp"
extern "C" void* memset(void*,int,up); extern "C" void* memcpy(void*,const void*,up); extern "C" void* memmove(void*,const void*,up);
static inline int len(const W*s){int n=0;while(s&&s[n])++n;return n;}
static inline void cp(W*d,const W*s,int cap){if(cap<=0)return;int i=0; if(s)for(;i<cap-1&&s[i];++i)d[i]=s[i];d[i]=0;}
static inline void cat(W*d,const W*s,int cap){int n=len(d);if(n<cap)cp(d+n,s,cap-n);}
static inline W fold(W c){return c>=L'A'&&c<=L'Z'?c+32:c;}
static inline bool eq(const W*a,const W*b){while(*a&&*b&&fold(*a)==fold(*b)){++a;++b;}return !*a&&!*b;}
static inline bool contains(const W*a,const W*b){if(!*b)return true;for(;*a;++a){int i=0;while(b[i]&&a[i]&&fold(a[i])==fold(b[i]))++i;if(!b[i])return true;}return false;}
static inline int num(const W*s){int n=0,sign=1;if(*s==L'-'){sign=-1;++s;}while(*s>=L'0'&&*s<=L'9'){if(n>100000000)return sign*n;n=n*10+*s++-L'0';}return n*sign;}
static inline void number(W*d,int n){W tmp[20];int k=0;unsigned v=n<0?0u-(unsigned)n:(unsigned)n;do{tmp[k++]=L'0'+v%10;v/=10;}while(v);int i=0;if(n<0)d[i++]=L'-';while(k)d[i++]=tmp[--k];d[i]=0;}
// Presentation only: negative scores mean no measured recognition (editor test/preview).
static inline void gestureScoreLabel(W*d,int cap,const W*name,int score){if(cap<=0)return;cp(d,name,cap);if(score<0||score>100)return;W suffix[16];cp(suffix,L" (",16);W n[16];number(n,score);cat(suffix,n,16);cat(suffix,L"%)",16);int tail=len(suffix);if(cap<=tail)return;cp(d,name,cap-tail);cat(d,suffix,cap);}
static inline int absn(int x){return x<0?-x:x;} static inline int maxn(int a,int b){return a>b?a:b;} static inline int minn(int a,int b){return a<b?a:b;} static inline int clamp(int x,int a,int b){return maxn(a,minn(x,b));}
static inline bool glob(const W*s,const W*p){const W*star=nullptr,*retry=nullptr;while(*s){if(*p==L'?'||fold(*p)==fold(*s)){++p;++s;}else if(*p==L'*'){star=p++;retry=s;}else if(star){p=star+1;s=++retry;}else return false;}while(*p==L'*')++p;return !*p;}
static inline const W* basename(const W*s){const W*r=s;for(;*s;++s)if(*s==L'\\'||*s==L'/')r=s+1;return r;}
static inline void* alloc(up n){return HeapAlloc(GetProcessHeap(),8,n);} static inline void* allocRaw(up n){return HeapAlloc(GetProcessHeap(),0,n);} static inline void freep(void*p){if(p)HeapFree(GetProcessHeap(),0,p);}

// Bounded Thompson NFA, case-insensitive, regex search (re: prefix).
// No backtracking; supports groups, alternation, anchors, classes, escapes, * + ?.
namespace { namespace mgregex {
struct State {int op,a,b,lo,hi;W ch;};
struct Frag {int start,head,tail;};
struct Regex {
 const W* p; const W* pattern; State st[520]; int links[1040]; int n=0; bool ok=true; int depth=0;
 int node(int op,int a=-1,int b=-1,W ch=0,int lo=0,int hi=0){if(n>=520){ok=false;return 0;}int i=n++;st[i]={op,a,b,lo,hi,ch};return i;}
 Frag single(int i,int slot=0){int k=2*i+slot;links[k]=-1;return {i,k,k};}
 Frag join(Frag a,Frag b,int start){if(!ok)return {start,-1,-1};if(a.head<0)return {start,b.head,b.tail};if(b.head<0)return {start,a.head,a.tail};links[a.tail]=b.head;return {start,a.head,b.tail};}
 void patch(Frag a,int dest){if(!ok)return;for(int i=a.head;i>=0;){int next=links[i];if(i&1)st[i/2].b=dest;else st[i/2].a=dest;i=next;}}
 Frag atom(){if(!ok)return single(node(0));W c=*p;if(!c||c==L')'||c==L'|')return single(node(0));++p;if(c==L'('){if(++depth>48){ok=false;return single(node(0));}auto f=expr();--depth;if(*p!=L')')ok=false;else ++p;return f;}if(c==L'['){int lo=(int)(p-pattern);if(*p==L'^')++p;bool any=false;while(*p&&*p!=L']'){any=true;if(*p==L'\\'&&p[1])++p;++p;}int hi=(int)(p-pattern);if(*p!=L']'||!any)ok=false;else ++p;return single(node(6,-1,-1,0,lo,hi));}if(c==L'\\'){if(!*p){ok=false;return single(node(0));}c=*p++;if(c==L'd'||c==L'w'||c==L's'||c==L'D'||c==L'W'||c==L'S')return single(node(7,-1,-1,c));if(c>=L'0'&&c<=L'9')ok=false;return single(node(1,-1,-1,c));}if(c==L'.')return single(node(2));if(c==L'^')return single(node(3));if(c==L'$')return single(node(4));if(c==L'*'||c==L'+'||c==L'?'||c==L'{'||c==L'}')ok=false;return single(node(1,-1,-1,c));}
 Frag term(){auto f=atom();if(!ok)return f;W c=*p;if(c==L'*'||c==L'+'||c==L'?'){++p;int i=node(5,f.start);if(c==L'?')f=join(f,single(i,1),i);else {patch(f,i);auto q=single(i,1);f={c==L'+'?f.start:i,q.head,q.tail};}}return f;}
 Frag seq(){auto f=term();while(ok&&*p&&*p!=L')'&&*p!=L'|'){auto g=term();patch(f,g.start);f={f.start,g.head,g.tail};}return f;}
 Frag expr(){auto f=seq();while(ok&&*p==L'|'){++p;auto g=seq();int i=node(5,f.start,g.start);f=join(f,g,i);}return f;}
 int compile(const W*s){p=pattern=s;if(len(s)>258)return -1;auto f=expr();if(!ok||*p)return -1;int end=node(8);if(!ok)return -1;patch(f,end);return f.start;}
 bool klass(const State&q,W c){const W*s=pattern+q.lo,*e=pattern+q.hi;bool neg=*s==L'^',hit=false;if(neg)++s;while(s<e){W a=*s++;if(a==L'\\'&&s<e)a=*s++;W b=a;if(s+1<e&&*s==L'-'){++s;b=*s++;if(b==L'\\'&&s<e)b=*s++;}if(fold(c)>=fold(a)&&fold(c)<=fold(b))hit=true;}return neg?!hit:hit;}
 bool special(W q,W c){W k=fold(q);bool d=c>=L'0'&&c<=L'9',w=d||(fold(c)>=L'a'&&fold(c)<=L'z')||c==L'_',s=c==L' '||c==L'\t'||c==L'\r'||c==L'\n';bool r=k==L'd'?d:k==L'w'?w:s;return q==k?r:!r;}
 struct Active {bool seen[520]{};int list[520];int count=0;bool matched=false;void clear(){for(int i=0;i<count;i++)seen[list[i]]=false;count=0;matched=false;}};
 void add(int i,Active& set,int pos,int size){if(i<0||i>=n||set.seen[i])return;set.seen[i]=true;set.list[set.count++]=i;auto&q=st[i];if(q.op==8)set.matched=true;else if(q.op==0||(q.op==3&&pos==0)||(q.op==4&&pos==size))add(q.a,set,pos,size);else if(q.op==5){add(q.a,set,pos,size);add(q.b,set,pos,size);}}
 bool match(const W*s,int start){Active sets[2];Active* a=&sets[0],*b=&sets[1];int size=len(s);for(int pos=0;pos<=size;pos++){add(start,*a,pos,size);if(a->matched)return true;if(pos==size)break;W c=s[pos];for(int j=0;j<a->count;j++){auto&q=st[a->list[j]];if((q.op==1&&fold(q.ch)==fold(c))||(q.op==2&&c!=L'\n')||(q.op==6&&klass(q,c))||(q.op==7&&special(q.ch,c)))add(q.a,*b,pos+1,size);}a->clear();auto* old=a;a=b;b=old;}return false;}

};
static inline bool is(const W*p){return *p&&fold(p[0])==L'r'&&fold(p[1])==L'e'&&p[2]==L':';}
static inline bool valid(const W*p){if(!is(p))return true;Regex r;return r.compile(p+3)>=0;}
static inline bool test(const W*s,const W*p){if(!is(p))return glob(s,p);Regex r;int i=r.compile(p+3);return i>=0&&r.match(s,i);}
// Match the same executable rule against full path and basename, compiling once.
static inline bool executableTest(const W*s,const W*p){if(!is(p))return glob(s,p)||glob(basename(s),p);Regex r;int i=r.compile(p+3);return i>=0&&(r.match(s,i)||r.match(basename(s),i));}
}
}
