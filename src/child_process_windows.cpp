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

    // Create an environment block
    // LPVOID lpEnvironment = NULL;
    // wchar_t* envVar = L"VARNAME=VALUE\0";
    // BOOL resultEnv = CreateEnvironmentBlock(&lpEnvironment, NULL, FALSE);
    // if (!resultEnv)
    // {
    //     printf("CreateEnvironmentBlock failed (%d).\n", GetLastError());
    //     return ret;
    // }

    // // Add your environment variable to the block
    // resultEnv = SetEnvironmentVariableW(envVar, lpEnvironment);
    // if (!resultEnv)
    // {
    //     printf("SetEnvironmentVariableW failed (%d).\n", GetLastError());
    //     return ret;
    // }

    // Start the child process. 
    if( !CreateProcess( 
        NULL,           // No module name (use command line)
        argv,           // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        LPTSTR(envp.c_str()),  // Use new environment
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
    std::wstring configArg = L"cloudfuse mount " + mountDir + L" --config-file=" + configFile;
    std::wstring aws_access_key_id_env = L"AWS_ACCESS_KEY_ID=" + accessKeyId;
    std::wstring aws_secret_access_key_env = L"AWS_SECRET_ACCESS_KEY=" + secretAccessKey;
    std::wstring aws_region_env = L"AWS_REGION=" + region;
    std::wstring envp(aws_access_key_id_env + L'\0' + aws_secret_access_key_env  + L'\0' + aws_region_env + L'\0'  + L'\0', aws_access_key_id_env.length() + aws_secret_access_key_env.length() + aws_region_env.length() + 4);
    
    std::wstring argv = std::wstring(configArg.begin(), configArg.end());
    wchar_t* arg = const_cast<wchar_t*>(argv.c_str());
    
    return spawnProcess(arg, envp);
}

processReturn CloudfuseMngr::mount(std::wstring accessKeyId, std::wstring secretAccessKey, std::wstring region, std::wstring endpoint, std::wstring bucketName) {
    std::wstring configArg = L"cloudfuse mount " + mountDir + L" --config-file=" + configFile;
    std::wstring aws_access_key_id_env = L"AWS_ACCESS_KEY_ID=" + accessKeyId;
    std::wstring aws_secret_access_key_env = L"AWS_SECRET_ACCESS_KEY=" + secretAccessKey;
    std::wstring aws_region_env = L"AWS_REGION=" + region;
    std::wstring envp(aws_access_key_id_env + L'\0' + aws_secret_access_key_env  + L'\0' + aws_region_env  + L'\0'  + L'\0', aws_access_key_id_env.length() + aws_secret_access_key_env.length() + aws_region_env.length() + 4);

    std::wstring argv = std::wstring(configArg.begin(), configArg.end());

    return spawnProcess(const_cast<wchar_t*>(argv.c_str()), envp);
}

processReturn CloudfuseMngr::unmount() {
    std::wstring configArg = L"cloudfuse unmount " + mountDir;
    std::wstring argv = std::wstring(configArg.begin(), configArg.end());
    std::wstring envp = L"\0\0";
    
    return spawnProcess(const_cast<wchar_t*>(argv.c_str()), envp);
}

bool CloudfuseMngr::isInstalled() {
    std::wstring argv = L"cloudfuse version";
    std::wstring envp = L"\0\0";
    
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
