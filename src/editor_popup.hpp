// Modal native Win32 editors: no web view, no extra runtime.
static int popupVk,popupMods;static bool popupArmed,popupActionKeys;static int popupLocalModifier;static HWND popInput,popList,popOk,popCancel,popClear,popConfirm;static HWND actionColumns[3]{};static int actionWidths[3]{},actionRows[3]{},actionVisibleColumns=3;static bool fillingActions=false,actionSizeChanged=false;static const W* confirmText;static bool shapeConflict=true,previewShapeConflict=false;static int popMode,popupAction=-1,conflictChoice=0;static bool popDone,popAccepted;static W popupText[96];static RECT popField,popBody,popArgsField;static HWND popArgs=nullptr,popProgramPick=nullptr;static PROC programPickOld=nullptr;static bool programPicking=false;static W programResult[256];static const int POP_INPUT=130,POP_LIST=131,POP_OK=132,POP_CANCEL=133,POP_CLEAR=134,POP_CONFIRM=135,POP_LIST2=136,POP_LIST3=137;
static HWND popControl(HWND parent,int id,const W*cls,const W*text,DWORD style,int x,int y,int w,int h){HWND v=CreateWindowExW(0,cls,text,0x50000000|style,px(x),px(y),px(w),px(h),parent,PTR(id),inst,nullptr);SendMessageW(v,0x30,(WP)font,1);if(id==POP_INPUT||id==POP_LIST||id==POP_LIST2||id==POP_LIST3||id==POP_CONFIRM)themeNative(v);return v;}
// Drag a crosshair onto another application to fill the executable field.
static void programPickCursor(int id){auto set=(H(CB*)(H))GetProcAddress(GetModuleHandleW(L"user32.dll"),"SetCursor");if(set)set(LoadCursorW(nullptr,(const W*)PTR(id)));}
static void cancelProgramPick(){if(!programPicking)return;programPicking=false;ReleaseCapture();programPickCursor(32512);}
static LR CB programPickProc(HWND h,UINT m,WP w,LP l){
 if(m==0x201){programPicking=true;SetFocus(h);SetCapture(h);programPickCursor(32515);return 0;}
 if(programPicking&&(m==0x200||m==0x20)){programPickCursor(32515);return m==0x20?1:0;}
 if(m==0x202&&programPicking){POINT p{};bool ok=GetCursorPos(&p);cancelProgramPick();if(!ok)return 0;Target t=target(WindowFromPoint(p));if(!t.hwnd||t.pid==GetCurrentProcessId())return 0;
  if(!*t.exe||len(t.exe)>=program_parameter::capacity){static const W* msg[]={L"The executable path is unavailable or too long.",L"O caminho do executável está indisponível ou é demasiado longo.",L"La ruta del ejecutable no está disponible o es demasiado larga.",L"Le chemin de l’exécutable est indisponible ou trop long.",L"Der Programmpfad ist nicht verfügbar oder zu lang.",L"Il percorso dell’eseguibile non è disponibile o è troppo lungo."};MessageBoxW(popup,msg[cfg.lang],L"miniGestures",0x30);return 0;}
  SetWindowTextW(popInput,t.exe);SetFocus(popInput);return 0;}
 if(m==0x215){programPicking=false;programPickCursor(32512);}else if(m==0x1f||m==0x2)cancelProgramPick();return CallWindowProcW(programPickOld,h,m,w,l);
}
static bool sortBefore(const W*a,const W*b){while(*a&&*b&&fold(*a)==fold(*b)){a++;b++;}return fold(*a)<fold(*b);}
static void popRow(const W*s,int data){int i=(int)SendMessageW(popList,0x180,0,(LP)s);if(i>=0)SendMessageW(popList,0x19a,i,data);}
static int popSelected(){int i=(int)SendMessageW(popList,0x188,0,0);return i<0?-1:(int)SendMessageW(popList,0x199,i,0);}
// Compact, independently measured columns. Native listboxes own scrolling.
static constexpr int actionRowHeight=21,actionMargin=12,actionGap=4;
static constexpr int actionMinWidth=500,actionMinHeight=250;
static constexpr DWORD actionWindowStyle=0x00c80000|0x02000000|0x00040000;
struct ActionLayoutPlan{
 int matches[9][ACTIONS]{},counts[9]{},groups[9]{},ng=0;
 int cuts[2]{},widths[3]{},rows[3]{},columns=1;
};
static ActionLayoutPlan planActions(const W* query,HDC dc){
 ActionLayoutPlan plan{};
 constexpr int order[]={0,1,3,4,5,6,7,8,2};
 int groupWidths[9]{};
 H old=dc?SelectObject(dc,font):nullptr;
 auto measure=[&](const W* label,int pad){
  POINT z{};if(dc)GetTextExtentPoint32W(dc,label,len(label),&z);
  else z.x=px(len(label)*7);
  return z.x+px(pad);
 };
 for(int c:order){
  int n=0;
  for(int a=0;a<ACTIONS;a++)if(actionGroup(a)==c&&
    (contains(defs[a].name[cfg.lang],query)||contains(et(WINDOW_CAT+c),query)||
     (a==0&&contains(L"Send shortcut hotkey",query))||
     (a==TASK_VIEW_ACTION&&contains(L"Taskview Win+Tab",query))))plan.matches[c][n++]=a;
  if(!n)continue;
  plan.counts[c]=n;plan.groups[plan.ng++]=c;
  groupWidths[c]=measure(et(WINDOW_CAT+c),32);
  for(int i=0;i<n;++i){int a=plan.matches[c][i];groupWidths[c]=maxn(groupWidths[c],measure(defs[a].name[cfg.lang],elevationAction(a)?38:19));}
  for(int i=1;i<n;i++)for(int j=i;j>0&&sortBefore(defs[plan.matches[c][j]].name[cfg.lang],defs[plan.matches[c][j-1]].name[cfg.lang]);j--){
   int tmp=plan.matches[c][j];plan.matches[c][j]=plan.matches[c][j-1];plan.matches[c][j-1]=tmp;
  }
 }
 if(dc&&old)SelectObject(dc,old);
 int prefix[10]{};for(int i=0;i<plan.ng;i++)prefix[i+1]=prefix[i]+plan.counts[plan.groups[i]]+1;
 int cut1=plan.ng,cut2=plan.ng,best=0x7fffffff;
 if(plan.ng>=3){for(int a=1;a<plan.ng-1;a++)for(int b=a+1;b<plan.ng;b++){
  int x=prefix[a],y=prefix[b]-prefix[a],z=prefix[plan.ng]-prefix[b];
  int score=maxn(x,maxn(y,z))*100+absn(x-y)+absn(y-z);
  if(score<best){best=score;cut1=a;cut2=b;}
 }}else if(plan.ng==2){cut1=1;cut2=2;}
 plan.cuts[0]=cut1;plan.cuts[1]=cut2;plan.columns=clamp(plan.ng,1,3);
 for(int i=0;i<plan.ng;i++){
  int col=i<cut1?0:i<cut2?1:2,c=plan.groups[i];
  plan.rows[col]+=plan.counts[c]+1;
  plan.widths[col]=maxn(plan.widths[col],groupWidths[c]);
 }
 for(int i=0;i<plan.columns;i++)plan.widths[i]=maxn(px(105),plan.widths[i]);
 return plan;
}
static POINT actionOuterSize(int clientW,int clientH){
 RECT r{0,0,clientW,clientH};
 using Adjust=B(*)(RECT*,DWORD,B,DWORD,UINT);
 auto adjust=(Adjust)GetProcAddress(GetModuleHandleW(L"user32.dll"),"AdjustWindowRectExForDpi");
 if(adjust&&adjust(&r,actionWindowStyle,0,1,dpi))return {r.right-r.left,r.bottom-r.top};
 return {clientW+px(16),clientH+px(40)};
}
API B RedrawWindow(HWND,const RECT*,H,UINT);
static void redrawActionWindow(HWND h,bool immediate=false){
 // Invalidate all children after their geometry is final. Native controls
 // retain responsibility for their erase, scrollbars and nonclient paint.
 RedrawWindow(h,nullptr,nullptr,0x0001|0x0004|0x0080|(immediate?0x0100:0));
}
static void layoutActionWindow(HWND h){
 if(!popInput||!popOk||!popCancel||!actionColumns[0]||!actionColumns[1]||!actionColumns[2])return;
 RECT r{};if(!GetClientRect(h,&r)||r.right<=0||r.bottom<=0)return;
 const int margin=px(actionMargin),gap=px(actionGap),top=px(56),bottom=r.bottom-px(48);
 popField={margin,px(12),r.right-margin,px(44)};
 popBody={margin,top-2,r.right-margin,bottom+2};
 // Batch every child position, including the search field and footer.
 // NOREDRAW + NOCOPYBITS prevents stale pixels from being stretched or
 // copied from an old column into the new one during a resize.
 struct Move{HWND h;int x,y,w,hgt;UINT flags;};
 Move moves[6]{};int nm=0;
 constexpr UINT baseFlags=4|16|8|0x100;
 auto move=[&](HWND child,int x,int y,int width,int height,UINT extra=0){
  moves[nm++]={child,x,y,width,height,baseFlags|extra};
 };
 auto band=mgtext::editBand(popField.bottom-popField.top,editTextHeight,maxn(1,px(2)));
 move(popInput,popField.left+px(8),popField.top+band.top,maxn(1,popField.right-popField.left-px(16)),band.height);
 const int listH=maxn(1,bottom-top);
 int needs[3]{},total=0;
 using Metric=int(CB*)(int,UINT);
 auto metric=(Metric)GetProcAddress(GetModuleHandleW(L"user32.dll"),"GetSystemMetricsForDpi");
 int scrollbar=metric?metric(2,dpi):GetSystemMetrics(2);
 for(int i=0;i<actionVisibleColumns;i++){
  needs[i]=maxn(px(105),actionWidths[i])+2;
  if(actionRows[i]*px(actionRowHeight)>listH)needs[i]+=scrollbar;
  total+=needs[i];
 }
 int available=maxn(actionVisibleColumns,r.right-2*margin-4-gap*(actionVisibleColumns-1));
 int left=margin+2,remaining=available,remainingNeed=total;
 for(int i=0;i<3;i++){
  HWND col=actionColumns[i];
  if(i>=actionVisibleColumns){move(col,0,0,0,0,0x80|1|2);continue;}
  int width=i==actionVisibleColumns-1?remaining:
    clamp((int)((long long)remaining*needs[i]/maxn(1,remainingNeed)),1,maxn(1,remaining-(actionVisibleColumns-i-1)));
  move(col,left,top,width,listH,0x40);
  left+=width+gap;remaining-=width;remainingNeed-=needs[i];
 }
 move(popCancel,r.right-margin-px(198),r.bottom-px(36),px(94),px(26));
 move(popOk,r.right-margin-px(98),r.bottom-px(36),px(98),px(26));
 H batch=BeginDeferWindowPos(nm);
 for(int i=0;i<nm&&batch;i++){auto& v=moves[i];batch=DeferWindowPos(batch,v.h,nullptr,v.x,v.y,v.w,v.hgt,v.flags);}
 if(!batch||!EndDeferWindowPos(batch))for(int i=0;i<nm;i++){auto& v=moves[i];SetWindowPos(v.h,nullptr,v.x,v.y,v.w,v.hgt,v.flags);}
 redrawActionWindow(h);
}
static void filterActions(){
 W q[128]{};GetWindowTextW(popInput,q,128);
 HDC dc=GetDC(popup);auto plan=planActions(q,dc);if(dc)ReleaseDC(popup,dc);
 actionVisibleColumns=plan.columns;
 for(int i=0;i<3;i++){actionWidths[i]=plan.widths[i];actionRows[i]=plan.rows[i];}
 fillingActions=true;
 for(int i=0;i<3;i++){SendMessageW(actionColumns[i],0x0b,0,0);SendMessageW(actionColumns[i],0x184,0,0);}
 int firstCol=-1;
 for(int i=0;i<plan.ng;i++){
  int col=i<plan.cuts[0]?0:i<plan.cuts[1]?1:2,c=plan.groups[i];popList=actionColumns[col];
  popRow(et(WINDOW_CAT+c),-2-c);for(int j=0;j<plan.counts[c];j++)popRow(defs[plan.matches[c][j]].name[cfg.lang],plan.matches[c][j]);
  if(firstCol<0)firstCol=col;
 }
 popList=actionColumns[firstCol<0?0:firstCol];
 if(firstCol>=0)SendMessageW(popList,0x186,1,0);
 for(int i=0;i<3;i++){SendMessageW(actionColumns[i],0x194,actionWidths[i],0);SendMessageW(actionColumns[i],0x0b,1,0);InvalidateRect(actionColumns[i],nullptr,1);}
 fillingActions=false;layoutActionWindow(popup);
}
static const W* actionKeyPrompt(bool help){static const W* waits[]={L"Press a key or key combination…",L"Prima uma tecla ou combinação…",L"Pulsa una tecla o combinación…",L"Appuyez sur une touche ou combinaison…",L"Taste oder Tastenkombination drücken…",L"Premi un tasto o una combinazione…"};static const W* hints[]={L"Press a single key or a combination. Click OK to save, Record to retry, or Cancel to exit.",L"Prima uma tecla ou combinação. Clique em OK para guardar, Gravar para repetir ou Cancelar para sair.",L"Pulsa una tecla o combinación. Pulsa Aceptar para guardar, Grabar para repetir o Cancelar para salir.",L"Appuyez sur une touche ou combinaison. OK pour enregistrer, Enregistrer pour réessayer ou Annuler pour quitter.",L"Drücken Sie eine Taste oder Kombination. OK zum Speichern, Aufzeichnen zum Wiederholen oder Abbrechen zum Beenden.",L"Premi un tasto o una combinazione. OK per salvare, Registra per riprovare o Annulla per uscire."};return (help?hints:waits)[cfg.lang];}
static void armPopupHotkey(bool arm){popupArmed=arm;InterlockedExchange(&recording,arm?(popupActionKeys?2:1):0);if(hooktid&&hookready&&WaitForSingleObject(hookready,0)==0)PostThreadMessageW(hooktid,MSG_REFRESH_HOOKS,0,0);if(arm){popupLocalModifier=0;popupVk=popupMods=0;SetWindowTextW(popInput,(popupActionKeys?actionKeyPrompt(false):et(HOTKEY_WAIT_E)));EnableWindow(popOk,0);SetFocus(popInput);}}
static void capturePopupHotkey(int vk,int m){if(!popupArmed||screenshotShortcut(vk,m)||(!popupActionKeys&&modifierKey(vk))||vk<=0||vk>255)return;if(!popupActionKeys&&vk==0x1b&&m==0){popDone=true;return;}popupVk=vk;popupMods=modifierKey(vk)?0:m&15;W text[128];keyname(vk,popupMods,text,128);SetWindowTextW(popInput,text);armPopupHotkey(false);EnableWindow(popOk,1);InvalidateRect(popup,nullptr,0);}
static void acceptPopup(){if(popMode==8){W path[256]{},args[256]{};GetWindowTextW(popInput,path,256);GetWindowTextW(popArgs,args,256);if(SendMessageW(popInput,0xe,0,0)>255||SendMessageW(popArgs,0xe,0,0)>255||!program_parameter::combine(path,args,programResult)){MessageBoxW(popup,et(PROGRAM_INVALID_E),L"miniGestures",0x30);SetFocus(popInput);return;}popAccepted=popDone=true;return;}if(popMode==6||popMode==7){conflictChoice=1;popAccepted=true;popDone=true;return;}if(popMode==5){if(!popupVk)return;popAccepted=true;popDone=true;return;}if(popMode==4){if(popConfirm&&SendMessageW(popConfirm,0xf0,0,0)==1)cfg.skipDeleteConfirm=1;popAccepted=true;popDone=true;return;}if(popMode==1){P sample[N];if(!normalize(drawn,drawnN,sample)){MessageBoxW(popup,et(DRAW_HELP),L"miniGestures",0x30);return;}}else if(popMode==2){int a=popSelected();if(a<0)return;popupAction=a;}else {GetWindowTextW(popInput,popupText,96);if(!*popupText)return;}popAccepted=true;popDone=true;}
static void categoryIcon(HDC dc,int group,int x,int y){
 auto line=[&](int a,int b,int c,int d){POINT p[]={{x+px(a),y+px(b)},{x+px(c),y+px(d)}};flatLine(dc,p,2,muted,px(1));};auto outline=[&](int a,int b,int c,int d){line(a,b,c,b);line(c,b,c,d);line(c,d,a,d);line(a,d,a,b);};
 switch(group){case 0:outline(1,2,13,12);line(1,5,13,5);break;case 1:outline(1,1,13,10);line(7,10,7,13);line(4,13,10,13);break;case 2:line(2,7,11,7);line(7,3,11,7);line(11,7,7,11);line(2,2,2,12);break;case 3:outline(0,3,14,11);for(int i=3;i<13;i+=3){line(i,5,i+1,5);line(i,7,i+1,7);}line(4,9,10,9);break;case 4:rounded(dc,{x+px(3),y+px(1),x+px(11),y+px(14)},px(4),categoryFill,muted,true,categoryFill);line(7,2,7,6);break;case 5:outline(1,2,13,12);line(1,5,13,5);line(3,3,4,3);line(6,3,7,3);break;case 6:line(5,4,9,1);line(9,1,9,13);line(9,13,5,10);line(5,10,2,10);line(2,10,2,4);line(2,4,5,4);line(11,4,13,7);line(13,7,11,10);break;case 7:outline(3,3,11,11);line(7,0,7,3);line(7,11,7,14);line(0,7,3,7);line(11,7,14,7);break;case 8:outline(2,3,12,13);outline(5,1,9,4);line(4,7,10,7);line(4,10,10,10);break;default:line(8,0,3,7);line(3,7,8,7);line(8,7,6,14);line(6,14,12,5);line(12,5,7,5);line(7,5,8,0);break;}
}
// Owner-drawn content only: buffer a complete row, including the full
// current client width rather than the listbox's old horizontal extent.
static void paintActionRowCore(DRAWITEM* d){
 RECT r=d->rect;FillRect(d->dc,&r,fieldBrush);if(d->item==(UINT)-1)return;W b[512]{};if(SendMessageW(d->hwnd,0x18a,d->item,0)<512)SendMessageW(d->hwnd,0x189,d->item,(LP)b);int data=(int)d->data;bool header=data<0;bool sel=!header&&(d->state&1);RECT t=r;t.left+=px(header?23:9);t.right-=px(6);if(header){RECT q=r;q.left+=px(3);q.right-=px(3);rounded(d->dc,q,px(3),categoryFill,frame,false,fieldFill);categoryIcon(d->dc,-2-data,r.left+px(6),(r.top+r.bottom-px(14))/2);}else if(sel){RECT q=r;q.left+=px(2);q.right-=px(2);q.top++;q.bottom--;DWORD selectionFill=rowSelectionFill(highContrastTheme);rounded(d->dc,q,px(4),selectionFill,selectionFill,false,fieldFill);}if(!header&&elevationAction(data)){elevationIcon(d->dc,t.left,(r.top+r.bottom-px(14))/2,sel?rowSelectionText(highContrastTheme):ink);t.left+=px(19);}textFlat(d->dc,b,t,header?muted:sel?rowSelectionText(highContrastTheme):ink);
}
static void paintActionRow(DRAWITEM* d){
 DRAWITEM row=*d;
 if(popMode==2&&row.hwnd){RECT client{};if(GetClientRect(row.hwnd,&client))row.rect.right=maxn(row.rect.right,client.right);}
 int w=row.rect.right-row.rect.left,h=row.rect.bottom-row.rect.top;
 if(w<=0||h<=0)return;
 bool cached=!buttonBuffer.busy&&(up)w*h<=131072&&buttonBuffer.ensure(row.dc,w,h);
 HDC mem=cached?buttonBuffer.dc:CreateCompatibleDC(row.dc);
 H bmp=cached?buttonBuffer.bmp:mem?CreateCompatibleBitmap(row.dc,w,h):nullptr;
 H old=bmp?SelectObject(mem,bmp):nullptr;
 if(!old||old==PTR(-1)){if(cached)buttonBuffer.clear();else{if(bmp)DeleteObject(bmp);if(mem)DeleteDC(mem);}paintActionRowCore(&row);return;}
 if(cached)buttonBuffer.busy=true;
 DRAWITEM copy=row;copy.dc=mem;copy.rect={0,0,w,h};paintActionRowCore(&copy);
 BitBlt(row.dc,row.rect.left,row.rect.top,w,h,mem,0,0,0xcc0020);
 SelectObject(mem,old);
 if(cached)buttonBuffer.busy=false;else{DeleteObject(bmp);DeleteDC(mem);}
}
static LR CB editorProc(HWND h,UINT m,WP w,LP l){LR uiState;if(nativeFocusState(h,m,w,l,uiState))return uiState;if(m==0x10){popDone=true;return 0;}if(popMode==2&&m==0x24&&l){auto* mm=(MINMAX*)l;mm->minTrack=actionOuterSize(px(actionMinWidth),px(actionMinHeight));return 0;}if(popMode==2&&m==0x5&&w!=1){if(popOk&&IsWindowVisible(h))actionSizeChanged=true;layoutActionWindow(h);return 0;}if(popMode==2&&m==0x232){redrawActionWindow(h,true);return 0;}if(m==0x14)return 1;if(m==0xf){PAINT p{};HDC dc=BeginPaint(h,&p);RECT r{};GetClientRect(h,&r);FillRect(dc,&r,white);if(popMode!=1&&popMode!=4&&popMode!=6&&popMode!=7)rounded(dc,popField,px(5),fieldFill,frame);if(popMode==1||popMode==2)rounded(dc,popBody,px(5),fieldFill,frame);if(popMode==8){rounded(dc,popArgsField,px(5),fieldFill,frame);RECT a{px(16),px(12),r.right-px(16),px(32)},b{px(16),px(79),r.right-px(16),px(99)};textFlat(dc,et(PROGRAM_PATH_E),a,ink);textFlat(dc,et(PROGRAM_ARGS_E),b,ink);}if(popMode==5){RECT label{px(16),px(12),r.right-px(16),px(58)};textFlat(dc,(popupActionKeys?actionKeyPrompt(true):et(HOTKEY_HELP_E)),label,ink,0x10|0x800);}if(popMode==4||popMode==6||popMode==7){RECT label{px(16),px(16),r.right-px(16),r.bottom-px(popMode==4?78:54)};textFlat(dc,confirmText,label,ink,0x10|0x800);}if(popMode==2){static const W*legend[]={L"May require administrator rights",L"Pode exigir direitos de administrador",L"Puede requerir permisos de administrador",L"Peut nécessiter des droits administrateur",L"Kann Administratorrechte erfordern",L"Può richiedere diritti di amministratore"};int y=r.bottom-px(36);elevationIcon(dc,px(14),y+px(6),muted);RECT note{px(33),y,r.right-px(216),y+px(26)};textFlat(dc,legend[cfg.lang],note,muted);}if(popMode==1){RECT label{px(16),px(12),r.right-px(16),px(42)};textFlat(dc,et(DRAW_HELP),label,muted,0x10|0x800);}EndPaint(h,&p);return 0;}if(m==0x133||m==0x134||m==0x138||m==0x135||m==0x136){return (LR)controlColor((HDC)w,m,(HWND)l);}if(m==0x2c){auto*d=(MEASUREITEM*)l;d->height=px(popMode==2?actionRowHeight:23);return 1;}if(m==0x2b){auto*d=(DRAWITEM*)l;if(d->type==4){paintButton(d);return 1;}if(d->type==2){paintActionRow(d);return 1;}}if(m==0x111){int id=LO(w),code=HI(w);if(id==POP_INPUT&&code==0x300&&popMode==2&&popList){filterActions();return 0;}if(popMode==2&&(id==POP_LIST||id==POP_LIST2||id==POP_LIST3)){if(fillingActions)return 0;HWND active=(HWND)l;if(active&&(code==1||code==2)){popList=active;fillingActions=true;for(auto col:actionColumns)if(col!=active)SendMessageW(col,0x186,(WP)-1,0);fillingActions=false;if(code==2)acceptPopup();}return 0;}if(id==POP_OK){acceptPopup();return 0;}if(id==POP_CANCEL){popDone=true;return 0;}if(id==POP_CLEAR&&popMode==8){W path[1024]{};if(filechoose(path,false,true,popup)){if(len(path)>=256){MessageBoxW(popup,et(PROGRAM_INVALID_E),L"miniGestures",0x30);return 0;}SetWindowTextW(popInput,path);SetFocus(popInput);}return 0;}if(id==POP_CLEAR&&popMode==6){conflictChoice=2;popDone=true;popAccepted=true;return 0;}if(id==POP_CLEAR&&popMode==5){armPopupHotkey(true);return 0;}if(id==POP_CLEAR){drawnN=0;InvalidateRect(drawCanvas,nullptr,0);return 0;}}return DefWindowProcW(h,m,w,l);}
static void compactMessageSize(int mode,int&ww,int&hh,const MONITOR&mon,bool hasMon){
 int minWidth=mode==6?560:mode==4?440:340,maxWidth=mode==6?660:mode==4?560:540;
 if(hasMon){maxWidth=minn(maxWidth,(mon.rcWork.right-mon.rcWork.left)*96/dpi);minWidth=minn(minWidth,maxWidth);}
 HDC dc=GetDC(mainwnd);if(!dc){ww=maxWidth;hh=mode==4?166:146;return;}
 H old=SelectObject(dc,font);int widest=0;const W*start=confirmText?confirmText:L"";for(const W*p=start;;p++){if(*p==L'\n'||!*p){POINT z{};GetTextExtentPoint32W(dc,start,(int)(p-start),&z);widest=maxn(widest,z.x);if(!*p)break;start=p+1;}}
 ww=clamp((widest*96+dpi-1)/dpi+54,minWidth,maxWidth);
 RECT measure{0,0,maxn(px(80),px(ww-48)),0};DrawTextW(dc,confirmText?confirmText:L"",-1,&measure,0x400|0x10|0x800);
 SelectObject(dc,old);ReleaseDC(mainwnd,dc);
 int textHeight=(measure.bottom-measure.top)*96/dpi+4;
 int clientHeight=maxn(mode==4?110:84,textHeight+16+(mode==4?78:54));
 using Adjust=B(*)(RECT*,DWORD,B,DWORD,UINT);auto adjust=(Adjust)GetProcAddress(GetModuleHandleW(L"user32.dll"),"AdjustWindowRectExForDpi");
 RECT outer{0,0,px(ww-16),px(clientHeight)};
 hh=adjust&&adjust(&outer,0x00c80000|0x02000000,0,0x1,dpi)?(outer.bottom-outer.top)*96/dpi+2:clientHeight+42;
 if(hasMon)hh=minn(hh,(mon.rcWork.bottom-mon.rcWork.top)*96/dpi);
}
static bool showEditor(int mode,const W*title,const W*initial,bool actionKeys){if(popup)return false;popupActionKeys=actionKeys;popMode=mode;actionSizeChanged=false;popDone=popAccepted=false;conflictChoice=0;popupAction=-1;popupText[0]=0;popArgs=popProgramPick=nullptr;programPicking=false;popInput=popList=popOk=popCancel=popClear=popConfirm=drawCanvas=nullptr;drawnN=0;drawdrag=0;RECT owner{};GetWindowRect(mainwnd,&owner);int ww=(mode==6||mode==7)?660:mode==5?520:mode==4?510:mode==3?440:620,hh=mode==8?220:(mode==6||mode==7)?285:mode==5?204:mode==4?192:mode==3?168:mode==1?480:600;MONITOR mon{sizeof(mon)};bool hasMon=GetMonitorInfoW(MonitorFromWindow(mainwnd,2),&mon);if(mode==4||mode==6||mode==7)compactMessageSize(mode,ww,hh,mon,hasMon);if(mode==2){
 // Select the monitor from the saved desktop position, rather than the
 // owner window. The nearest monitor is used if a display was removed.
 if(cfg.actionWindowPositionSaved){
  POINT saved{cfg.actionWindowX+px(cfg.actionWindowWidth)/2,cfg.actionWindowY+px(cfg.actionWindowHeight)/2};
  MONITOR targetMon{sizeof(targetMon)};
  if(GetMonitorInfoW(MonitorFromPoint(saved,2),&targetMon)){mon=targetMon;hasMon=true;}
 }
 HDC dc=GetDC(mainwnd);auto plan=planActions(L"",dc);if(dc)ReleaseDC(mainwnd,dc);
 int naturalW=px(2*actionMargin+2*actionGap)+4,rows=0;
 for(int i=0;i<3;i++){naturalW+=plan.widths[i]+2;rows=maxn(rows,plan.rows[i]);}
 auto outer=actionOuterSize(maxn(px(actionMinWidth),naturalW),maxn(px(actionMinHeight),rows*px(actionRowHeight)+px(104)+4));
 auto minimum=actionOuterSize(px(actionMinWidth),px(actionMinHeight));
 ww=cfg.actionWindowWidth>0?maxn(cfg.actionWindowWidth,(minimum.x*96+dpi-1)/dpi):(outer.x*96+dpi-1)/dpi;
 hh=cfg.actionWindowHeight>0?maxn(cfg.actionWindowHeight,(minimum.y*96+dpi-1)/dpi):(outer.y*96+dpi-1)/dpi;
 if(hasMon){ww=minn(ww,(mon.rcWork.right-mon.rcWork.left)*96/dpi);hh=minn(hh,(mon.rcWork.bottom-mon.rcWork.top)*96/dpi);}
 }if(mode!=2)hh+=mode==8?8:4;if(hasMon)hh=minn(hh,(mon.rcWork.bottom-mon.rcWork.top)*96/dpi);int xx=owner.left+(owner.right-owner.left-px(ww))/2,yy=owner.top+(owner.bottom-owner.top-px(hh))/2;if(mode==2&&cfg.actionWindowPositionSaved){xx=cfg.actionWindowX;yy=cfg.actionWindowY;}if(hasMon){xx=clamp(xx,mon.rcWork.left,maxn(mon.rcWork.left,mon.rcWork.right-px(ww)));yy=clamp(yy,mon.rcWork.top,maxn(mon.rcWork.top,mon.rcWork.bottom-px(hh)));}popup=CreateWindowExW(0x1,L"miniGesturesEditor",title,mode==2?actionWindowStyle:0x00c80000|0x02000000,xx,yy,px(ww),px(hh),mainwnd,nullptr,inst,nullptr);if(!popup)return false;if(mode==1)InterlockedExchange(&shapeRecording,1);themeCaption(popup);themeNative(popup);RECT r{};GetClientRect(popup,&r);int w=r.right*96/dpi,h=r.bottom*96/dpi;popField={px(16),px(16),px(w-16),px(50)};popBody={px(16),px(mode==1?48:62),px(w-16),px(h-56)};if(mode==8){W path[256]{},args[256]{};program_parameter::split(initial,path,args);int bw=captionWidth(BROWSEBTN)+12;bw=maxn(90,bw);const W* pickLabels[]={L"⊕ Drag",L"⊕ Arrastar",L"⊕ Arrastrar",L"⊕ Glisser",L"⊕ Ziehen",L"⊕ Trascina"};int pw=100;popField={px(16),px(36),px(w-28-bw-pw),px(64)};popArgsField={px(16),px(105),px(w-16),px(133)};int eh=maxn(16,editLineHeight*96/dpi);popInput=popControl(popup,POP_INPUT,L"EDIT",path,0x10000|0x80,22,36+(28-eh)/2,w-50-bw-pw,eh);popArgs=popControl(popup,138,L"EDIT",args,0x10000|0x80,22,105+(28-eh)/2,w-44,eh);themeNative(popArgs);SendMessageW(popInput,0xc5,32767,0);SendMessageW(popArgs,0xc5,32767,0);popClear=popControl(popup,POP_CLEAR,L"BUTTON",tr(BROWSE),0x10000|0xb,w-22-bw-pw,36,bw,28);popProgramPick=popControl(popup,139,L"BUTTON",pickLabels[cfg.lang],0x10000|0xb,w-16-pw,36,pw,28);programPickOld=(PROC)SetWindowLongPtrW(popProgramPick,-4,(ip)programPickProc);}else if(mode==5){popupVk=popupMods=0;popupArmed=false;popField={px(16),px(64),px(w-16),px(100)};popInput=popControl(popup,POP_INPUT,L"EDIT",(popupActionKeys?actionKeyPrompt(false):et(HOTKEY_WAIT_E)),0x10000|0x80|0x800,24,72,w-48,20);popClear=popControl(popup,POP_CLEAR,L"BUTTON",tr(RECORD),0x10000|0xb,16,h-40,110,26);}else if(mode==6||mode==7){if(mode==6)popClear=popControl(popup,POP_CLEAR,L"BUTTON",et(shapeConflict?NEW_SHAPE_E:CHANGE_TRIGGER_E),0x10000|0xb,16,h-40,190,26);}else if(mode==4){popConfirm=popControl(popup,POP_CONFIRM,L"BUTTON",et(NO_CONFIRM_E),0x10000|3,16,h-72,w-32,23);}else if(mode==1){drawCanvas=popControl(popup,POP_LIST,L"miniGesturesCanvas",L"",0,20,52,w-40,h-114);popClear=popControl(popup,POP_CLEAR,L"BUTTON",et(RESET_DRAW),0x10000|0xb,16,h-40,130,26);}else{HDC dc=GetDC(popup);H old=SelectObject(dc,font);POINT ext{};GetTextExtentPoint32W(dc,L"Ag",2,&ext);SelectObject(dc,old);ReleaseDC(popup,dc);int eh=maxn(16,(ext.y*96+dpi-1)/dpi);popInput=popControl(popup,POP_INPUT,L"EDIT",initial,0x10000|0x80,24,16+(34-eh)/2,w-48,eh);SendMessageW(popInput,0xc5,mode==3?95:127,0);SendMessageW(popInput,0xd3,3,0);if(mode==2){SendMessageW(popInput,0x1501,0,(LP)et(SEARCH_ACTIONS));int ids[]={POP_LIST,POP_LIST2,POP_LIST3};int available=w-40;for(int i=0;i<3;i++){int left=20+i*available/3,right=20+(i+1)*available/3;actionColumns[i]=popControl(popup,ids[i],L"LISTBOX",L"",0x04000000|0x10000|0x200000|0x100000|1|0x100|0x10|0x40,left,66,right-left-2,h-128);SendMessageW(actionColumns[i],0x1a0,0,px(actionRowHeight));}popList=actionColumns[0];filterActions();}}popCancel=popControl(popup,POP_CANCEL,L"BUTTON",et(CANCEL_E),0x10000|0xb,w-(mode==6?344:214),h-40,94,26);popOk=popControl(popup,POP_OK,L"BUTTON",mode==1?et(ADD_SHAPE):mode==4?tr(REMOVE):mode==6?et(previewShapeConflict?REVIEW_ON_SAVE_E:TRANSFER_E):mode==7?et(FIX_E):et(OK_E),0x10000|0xb,w-(mode==6?244:114),h-40,mode==6?228:98,26);if(mode==2)layoutActionWindow(popup);else if(popInput)placeNativeEdit(popInput,popup,popField,px(mode==8?6:8),px(mode==8?6:8),editTextHeight,maxn(1,px(2)));if(popArgs)placeNativeEdit(popArgs,popup,popArgsField,px(6),px(6),editTextHeight,maxn(1,px(2)));hideNativeFocus(popup);beginDialogOwner();ShowWindow(popup,5);UpdateWindow(popup);if(mode==4||mode==6||mode==7)SetFocus(popCancel);if(mode==5)armPopupHotkey(true);if(popInput){SetFocus(popInput);if(mode==3||mode==8)SendMessageW(popInput,0xb1,0,-1);}MSG m{};while(!popDone){int result=GetMessageW(&m,nullptr,0,0);if(result<=0){if(!result)PostQuitMessage((int)m.wParam);break;}if(mode==5){if(m.message==MSG_RECORD){capturePopupHotkey((int)m.wParam,(int)m.lParam);continue;}if((m.message==0x100||m.message==0x104)&&popupArmed){int key=(int)m.wParam;if(popupActionKeys&&modifierKey(key)){if(!((u64)m.lParam&(1ULL<<30)))popupLocalModifier=popupLocalModifier? -1:key;continue;}popupLocalModifier=0;capturePopupHotkey(key,mods());continue;}if(popupActionKeys&&popupArmed&&(m.message==0x101||m.message==0x105)&&modifierKey((int)m.wParam)){if(popupLocalModifier==(int)m.wParam)capturePopupHotkey((int)m.wParam,0);popupLocalModifier=0;continue;}if(popupArmed&&(m.message==0x102||m.message==0x106))continue;}if(m.message==0x100&&m.wParam==0x1b){if(mode==8&&programPicking){cancelProgramPick();continue;}popDone=true;break;}if(mode==5&&m.message==0x100&&m.wParam==13&&((u64)m.lParam&(1ULL<<30)))continue;if(m.message==0x100&&m.wParam==13&&mode!=1){if(m.hwnd==popCancel)popDone=true;else if(mode==8&&m.hwnd==popProgramPick){/* drag target only */}else if(mode==8&&m.hwnd==popClear)SendMessageW(popup,0x111,POP_CLEAR,(LP)popClear);else if((mode==4||mode==6||mode==7)&&m.hwnd!=popOk){/* only an explicit Delete button confirms */}else acceptPopup();continue;}if(mode==2&&m.message==0x100&&m.hwnd==popInput&&(m.wParam==0x28||m.wParam==0x26)){SetFocus(popList);continue;}if(!IsDialogMessageW(popup,&m)){TranslateMessage(&m);DispatchMessageW(&m);}}if(mode==5)armPopupHotkey(false);if(mode==8)cancelProgramPick();if(mode==2&&!IsIconic(popup)&&!IsZoomed(popup)){
 RECT size{};
 if(GetWindowRect(popup,&size)&&size.right>size.left&&size.bottom>size.top){
  int width=cfg.actionWindowWidth,height=cfg.actionWindowHeight;
  if(actionSizeChanged||!width||!height){
   width=clamp(((size.right-size.left)*96+dpi/2)/dpi,1,16384);
   height=clamp(((size.bottom-size.top)*96+dpi/2)/dpi,1,16384);
  }
  if(!cfg.actionWindowPositionSaved||cfg.actionWindowX!=size.left||cfg.actionWindowY!=size.top||cfg.actionWindowWidth!=width||cfg.actionWindowHeight!=height){
   cfg.actionWindowX=size.left;cfg.actionWindowY=size.top;cfg.actionWindowPositionSaved=1;
   cfg.actionWindowWidth=width;cfg.actionWindowHeight=height;
   geometryDirty=true;saveGeometry();
  }
 }
}DestroyWindow(popup);popup=nullptr;drawCanvas=nullptr;drawdrag=0;if(mode==1){rasterScratch.clear();InterlockedExchange(&shapeRecording,0);}for(auto&col:actionColumns)col=nullptr;endDialogOwner();SetForegroundWindow(mainwnd);return popAccepted;}

static bool confirmDeletion(const W*message){if(cfg.skipDeleteConfirm)return true;confirmText=message;return showEditor(4,tr(REMOVE),L"");}

static bool recordActionHotkey(W*out,int capacity){if(!showEditor(5,et(HOTKEY_TITLE_E),L"",true))return false;canonicalHotkey(popupVk,popupMods,out,capacity);return *out!=0;}

static bool editProgramAction(int type,W*arg){if(!showEditor(8,defs[type].name[cfg.lang],arg))return false;cp(arg,programResult,256);return true;}
