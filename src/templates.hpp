#pragma once
#include "recognition.hpp"
// Original single-stroke shapes. Direction numbers correspond to the 8-point compass.
static constexpr P dirs[8]={{0,-100},{100,-100},{100,0},{100,100},{0,100},{-100,100},{-100,0},{-100,-100}};
static inline bool compass(int a,int b,P*out){if(a<0||a>=8||b>7)return false;P p[3]={{0,0},dirs[a],{0,0}};if(b>=0)p[2]={p[1].x+dirs[b].x,p[1].y+dirs[b].y};return normalize(p,b<0?2:3,out);}
struct Shape{const wchar_t*name;const char*points;};
// Coordinates encoded in a 10-by-10 grid, paired digits. These are original templates, not copied training data.
static constexpr Shape shapes[]={
{L"A","099059991569"},{L"B","09006081836068896909"},{L"C","905000090989"},{L"D","09006093896909"},{L"E","90000999055505"},{L"F","9000090555"},{L"G","9050000909899595"},{L"H","00090905559990"},{L"I","00904545900999"},{L"J","00909098090903"},{L"K","000904904499"},{L"L","000999"},{L"M","0900459099"},{L"N","09009990"},{L"O","50003100909959890900"},{L"P","09006081836005"},{L"Q","500031009099598909004699"},{L"R","0900608183600599"},{L"S","905000034695998909"},{L"T","00904549"},{L"U","00080919989990"},{L"V","004990"},{L"W","000939659990"},{L"X","0099459009"},{L"Y","0045904549"},{L"Z","00900999"},
{L"0","50003100909959890900"},{L"1","234049"},{L"2","00509093580999"},{L"3","00909345959909"},{L"4","7005954990"},{L"5","90000565959909"},{L"6","900004095989956505"},{L"7","009049"},{L"8","500031004695998909064150"},{L"9","909599659500009090"},
{L"○ ↻","5000301000091959899990806050"},{L"○ ↺","5000608090998959190900103050"},{L"□","000090990900"},{L"△","0905909909"},{L"✓","034999"},{L"?","005090934549"},{L"↯","00900999"},{L"↻","50003010000919598999908060506070"},{L"∞","054390959943050005"},{L"↕","490049"},{L"↔","059505"}};
constexpr int SHAPES=sizeof(shapes)/sizeof(shapes[0]);
static inline bool shape(int index,bool reverse,P*out){if(index<0||index>=SHAPES)return false;const char*s=shapes[index].points;P p[64];int n=0;while(s[0]&&s[1]&&n<64){p[n++]={(s[0]-'0')*20,(s[1]-'0')*20};s+=2;}if(reverse)for(int i=0;i<n/2;i++){P t=p[i];p[i]=p[n-1-i];p[n-1-i]=t;}return normalize(p,n,out);}
