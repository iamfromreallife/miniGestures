#pragma once
// Numeric versions only: the release checker already removes tag suffixes.
namespace mgupdate {
struct Version {int major,minor,patch;};
inline bool parse(const wchar_t* text,Version& out){
 if(!text)return false;
 int parts[3]{};
 for(int i=0;i<3;++i){
  int digits=0;
  while(*text>=L'0'&&*text<=L'9'){
   if(++digits>5)return false;
   parts[i]=parts[i]*10+(*text++-L'0');
  }
  if(!digits)return false;
  if(i<2){if(*text!=L'.')return false;++text;}
 }
 if(*text)return false;
 out={parts[0],parts[1],parts[2]};return true;
}
inline bool ignored(const wchar_t* text,int major,int minor,int patch){
 Version v{};
 return (major||minor||patch)&&parse(text,v)&&v.major==major&&v.minor==minor&&v.patch==patch;
}
}
