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
    CloudfuseMngr();
    #ifdef _WIN32
    CloudfuseMngr(std::string mountDir, std::string configFile, std::string fileCachePath);
    processReturn dryRun(std::string passphrase);
    processReturn mount(std::string passphrase);
    processReturn genS3Config(std::string accessKeyId, std::string secretAccessKey, std::string region, std::string endpoint, std::string bucketName, std::string passphrase);
    #elif defined(__linux__) || defined(__APPLE__)
    CloudfuseMngr(std::string mountDir, std::string fileCacheDir, std::string configFile, std::string templateFile);
    processReturn dryRun(std::string accessKeyId, std::string secretAccessKey, std::string passphrase);
    processReturn mount(std::string accessKeyId, std::string secretAccessKey, std::string passphrase);
    processReturn genS3Config(std::string region, std::string endpoint, std::string bucketName, std::string passphrase);
    #endif
    std::string getMountDir();
    std::string getFileCacheDir();
    processReturn unmount();
    bool isInstalled();
    bool isMounted();
private:
    std::string mountDir;
    std::string configFile;
    std::string fileCacheDir;
    std::string templateFile;
    #ifdef _WIN32
    processReturn spawnProcess(wchar_t* argv, std::wstring envp);
    processReturn encryptConfig(std::string passphrase);
    #elif defined(__linux__) || defined(__APPLE__)
    processReturn spawnProcess(char *const argv[], char *const envp[]);
    #endif
};

#endif
