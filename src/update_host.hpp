#pragma once
// Read-only GitHub release metadata check. No package download or installation.
// The fixed PowerShell command treats release notes as data only.
namespace mg_update {
static bool validVersionText(const char* s,size_t n){
 if(n<5||n>17)return false;unsigned parts=1,digits=0;
 for(size_t i=0;i<n;++i){char c=s[i];if(c=='.'){if(!digits||parts==3)return false;++parts;digits=0;}else if(c>='0'&&c<='9'){if(++digits>5)return false;}else return false;}
 return parts==3&&digits!=0;
}
static HANDLE worker=nullptr;
static std::wstring stage;
static wchar_t latest[18]{};
static std::wstring notes, error;
static std::wstring readText(const wchar_t* name){
 if(stage.empty())return {};
 HANDLE f=CreateFileW((stage+L"\\"+name).c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(f==INVALID_HANDLE_VALUE)return {};
 LARGE_INTEGER size{};if(!GetFileSizeEx(f,&size)||size.QuadPart<=0||size.QuadPart>262144){CloseHandle(f);return {};}
 std::string bytes(static_cast<size_t>(size.QuadPart),'\0');DWORD n=0;BOOL ok=ReadFile(f,&bytes[0],static_cast<DWORD>(bytes.size()),&n,nullptr);CloseHandle(f);
 if(!ok||n!=bytes.size())return {};
 int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes.data(),static_cast<int>(n),nullptr,0);if(count<=0)return {};
 std::wstring result(count,L'\0');if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes.data(),static_cast<int>(n),&result[0],count))return {};
 std::wstring normalized;for(wchar_t c:result){if(c==L'\r'||c==0)continue;if(c==L'\n')normalized+=L'\r';normalized+=c;}
 return normalized;
}
static void readLatest(){
 if(stage.empty()||latest[0])return;
 HANDLE f=CreateFileW((stage+L"\\latest.version").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(f==INVALID_HANDLE_VALUE)return;
 char b[18]{};DWORD n=0;BOOL ok=ReadFile(f,b,sizeof(b),&n,nullptr);CloseHandle(f);
 if(!ok||!validVersionText(b,n))return;
 for(DWORD i=0;i<n;++i)latest[i]=static_cast<wchar_t>(b[i]);latest[n]=0;
}
static ULONGLONG started=0;
static std::wstring encoded(const std::wstring& s){
 const char* alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
 const auto* p=reinterpret_cast<const unsigned char*>(s.data());size_t n=s.size()*sizeof(wchar_t);std::wstring out;
 for(size_t i=0;i<n;i+=3){unsigned v=unsigned(p[i])<<16;if(i+1<n)v|=unsigned(p[i+1])<<8;if(i+2<n)v|=p[i+2];out+=alphabet[(v>>18)&63];out+=alphabet[(v>>12)&63];out+=i+1<n?alphabet[(v>>6)&63]:'=';out+=i+2<n?alphabet[v&63]:'=';}return out;
}
static std::wstring literal(const std::wstring& s){return L"[Text.Encoding]::Unicode.GetString([Convert]::FromBase64String('"+encoded(s)+L"'))";}
static HANDLE launch(const std::wstring& script){wchar_t sys[32768];UINT n=GetSystemDirectoryW(sys,32768);if(!n||n>=32768)return nullptr;
 std::wstring ps=std::wstring(sys,n)+L"\\WindowsPowerShell\\v1.0\\powershell.exe";
 std::wstring cmd=mg_host::quote(ps)+L" -NoLogo -NoProfile -NonInteractive -EncodedCommand "+encoded(script);
 if(cmd.size()>=32767)return nullptr;STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};
 if(!CreateProcessW(ps.c_str(),&cmd[0],nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&si,&pi))return nullptr;CloseHandle(pi.hThread);return pi.hProcess;
}
static void clean(){if(stage.empty())return;for(auto name:{L"latest.version",L"release.notes",L"update.error"})DeleteFileW((stage+L"\\"+name).c_str());RemoveDirectoryW(stage.c_str());stage.clear();}
static bool start(const std::wstring& path){if(worker||!stage.empty()||path.empty())return false;latest[0]=0;error.clear();GUID g{};wchar_t id[40]{};if(FAILED(CoCreateGuid(&g))||!StringFromGUID2(g,id,40))return false;
 stage=mg_host::directory(path)+L"\\.miniGestures-update-"+id;
 if(!CreateDirectoryW(stage.c_str(),nullptr)){error=L"CreateDirectoryW: "+std::to_wstring(GetLastError());stage.clear();return false;}
 std::wstring script=L"$ErrorActionPreference='Stop'; $stage="+literal(stage)+L"; $exe="+literal(path)+L";\n"+LR"PS(
try {
 [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
 $headers=@{'User-Agent'='miniGestures-Updater'; 'Accept'='application/vnd.github+json'}
 $r=Invoke-RestMethod -Uri 'https://api.github.com/repos/iamfromreallife/miniGestures/releases/latest' -Headers $headers -TimeoutSec 25
 if($r.draft -or $r.prerelease){throw 'Not a stable release'}
 if($r.tag_name -cmatch '^v?(\d{1,5}\.\d{1,5}\.\d{1,5})(?:[_-][A-Za-z0-9._-]{1,80})?$'){$v=[version]$Matches[1]}else{throw 'Invalid stable release tag'}; $installed=[Diagnostics.FileVersionInfo]::GetVersionInfo($exe)
 [IO.File]::WriteAllText((Join-Path $stage 'latest.version'),$v.ToString(),[Text.Encoding]::ASCII)
 $current=[version]::new($installed.FileMajorPart,$installed.FileMinorPart,$installed.FileBuildPart)
 if($v -le $current){exit 10}
 $body=[string]$r.body
 if($body.Length -gt 60000){$body=$body.Substring(0,60000)+"`r`n[Release notes truncated]"}
 [IO.File]::WriteAllText((Join-Path $stage 'release.notes'),$body,[Text.UTF8Encoding]::new($false))
 exit 0
} catch {try{[IO.File]::WriteAllText((Join-Path $stage 'update.error'),$_.Exception.Message,[Text.UTF8Encoding]::new($false))}catch{};exit 11}
)PS";
 worker=launch(script);if(!worker){error=L"Could not start Windows PowerShell.";clean();return false;}started=GetTickCount64();return true;
}
// 0 pending, 1 newer release, 2 up to date, 3 error.
static int poll(){
 if(!worker)return 3;
 DWORD wait=WaitForSingleObject(worker,0);
 if(wait==WAIT_TIMEOUT){if(GetTickCount64()-started<45000)return 0;TerminateProcess(worker,11);WaitForSingleObject(worker,5000);error=L"Update request timed out.";}
 DWORD code=11;if(wait==WAIT_OBJECT_0)GetExitCodeProcess(worker,&code);CloseHandle(worker);worker=nullptr;
 if(code==0||code==10){readLatest();if(!latest[0]){error=L"Missing or invalid release version.";code=11;}else if(code==0){notes=readText(L"release.notes");}else notes.clear();}
 if(code!=0&&code!=10&&error.empty()){error=readText(L"update.error");if(error.empty())error=L"PowerShell worker exit code: "+std::to_wstring(code);}
 clean();return code==0?1:code==10?2:3;
}
static void cancel(){if(worker){TerminateProcess(worker,11);WaitForSingleObject(worker,5000);CloseHandle(worker);worker=nullptr;}clean();}
}
