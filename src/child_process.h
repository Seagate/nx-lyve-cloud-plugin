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
    CloudfuseMngr(std::string mountDir, std::string configFile);
    processReturn dryRun(std::string accessKeyId, std::string secretAccessKey, std::string region, std::string endpoint, std::string bucketName);
    processReturn mount(std::string accessKeyId, std::string secretAccessKey, std::string region, std::string endpoint, std::string bucketName);
    processReturn unmount();
    bool isInstalled();
    bool isMounted();
private:
    std::string mountDir;
    std::string configFile;
    #ifdef _WIN32
    processReturn spawnProcess(wchar_t *argv);
    #elif defined(__linux__) || defined(__APPLE__)
    processReturn spawnProcess(char *const argv[], char *const envp[]);
    #endif
};

#endif
