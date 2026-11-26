#pragma once
// Opt-in in-memory diagnostics. No disk I/O, waits, allocation, or foreign
// messages in a hook callback. A bounded ring keeps the most recent events.
#ifdef _WIN32
namespace mgdiag {
struct Record { const char* stage; DWORD tick,tid; u64 v[8]; };
static constexpr unsigned capacity=32768;
static Record* records=nullptr;
static volatile long active=0, guard=0, contention=0;
static unsigned count=0,head=0,overwritten=0;
static DWORD startTick=0;
static bool enabled(){return InterlockedExchangeAdd(&active,0)!=0;}
static void lock(){while(InterlockedExchange(&guard,1))Sleep(1);}
static void unlock(){InterlockedExchange(&guard,0);}
static bool start(){lock();if(!records)records=(Record*)allocRaw(sizeof(Record)*capacity);if(!records){unlock();return false;}count=head=overwritten=0;InterlockedExchange(&contention,0);startTick=GetTickCount();InterlockedExchange(&active,1);unlock();return true;}
static void stop(){lock();InterlockedExchange(&active,0);unlock();}
static bool pending(){return records&&!enabled();} // only called by editor thread
static void discard(){lock();if(!enabled()){freep(records);records=nullptr;count=head=overwritten=0;}unlock();}
static void event(const char* stage,u64 a=0,u64 b=0,u64 c=0,u64 d=0,u64 e=0,u64 f=0,u64 g=0,u64 h=0){
 if(!enabled())return;
 if(InterlockedExchange(&guard,1)){InterlockedIncrement(&contention);return;}
 if(enabled()&&records){records[head]={stage,GetTickCount()-startTick,GetCurrentThreadId(),{a,b,c,d,e,f,g,h}};head=(head+1)%capacity;if(count<capacity)++count;else ++overwritten;}
 unlock();
}
// Save only after recording stops. Stage strings are fixed literals; all
// fields are numeric. No keyboard text, titles, URLs, or configured arguments.
struct Line{char b[512];unsigned n=0;void add(const char*s){while(*s&&n<sizeof(b)-1)b[n++]=*s++;}void dec(u64 v){char t[24];int i=0;do{t[i++]=char('0'+v%10);v/=10;}while(v);while(i&&n<sizeof(b)-1)b[n++]=t[--i];}void hex(u64 v){add("0x");char t[16];int i=0;do{t[i++]="0123456789ABCDEF"[v&15];v>>=4;}while(v);while(i&&n<sizeof(b)-1)b[n++]=t[--i];}};
static bool writeAll(H h,const void*p,DWORD n){DWORD done=0;return WriteFile(h,p,n,&done,nullptr)&&done==n;}
static bool save(const W* path){if(!pending())return false;H file=CreateFileW(path,0x40000000,0,nullptr,2,0x80,nullptr);if(file==PTR(-1))return false;char buffer[32768];unsigned used=0;bool ok=true;
 auto append=[&](const Line& l){if(!ok)return;if(used+l.n>sizeof(buffer)){ok=writeAll(file,buffer,used);used=0;}if(ok){memcpy(buffer+used,l.b,l.n);used+=l.n;}};
 Line intro;intro.add("miniGestures 2.0.11 diagnostic capture\r\nValues are hexadecimal, in the order named by each event. Time is milliseconds from capture start. No physical keystrokes, titles, URLs, or typed text.\r\nRecords: ");intro.dec(count);intro.add("; overwritten: ");intro.dec(overwritten);intro.add("; busy drops: ");intro.dec((unsigned)InterlockedExchangeAdd(&contention,0));intro.add("\r\n");append(intro);
 for(unsigned i=0;i<count&&ok;i++){auto&r=records[(head+capacity-count+i)%capacity];Line l;l.dec(r.tick);l.add(" ms tid=");l.dec(r.tid);l.add(" ");l.add(r.stage);l.add(" | ");for(int j=0;j<8;j++){if(j)l.add(" ");l.hex(r.v[j]);}l.add("\r\n");append(l);}
 if(ok&&used)ok=writeAll(file,buffer,used);if(ok)ok=FlushFileBuffers(file)!=0;CloseHandle(file);return ok;
}
}
#define MG_LOG(...) do{if(mgdiag::enabled())mgdiag::event(__VA_ARGS__);}while(0)
#else
namespace mgdiag { static inline bool enabled(){return false;} }
#define MG_LOG(...) ((void)0)
#endif
