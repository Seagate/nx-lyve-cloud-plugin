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

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE hRead, hWrite;

    // Create a pipe for the child process's STDOUT. 
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
        exit(1); 
    }
    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if (!SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0) ){
        exit(1);
    }

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    ZeroMemory( &pi, sizeof(pi) );

    DWORD dwRet;
    LPWSTR pszOldVal = (LPWSTR) malloc(4096*sizeof(WCHAR));
    if (NULL == pszOldVal) {
        exit(1);
    }

    dwRet = GetEnvironmentVariableW(L"AppData", pszOldVal, 4096);
    std::wstring envnew = L"AppData=";
    for (int i = 0; i < dwRet; i++) {
        envnew += pszOldVal[i];
    }
    envnew += L'\0';

    std::wstring newEnv = envnew + envp;

    // Start the child process. 
    if( !CreateProcessW( 
        NULL,           // No module name (use command line)
        LPWSTR(argv),           // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        CREATE_UNICODE_ENVIRONMENT,              // No creation flags
        LPVOID(newEnv.c_str()),  // Use new environment
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
    ) 
    {
        CloseHandle(hWrite);
        CloseHandle(hRead);
        printf( "CreateProcess failed (%d).\n", GetLastError() );
        return ret;
    }
    CloseHandle(hWrite);

    const int BUFSIZE = 4096;
    DWORD bytesRead;
    CHAR buffer[BUFSIZE];
    while (ReadFile(hRead, buffer, BUFSIZE-1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = 0;
        ret.output.append(buffer);
    }

    // Wait until child process exits.
    unsigned long errCode;
    WaitForSingleObject(pi.hProcess, INFINITE );
    GetExitCodeProcess(pi.hProcess, &errCode);
    ret.errCode = errCode;

    // Close process and thread handles.
    CloseHandle(hRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return ret;
}

processReturn CloudfuseMngr::dryRun(std::wstring accessKeyId, std::wstring secretAccessKey, std::wstring region, std::wstring endpoint, std::wstring bucketName) {
    std::wstring configArg = L"./cloudfuse.exe mount " + mountDir + L" --config-file=" + configFile + L" --dry-run";
    std::wstring aws_access_key_id_env = L"AWS_ACCESS_KEY_ID=" + accessKeyId;
    std::wstring aws_secret_access_key_env = L"AWS_SECRET_ACCESS_KEY=" + secretAccessKey;
    std::wstring aws_region_env = L"AWS_REGION=" + region;
    std::wstring envp = aws_access_key_id_env + L'\0'+ aws_secret_access_key_env + L'\0' + aws_region_env + L'\0' + L'\0';
    
    return spawnProcess(const_cast<wchar_t*>(configArg.c_str()), envp);
}

processReturn CloudfuseMngr::mount(std::wstring accessKeyId, std::wstring secretAccessKey, std::wstring region, std::wstring endpoint, std::wstring bucketName) {
    std::wstring configArg = L"./cloudfuse.exe mount " + mountDir + L" --config-file=" + configFile;
    std::wstring aws_access_key_id_env = L"AWS_ACCESS_KEY_ID=" + accessKeyId;
    std::wstring aws_secret_access_key_env = L"AWS_SECRET_ACCESS_KEY=" + secretAccessKey;
    std::wstring aws_region_env = L"AWS_REGION=" + region;
    std::wstring envp = aws_access_key_id_env + L'\0'+ aws_secret_access_key_env + L'\0' + aws_region_env + L'\0' + L'\0';

    return spawnProcess(const_cast<wchar_t*>(configArg.c_str()), envp);
}

processReturn CloudfuseMngr::unmount() {
    std::wstring configArg = L"cloudfuse unmount " + mountDir;
    std::wstring argv = std::wstring(configArg.begin(), configArg.end());
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
