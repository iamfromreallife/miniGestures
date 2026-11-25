#pragma once
#include "model.hpp"
// Editor thread only. The key includes every input to the thumbnail rasterizer;
// row position is deliberately absent because the bitmap is drawn at its origin.
struct PreviewKey {
 const P* points;int w,h,arrow,thickness;DWORD color,bg;
};
struct PreviewCache {
 static constexpr int capacity=64,maxPixels=4096;
 struct Entry {P points[N];int w,h,arrow,thickness;DWORD color,bg;unsigned long long age;u32* pixels;};
 Entry entries[capacity]{};unsigned long long clock=0;
 static bool cacheable(const PreviewKey& k){return k.w>0&&k.h>0&&(up)k.w*k.h<=maxPixels;}
 const u32* find(const PreviewKey& k){if(!cacheable(k))return nullptr;for(auto& e:entries)if(e.pixels&&e.w==k.w&&e.h==k.h&&e.arrow==k.arrow&&e.thickness==k.thickness&&e.color==k.color&&e.bg==k.bg&&sameShape(e.points,k.points)){e.age=++clock;return e.pixels;}return nullptr;}
 // Ownership transferred only on success. Retain at most 1 MiB of pixels.
 bool remember(const PreviewKey& k,u32* pixels){if(!pixels||!cacheable(k))return false;Entry* victim=&entries[0];for(auto& e:entries){if(!e.pixels){victim=&e;break;}if(e.age<victim->age)victim=&e;}freep(victim->pixels);memcpy(victim->points,k.points,sizeof(victim->points));victim->w=k.w;victim->h=k.h;victim->arrow=k.arrow;victim->thickness=k.thickness;victim->color=k.color;victim->bg=k.bg;victim->age=++clock;victim->pixels=pixels;return true;}
 void clear(){for(auto& e:entries){freep(e.pixels);e.pixels=nullptr;e.age=0;}clock=0;}
};
