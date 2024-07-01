#include <string>

struct processReturn
{
    int errCode;        // 0 if successful, failed otherwise
    std::string output; // std err and std out from cloudfuse command
};

class CloudfuseMngr
{
  public:
    CloudfuseMngr();
#ifdef _WIN32
    CloudfuseMngr(const std::string mountDir, const std::string configFile, const std::string fileCachePath);
    processReturn dryRun(const std::string passphrase);
    processReturn mount(const std::string passphrase);
    processReturn genS3Config(const std::string accessKeyId, const std::string secretAccessKey, const std::string region,
                              const std::string endpoint, const std::string bucketName, const std::string passphrase);
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
    processReturn spawnProcess(wchar_t *argv, std::wstring envp);
    processReturn encryptConfig(const std::string passphrase);
#elif defined(__linux__) || defined(__APPLE__)
    processReturn spawnProcess(char *const argv[], char *const envp[]);
#endif
};
