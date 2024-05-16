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
#include <cstdlib>

CloudfuseMngr::CloudfuseMngr(std::string mountDir, std::string configFile) {
    this->mountDir = mountDir;
    this->configFile = configFile;
}

processReturn CloudfuseMngr::spawnProcess(wchar_t* argv) {
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

    // Start the child process. 
    if( !CreateProcess( 
        NULL,           // No module name (use command line)
        argv,           // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
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

processReturn CloudfuseMngr::dryRun() {
    std::string configArg = "cloudfuse mount " + mountDir + " --config-file=" + configFile + " --dry-run";
    std::wstring argv = std::wstring(configArg.begin(), configArg.end());
    wchar_t* arg = const_cast<wchar_t*>(argv.c_str());
    
    return spawnProcess(arg);
}

processReturn CloudfuseMngr::mount() {
    std::string configArg = "cloudfuse mount " + mountDir + " --config-file=" + configFile;
    std::cout << configArg << std::endl;
    std::wstring argv = std::wstring(configArg.begin(), configArg.end());
    
    return spawnProcess(const_cast<wchar_t*>(argv.c_str()));
}

processReturn CloudfuseMngr::unmount() {
    std::string configArg = "cloudfuse unmount " + mountDir;
    std::wstring argv = std::wstring(configArg.begin(), configArg.end());
    
    return spawnProcess(const_cast<wchar_t*>(argv.c_str()));
}

bool CloudfuseMngr::isInstalled() {
    std::string configArg = "cloudfuse version";
    std::wstring argv = std::wstring(configArg.begin(), configArg.end());
    
    return spawnProcess(const_cast<wchar_t*>(argv.c_str())).errCode == 0;
}

bool CloudfuseMngr::isMounted() {
    std::wstring mountDirW = std::wstring(mountDir.begin(), mountDir.end());
    DWORD fileAttributes = GetFileAttributes(mountDirW.c_str());
    if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    return true;
}

int main() {
    // Run test with non existent config file
    std::string mountDir = "Z:";
    std::string configFile = "notExist";
    CloudfuseMngr errCf = CloudfuseMngr(mountDir, configFile);

    std::cout << "\n" << "Starting cloudfuse with incorrect config file" << '\n';
    std::cout << "Is cloudfuse installed: " << errCf.isInstalled() << '\n';
    std::cout << "Try dry run: " << errCf.dryRun().errCode << '\n';
    std::cout << "Try mount: " << errCf.mount().errCode << '\n';
    std::cout << "Is cloudfuse mounted: " << errCf.isMounted() << '\n';
    std::cout << "Try unmount: " << errCf.unmount().errCode << '\n';

    // Run test with existing config file
    configFile = "config.yaml";
    CloudfuseMngr workingCf = CloudfuseMngr(mountDir, configFile);

    std::cout << "\n" << "Starting cloudfuse with correct config file" << '\n';
    std::cout << "Is cloudfuse installed: " << workingCf.isInstalled() << '\n';
    std::cout << "Try dry run: " << workingCf.dryRun().errCode << '\n';
    std::cout << "Try mount: " << workingCf.mount().errCode << '\n';
    std::cout << "Is cloudfuse mounted: " << workingCf.isMounted() << '\n';
    std::cout << "Try unmount: " << workingCf.unmount().errCode << '\n';

    return 0;
}

#endif
