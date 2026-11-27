#pragma once
#include "model.hpp"
#include "capture_log.hpp"
// Fits in 32 bits even when an input bridge transports ULONG_PTR as DWORD.
constexpr up MARK=0x4D474553ULL;
struct ActionDef{const W*name[6];const W*hint;int vk,mods;};
#define A(en,pt,es,fr,de,it,h,k,m) {{L##en,L##pt,L##es,L##fr,L##de,L##it},L##h,k,m}
static const ActionDef defs[]={
A("Send hotkey","Enviar atalho","Enviar atajo","Envoyer raccourci","Tastenkürzel senden","Invia scorciatoia","Ctrl+Shift+A",0,0),
A("Maximize window","Maximizar janela","Maximizar ventana","Agrandir fenêtre","Fenster maximieren","Massimizza finestra","",0,0),
A("Minimize window","Minimizar janela","Minimizar ventana","Réduire fenêtre","Fenster minimieren","Minimizza finestra","",0,0),
A("Close window","Fechar janela","Cerrar ventana","Fermer fenêtre","Fenster schließen","Chiudi finestra","",0,0),
A("Restore window","Restaurar janela","Restaurar ventana","Restaurer fenêtre","Fenster wiederherstellen","Ripristina finestra","",0,0),
A("Toggle maximize","Alternar maximização","Alternar maximización","Basculer maximisation","Maximierung umschalten","Alterna massimizzazione","",0,0),
A("Center window","Centrar janela","Centrar ventana","Centrer fenêtre","Fenster zentrieren","Centra finestra","",0,0),
A("Always on top","Sempre visível","Siempre visible","Toujours au-dessus","Immer im Vordergrund","Sempre in primo piano","",0,0),
A("Browser back","Browser: anterior","Navegador: atrás","Navigateur : retour","Browser zurück","Browser: indietro","",0x25,1),
A("Browser forward","Browser: seguinte","Navegador: adelante","Navigateur : suivant","Browser vorwärts","Browser: avanti","",0x27,1),
A("New tab","Novo separador","Nueva pestaña","Nouvel onglet","Neuer Tab","Nuova scheda","",'T',2),
A("Close tab","Fechar separador","Cerrar pestaña","Fermer onglet","Tab schließen","Chiudi scheda","",'W',2),
A("Reopen tab","Reabrir separador","Reabrir pestaña","Rouvrir onglet","Tab wieder öffnen","Riapri scheda","",'T',6),
A("Show desktop","Mostrar ambiente de trabalho","Mostrar escritorio","Afficher bureau","Desktop anzeigen","Mostra desktop","",'D',8),
A("Refresh","Atualizar","Actualizar","Actualiser","Aktualisieren","Aggiorna","",0x74,0),
A("Next tab","Separador seguinte","Pestaña siguiente","Onglet suivant","Nächster Tab","Scheda successiva","",0x09,2),
A("Previous tab","Separador anterior","Pestaña anterior","Onglet précédent","Vorheriger Tab","Scheda precedente","",0x09,6),
A("Copy","Copiar","Copiar","Copier","Kopieren","Copia","",'C',2),
A("Cut","Cortar","Cortar","Couper","Ausschneiden","Taglia","",'X',2),
A("Paste","Colar","Pegar","Coller","Einfügen","Incolla","",'V',2),
A("Type text","Escrever texto","Escribir texto","Saisir texte","Text eingeben","Digita testo","Text",0,0),
A("Open file / URL","Abrir ficheiro / URL","Abrir archivo / URL","Ouvrir fichier / URL","Datei / URL öffnen","Apri file / URL","Path or URL",0,0),
A("Run program","Executar programa","Ejecutar programa","Lancer programme","Programm starten","Avvia programma","Executable|Arguments",0,0),
A("Run elevated","Executar como administrador","Ejecutar como administrador","Exécuter en administrateur","Als Administrator starten","Esegui come amministratore","Executable|Arguments",0,0),
A("Volume up","Aumentar volume","Subir volumen","Augmenter volume","Lauter","Alza volume","",0xAF,0),
A("Volume down","Diminuir volume","Bajar volumen","Diminuer volume","Leiser","Abbassa volume","",0xAE,0),
A("Mute","Silenciar","Silenciar","Couper son","Stumm","Disattiva audio","",0xAD,0),
A("Play / pause","Reproduzir / pausar","Reproducir / pausar","Lecture / pause","Wiedergabe / Pause","Riproduci / pausa","",0xB3,0),
A("Next track","Faixa seguinte","Pista siguiente","Piste suivante","Nächster Titel","Brano successivo","",0xB0,0),
A("Previous track","Faixa anterior","Pista anterior","Piste précédente","Vorheriger Titel","Brano precedente","",0xB1,0),
A("Desktop left","Ambiente de trabalho anterior","Escritorio anterior","Bureau précédent","Desktop links","Desktop precedente","",0x25,10),
A("Desktop right","Ambiente de trabalho seguinte","Escritorio siguiente","Bureau suivant","Desktop rechts","Desktop successivo","",0x27,10),
A("New desktop","Novo ambiente de trabalho","Nuevo escritorio","Nouveau bureau","Neuer Desktop","Nuovo desktop","",'D',10),
A("Close desktop","Fechar ambiente de trabalho","Cerrar escritorio","Fermer bureau","Desktop schließen","Chiudi desktop","",0x73,10),
A("Task View","Vista de tarefas","Vista de tareas","Vue des tâches","Taskansicht","Visualizzazione attività","",0x09,8),
A("Snap left","Encaixar à esquerda","Ajustar izquierda","Ancrer à gauche","Links andocken","Affianca a sinistra","",0x25,8),
A("Snap right","Encaixar à direita","Ajustar derecha","Ancrer à droite","Rechts andocken","Affianca a destra","",0x27,8),
A("Find","Localizar","Buscar","Rechercher","Suchen","Trova","",'F',2),
A("Address bar","Barra de endereço","Barra de direcciones","Barre d'adresse","Adressleiste","Barra indirizzi","",'L',2),
A("Zoom in","Ampliar","Acercar","Agrandir","Vergrößern","Ingrandisci","",0xBB,2),
A("Zoom out","Reduzir","Alejar","Réduire","Verkleinern","Riduci","",0xBD,2),
A("Reset zoom","Repor zoom","Restablecer zoom","Réinitialiser zoom","Zoom zurücksetzen","Ripristina zoom","",'0',2),
A("Lock computer","Bloquear computador","Bloquear equipo","Verrouiller ordinateur","Computer sperren","Blocca computer","",0,0),
A("Task Manager","Gestor de Tarefas","Administrador de tareas","Gestionnaire des tâches","Task-Manager","Gestione attività","",0x1B,6),
A("File Explorer","Explorador de Ficheiros","Explorador de archivos","Explorateur de fichiers","Datei-Explorer","Esplora file","",'E',8),
A("Windows Settings","Definições do Windows","Configuración de Windows","Paramètres Windows","Windows-Einstellungen","Impostazioni Windows","",'I',8),
A("","","","","","","",0,0),
A("Mouse click","Clique do rato","Clic del ratón","Clic souris","Mausklick","Clic mouse","1=left, 2=right, 3=middle, 4/5=X",0,0),
A("Scroll vertical","Deslocar verticalmente","Desplazar vertical","Défilement vertical","Vertikal scrollen","Scorri verticale","120 / -120",0,0),
A("Scroll horizontal","Deslocar horizontalmente","Desplazar horizontal","Défilement horizontal","Horizontal scrollen","Scorri orizzontale","120 / -120",0,0),
A("Move cursor","Mover cursor","Mover cursor","Déplacer curseur","Cursor bewegen","Sposta cursore","x,y",0,0),
A("Set clipboard text","Definir texto da área de transferência","Definir portapapeles","Définir presse-papiers","Zwischenablage setzen","Imposta appunti","Text",0,0),
A("Turn displays off","Desligar ecrãs","Apagar pantallas","Éteindre écrans","Bildschirme ausschalten","Spegni schermi","",0,0),
A("Activate window","Ativar janela","Activar ventana","Activer fenêtre","Fenster aktivieren","Attiva finestra","",0,0),
A("Screenshot","Captura de ecrã","Captura de pantalla","Capture d'écran","Screenshot","Schermata","",'S',12),
A("Downloads","Transferências","Descargas","Téléchargements","Downloads","Download","",'J',2),
A("History","Histórico","Historial","Historique","Verlauf","Cronologia","",'H',2),
A("Private window","Janela privada","Ventana privada","Fenêtre privée","Privates Fenster","Finestra privata","",'N',6),
A("Toggle full screen","Alternar ecrã inteiro","Alternar pantalla completa","Basculer plein écran","Vollbild umschalten","Alterna schermo intero","",0x7A,0),
A("Stop media","Parar multimédia","Detener multimedia","Arrêter média","Medien stoppen","Ferma media","",0xB2,0),
A("Send window to next monitor","Enviar janela para o monitor seguinte","Enviar ventana al monitor siguiente","Envoyer fenêtre au moniteur suivant","Fenster auf nächsten Monitor verschieben","Sposta finestra sul monitor successivo","",0,0),
A("Send window to previous monitor","Enviar janela para o monitor anterior","Enviar ventana al monitor anterior","Envoyer fenêtre au moniteur précédent","Fenster auf vorherigen Monitor verschieben","Sposta finestra sul monitor precedente","",0,0),
A("Show miniGestures window","Mostrar janela do miniGestures","Mostrar ventana de miniGestures","Afficher la fenêtre miniGestures","miniGestures-Fenster anzeigen","Mostra finestra miniGestures","",0,0),
A("Go to first desktop","Ir para o primeiro ambiente de trabalho","Ir al primer escritorio","Aller au premier bureau","Zum ersten Desktop wechseln","Vai al primo desktop","",0,0),
A("Go to last desktop","Ir para o último ambiente de trabalho","Ir al último escritorio","Aller au dernier bureau","Zum letzten Desktop wechseln","Vai all’ultimo desktop","",0,0),
A("","","","","","","",0,0),
A("Kill app","Terminar aplicação","Forzar cierre de aplicación","Forcer l’arrêt de l’application","App zwangsweise beenden","Termina app forzatamente","",0,0),
A("Go to Home","Ir para a página inicial","Ir a la página de inicio","Aller à la page d’accueil","Zur Startseite","Vai alla pagina iniziale","",0,0),
A("Toggle hidden files","Alternar ficheiros ocultos","Alternar archivos ocultos","Afficher/masquer les fichiers cachés","Versteckte Dateien umschalten","Mostra/nascondi file nascosti","",0,0),
A("Toggle hidden files including system protected","Alternar ficheiros ocultos e de sistema protegidos","Alternar archivos ocultos y de sistema protegidos","Fichiers cachés et système protégés","Versteckte und geschützte Systemdateien umschalten","Mostra/nascondi file nascosti e di sistema protetti","",0,0)};
#undef A
constexpr int ACTIONS=sizeof(defs)/sizeof(defs[0]);
static_assert(ACTIONS==ACTION_COUNT,"Action IDs and persistence range must agree");
static inline int mods(){return ((GetAsyncKeyState(0x12)&0x8000)?1:0)|((GetAsyncKeyState(0x11)&0x8000)?2:0)|((GetAsyncKeyState(0x10)&0x8000)?4:0)|(((GetAsyncKeyState(0x5b)|GetAsyncKeyState(0x5c))&0x8000)?8:0);}
#include "hotkey.hpp"
static inline bool sendkey(int vk,int m){INPUT a[10]{};int n=0;int keys[4]={0x12,0x11,0x10,0x5b};int owned=0;auto push=[&](int k,int flags){auto&i=a[n++];i.type=1;i.ki.vk=(u16)k;i.ki.flags=flags|((k>=0x21&&k<=0x2e||k==0x5b||k==0x5c||k==0x6f)?1:0);i.ki.extra=MARK;};for(int j=0;j<4;j++)if((m&(1<<j))&&!(GetAsyncKeyState(keys[j])&0x8000)){push(keys[j],0);owned|=1<<j;}push(vk,0);push(vk,2);for(int j=3;j>=0;j--)if(owned&(1<<j))push(keys[j],2);UINT sent=SendInput(n,a,sizeof(INPUT));
#ifdef _WIN32
 DWORD error=sent==(UINT)n?0:GetLastError();MG_LOG("SendInput vk mods owned requested accepted error foreground",vk,m,owned,n,sent,error,(up)GetForegroundWindow());
#endif
 return sent==(UINT)n;}
static inline void mouseflags(int b,DWORD&down,DWORD&upf,DWORD&data){const DWORD ds[]={0,2,8,32,128,128},us[]={0,4,16,64,256,256};b=clamp(b,1,5);down=ds[b];upf=us[b];data=b>=4?b-3:0;}
static inline bool click(int b){DWORD d,u,x;mouseflags(b,d,u,x);INPUT a[2]{};a[0].mi.dwFlags=d;a[1].mi.dwFlags=u;for(auto&i:a){i.mi.mouseData=x;i.mi.extra=MARK;}return SendInput(2,a,sizeof(INPUT))==2;}
static inline bool valid(const Target&t){DWORD pid=0;return IsWindow(t.hwnd)&&GetWindowThreadProcessId(t.hwnd,&pid)&&pid==t.pid;}
API B ShowWindowAsync(HWND,int);
API B EnumDisplayMonitors(HDC,const RECT*,B(CB*)(H,HDC,RECT*,LP),LP);
struct MonitorList{H handles[64];RECT rect[64];int n;};
static B CB monitorAdd(H h,HDC,RECT*r,LP l){auto&v=*(MonitorList*)l;if(v.n<64){v.handles[v.n]=h;v.rect[v.n++]=*r;}return 1;}
static inline bool moveMonitor(HWND h,int dir){MonitorList v{};if(!EnumDisplayMonitors(nullptr,nullptr,monitorAdd,(LP)&v)||v.n<2)return false;for(int i=1;i<v.n;i++)for(int j=i;j>0;j--){auto a=v.rect[j-1],b=v.rect[j];if(a.left<b.left||(a.left==b.left&&a.top<=b.top))break;v.rect[j-1]=b;v.rect[j]=a;H t=v.handles[j];v.handles[j]=v.handles[j-1];v.handles[j-1]=t;}H cur=MonitorFromWindow(h,2);int i=0;while(i<v.n&&v.handles[i]!=cur)i++;if(i==v.n)return false;MONITOR a{sizeof(a)},b{sizeof(b)};if(!GetMonitorInfoW(cur,&a)||!GetMonitorInfoW(v.handles[(i+dir+v.n)%v.n],&b))return false;bool max=IsZoomed(h);if(max)ShowWindow(h,4);RECT r{};if(!GetWindowRect(h,&r))return false;int w=minn(r.right-r.left,b.rcWork.right-b.rcWork.left),hh=minn(r.bottom-r.top,b.rcWork.bottom-b.rcWork.top);int x=clamp(b.rcWork.left+r.left-a.rcWork.left,b.rcWork.left,b.rcWork.right-w),y=clamp(b.rcWork.top+r.top-a.rcWork.top,b.rcWork.top,b.rcWork.bottom-hh);bool ok=SetWindowPos(h,nullptr,x,y,w,hh,4|16);if(max)PostMessageW(h,0x112,0xf030,0);return ok;}
API B IsWindowEnabled(HWND);
static H stopActionEvent;
#include "close_window.hpp"
#include "background_actions.hpp"
#include "center_window.hpp"
#include "show_editor_action.hpp"
#include "desktop_windows.hpp"
#include "kill_app.hpp"
extern "C" int CB NativeToggleHiddenFiles(int includeProtected);
static inline bool execute(const RuntimeAction&a,const Target&t,bool ordered=false){if(a.type<0||a.type>=ACTIONS||retiredAction(a.type))return false;auto&d=defs[a.type];HWND h=t.hwnd;bool v=valid(t);if(a.type==35||a.type==36)return v&&snapTarget(h,a.type==36);if(d.vk){if((a.type<24&&a.type!=13)||(a.type>=37&&a.type<=41)||(a.type>=55&&a.type<=58))return targetKey(t,d.vk,d.mods);return sendkey(d.vk,d.mods);}switch(a.type){case TOGGLE_HIDDEN_FILES_ACTION:case TOGGLE_ALL_HIDDEN_FILES_ACTION:return NativeToggleHiddenFiles(a.type==TOGGLE_ALL_HIDDEN_FILES_ACTION)!=0;case KILL_APP_ACTION:return killTargetApp(t);case BROWSER_HOME_ACTION:return targetKey(t,0x24,1);case FIRST_DESKTOP_ACTION:case LAST_DESKTOP_ACTION:return desktopEdge(a.type==LAST_DESKTOP_ACTION);case SHOW_MINIGESTURES_ACTION:return showMiniGesturesWindow();case 60:case 61:return v&&moveMonitor(h,a.type==60?1:-1);case 0:{int k=a.key,m=a.mods;if(!k&&!parsekey(a.arg,k,m))return false;return targetKey(t,k,m);}case 1:case 2:case 4:case 5:if(!v)return false;{int op=a.type==1?3:a.type==2?minimizeShowCommand(t):a.type==4?4:IsZoomed(h)?4:3;MG_LOG("window show action target operation foreground iconic",a.type,(up)h,op,(up)GetForegroundWindow(),IsIconic(h));if(op==3&&!targetIsForeground(t))return PostMessageW(h,0x112,0xf030,0)!=0;if(ordered){ShowWindow(h,op);return true;}return ShowWindowAsync(h,op)!=0;}case 3:return closeTargetWindow(t);case 6:return v&&centerWindow(h,ordered);case 7:return v&&SetWindowPos(h,PTR((GetWindowLongPtrW(h,-20)&8)?-2:-1),0,0,0,0,1|2|16|(ordered?0:0x4000));case 20:return targetText(t,a.arg);case 21:return ShellExecuteW(nullptr,L"open",a.arg,nullptr,nullptr,1)>32;case 22:case 23:{W path[256];cp(path,a.arg,256);W*args=path+clamp(a.separator,0,255);if(a.separator<0){args=path;while(*args&&*args!=L'|')args++;}if(*args)*args++=0;return ShellExecuteW(nullptr,a.type==23?L"runas":L"open",path,args,nullptr,1)>32;}case 42:return LockWorkStation();case 47:return targetClick(t,clamp(num(a.arg),1,5));case 48:case 49:return targetScroll(t,num(a.arg),a.type==49);case 50:{const W*p=a.arg;while(*p&&*p!=L',')p++;return *p&&SetCursorPos(num(a.arg),num(p+1));}case 51:{if(!OpenClipboard(nullptr))return false;H mem=GlobalAlloc(2,(len(a.arg)+1)*sizeof(W));bool ok=false;if(mem){void*p=GlobalLock(mem);if(p){memcpy(p,a.arg,(len(a.arg)+1)*sizeof(W));GlobalUnlock(mem);if(EmptyClipboard()&&SetClipboardData(13,mem))ok=true;}if(!ok)GlobalFree(mem);}CloseClipboard();return ok;}case 52:return PostMessageW(PTR(0xffff),0x112,0xf170,2);case 53:return v&&ensureTargetForeground(t);}return false;}
