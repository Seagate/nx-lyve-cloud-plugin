// MyFile.h
#ifndef CHILD_PROCESS_H
#define MYFILECHILD_PROCESS_H_H

#include <string>

class CloudfuseMngr {
public:
    CloudfuseMngr(std::string mountDir, std::string configFile);
    bool dryRun();
    bool mount();
    bool unmount();
    bool isInstalled();
    bool isMounted();
private:
    std::string mountDir;
    std::string configFile;
    int spawnProcess(char *const argv[], char *const envp[]);
};

#endif
