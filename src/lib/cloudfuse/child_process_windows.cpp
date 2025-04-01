/*
   Licensed under the MIT License <http://opensource.org/licenses/MIT>.

   Copyright © 2024 Seagate Technology LLC and/or its Affiliates

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE
*/

#ifdef _WIN32
#include "child_process.h"

#ifndef UNICODE
#define UNICODE
#define UNICODE_WAS_UNDEFINED
#endif
#include <windows.h>
#ifdef UNICODE_WAS_UNDEFINED
#undef UNICODE
#endif

#include <filesystem>
namespace fs = std::filesystem;

#include <codecvt>
#include <fstream>
#include <locale>

// Return available drive letter to mount
std::string getAvailableDriveLetter()
{
    DWORD driveMask = GetLogicalDrives();
    for (char letter = 'Z'; letter >= 'A'; --letter)
    {
        if (!(driveMask & (1 << (letter - 'A'))))
        {
            return std::string(1, letter) + ":";
        }
    }
    return "Z:";
}

std::string getSystemName()
{
    std::string systemName;
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName) / sizeof(char);
    if (GetComputerNameA(computerName, &size))
    {
        systemName = computerName;
    }

    return systemName;
}

CloudfuseMngr::CloudfuseMngr()
{
    // generate template contents
    std::string systemName = getSystemName();
    // NOTE: increment the version number when the config template changes
    templateVersionString = "template-version: 0.5";
    config_template = templateVersionString + R"(
allow-other: true
logging:
  type: base
  max-file-size-mb: 32

components:
- libfuse
- file_cache
- attr_cache
- s3storage

libfuse:
  attribute-expiration-sec: 1800
  entry-expiration-sec: 1800
  negative-entry-expiration-sec: 1800
  ignore-open-flags: true
  network-share: true
  display-capacity-mb: { DISPLAY_CAPACITY }

file_cache:
  path: { 0 }
  timeout-sec: 180
  allow-non-empty-temp: true
  cleanup-on-start: false

attr_cache:
  timeout-sec: 3600

s3storage:
  key-id: { AWS_ACCESS_KEY_ID }
  secret-key: { AWS_SECRET_ACCESS_KEY }
  bucket-name: { BUCKET_NAME }
  endpoint: { ENDPOINT }
  subdirectory: )" + systemName +
                      "\n";

    std::string appdataEnv;
    char *buf = nullptr;
    size_t len;
    if (_dupenv_s(&buf, &len, "APPDATA") == 0 && buf != nullptr)
    {
        appdataEnv = std::string(buf);
        free(buf);
    }
    else
    {
        appdataEnv = "";
    }

    const fs::path appdata(appdataEnv);
    const fs::path fileCacheDirPath = appdata / fs::path("Cloudfuse\\cloudfuse_cache");
    const fs::path configFilePath = appdata / fs::path("Cloudfuse\\nx_plugin_config.aes");
    const fs::path templateFilePath = appdata / fs::path("Cloudfuse\\nx_plugin_config.yaml");

    mountDir = getAvailableDriveLetter();
    fileCacheDir = fileCacheDirPath.generic_string();
    configFile = configFilePath.generic_string();
    templateFile = templateFilePath.generic_string();

    if (!templateValid())
    {
        writeTemplate();
    }
}

processReturn CloudfuseMngr::spawnProcess(wchar_t *argv, std::wstring envp)
{
    processReturn ret;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    // Set the bInheritHandle flag so pipe handles are inherited.
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE hReadStdOut, hWriteStdOut, hReadStdErr, hWriteStdErr;
    // Create a pipe for the child process's STDOUT.
    if (!CreatePipe(&hReadStdOut, &hWriteStdOut, &sa, 0))
    {
        ret.errCode = 1;
        return ret;
    }
    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if (!SetHandleInformation(hReadStdOut, HANDLE_FLAG_INHERIT, 0))
    {
        ret.errCode = 1;
        return ret;
    }

    // Create a pipe for the child process's STDERR.
    if (!CreatePipe(&hReadStdErr, &hWriteStdErr, &sa, 0))
    {
        ret.errCode = 1;
        return ret;
    }
    // Ensure the read handle to the pipe for STDERR is not inherited.
    if (!SetHandleInformation(hReadStdErr, HANDLE_FLAG_INHERIT, 0))
    {
        ret.errCode = 1;
        return ret;
    }

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdOutput = hWriteStdOut;
    si.hStdError = hWriteStdErr;
    si.dwFlags |= STARTF_USESTDHANDLES;
    ZeroMemory(&pi, sizeof(pi));

    LPVOID lpvEnv = GetEnvironmentStringsW();

    // If the returned pointer is NULL, exit.
    if (lpvEnv == NULL)
    {
        printf("GetEnvironmentStrings failed (%d)\n", GetLastError());
        ret.errCode = 1;
        return ret;
    }

    // Append current environment to new environment variables
    LPTSTR lpszVariable = (LPTSTR)lpvEnv;
    while (*lpszVariable)
    {
        envp += lpszVariable;
        envp += L'\0';
        lpszVariable += lstrlen(lpszVariable) + 1;
    }

    // Environment block must be double null terminated
    envp += L'\0';

    // Start the child process.
    if (!CreateProcessW(NULL,                       // No module name (use command line)
                        LPWSTR(argv),               // Command line
                        NULL,                       // Process handle not inheritable
                        NULL,                       // Thread handle not inheritable
                        TRUE,                       // Set handle inheritance to FALSE
                        CREATE_UNICODE_ENVIRONMENT, // Use unicode environment
                        (LPVOID)envp.c_str(),       // Use new environment
                        NULL,                       // Use parent's starting directory
                        &si,                        // Pointer to STARTUPINFO structure
                        &pi)                        // Pointer to PROCESS_INFORMATION structure
    )
    {
        CloseHandle(hWriteStdOut);
        CloseHandle(hReadStdOut);
        CloseHandle(hWriteStdErr);
        CloseHandle(hReadStdErr);
        printf("CreateProcess failed (%d).\n", GetLastError());
        ret.errCode = 1;
        return ret;
    }

    CloseHandle(hWriteStdOut);
    CloseHandle(hWriteStdErr);

    // Wait until child process exits.
    unsigned long errCode;
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &errCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    ret.errCode = errCode;

    const int BUFSIZE = 4096;
    DWORD bytesRead;
    CHAR buffer[BUFSIZE];
    while (ReadFile(hReadStdOut, buffer, BUFSIZE - 1, &bytesRead, NULL) && bytesRead > 0)
    {
        buffer[bytesRead] = 0;
        ret.output.append(buffer);
    }
    while (ReadFile(hReadStdErr, buffer, BUFSIZE - 1, &bytesRead, NULL) && bytesRead > 0)
    {
        buffer[bytesRead] = 0;
        ret.output.append(buffer);
    }

    // Close handles.
    CloseHandle(hReadStdOut);
    CloseHandle(hReadStdErr);
    return ret;
}

processReturn CloudfuseMngr::genS3Config(const std::string accessKeyId, const std::string secretAccessKey,
                                         const std::string endpoint, const std::string bucketName,
                                         const uint64_t bucketSizeMb, const std::string passphrase)
{
    if (!templateValid() && !writeTemplate())
    {
        return processReturn{1, "Failed to overwrite invalid template file: " + templateFile};
    }
    const std::string argv = "cloudfuse gen-config --config-file=" + templateFile + " --output-file=" + configFile +
                             " --temp-path=" + fileCacheDir + " --passphrase=" + passphrase;
    const std::string aws_access_key_id_env = "AWS_ACCESS_KEY_ID=" + accessKeyId;
    const std::string aws_secret_access_key_env = "AWS_SECRET_ACCESS_KEY=" + secretAccessKey;
    const std::string endpoint_env = "ENDPOINT=" + endpoint;
    const std::string bucket_name_env = "BUCKET_NAME=" + bucketName;
    const std::string bucket_size_env = "DISPLAY_CAPACITY=" + std::to_string(bucketSizeMb);
    const std::string envp = aws_access_key_id_env + '\0' + aws_secret_access_key_env + '\0' + endpoint_env + '\0' +
                             bucket_name_env + '\0' + bucket_size_env + '\0';

    const std::wstring wargv = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(argv);
    const std::wstring wenvp = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(envp);

    return spawnProcess(const_cast<wchar_t *>(wargv.c_str()), wenvp);
}

processReturn CloudfuseMngr::dryRun(const std::string passphrase)
{
    const std::string argv =
        "cloudfuse mount " + mountDir + " --config-file=" + configFile + " --passphrase=" + passphrase + " --dry-run";
    const std::string envp = "";

    const std::wstring wargv = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(argv);
    const std::wstring wenvp = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(envp);

    return spawnProcess(const_cast<wchar_t *>(wargv.c_str()), wenvp);
}

processReturn CloudfuseMngr::mount(const std::string passphrase)
{
    const std::string argv =
        "cloudfuse mount " + mountDir + " --config-file=" + configFile + " --passphrase=" + passphrase;
    const std::string envp = "";

    const std::wstring wargv = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(argv);
    const std::wstring wenvp = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(envp);

    return spawnProcess(const_cast<wchar_t *>(wargv.c_str()), wenvp);
}

processReturn CloudfuseMngr::unmount()
{
    const std::string argv = "cloudfuse unmount " + mountDir;
    const std::string envp = "";

    const std::wstring wargv = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(argv);
    const std::wstring wenvp = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(envp);

    return spawnProcess(const_cast<wchar_t *>(wargv.c_str()), wenvp);
}

bool CloudfuseMngr::isInstalled()
{
    const std::string argv = "cloudfuse version";
    const std::string envp = "";

    const std::wstring wargv = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(argv);
    const std::wstring wenvp = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(envp);

    return spawnProcess(const_cast<wchar_t *>(wargv.c_str()), wenvp).errCode == 0;
}

bool CloudfuseMngr::isMounted()
{
    const std::wstring mountDirW = std::wstring(mountDir.begin(), mountDir.end());
    const DWORD fileAttributes = GetFileAttributes(mountDirW.c_str());
    if (fileAttributes == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }
    return true;
}

#endif
