#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace mg_host {
using Args = std::vector<std::wstring>;
// CommandLineToArgvW / MS CRT-compatible quoting, including trailing backslashes.
inline std::wstring quote(const std::wstring& arg) {
    std::wstring out = L"\"";
    std::size_t slashes = 0;
    for (wchar_t c : arg) {
        if (c == L'\\') { ++slashes; continue; }
        if (c == L'"') { out.append(slashes * 2 + 1, L'\\'); out += c; }
        else { out.append(slashes, L'\\'); out += c; }
        slashes = 0;
    }
    out.append(slashes * 2, L'\\');
    out += L'"';
    return out;
}
inline std::wstring commandLine(const std::wstring& exe, const Args& args) {
    std::wstring out = quote(exe);
    for (const auto& a : args) { out += L' '; out += quote(a); }
    return out;
}
inline std::wstring directory(const std::wstring& path) {
    auto pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return {};
    // Preserve the trailing slash of drive roots (C:\ or \\?\C:\).
    if (pos && path[pos - 1] == L':') ++pos;
    return path.substr(0, pos);
}
inline bool binaryFlag(const std::wstring& s) { return s == L"0" || s == L"1"; }
inline bool restartArgs(const Args& args, const std::wstring& sid, std::uint32_t& pid, bool& admin) {
    if (args.size() != 4 || sid.empty() || args[3] != sid || !binaryFlag(args[2]) || args[1].empty()) return false;
    std::uint64_t value = 0;
    for (auto ch : args[1]) {
        if (ch < L'0' || ch > L'9') return false;
        value = value * 10 + (ch - L'0');
        if (value > 0xffffffffULL) return false;
    }
    if (!value) return false;
    pid = static_cast<std::uint32_t>(value); admin = args[2] == L"1";
    return true;
}
// Fresh per-restart event names are restricted to the exact GUID format.
inline bool restartEventName(const std::wstring& name) {
    const std::wstring prefix = L"Local\\miniGestures.restart.";
    if (name.size() != prefix.size() + 38 || name.compare(0, prefix.size(), prefix) != 0) return false;
    const auto guid = name.substr(prefix.size());
    if (guid.front() != L'{' || guid.back() != L'}') return false;
    for (std::size_t i=1;i<37;++i) {
        const auto c=guid[i];
        if (i==9||i==14||i==19||i==24) { if(c!=L'-') return false; }
        else if (!((c>=L'0'&&c<=L'9')||(c>=L'a'&&c<=L'f')||(c>=L'A'&&c<=L'F'))) return false;
    }
    return true;
}
inline std::wstring commitEventName(const std::wstring& ready) { return ready + L".commit"; }
inline bool handoffArgs(const Args& args, const std::wstring& sid, std::uint32_t& pid, bool& admin) {
    return args.size()==5 && args[0]==L"--restart-wait" && restartEventName(args[4]) &&
        restartArgs(Args(args.begin(),args.begin()+4),sid,pid,admin);
}
inline bool configureArgs(const Args& args, const std::wstring& sid) {
    return args.size() == 4 && !sid.empty() && args[3] == sid && binaryFlag(args[1]) && binaryFlag(args[2]);
}
}
