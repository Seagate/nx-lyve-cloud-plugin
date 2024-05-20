// MyFile.h
#ifndef CHILD_PROCESS_H
#define MYFILECHILD_PROCESS_H_H

#include <string>

struct processReturn {
    int errCode; // 0 if successful, failed otherwise
    std::string output; // std err and std out from cloudfuse command
};

class CloudfuseMngr {
public:
    #ifdef _WIN32
    CloudfuseMngr(std::wstring mountDir, std::wstring configFile);
    processReturn dryRun(std::wstring accessKeyId, std::wstring secretAccessKey, std::wstring region, std::wstring endpoint, std::wstring bucketName);
    processReturn mount(std::wstring accessKeyId, std::wstring secretAccessKey, std::wstring region, std::wstring endpoint, std::wstring bucketName);
    #elif defined(__linux__) || defined(__APPLE__)
    CloudfuseMngr(std::string mountDir, std::string configFile);
    processReturn dryRun(std::string accessKeyId, std::string secretAccessKey, std::string region, std::string endpoint, std::string bucketName);
    processReturn mount(std::string accessKeyId, std::string secretAccessKey, std::string region, std::string endpoint, std::string bucketName);
    #endif
    processReturn unmount();
    bool isInstalled();
    bool isMounted();
private:
    #ifdef _WIN32
    std::wstring mountDir;
    std::wstring configFile;
    processReturn spawnProcess(wchar_t* argv, std::wstring envp);
    #elif defined(__linux__) || defined(__APPLE__)
    std::string mountDir;
    std::string configFile;
    processReturn spawnProcess(char *const argv[], char *const envp[]);
    #endif
};

#endif
