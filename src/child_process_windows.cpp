#ifdef _WIN32
#include "child_process.h"
#include <iostream>
#include <stdio.h>
#include <unistd.h>


#include <tchar.h>

#ifndef UNICODE
#define UNICODE
#define UNICODE_WAS_UNDEFINED
#endif
#include <windows.h>
#ifdef UNICODE_WAS_UNDEFINED
#undef UNICODE
#endif
#include <strsafe.h>
#include <UserEnv.h>
#include <cstdlib>

CloudfuseMngr::CloudfuseMngr(std::wstring mountDir, std::wstring configFile) {
    this->mountDir = mountDir;
    this->configFile = configFile;
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
        FALSE,                      // Set handle inheritance to FALSE
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

    // Wait until child process exits.
    unsigned long errCode;
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &errCode);
    ret.errCode = errCode;

    // Close process and thread handles.
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadStdOut);
    CloseHandle(hReadStdErr);
    return ret;
}

processReturn CloudfuseMngr::genS3Config(std::wstring accessKeyId, std::wstring secretAccessKey, std::wstring region, std::wstring endpoint, std::wstring bucketName, std::wstring passphrase) {
    std::wstring argv = L"./cloudfuse.exe gen-test-config --config-file=nx_plugin_config.yaml --output-file=temp.yaml";
    std::wstring aws_access_key_id_env = L"AWS_ACCESS_KEY_ID=" + accessKeyId;
    std::wstring aws_secret_access_key_env = L"AWS_SECRET_ACCESS_KEY=" + secretAccessKey;
    std::wstring aws_region_env = L"AWS_REGION=" + region;
    std::wstring endpoint_env = L"ENDPOINT=" + endpoint;
    std::wstring bucket_name_env = L"BUCKET_NAME=" + bucketName;
    std::wstring envp = aws_access_key_id_env + L'\0'+ aws_secret_access_key_env + L'\0' + aws_region_env + L'\0' + endpoint_env + L'\0' + bucket_name_env + L'\0';
    
    processReturn ret = spawnProcess(const_cast<wchar_t*>(argv.c_str()), envp);

    if (ret.errCode == 0) {
        return encryptConfig(passphrase);
    }

    return ret;
}

processReturn CloudfuseMngr::encryptConfig(std::wstring passphrase) {
    std::wstring argv = L"./cloudfuse.exe secure encrypt --config-file=temp.yaml --output-file=" + configFile + L" --passphrase=" + passphrase;
    std::wstring envp = L"";
    
    return spawnProcess(const_cast<wchar_t*>(argv.c_str()), envp);
}

processReturn CloudfuseMngr::dryRun(std::wstring passphrase) {
    std::wstring argv = L"./cloudfuse.exe mount " + mountDir + L" --config-file=" + configFile + L" --passphrase=" + passphrase + L" --dry-run";
    std::wstring envp = L"";

    std::wcout << argv << std::endl;
    
    return spawnProcess(const_cast<wchar_t*>(argv.c_str()), envp);
}

processReturn CloudfuseMngr::mount(std::wstring passphrase) {
    std::wstring argv = L"./cloudfuse.exe mount " + mountDir + L" --config-file=" + configFile + L" --passphrase=" + passphrase;
    std::wstring envp = L"";

    return spawnProcess(const_cast<wchar_t*>(argv.c_str()), envp);
}

processReturn CloudfuseMngr::unmount() {
    std::wstring argv = L"cloudfuse unmount " + mountDir;
    std::wstring envp = L"";
    
    return spawnProcess(const_cast<wchar_t*>(argv.c_str()), envp);
}

bool CloudfuseMngr::isInstalled() {
    std::wstring argv = L"cloudfuse version";
    std::wstring envp = L"";
    
    return spawnProcess(const_cast<wchar_t*>(argv.c_str()), envp).errCode == 0;
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
