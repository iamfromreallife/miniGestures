#pragma once
#include "model.hpp"
// Editor thread only. The key includes every input to the thumbnail rasterizer;
// row position is deliberately absent because the bitmap is drawn at its origin.
struct PreviewKey {
 const P* points;int w,h,arrow,thickness;DWORD color,bg;
};
struct PreviewCache {
 static constexpr int capacity=64,maxPixels=65536;static constexpr up budgetPixels=262144;
 struct Entry {P points[N];int w,h,arrow,thickness;DWORD color,bg;unsigned long long age;u32* pixels;};
 Entry entries[capacity]{};unsigned long long clock=0;up usedPixels=0;
 void drop(Entry& e){if(e.pixels){usedPixels-=(up)e.w*e.h;freep(e.pixels);e.pixels=nullptr;}e.age=0;}
 static bool cacheable(const PreviewKey& k){return k.w>0&&k.h>0&&(up)k.w*k.h<=maxPixels;}
 const u32* find(const PreviewKey& k){if(!cacheable(k))return nullptr;for(auto& e:entries)if(e.pixels&&e.w==k.w&&e.h==k.h&&e.arrow==k.arrow&&e.thickness==k.thickness&&e.color==k.color&&e.bg==k.bg&&sameShape(e.points,k.points)){e.age=++clock;return e.pixels;}return nullptr;}
 // Ownership transferred only on success. Retain at most 1 MiB of pixels.
 bool remember(const PreviewKey& k,u32* pixels){if(!pixels||!cacheable(k))return false;up n=(up)k.w*k.h;while(usedPixels+n>budgetPixels){Entry* oldest=nullptr;for(auto& e:entries)if(e.pixels&&(!oldest||e.age<oldest->age))oldest=&e;if(!oldest)return false;drop(*oldest);}Entry* victim=&entries[0];for(auto& e:entries){if(!e.pixels){victim=&e;break;}if(e.age<victim->age)victim=&e;}drop(*victim);usedPixels+=n;memcpy(victim->points,k.points,sizeof(victim->points));victim->w=k.w;victim->h=k.h;victim->arrow=k.arrow;victim->thickness=k.thickness;victim->color=k.color;victim->bg=k.bg;victim->age=++clock;victim->pixels=pixels;return true;}
 void clear(){for(auto& e:entries)drop(e);clock=0;usedPixels=0;}
};
