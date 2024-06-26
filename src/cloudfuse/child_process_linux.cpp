#if defined(__linux__)
#include "child_process.h"
#include <fstream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

std::string config_template = R"(
allow-other: true
logging:
  level: log_err
  type: syslog

components:
  - libfuse
  - file_cache
  - attr_cache
  - s3storage

libfuse:
  attribute-expiration-sec: 1800
  entry-expiration-sec: 1800
  negative-entry-expiration-sec: 1800
  ignore-open-flags: true
  network-share: true

file_cache:
  path: { 0 }
  timeout-sec: 180
  allow-non-empty-temp: true
  cleanup-on-start: false

attr_cache:
  timeout-sec: 3600

s3storage:
  bucket-name: { BUCKET_NAME }
  endpoint: { ENDPOINT }
  region: { AWS_REGION }
)";

CloudfuseMngr::CloudfuseMngr()
{
    std::string homeEnv;
    const char *home = std::getenv("HOME");
    if (home == nullptr)
    {
        homeEnv = "";
    }
    else
    {
        homeEnv = home;
    }
    mountDir = homeEnv + "/cloudfuse";
    fileCacheDir = homeEnv + "/cloudfuse_cache";
    configFile = homeEnv + "/nx_plugin_config.aes";
    templateFile = homeEnv + "/nx_plugin_config.yaml";

    std::ifstream in(templateFile);
    if (!in.good())
    {
        std::ofstream out(templateFile);
        out << config_template;
        out.close();
    }
}

CloudfuseMngr::CloudfuseMngr(std::string mountDir, std::string fileCacheDir, std::string configFile,
                             std::string templateFile)
{
    this->mountDir = mountDir;
    this->configFile = configFile;
    this->fileCacheDir = fileCacheDir;
    this->templateFile = templateFile;
}

processReturn CloudfuseMngr::spawnProcess(char *const argv[], char *const envp[])
{
    processReturn ret;

    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        throw std::runtime_error("Failed to create pipe.");
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        // Fork failed
        throw std::runtime_error("Failed to fork process.");
    }
    else if (pid != 0)
    {
        // Parent process
        close(pipefd[1]); // Close write end of pipe

        char buffer[4096];
        int bytesRead;
        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer))) > 0)
        {
            ret.output.append(buffer, bytesRead);
        }

        close(pipefd[0]); // Close read end of pipe

        // Wait for cloudfuse command to stop
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
        {
            ret.errCode = WEXITSTATUS(status);
            return ret;
        }
        else if (WIFSIGNALED(status))
        {
            ret.errCode = WTERMSIG(status);
            return ret;
        }

        ret.errCode = WTERMSIG(status);
        return ret;
    }
    else
    {
        // Child process
        close(pipefd[0]); // Close read end of pipe

        if (dup2(pipefd[1], STDOUT_FILENO) == -1 || dup2(pipefd[1], STDERR_FILENO) == -1)
        {
            exit(EXIT_FAILURE);
        }

        close(pipefd[1]); // Close write end of pipe

        if (execve(argv[0], argv, envp) == -1)
        {
            exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);
    }
}

processReturn CloudfuseMngr::genS3Config(std::string region, std::string endpoint, std::string bucketName,
                                         std::string passphrase)
{
    std::string configArg = "--config-file=" + templateFile;
    std::string outputArg = "--output-file=" + configFile;
    std::string fileCachePathArg = "--temp-path=" + fileCacheDir;
    std::string passphraseArg = "--passphrase=" + passphrase;
    char *const argv[] = {const_cast<char *>("/bin/cloudfuse"),
                          const_cast<char *>("gen-config"),
                          const_cast<char *>(configArg.c_str()),
                          const_cast<char *>(outputArg.c_str()),
                          const_cast<char *>(fileCachePathArg.c_str()),
                          const_cast<char *>(passphraseArg.c_str()),
                          NULL};

    std::string bucketNameEnv = "BUCKET_NAME=" + bucketName;
    std::string endpointEnv = "ENDPOINT=" + endpoint;
    std::string regionEnv = "AWS_REGION=" + region;
    char *const envp[] = {const_cast<char *>(bucketNameEnv.c_str()), const_cast<char *>(endpointEnv.c_str()),
                          const_cast<char *>(regionEnv.c_str()), NULL};

    return spawnProcess(argv, envp);
}

processReturn CloudfuseMngr::dryRun(std::string accessKeyId, std::string secretAccessKey, std::string passphrase)
{
    std::string configArg = "--config-file=" + configFile;
    std::string passphraseArg = "--passphrase=" + passphrase;
    char *const argv[] = {const_cast<char *>("/bin/cloudfuse"),
                          const_cast<char *>("mount"),
                          const_cast<char *>(mountDir.c_str()),
                          const_cast<char *>(configArg.c_str()),
                          const_cast<char *>(passphraseArg.c_str()),
                          const_cast<char *>("--dry-run"),
                          NULL};

    std::string awsAccessKeyIdEnv = "AWS_ACCESS_KEY_ID=" + accessKeyId;
    std::string awsSecretAccessKeyEnv = "AWS_SECRET_ACCESS_KEY=" + secretAccessKey;
    char *const envp[] = {const_cast<char *>(awsAccessKeyIdEnv.c_str()),
                          const_cast<char *>(awsSecretAccessKeyEnv.c_str()), NULL};

    return spawnProcess(argv, envp);
}

processReturn CloudfuseMngr::mount(std::string accessKeyId, std::string secretAccessKey, std::string passphrase)
{
    std::string configArg = "--config-file=" + configFile;
    std::string passphraseArg = "--passphrase=" + passphrase;
    char *const argv[] = {const_cast<char *>("/bin/cloudfuse"),      const_cast<char *>("mount"),
                          const_cast<char *>(mountDir.c_str()),      const_cast<char *>(configArg.c_str()),
                          const_cast<char *>(passphraseArg.c_str()), NULL};

    std::string awsAccessKeyIdEnv = "AWS_ACCESS_KEY_ID=" + accessKeyId;
    std::string awsSecretAccessKeyEnv = "AWS_SECRET_ACCESS_KEY=" + secretAccessKey;
    char *const envp[] = {const_cast<char *>(awsAccessKeyIdEnv.c_str()),
                          const_cast<char *>(awsSecretAccessKeyEnv.c_str()), NULL};

    return spawnProcess(argv, envp);
}

processReturn CloudfuseMngr::unmount()
{
    char *const argv[] = {const_cast<char *>("/bin/cloudfuse"), const_cast<char *>("unmount"),
                          const_cast<char *>(mountDir.c_str()), const_cast<char *>("-z"), NULL};
    char *const envp[] = {const_cast<char *>("PATH=/bin"), NULL};

    return spawnProcess(argv, envp);
}

bool CloudfuseMngr::isInstalled()
{
    char *const argv[] = {const_cast<char *>("/bin/cloudfuse"), const_cast<char *>("version"), NULL};

    return spawnProcess(argv, NULL).errCode == 0;
}

bool CloudfuseMngr::isMounted()
{
    // Logic based on os.ismount implementation in Python.

    struct stat buf1, buf2;

    if (lstat(mountDir.c_str(), &buf1) != 0)
    {
        // Folder doesn't exist, so not mounted
        return false;
    }

    if S_ISLNK (buf1.st_mode)
    {
        // Mount can't be a symbolic link
        return false;
    }

    std::string parent = mountDir + "/..";
    if (lstat(parent.c_str(), &buf2) != 0)
    {
        return false;
    }

    if (buf1.st_dev != buf2.st_dev)
    {
        // Directory on a different path from parent, so this is mounted
        return true;
    }

    if (buf1.st_ino == buf2.st_ino)
    {
        // These point to the same inode, so this is mounted
        return true;
    }

    return false;
}

#endif
