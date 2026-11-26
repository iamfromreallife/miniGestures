#pragma once
#include "update_version.hpp"
extern "C" int __stdcall NativeUpdateStart();
extern "C" int __stdcall NativeUpdatePoll();
extern "C" int __stdcall NativeUpdateNotes(W*,int);
extern "C" int __stdcall NativeUpdateError(W*,int);
extern "C" int __stdcall NativeUpdateLatest(W*,int);
extern "C" void __stdcall NativeUpdateCancel();
extern "C" int __stdcall NativeUpdateMinute();
static int updateState=0; // 0 idle, 1 checking
static bool updateShowPending=false;
static bool updateManual=false;
static bool updatePrompt=false;
static bool updateChecked=false;
static W latestAvailable[18]{};
static const W* updateText(int kind){
 static const W* const labels[][6]={
 {L"Check for updates",L"Procurar atualizações",L"Buscar actualizaciones",L"Rechercher des mises à jour",L"Nach Updates suchen",L"Verifica aggiornamenti"},
 {L"Manual",L"Manual",L"Manual",L"Manuel",L"Manuell",L"Manuale"},
 {L"Once a day",L"Uma vez por dia",L"Una vez al día",L"Une fois par jour",L"Einmal täglich",L"Una volta al giorno"},
 {L"Once a week",L"Uma vez por semana",L"Una vez por semana",L"Une fois par semaine",L"Einmal wöchentlich",L"Una volta a settimana"},
 {L"Checking...",L"A procurar...",L"Buscando...",L"Recherche...",L"Suche...",L"Ricerca..."},
 {L"miniGestures is up to date.",L"O miniGestures está atualizado.",L"miniGestures está actualizado.",L"miniGestures est à jour.",L"miniGestures ist aktuell.",L"miniGestures è aggiornato."},
 {L"Update check failed.",L"Falha ao procurar atualizações.",L"Error al buscar actualizaciones.",L"Échec de la recherche de mises à jour.",L"Updateprüfung fehlgeschlagen.",L"Verifica degli aggiornamenti non riuscita."},
 {L"Get the new version from GitHub Releases.",L"Obtenha a nova versão nas versões do GitHub.",L"Obtenga la nueva versión en GitHub Releases.",L"Obtenez la nouvelle version sur GitHub Releases.",L"Die neue Version finden Sie auf GitHub Releases.",L"Scarica la nuova versione da GitHub Releases."},
 {L"Update available",L"Atualização disponível",L"Actualización disponible",L"Mise à jour disponible",L"Update verfügbar",L"Aggiornamento disponibile"},
 {L"Updates",L"Atualizações",L"Actualizaciones",L"Mises à jour",L"Updates",L"Aggiornamenti"},
 {L"Could not open the release page.",L"Não foi possível abrir a página da versão.",L"No se pudo abrir la página de la versión.",L"Impossible d’ouvrir la page de la version.",L"Die Release-Seite konnte nicht geöffnet werden.",L"Impossibile aprire la pagina della versione."}
 ,{L"Installed version:",L"Versão instalada:",L"Versión instalada:",L"Version installée :",L"Installierte Version:",L"Versione installata:"}
 ,{L"Latest available:",L"Última disponível:",L"Última disponible:",L"Dernière disponible :",L"Neueste verfügbare Version:",L"Ultima disponibile:"}
 ,{L"Unavailable",L"Indisponível",L"No disponible",L"Indisponible",L"Nicht verfügbar",L"Non disponibile"}
 ,{L"Ignore version",L"Ignorar versão",L"Ignorar versión",L"Ignorer la version",L"Version ignorieren",L"Ignora versione"}
 ,{L"Release notes",L"Notas da versão",L"Notas de la versión",L"Notes de version",L"Versionshinweise",L"Note di rilascio"}
 ,{L"Open release page",L"Abrir página da versão",L"Abrir página de la versión",L"Ouvrir la page de version",L"Release-Seite öffnen",L"Apri pagina di rilascio"}
 ,{L"No release notes provided.",L"Não foram fornecidas notas da versão.",L"No hay notas de la versión.",L"Aucune note de version.",L"Keine Versionshinweise vorhanden.",L"Nessuna nota di rilascio."}
 };return labels[kind][clamp(cfg.lang,0,5)];
}
static void versionStatusText(W* out,int capacity,const W* value){cp(out,updateText(11),capacity);cat(out,L" 2.0.24    ",capacity);cat(out,updateText(12),capacity);cat(out,L" ",capacity);cat(out,value,capacity);}
static void updateVersionStatus(){if(!settingsWnd||!settingsControls[UPDATEVERSIONS])return;W text[256]{};versionStatusText(text,256,updateState==1?updateText(4):*latestAvailable?latestAvailable:updateChecked?updateText(13):L"—");SetWindowTextW(settingsControls[UPDATEVERSIONS],text);}
static void updateButton(){updateVersionStatus();if(settingsWnd&&settingsControls[UPDATEBTN]){SetWindowTextW(settingsControls[UPDATEBTN],updateText(updateState==1?4:0));EnableWindow(settingsControls[UPDATEBTN],updateState!=1);InvalidateRect(settingsControls[UPDATEBTN],nullptr,0);}}
static void showUpdateError(HWND owner){W message[1400]{},detail[1000]{};cp(message,updateText(6),1400);if(NativeUpdateError(detail,1000)){cat(message,L"\r\n\r\n",1400);cat(message,detail,1400);}bool old=updatePrompt;updatePrompt=true;MessageBoxW(owner,message,L"miniGestures",0x30);updatePrompt=old;}
static bool updateTimerActive=false;
static void syncUpdateTimer(){if(!controllerwnd||exiting)return;bool wanted=configValid&&!noHooks&&!shutdownTestMode&&(cfg.updateMode!=0||updateState||updateShowPending);if(wanted&&!updateTimerActive)updateTimerActive=SetTimer(controllerwnd,102,1000,nullptr)!=0;else if(!wanted&&updateTimerActive){KillTimer(controllerwnd,102);updateTimerActive=false;}}
static void startUpdate(bool manual){if(exiting||!configValid||noHooks||shutdownTestMode||updatePrompt)return;
 if(updateState){if(manual)updateManual=true;return;}
 int now=NativeUpdateMinute(),last=cfg.updateLastCheck,interval=cfg.updateMode==1?1440:10080;
 if(!manual&&(cfg.updateMode==0||(last>0&&now>=last&&now-last<interval)))return;
 if(now<=0)return;
 cfg.updateLastCheck=now;if(!saveconfig(cfg,configpath)){cfg.updateLastCheck=last;if(manual)MessageBoxW(settingsWnd,updateText(6),L"miniGestures",0x30);return;}
 updateManual=manual;updateChecked=true;if(!NativeUpdateStart()){updateButton();if(manual)showUpdateError(settingsWnd);return;}
 updateState=1;updateButton();syncUpdateTimer();
}
static void tickUpdate();
