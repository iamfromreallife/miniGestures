#pragma once
namespace mgexport {
// ISO local calendar date, zero-padded and safe for Windows filenames.
template <unsigned Capacity> inline void filename(wchar_t (&out)[Capacity], unsigned year, unsigned month, unsigned day) {
    constexpr wchar_t pattern[]=L"miniGestures_0000-00-00.json";
    static_assert(Capacity>=sizeof(pattern)/sizeof(pattern[0]), "Export filename buffer too small");
    for(unsigned i=0;i<sizeof(pattern)/sizeof(pattern[0]);++i) out[i]=pattern[i];
    for(int i=16;i>=13;--i){out[i]=static_cast<wchar_t>(L'0'+year%10);year/=10;}
    out[18]=static_cast<wchar_t>(L'0'+month/10);out[19]=static_cast<wchar_t>(L'0'+month%10);
    out[21]=static_cast<wchar_t>(L'0'+day/10);out[22]=static_cast<wchar_t>(L'0'+day%10);
}
}
