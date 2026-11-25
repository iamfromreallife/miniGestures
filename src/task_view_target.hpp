#pragma once
// Pointer targeting uses current stacking order, not foreground activation.
namespace mgtaskview {
template<class Equal> inline bool isTaskView(const wchar_t* cls,const wchar_t* title,Equal equal){
 if(!equal(cls,L"XamlExplorerHostIslandWindow")&&!equal(cls,L"MultitaskingViewFrame")&&!equal(cls,L"Windows.UI.Core.CoreWindow"))return false;
 const wchar_t* names[]={L"Task View",L"Vista de tarefas",L"Visão de tarefas",L"Visualização de tarefas",L"Vista de tareas",L"Affichage des tâches",L"Vue des tâches",L"Aufgabenansicht",L"Taskansicht",L"Visualizzazione attività",L"Visualizzazione delle attività"};
 for(auto n:names)if(equal(title,n))return true;return false;
}
// Stop on a protected surface or the desktop; never search behind it.
// UseOriginal preserves the actual child hit when we reach its root.
enum Candidate { Skip, Stop, Application, UseOriginal };
template<class Handle,class Ops> inline Handle resolve(Handle original,Ops& ops){
 if(!ops.ready())return original;
 Handle h=ops.first();
 for(int i=0;h&&i<256;++i){
  auto state=ops.classify(h);
  if(state==Stop||state==UseOriginal)return original;
  if(state==Application)return ops.child(h);
  auto next=ops.next(h);if(next==h)break;h=next;
 }
 return original;
}
}
