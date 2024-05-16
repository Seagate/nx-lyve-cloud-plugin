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
    processReturn dryRun();
    processReturn mount();
    processReturn unmount();
    bool isInstalled();
    bool isMounted();
private:
    std::string mountDir;
    std::string configFile;
    processReturn spawnProcess(char *const argv[], char *const envp[]);
};

#endif
