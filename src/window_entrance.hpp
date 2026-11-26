#pragma once
// Same synchronized fade and slide as NightLight. Elapsed unsigned time
// permits GetTickCount wraparound; the timer never gates further clicks.
namespace mgentrance {
constexpr unsigned duration=150;
constexpr int slideDip=12,initialAlpha=24;
constexpr double progress(unsigned elapsed){
 if(elapsed>=duration)return 1.0;
 double remaining=1.0-(double)elapsed/duration;
 return 1.0-(1.0-initialAlpha/255.0)*remaining*remaining*remaining;
}
constexpr int alpha(double p){return (int)(255*p+0.5);}
constexpr int offset(int distance,double p){return (int)(distance*(1.0-p)+0.5);}
static_assert(alpha(progress(0))==initialAlpha);
static_assert(alpha(progress(duration))==255&&offset(12,progress(duration))==0);
static_assert(progress(50)<progress(100));
}
