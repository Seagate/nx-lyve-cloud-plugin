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

#include <codecvt>
#include <fstream>
#include <locale>

std::string config_template = R"(
allow-other: true
logging:
  level: log_err
  type: syslog

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
  region: { AWS_REGION }
)";

CloudfuseMngr::CloudfuseMngr() {
    std::string appdataEnv;
    const char *appdata = std::getenv("APPDATA");
    std::string appdataString = std::string(appdata);
    if (appdata == nullptr) {
        appdataEnv = "";
    } else {
        appdataEnv = appdataString;
    }
    mountDir = "Z:";
    fileCacheDir = appdataEnv + "\\.cloudfuse\\cloudfuse_cache";
    configFile = appdataEnv + "\\.cloudfuse\\nx_plugin_config.aes";
    templateFile = appdataEnv + "\\.cloudfuse\\nx_plugin_config.yaml";

    std::ifstream in(templateFile.c_str());
    if (!in.good()) {
        std::ofstream out(templateFile.c_str());
        out << config_template;
        out.close();
    }
}

CloudfuseMngr::CloudfuseMngr(std::string mountDir, std::string configFile, std::string fileCacheDir) {
    this->mountDir = mountDir;
    this->configFile = configFile;
    this->fileCacheDir = fileCacheDir;
    templateFile = "./nx_plugin_config.yam";

    std::ifstream in(templateFile.c_str());
    if (!in.good()) {
        std::ofstream out(templateFile.c_str());
        out << config_template;
        out.close();
    }
}

processReturn CloudfuseMngr::spawnProcess(wchar_t* argv, std::wstring envp) {
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
    if (!CreatePipe(&hReadStdOut, &hWriteStdOut, &sa, 0)) {
        exit(1); 
    }
    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if (!SetHandleInformation(hReadStdOut, HANDLE_FLAG_INHERIT, 0) ){
        exit(1);
    }

    // Create a pipe for the child process's STDERR. 
    if (!CreatePipe(&hReadStdErr, &hWriteStdErr, &sa, 0)) {
        exit(1); 
    }
    // Ensure the read handle to the pipe for STDERR is not inherited.
    if (!SetHandleInformation(hReadStdErr, HANDLE_FLAG_INHERIT, 0) ){
        exit(1);
    }

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdOutput = hWriteStdOut;
    si.hStdError = hWriteStdErr;
    si.dwFlags |= STARTF_USESTDHANDLES;
    ZeroMemory(&pi, sizeof(pi));

    LPVOID lpvEnv = GetEnvironmentStringsW();

    // If the returned pointer is NULL, exit.
    if (lpvEnv == NULL) {
        printf("GetEnvironmentStrings failed (%d)\n", GetLastError()); 
        exit(1);
    }
    
    // Append current environment to new environment variables
    LPTSTR lpszVariable = (LPTSTR) lpvEnv;
    while (*lpszVariable) {
        envp += lpszVariable;
        envp += L'\0';
        lpszVariable += lstrlen(lpszVariable) + 1;
    }

    // Environment block must be double null terminated
    envp += L'\0';

    // Start the child process. 
    if( !CreateProcessW( 
        NULL,                       // No module name (use command line)
        LPWSTR(argv),               // Command line
        NULL,                       // Process handle not inheritable
        NULL,                       // Thread handle not inheritable
        TRUE,                      // Set handle inheritance to FALSE
        CREATE_UNICODE_ENVIRONMENT, // Use unicode environment
        (LPVOID)envp.c_str(),     // Use new environment
        NULL,                       // Use parent's starting directory 
        &si,                        // Pointer to STARTUPINFO structure
        &pi )                       // Pointer to PROCESS_INFORMATION structure
    ) 
    {
        CloseHandle(hWriteStdOut);
        CloseHandle(hReadStdOut);
        CloseHandle(hWriteStdErr);
        CloseHandle(hReadStdErr);
        printf( "CreateProcess failed (%d).\n", GetLastError() );
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
    while (ReadFile(hReadStdOut, buffer, BUFSIZE-1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = 0;
        ret.output.append(buffer);
    }
    while (ReadFile(hReadStdErr, buffer, BUFSIZE-1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = 0;
        ret.output.append(buffer);
    }

    // Close handles.
    CloseHandle(hReadStdOut);
    CloseHandle(hReadStdErr);
    return ret;
}

processReturn CloudfuseMngr::genS3Config(std::string accessKeyId, std::string secretAccessKey, std::string region, std::string endpoint, std::string bucketName, std::string passphrase) {
    std::string argv = "cloudfuse gen-config --config-file=" + templateFile + " --output-file=" + configFile + " --temp-path=" + fileCacheDir + " --passphrase=" + passphrase;
    std::string aws_access_key_id_env = "AWS_ACCESS_KEY_ID=" + accessKeyId;
    std::string aws_secret_access_key_env = "AWS_SECRET_ACCESS_KEY=" + secretAccessKey;
    std::string aws_region_env = "AWS_REGION=" + region;
    std::string endpoint_env = "ENDPOINT=" + endpoint;
    std::string bucket_name_env = "BUCKET_NAME=" + bucketName;
    std::string envp = aws_access_key_id_env + '\0'+ aws_secret_access_key_env + '\0' + aws_region_env + '\0' + endpoint_env + '\0' + bucket_name_env + '\0';

    std::wstring wargv = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(argv);
    std::wstring wenvp = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(envp);
    
    return spawnProcess(const_cast<wchar_t*>(wargv.c_str()), wenvp);
}

processReturn CloudfuseMngr::dryRun(std::string passphrase) {
    std::string argv = "cloudfuse mount " + mountDir + " --config-file=" + configFile + " --passphrase=" + passphrase + " --dry-run";
    std::string envp = "";

    std::wstring wargv = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(argv);
    std::wstring wenvp = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(envp);
    
    return spawnProcess(const_cast<wchar_t*>(wargv.c_str()), wenvp);
}

processReturn CloudfuseMngr::mount(std::string passphrase) {
    std::string argv = "cloudfuse mount " + mountDir + " --config-file=" + configFile + " --passphrase=" + passphrase;
    std::string envp = "";

    std::wstring wargv = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(argv);
    std::wstring wenvp = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(envp);
    
    return spawnProcess(const_cast<wchar_t*>(wargv.c_str()), wenvp);
}

processReturn CloudfuseMngr::unmount() {
    std::string argv = "cloudfuse unmount " + mountDir;
    std::string envp = "";
    
    std::wstring wargv = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(argv);
    std::wstring wenvp = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(envp);
    
    return spawnProcess(const_cast<wchar_t*>(wargv.c_str()), wenvp);
}

bool CloudfuseMngr::isInstalled() {
    std::string argv = "cloudfuse version";
    std::string envp = "";
    
    std::wstring wargv = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(argv);
    std::wstring wenvp = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(envp);
    
    return spawnProcess(const_cast<wchar_t*>(wargv.c_str()), wenvp).errCode == 0;
}

bool CloudfuseMngr::isMounted() {
    std::wstring mountDirW = std::wstring(mountDir.begin(), mountDir.end());
    DWORD fileAttributes = GetFileAttributes(mountDirW.c_str());
    if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    return true;
}

#endif
