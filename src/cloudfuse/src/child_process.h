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
    CloudfuseMngr(std::wstring mountDir, std::wstring configFile, std::wstring fileCachePath);
    processReturn dryRun(std::wstring passphrase);
    processReturn mount(std::wstring passphrase);
    processReturn genS3Config(std::wstring accessKeyId, std::wstring secretAccessKey, std::wstring region, std::wstring endpoint, std::wstring bucketName, std::wstring passphrase);
    #elif defined(__linux__) || defined(__APPLE__)
    CloudfuseMngr();
    CloudfuseMngr(std::string mountDir, std::string fileCacheDir, std::string configFile, std::string templateFile);
    processReturn dryRun(std::string accessKeyId, std::string secretAccessKey, std::string passphrase);
    processReturn mount(std::string accessKeyId, std::string secretAccessKey, std::string passphrase);
    processReturn genS3Config(std::string region, std::string endpoint, std::string bucketName, std::string passphrase);
    std::string getMountDir();
    std::string getFileCacheDir();
    #endif
    processReturn unmount();
    bool isInstalled();
    bool isMounted();
private:
    #ifdef _WIN32
    std::wstring mountDir;
    std::wstring configFile;
    std::wstring fileCachePath;
    std::wstring templateFile;
    processReturn spawnProcess(wchar_t* argv, std::wstring envp);
    processReturn encryptConfig(std::wstring passphrase);
    #elif defined(__linux__) || defined(__APPLE__)
    std::string mountDir;
    std::string configFile;
    std::string fileCacheDir;
    std::string templateFile;
    processReturn spawnProcess(char *const argv[], char *const envp[]);
    #endif
};

#endif
