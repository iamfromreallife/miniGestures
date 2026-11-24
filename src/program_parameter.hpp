#pragma once
// Keep the existing executable|arguments storage format. Do not truncate.
namespace program_parameter {
constexpr int capacity=256;
inline bool space(wchar_t c){return c==L' '||c==L'\t'||c==L'\r'||c==L'\n';}
inline int length(const wchar_t*s){int n=0;while(s[n])++n;return n;}
inline void split(const wchar_t*src,wchar_t*path,wchar_t*args){int n=0;while(*src&&*src!=L'|'&&n<capacity-1)path[n++]=*src++;path[n]=0;if(*src==L'|')++src;n=0;while(*src&&n<capacity-1)args[n++]=*src++;args[n]=0;}
inline bool combine(const wchar_t*path,const wchar_t*args,wchar_t*out){while(space(*path))++path;int n=length(path);while(n&&space(path[n-1]))--n;if(n>=2&&path[0]==L'"'&&path[n-1]==L'"'){++path;n-=2;}if(!n)return false;for(int i=0;i<n;++i)if(path[i]==L'|'||path[i]==L'"'||path[i]==L'\n'||path[i]==L'\r')return false;int a=length(args);if(n+(a?1+a:0)>=capacity)return false;int k=0;for(int i=0;i<n;++i)out[k++]=path[i];if(a){out[k++]=L'|';for(int i=0;i<a;++i)out[k++]=args[i];}out[k]=0;return true;}
inline bool isProgram(int type){return type==22||type==23;}
}
