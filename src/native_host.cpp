// Native Microsoft C++ / Windows SDK host. No Go runtime, cgo, or PowerShell.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#include <sddl.h>
#include <taskschd.h>
#include <oleauto.h>
#include <shlobj.h>
#include <shldisp.h>
#include <exdisp.h>
#include <shlguid.h>
#include <servprov.h>
#include <cstdint>
#include <string>
#include <vector>
#include "native_host_logic.hpp"

// The unchanged core uses its own minimal x64 ABI declarations in a separate TU.
extern "C" unsigned int WINAPI NativeStart(void*);
extern "C" int WINAPI NativeRequestedElevation(const wchar_t*);

namespace {
using mg_host::Args;
struct Handle {
    HANDLE h = nullptr;
    explicit Handle(HANDLE v = nullptr) : h(v) {}
    ~Handle() { if (h && h != INVALID_HANDLE_VALUE) CloseHandle(h); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    explicit operator bool() const { return h && h != INVALID_HANDLE_VALUE; }
};
template<class T> struct Com {
    T* p = nullptr;
    ~Com() { if (p) p->Release(); }
    Com() = default;
    Com(const Com&) = delete;
    Com& operator=(const Com&) = delete;
    T* operator->() const { return p; }
    T** out() { return &p; }
};
struct Bstr {
    BSTR p;
    explicit Bstr(const std::wstring& s) : p(SysAllocStringLen(s.data(), static_cast<UINT>(s.size()))) {}
    ~Bstr() { SysFreeString(p); }
    Bstr(const Bstr&) = delete;
    Bstr& operator=(const Bstr&) = delete;
};
struct ComApartment {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    ~ComApartment() { if (SUCCEEDED(hr)) CoUninitialize(); }
};
struct Identity { std::wstring sid; bool elevated = false; bool elevationKnown = false; };
Identity tokenIdentity(HANDLE token) {
    Identity id;
    DWORD n = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &n);
    if (!n) return id;
    std::vector<BYTE> buf(n);
    if (!GetTokenInformation(token, TokenUser, buf.data(), n, &n)) return id;
    const auto* user = reinterpret_cast<const TOKEN_USER*>(buf.data());
    LPWSTR text = nullptr;
    if (!ConvertSidToStringSidW(user->User.Sid, &text)) return id;
    id.sid = text;
    LocalFree(text);
    TOKEN_ELEVATION elevation{};
    id.elevationKnown = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &n) != FALSE;
    id.elevated = id.elevationKnown && elevation.TokenIsElevated != 0;
    return id;
}
Identity currentIdentity() {
    Handle token;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token.h)) return {};
    return tokenIdentity(token.h);
}
std::wstring executable() {
    std::vector<wchar_t> buf(32768);
    DWORD n = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    if (!n || n >= buf.size()) return {};
    return std::wstring(buf.data(), n);
}
int taskError(HRESULT hr) { return hr == E_ACCESSDENIED ? 10 : 11; }

// Equivalent to the previous Go-generated Schedule.Service script. Only this
// executable and the fixed --login-start argument are registered. The task is
// scoped to the actual current user SID, never SYSTEM or another account.
int configureStartup(const Args& args, const Identity& id) {
    if (!mg_host::configureArgs(args, id.sid)) return 12;
    const bool enabled = args[1] == L"1", highest = args[2] == L"1";
    if (enabled && highest && !id.elevated) return 10;
    const auto exe = executable(), dir = mg_host::directory(exe);
    if (exe.empty() || dir.empty()) return 11;
    ComApartment apartment;
    if (FAILED(apartment.hr)) return taskError(apartment.hr);
    // Match the old helper's 30s task-operation bound even if the RPC stalls.
    // No global timers or hooks are started in this short-lived helper process.
    Handle done(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!done) return 11;
    auto watchdog = [](void* p) -> DWORD {
        if (WaitForSingleObject(static_cast<HANDLE>(p), 30000) != WAIT_OBJECT_0)
            TerminateProcess(GetCurrentProcess(), 11);
        return 0;
    };
    Handle timer(CreateThread(nullptr, 0, watchdog, done.h, 0, nullptr));
    if (!timer) return 11;
    // Ensure the event outlives the watchdog, on every early return.
    struct Finish { HANDLE event, thread; ~Finish() { SetEvent(event); WaitForSingleObject(thread, INFINITE); } } finish{done.h, timer.h};
    VARIANT empty{}; VariantInit(&empty);
    Com<ITaskService> service; Com<ITaskFolder> folder;
    Bstr root(L"\\"), name(L"miniGestures-" + id.sid), sid(id.sid), path(exe), workingDir(dir),
         description(L"miniGestures login startup"), loginArg(L"--login-start"), noLimit(L"PT0S");
    if (!root.p || !name.p || !sid.p || !path.p || !workingDir.p || !description.p || !loginArg.p || !noLimit.p) return 11;
    HRESULT hr;
#define CHECK(expr) do { hr = (expr); if (FAILED(hr)) return taskError(hr); } while (false)
    CHECK(CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, reinterpret_cast<void**>(service.out())));
    CHECK(service->Connect(empty, empty, empty, empty));
    CHECK(service->GetFolder(root.p, folder.out()));
    if (!enabled) {
        Com<IRegisteredTask> existing;
        hr = folder->GetTask(name.p, existing.out());
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) return 0;
        if (FAILED(hr)) return taskError(hr);
        hr = folder->DeleteTask(name.p, 0);
        return SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ? 0 : taskError(hr);
    }
    Com<ITaskDefinition> def;
    Com<IRegistrationInfo> info;
    Com<IPrincipal> principal;
    Com<ITriggerCollection> triggers;
    Com<ITrigger> trigger;
    Com<ILogonTrigger> logon;
    Com<ITaskSettings> settings;
    Com<IActionCollection> actions;
    Com<IAction> action;
    Com<IExecAction> exec;
    Com<IRegisteredTask> registered;
    CHECK(service->NewTask(0, def.out()));
    CHECK(def->get_RegistrationInfo(info.out()));
    CHECK(info->put_Description(description.p));
    CHECK(def->get_Principal(principal.out()));
    CHECK(principal->put_UserId(sid.p));
    CHECK(principal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN));
    CHECK(principal->put_RunLevel(highest ? TASK_RUNLEVEL_HIGHEST : TASK_RUNLEVEL_LUA));
    CHECK(def->get_Triggers(triggers.out()));
    CHECK(triggers->Create(TASK_TRIGGER_LOGON, trigger.out()));
    CHECK(trigger->QueryInterface(IID_ILogonTrigger, reinterpret_cast<void**>(logon.out())));
    CHECK(logon->put_UserId(sid.p));
    CHECK(logon->put_Enabled(VARIANT_TRUE));
    CHECK(def->get_Settings(settings.out()));
    CHECK(settings->put_Enabled(VARIANT_TRUE));
    CHECK(settings->put_AllowDemandStart(VARIANT_TRUE));
    CHECK(settings->put_DisallowStartIfOnBatteries(VARIANT_FALSE));
    CHECK(settings->put_StopIfGoingOnBatteries(VARIANT_FALSE));
    CHECK(settings->put_ExecutionTimeLimit(noLimit.p));
    CHECK(settings->put_MultipleInstances(TASK_INSTANCES_IGNORE_NEW));
    CHECK(def->get_Actions(actions.out()));
    CHECK(actions->Create(TASK_ACTION_EXEC, action.out()));
    CHECK(action->QueryInterface(IID_IExecAction, reinterpret_cast<void**>(exec.out())));
    CHECK(exec->put_Path(path.p));
    CHECK(exec->put_Arguments(loginArg.p));
    CHECK(exec->put_WorkingDirectory(workingDir.p));
    VARIANT user{}; VariantInit(&user); user.vt = VT_BSTR; user.bstrVal = sid.p;
    CHECK(folder->RegisterTaskDefinition(name.p, def.p, TASK_CREATE_OR_UPDATE, user, empty, TASK_LOGON_INTERACTIVE_TOKEN, empty, registered.out()));
#undef CHECK
    return 0;
}

// Launch via the desktop's existing Shell automation object, not a new
// Shell.Application object (which might inherit our elevated identity).
// Microsoft's Execute in Explorer pattern avoids token duplication entirely.
struct ShellApartment {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    ~ShellApartment() { if (SUCCEEDED(hr)) CoUninitialize(); }
};
DWORD shellError(HRESULT hr) {
    return FACILITY_WIN32 == HRESULT_FACILITY(hr) ? HRESULT_CODE(hr) : static_cast<DWORD>(hr);
}
std::wstring argumentString(const Args& args) {
    std::wstring params;
    for (const auto& arg : args) { if (!params.empty()) params += L' '; params += mg_host::quote(arg); }
    return params;
}
DWORD startThroughExplorer(const std::wstring& exe, const Args& args, const std::wstring& sid, std::wstring& stage, bool screenClip=false) {
    const auto dir = screenClip ? mg_host::directory(executable()) : mg_host::directory(exe);
    // This mode is only for the fixed system capture URI; never accept an
    // arbitrary caller-supplied protocol as an elevated shell request.
    if (screenClip && (exe!=L"ms-screenclip://?source=PrintScreen" || !args.empty())) return ERROR_INVALID_PARAMETER;
    if (exe.empty() || dir.empty()) return ERROR_PATH_NOT_FOUND;
    stage=L"Explorer identity";
    HWND shell=GetShellWindow(); DWORD pid=0, shellSession=0, ourSession=0;
    if (!shell || !GetWindowThreadProcessId(shell,&pid) || !pid) return ERROR_NOT_FOUND;
    Handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid)), token;
    if (!process) return GetLastError();
    if (!OpenProcessToken(process.h,TOKEN_QUERY,&token.h)) return GetLastError();
    const auto identity=tokenIdentity(token.h);
    if (sid.empty() || identity.sid!=sid || !identity.elevationKnown || identity.elevated ||
        !ProcessIdToSessionId(pid,&shellSession) || !ProcessIdToSessionId(GetCurrentProcessId(),&ourSession) || shellSession!=ourSession)
        return ERROR_ACCESS_DENIED;
    stage=L"Explorer COM initialization";
    ShellApartment apartment;
    if (FAILED(apartment.hr) && apartment.hr!=RPC_E_CHANGED_MODE) return shellError(apartment.hr);
    Com<IShellWindows> windows;
    Com<IDispatch> desktop, background, application;
    Com<IServiceProvider> provider;
    Com<IShellBrowser> browser;
    Com<IShellView> view;
    Com<IShellFolderViewDual> folder;
    Com<IShellDispatch2> dispatch;
    HRESULT hr;
#define SHELL_CHECK(label, expr) do { stage=L##label; hr=(expr); if (FAILED(hr)) return shellError(hr); } while (false)
    SHELL_CHECK("Explorer shell windows", CoCreateInstance(CLSID_ShellWindows,nullptr,CLSCTX_LOCAL_SERVER,IID_PPV_ARGS(windows.out())));
    VARIANT location{}, root{}; location.vt=VT_I4; location.lVal=CSIDL_DESKTOP;
    long desktopHwnd=0;
    SHELL_CHECK("Explorer desktop", windows->FindWindowSW(&location,&root,SWC_DESKTOP,&desktopHwnd,SWFO_NEEDDISPATCH,desktop.out()));
    if (hr!=S_OK || !desktop.p) return ERROR_NOT_FOUND;
    SHELL_CHECK("Explorer desktop service", desktop->QueryInterface(IID_PPV_ARGS(provider.out())));
    SHELL_CHECK("Explorer desktop browser", provider->QueryService(SID_STopLevelBrowser,IID_PPV_ARGS(browser.out())));
    HWND browserWindow=nullptr; DWORD browserPid=0;
    SHELL_CHECK("Explorer desktop window", browser->GetWindow(&browserWindow));
    if (!browserWindow || !GetWindowThreadProcessId(browserWindow,&browserPid) || browserPid!=pid || GetShellWindow()!=shell) return ERROR_ACCESS_DENIED;
    SHELL_CHECK("Explorer desktop view", browser->QueryActiveShellView(view.out()));
    SHELL_CHECK("Explorer background object", view->GetItemObject(SVGIO_BACKGROUND,IID_PPV_ARGS(background.out())));
    SHELL_CHECK("Explorer folder view", background->QueryInterface(IID_PPV_ARGS(folder.out())));
    SHELL_CHECK("Explorer application object", folder->get_Application(application.out()));
    if (!application.p) return ERROR_NOT_FOUND;
    SHELL_CHECK("Explorer ShellExecute interface", application->QueryInterface(IID_PPV_ARGS(dispatch.out())));
    Bstr file(exe), params(argumentString(args)), workingDir(dir), verb(L"open");
    if (!file.p || !params.p || !workingDir.p || !verb.p) return ERROR_OUTOFMEMORY;
    VARIANT vArgs{}, vDir{}, vVerb{}, vShow{};
    vArgs.vt=VT_BSTR; vArgs.bstrVal=params.p;
    vDir.vt=VT_BSTR; vDir.bstrVal=workingDir.p;
    vVerb.vt=VT_BSTR; vVerb.bstrVal=verb.p;
    vShow.vt=VT_I4; vShow.lVal=SW_SHOWNORMAL;
    SHELL_CHECK("Explorer ShellExecute", dispatch->ShellExecute(file.p,vArgs,vDir,vVerb,vShow));
#undef SHELL_CHECK
    return ERROR_SUCCESS;
}
// Legacy broker compatibility; ordinary same-level launch only.
DWORD startChild(const std::wstring& exe, const Args& args) {
    auto cmd=mg_host::commandLine(exe,args); const auto dir=mg_host::directory(exe);
    if (exe.empty() || dir.empty()) return ERROR_PATH_NOT_FOUND;
    STARTUPINFOW si{}; si.cb=sizeof(si); PROCESS_INFORMATION pi{};
    if (!CreateProcessW(exe.c_str(),cmd.data(),nullptr,nullptr,FALSE,0,nullptr,dir.c_str(),&si,&pi)) return GetLastError();
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess); return ERROR_SUCCESS;
}
// Hold the original process handle across the readiness notification, avoiding
// PID reuse between acknowledgment and the actual wait. If access is denied
// during de-elevation, use the existing single-instance guard as the fallback.
bool waitForRestart(DWORD pid, const std::wstring& eventName = {}) {
    if (pid == GetCurrentProcessId()) return false;
    Handle process(OpenProcess(SYNCHRONIZE, FALSE, pid));
    if (!process && !eventName.empty() && GetLastError() != ERROR_ACCESS_DENIED) return false;
    if (!eventName.empty()) {
        // The parent must explicitly approve the handoff. A late child from a
        // timed-out launch must not start later when the original app exits.
        Handle commit(OpenEventW(SYNCHRONIZE,FALSE,mg_host::commitEventName(eventName).c_str()));
        Handle ready(OpenEventW(EVENT_MODIFY_STATE,FALSE,eventName.c_str()));
        if (!commit || !ready || !SetEvent(ready.h)) return false;
        if (WaitForSingleObject(commit.h,20000)!=WAIT_OBJECT_0) return false;
    }
    if (process) return WaitForSingleObject(process.h, 30000) == WAIT_OBJECT_0;
    const ULONGLONG start = GetTickCount64();
    while (GetTickCount64() - start < 30000) {
        Handle guard(OpenMutexW(SYNCHRONIZE, FALSE, L"Local\\miniGestures.v1"));
        if (!guard && GetLastError() == ERROR_FILE_NOT_FOUND) return true;
        Sleep(50);
    }
    return false;
}
// Legacy helper arguments remain supported for compatibility, but the UI no
// longer uses the intermediary broker when requesting a restart.
int restartBroker(const Args& args, const Identity& id) {
    std::uint32_t pid = 0; bool admin = false;
    if (!mg_host::restartArgs(args, id.sid, pid, admin) || !id.elevationKnown || (admin && !id.elevated)) return 12;
    Args child{L"--restart-wait", std::to_wstring(pid), args[2], id.sid};
    std::wstring stage;
    const DWORD result=id.elevated&&!admin?startThroughExplorer(executable(),child,id.sid,stage):startChild(executable(),child);
    return result==ERROR_SUCCESS?0:11;
}
DWORD restartDirect(HWND owner, bool admin, std::wstring& stage) {
    stage=L"Token";
    const auto id=currentIdentity();
    if (id.sid.empty() || !id.elevationKnown) return ERROR_ACCESS_DENIED;
    if (id.elevated==admin) return ERROR_SUCCESS; // already at the requested integrity level (handled by core)
    stage=L"Executable";
    const auto exe=executable(), dir=mg_host::directory(exe);
    if (exe.empty() || dir.empty()) return ERROR_PATH_NOT_FOUND;
    stage=L"Restart event";
    GUID guid{}; wchar_t guidText[40]{};
    if (FAILED(CoCreateGuid(&guid)) || !StringFromGUID2(guid,guidText,40)) return ERROR_GEN_FAILURE;
    const std::wstring name=L"Local\\miniGestures.restart."+std::wstring(guidText);
    // Allow only SYSTEM and this user. A medium mandatory label lets the
    // verified Explorer-token replacement acknowledge a de-elevation too.
    PSECURITY_DESCRIPTOR sd=nullptr;
    const std::wstring sddl=L"D:P(A;;GA;;;SY)(A;;GA;;;"+id.sid+L")"+(id.elevated?L"S:(ML;;NW;;;ME)":L"");
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(),SDDL_REVISION_1,&sd,nullptr)) return GetLastError();
    SECURITY_ATTRIBUTES sa{sizeof(sa),sd,FALSE};
    SetLastError(ERROR_SUCCESS);
    Handle ready(CreateEventW(&sa,TRUE,FALSE,name.c_str()));
    const DWORD eventError=GetLastError();
    SetLastError(ERROR_SUCCESS);
    Handle commit(CreateEventW(&sa,TRUE,FALSE,mg_host::commitEventName(name).c_str()));
    const DWORD commitError=GetLastError(); LocalFree(sd);
    if (!ready || eventError==ERROR_ALREADY_EXISTS) return eventError ? eventError : ERROR_GEN_FAILURE;
    if (!commit || commitError==ERROR_ALREADY_EXISTS) return commitError ? commitError : ERROR_GEN_FAILURE;
    Args args{L"--restart-wait",std::to_wstring(GetCurrentProcessId()),admin?L"1":L"0",id.sid,name};
    Handle child;
    if (admin) {
        stage=L"UAC / ShellExecuteExW";
        ShellApartment apartment;
        if (FAILED(apartment.hr) && apartment.hr!=RPC_E_CHANGED_MODE) return static_cast<DWORD>(apartment.hr);
        const auto params=argumentString(args);
        SHELLEXECUTEINFOW info{}; info.cbSize=sizeof(info);
        info.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NOASYNC|SEE_MASK_FLAG_NO_UI;
        info.hwnd=owner; info.lpVerb=L"runas"; info.lpFile=exe.c_str(); info.lpParameters=params.c_str(); info.lpDirectory=dir.c_str(); info.nShow=SW_SHOWNORMAL;
        if (!ShellExecuteExW(&info)) { const DWORD error=GetLastError(); return error?error:ERROR_GEN_FAILURE; }
        child.h=info.hProcess;
        if (!child) return ERROR_INVALID_HANDLE;
    } else {
        const DWORD error=startThroughExplorer(exe,args,id.sid,stage);
        if(error) return error;
        // Shell automation intentionally does not return a process handle.
        // The child still verifies the SID and actual elevation before signaling.
        stage=L"Non-elevated replacement readiness";
        const DWORD wait=WaitForSingleObject(ready.h,15000);
        if(wait==WAIT_OBJECT_0) return SetEvent(commit.h)?ERROR_SUCCESS:GetLastError();
        return wait==WAIT_TIMEOUT?ERROR_TIMEOUT:GetLastError();
    }
    stage=L"Replacement readiness";
    HANDLE waits[]={child.h,ready.h};
    const DWORD wait=WaitForMultipleObjects(2,waits,FALSE,15000);
    if(wait==WAIT_OBJECT_0+1 && WaitForSingleObject(child.h,0)==WAIT_TIMEOUT) return SetEvent(commit.h)?ERROR_SUCCESS:GetLastError();
    if(wait==WAIT_TIMEOUT) return ERROR_TIMEOUT;
    if(wait==WAIT_FAILED) return GetLastError();
    DWORD exitCode=0;
    if(GetExitCodeProcess(child.h,&exitCode) && exitCode==13) return ERROR_ACCESS_DENIED;
    return ERROR_PROCESS_ABORTED;
}
bool launchElevated(const std::wstring& exe, const std::wstring& sid) {
    const auto args = L"--elevated-user " + mg_host::quote(sid), dir = mg_host::directory(exe);
    return reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"runas", exe.c_str(), args.c_str(), dir.c_str(), SW_SHOWNORMAL)) > 32;
}
// -1 means continue into the existing native UI. All helper invocations exit.
int preflight(const Args& args) {
    const auto id = currentIdentity();
    if (!args.empty()) {
        if (args[0] == L"--restart-broker") return restartBroker(args, id);
        if (args[0] == L"--configure-startup") return configureStartup(args, id);
        if (args[0] == L"--restart-wait") {
            std::uint32_t pid = 0; bool admin = false;
            const bool handoff=args.size()==5;
            if (!(handoff?mg_host::handoffArgs(args,id.sid,pid,admin):mg_host::restartArgs(args,id.sid,pid,admin)) || !id.elevationKnown || admin!=id.elevated) return 13;
            // Set token-derived environment before acknowledging readiness.
            if (!SetEnvironmentVariableW(L"MINIGESTURES_USER_SID", id.sid.c_str()) || !SetEnvironmentVariableW(L"MINIGESTURES_ELEVATED",id.elevated?L"1":L"0")) return 11;
            if (!waitForRestart(pid,handoff?args[4]:std::wstring{})) return 13;
        }
        if (args[0] == L"--elevated-user" && (args.size() != 2 || id.sid.empty() || args[1] != id.sid || !id.elevated)) return 12;
    }
    // Overwrite any inherited values using the real process token.
    if (!SetEnvironmentVariableW(L"MINIGESTURES_USER_SID", id.sid.c_str()) ||
        !SetEnvironmentVariableW(L"MINIGESTURES_ELEVATED", id.elevated ? L"1" : L"0")) return 11;
    if (!args.empty() || id.elevated || id.sid.empty() || !id.elevationKnown) return -1;
    const auto exe = executable(), dir = mg_host::directory(exe);
    if (exe.empty() || dir.empty() || !NativeRequestedElevation((dir + L"\\miniGestures.json").c_str())) return -1;
    HWND existing = FindWindowW(L"miniGesturesController", nullptr);
    if (!existing) existing = FindWindowW(L"miniGesturesMain", nullptr);
    if (existing) return PostMessageW(existing, WM_APP + 0x11, 0, 0) ? 0 : -1;
    return launchElevated(exe, id.sid) ? 0 : -1;
}
Args getArgs() {
    int count = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!argv) throw 11;
    struct Free { LPWSTR* p; ~Free() { LocalFree(p); } } cleanup{argv};
    Args args;
    for (int i = 1; i < count; ++i) args.emplace_back(argv[i]);
    return args;
}
}

// Respect the existing Windows Print Screen preference. Windows 11 defaults
// to opening screen snipping when the per-user value has not been created.
extern "C" int WINAPI NativeScreenClipEnabled() {
    DWORD value=0, size=sizeof(value);
    const LSTATUS result=RegGetValueW(HKEY_CURRENT_USER,L"Control Panel\\Keyboard",L"PrintScreenKeyForSnippingEnabled",RRF_RT_REG_DWORD,nullptr,&value,&size);
    if(result==ERROR_SUCCESS) return value!=0;
    if(result!=ERROR_FILE_NOT_FOUND && result!=ERROR_PATH_NOT_FOUND) return 0;
    // Use the real OS build, not a manifest-sensitive compatibility version.
    struct OSVersion { ULONG size, major, minor, build, platform; WCHAR servicePack[128]; } v{};
    v.size=sizeof(v);
    using VersionFn=LONG(WINAPI*)(OSVersion*);
    auto version=reinterpret_cast<VersionFn>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"RtlGetVersion"));
    return version&&version(&v)==0&&v.major>=10&&v.build>=22000;
}
namespace {
volatile LONG screenClipLaunching=0;
DWORD WINAPI screenClipWorker(void*) {
    DWORD foregroundPid=0;GetWindowThreadProcessId(GetForegroundWindow(),&foregroundPid);
    if(foregroundPid!=GetCurrentProcessId()){InterlockedExchange(&screenClipLaunching,0);return 0;}
    bool launched=false;
    try {
        const auto id=currentIdentity();
        // Use the same shell protocol as Windows' physical Print Screen
        // shortcut. Do not request a result, capture pixels ourselves or
        // register a new protocol handler.
        const wchar_t* uri=L"ms-screenclip://?source=PrintScreen";
        if(id.elevationKnown&&id.elevated){
            std::wstring stage;
            launched=startThroughExplorer(uri,{},id.sid,stage,true)==ERROR_SUCCESS;
        } else if(id.elevationKnown) {
            ShellApartment apartment;
            SHELLEXECUTEINFOW info{};info.cbSize=sizeof(info);info.fMask=SEE_MASK_FLAG_NO_UI|SEE_MASK_NOASYNC;
            info.lpVerb=L"open";info.lpFile=uri;info.nShow=SW_SHOWNORMAL;
            launched=ShellExecuteExW(&info)!=FALSE;
        }
    } catch (...) {}
    if(!launched){
        // On a missing protocol or failed Explorer launch, preserve the
        // normal Print Screen behavior, only while our window is still active.
        DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);
        if(pid==GetCurrentProcessId()&&!((GetAsyncKeyState(VK_SHIFT)|GetAsyncKeyState(VK_CONTROL)|GetAsyncKeyState(VK_MENU)|GetAsyncKeyState(VK_LWIN)|GetAsyncKeyState(VK_RWIN))&0x8000)){
            INPUT keys[2]{};
            for(auto& key:keys){key.type=INPUT_KEYBOARD;key.ki.wVk=VK_SNAPSHOT;key.ki.dwFlags=KEYEVENTF_EXTENDEDKEY;key.ki.dwExtraInfo=0x53544c495445ULL;}
            keys[1].ki.dwFlags|=KEYEVENTF_KEYUP;SendInput(2,keys,sizeof(INPUT));
        }
    }
    InterlockedExchange(&screenClipLaunching,0);return 0;
}
}
extern "C" int WINAPI NativeQueueScreenClip(){
    if(InterlockedCompareExchange(&screenClipLaunching,1,0)!=0)return 1;
    HANDLE thread=CreateThread(nullptr,0,screenClipWorker,nullptr,0,nullptr);
    if(!thread){InterlockedExchange(&screenClipLaunching,0);return 0;}
    CloseHandle(thread);return 1;
}

#include "update_host.hpp"
extern "C" int WINAPI NativeUpdateStart(){try{return mg_update::start(executable())?1:0;}catch(...){mg_update::cancel();return 0;}}
extern "C" int WINAPI NativeUpdateLatest(wchar_t* out,int cap){if(!out||cap<18)return 0;int i=0;for(;i<17&&mg_update::latest[i];++i)out[i]=mg_update::latest[i];out[i]=0;return i!=0;}
static int copyUpdateText(const std::wstring& text,wchar_t* out,int cap){if(!out||cap<1)return 0;size_t n=text.size();if(n>=static_cast<size_t>(cap))n=cap-1;for(size_t i=0;i<n;++i)out[i]=text[i];out[n]=0;return static_cast<int>(n);}
extern "C" int WINAPI NativeUpdateNotes(wchar_t* out,int cap){return copyUpdateText(mg_update::notes,out,cap);}
extern "C" int WINAPI NativeUpdateError(wchar_t* out,int cap){return copyUpdateText(mg_update::error,out,cap);}
extern "C" int WINAPI NativeUpdatePoll(){try{return mg_update::poll();}catch(...){mg_update::cancel();return 3;}}
extern "C" void WINAPI NativeUpdateCancel(){mg_update::cancel();}
extern "C" int WINAPI NativeUpdateMinute(){FILETIME ft{};GetSystemTimeAsFileTime(&ft);ULARGE_INTEGER v{};v.LowPart=ft.dwLowDateTime;v.HighPart=ft.dwHighDateTime;return static_cast<int>((v.QuadPart-116444736000000000ULL)/600000000ULL);}
// SDK boundary for the freestanding UI translation unit. Keep errors visible
// and do not close the old instance if UAC or child preflight fails.
extern "C" DWORD WINAPI NativeRestartApplication(HWND owner, int admin, wchar_t* detail, DWORD capacity) {
    if (detail && capacity) detail[0]=0;
    DWORD error=ERROR_GEN_FAILURE;
    try {
        std::wstring stage;
        error=restartDirect(owner,admin!=0,stage);
        if (error && detail && capacity) {
            wchar_t system[512]{};
            FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,nullptr,error,0,system,512,nullptr);
            const auto message=stage+L" ("+std::to_wstring(error)+L")\n"+system;
            lstrcpynW(detail,message.c_str(),static_cast<int>(capacity));
        }
    } catch (...) { error=ERROR_GEN_FAILURE; }
    return error;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    // The native core retains its asynchronous workers and shutdown watchdog.
    // The standard Microsoft CRT initializes first; the UI runs directly on
    // this thread with the same 8 MiB stack reservation as the old Go host.
    int code;
    try { code = preflight(getArgs()); } catch (...) { return 11; }
    if (code >= 0) return code;
    code = static_cast<int>(NativeStart(nullptr));
    // Preserve the established shutdown behavior, including timeout exit codes.
    // NativeStart already joins workers and saves configuration where safe.
    TerminateProcess(GetCurrentProcess(), static_cast<UINT>(code));
    return code;
}


// The notification area uses SystemUsesLightTheme, independently of app themes.
extern "C" int WINAPI NativeTrayDark() {
    DWORD light=1, size=sizeof(light);
    const wchar_t* key=L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
    if(RegGetValueW(HKEY_CURRENT_USER,key,L"SystemUsesLightTheme",RRF_RT_REG_DWORD,nullptr,&light,&size)!=ERROR_SUCCESS)
        return 0;
    return light==0;
}

// Generate and cache a contrasting tray-only icon from each existing resource.
// DrawIconEx supplies premultiplied alpha; invert neutral tones within alpha,
// preserving transparent edges and colored pause/update badges.
extern "C" HICON WINAPI NativeInvertTrayIcon(HICON original) {
    if(!original)return nullptr;
    ICONINFO source{};
    if(!GetIconInfo(original,&source))return nullptr;
    HICON result=nullptr;
    BITMAP bm{};
    if(source.hbmColor && GetObjectW(source.hbmColor,sizeof(bm),&bm)==sizeof(bm) &&
       bm.bmWidth>0 && bm.bmHeight>0 && bm.bmWidth<=256 && bm.bmHeight<=256) {
        const int w=bm.bmWidth,h=bm.bmHeight;
        BITMAPINFO info{};
        info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;
        info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
        info.bmiHeader.biCompression=BI_RGB;
        void* bits=nullptr;
        HDC dc=CreateCompatibleDC(nullptr);
        HBITMAP color=dc?CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0):nullptr;
        if(color&&bits){
            HGDIOBJ old=SelectObject(dc,color);
            if(old&&old!=HGDI_ERROR){
                auto* pixels=static_cast<unsigned char*>(bits);
                ZeroMemory(pixels,static_cast<size_t>(w)*h*4);
                const BOOL drawn=DrawIconEx(dc,0,0,original,w,h,0,nullptr,DI_NORMAL);
                GdiFlush();
                SelectObject(dc,old);
                if(drawn){
                    bool hasAlpha=false;
                    for(int i=0;i<w*h;++i){
                        auto* p=pixels+i*4;const unsigned a=p[3];
                        if(!a)continue;
                        hasAlpha=true;
                        unsigned hi=p[0],lo=p[0];
                        for(int c=1;c<3;++c){if(p[c]>hi)hi=p[c];if(p[c]<lo)lo=p[c];}
                        if((hi-lo)*255u<=a*24u)
                            for(int c=0;c<3;++c)p[c]=static_cast<unsigned char>(a-(p[c]>a?a:p[c]));
                    }
                    if(hasAlpha){
                        ICONINFO out=source;out.hbmColor=color;
                        result=CreateIconIndirect(&out);
                    }
                }
            }
        }
        if(color)DeleteObject(color);
        if(dc)DeleteDC(dc);
    }
    if(source.hbmColor)DeleteObject(source.hbmColor);
    if(source.hbmMask)DeleteObject(source.hbmMask);
    return result;
}
extern "C" void WINAPI NativeReleaseTrayIcon(HICON icon){if(icon)DestroyIcon(icon);}


// Toggle ordinary hidden files using the same shell setting and view-refresh
// method as Hidden Files Hotkey. Executed by the action worker, never the hook
// or editor UI thread. No Explorer restart, injected keys, or Windhawk runtime.
namespace {
SRWLOCK hiddenFilesLock=SRWLOCK_INIT;

void refreshHiddenFileViews(){
    Com<IShellWindows> windows;
    if(FAILED(CoCreateInstance(CLSID_ShellWindows,nullptr,CLSCTX_LOCAL_SERVER,
                              IID_PPV_ARGS(windows.out())))||!windows.p)return;
    long count=0;
    if(FAILED(windows->get_Count(&count)))return;
    for(long i=0;i<count;++i){
        VARIANT index{};index.vt=VT_I4;index.lVal=i;
        Com<IDispatch> dispatch;
        if(FAILED(windows->Item(index,dispatch.out()))||!dispatch.p)continue;
        Com<IServiceProvider> provider;
        if(FAILED(dispatch->QueryInterface(IID_PPV_ARGS(provider.out())))||!provider.p)continue;
        Com<IShellBrowser> browser;
        if(FAILED(provider->QueryService(SID_STopLevelBrowser,IID_PPV_ARGS(browser.out())))||!browser.p)continue;
        Com<IShellView> view;
        if(SUCCEEDED(browser->QueryActiveShellView(view.out()))&&view.p)view->Refresh();
    }
}

BOOL CALLBACK refreshHiddenFileWindow(HWND window,LPARAM){
    wchar_t name[64]{};
    if(GetClassNameW(window,name,static_cast<int>(sizeof(name)/sizeof(name[0])))&&
       (lstrcmpW(name,L"CabinetWClass")==0||lstrcmpW(name,L"ExploreWClass")==0))
        PostMessageW(window,WM_COMMAND,41504,0);
    return TRUE;
}
}

extern "C" int WINAPI NativeToggleHiddenFiles(int includeProtected){
    // Serialize the read/modify/write even when gestures run in parallel.
    AcquireSRWLockExclusive(&hiddenFilesLock);
    const DWORD mask=SSF_SHOWALLOBJECTS|(includeProtected?SSF_SHOWSUPERHIDDEN:0);
    SHELLSTATE state{};
    SHGetSetSettings(&state,mask,FALSE);
    // If either setting is off, reveal both. Only hide both when both
    // are already enabled. The ordinary action never changes system files.
    const bool show=!(state.fShowAllObjects&&(!includeProtected||state.fShowSuperHidden));
    // SHELLSTATE uses one-bit BOOL fields, so assign the Boolean directly.
    state.fShowAllObjects=show;
    if(includeProtected)state.fShowSuperHidden=show;
    SHGetSetSettings(&state,mask,TRUE);
    SHELLSTATE actual{};
    SHGetSetSettings(&actual,mask,FALSE);
    const bool applied=((actual.fShowAllObjects!=0)==show)&&
                       (!includeProtected||((actual.fShowSuperHidden!=0)==show));
    ReleaseSRWLockExclusive(&hiddenFilesLock);
    // Refresh all currently open views. The asynchronous fallback covers
    // windows not exposed by the COM enumeration.
    SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST,nullptr,nullptr);
    ComApartment apartment;
    if(SUCCEEDED(apartment.hr)||apartment.hr==RPC_E_CHANGED_MODE)refreshHiddenFileViews();
    EnumWindows(refreshHiddenFileWindow,0);
    return applied?1:0;
}
