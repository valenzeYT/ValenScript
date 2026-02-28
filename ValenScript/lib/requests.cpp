#include <array>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace {
std::string runPowerShell(const std::string& script) {
    std::string command = "powershell -NoProfile -Command \"" + script + "\" 2>&1";
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("requests: failed to start PowerShell");
    }

    std::string output;
    std::array<char, 512> buffer{};
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }

    int exitCode = _pclose(pipe);
    if (exitCode != 0) {
        if (output.empty()) {
            throw std::runtime_error("requests: PowerShell request failed");
        }
        throw std::runtime_error(output);
    }
    return output;
}

std::string request(const std::string& method, const std::string& url, const std::string& body) {
    _putenv_s("VALEN_REQ_METHOD", method.c_str());
    _putenv_s("VALEN_REQ_URL", url.c_str());
    _putenv_s("VALEN_REQ_BODY", body.c_str());

    const std::string script =
        "$ErrorActionPreference='Stop'; "
        "$m=$env:VALEN_REQ_METHOD; $u=$env:VALEN_REQ_URL; $b=$env:VALEN_REQ_BODY; "
        "try { "
        "if ($m -eq 'GET' -or $m -eq 'DELETE' -or [string]::IsNullOrEmpty($b)) { "
        "$r=Invoke-WebRequest -UseBasicParsing -Method $m -Uri $u; "
        "} else { "
        "$r=Invoke-WebRequest -UseBasicParsing -Method $m -Uri $u -Body $b -ContentType 'application/json'; "
        "} "
        "[Console]::Out.Write($r.Content); "
        "} catch { "
        "if ($_.Exception.Response -and $_.Exception.Response.GetResponseStream()) { "
        "$sr=New-Object System.IO.StreamReader($_.Exception.Response.GetResponseStream()); "
        "[Console]::Out.Write($sr.ReadToEnd()); "
        "} else { throw } "
        "}";

    return runPowerShell(script);
}
} // namespace

namespace requests_lib {

std::string get(const std::string& url) { return request("GET", url, ""); }
std::string del(const std::string& url) { return request("DELETE", url, ""); }
std::string post(const std::string& url, const std::string& body) { return request("POST", url, body); }
std::string put(const std::string& url, const std::string& body) { return request("PUT", url, body); }
std::string patch(const std::string& url, const std::string& body) { return request("PATCH", url, body); }

void download(const std::string& url, const std::string& outPath) {
    _putenv_s("VALEN_REQ_URL", url.c_str());
    _putenv_s("VALEN_REQ_OUT", outPath.c_str());
    const std::string script =
        "$ErrorActionPreference='Stop'; "
        "Invoke-WebRequest -UseBasicParsing -Uri $env:VALEN_REQ_URL -OutFile $env:VALEN_REQ_OUT";
    runPowerShell(script);
}

} // namespace requests_lib
