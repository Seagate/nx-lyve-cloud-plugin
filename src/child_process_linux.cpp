#if defined(__linux__) || defined(__APPLE__)
#include "child_process.h"
#include <iostream>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>

CloudfuseMngr::CloudfuseMngr(std::string mountDir, std::string configFile) {
    this->mountDir = mountDir;
    this->configFile = configFile;
}

processReturn CloudfuseMngr::spawnProcess(char *const argv[], char *const envp[]) {
    processReturn ret;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        throw std::runtime_error("Failed to create pipe.");
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        // Fork failed
        throw std::runtime_error("Failed to fork process.");
    } else if (pid != 0) {
        // Parent process
        close(pipefd[1]); // Close write end of pipe

        char buffer[4096];
        int bytesRead;
        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            ret.output.append(buffer, bytesRead);
        }

        close(pipefd[0]); // Close read end of pipe
        
        // Wait for cloudfuse command to stop
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            ret.errCode = WEXITSTATUS(status);
            return ret;
        } else if (WIFSIGNALED(status)) {
            ret.errCode = WTERMSIG(status);
            return ret;
        }

        ret.errCode = WTERMSIG(status);
        return ret;
    } else {
        // Child process
        close(pipefd[0]); // Close read end of pipe

        if (dup2(pipefd[1], STDOUT_FILENO) == -1 || dup2(pipefd[1], STDERR_FILENO) == -1) {
            exit(EXIT_FAILURE);
        }
        
        close(pipefd[1]); // Close write end of pipe

        if (execve(argv[0], argv, envp) == -1) {
            exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);
    }
}

processReturn CloudfuseMngr::dryRun() {
    std::string configArg = "--config-file=" + configFile;
    char *const argv[] = {const_cast<char*>("/bin/cloudfuse"), const_cast<char*>("mount"), const_cast<char*>(mountDir.c_str()), const_cast<char*>(configArg.c_str()), const_cast<char*>("--dry-run"), NULL};
    
    return spawnProcess(argv, NULL);
}

processReturn CloudfuseMngr::mount() {
    std::string configArg = "--config-file=" + configFile;
    char *const argv[] = {const_cast<char*>("/bin/cloudfuse"), const_cast<char*>("mount"), const_cast<char*>(mountDir.c_str()), const_cast<char*>(configArg.c_str()), NULL};
    
    return spawnProcess(argv, NULL);
}

processReturn CloudfuseMngr::unmount() {
    char *const argv[] = {const_cast<char*>("/bin/cloudfuse"), const_cast<char*>("unmount"), const_cast<char*>(mountDir.c_str()), const_cast<char*>("-z"), NULL};
    char *const envp[] = {const_cast<char*>("PATH=/bin"), NULL};

    return spawnProcess(argv, envp);
}

bool CloudfuseMngr::isInstalled() {
    char *const argv[] = {const_cast<char*>("/bin/cloudfuse"), const_cast<char*>("version"), NULL};
    
    return spawnProcess(argv, NULL).errCode == 0;
}

bool CloudfuseMngr::isMounted() {
    // Logic based on os.ismount implementation in Python.

    struct stat buf1, buf2;

    if (lstat(mountDir.c_str(), &buf1) != 0) {
        // Folder doesn't exist, so not mounted
        return false;
    }

    if S_ISLNK(buf1.st_mode) {
        // Mount can't be a symbolic link
        return false;
    }

    std::string parent = mountDir + "/..";
    if (lstat(parent.c_str(), &buf2) != 0) {
        return false;
    }

    if (buf1.st_dev != buf2.st_dev) {
        // Directory on a different path from parent, so this is mounted
        return true;
    }

    if (buf1.st_ino == buf2.st_ino) {
        // These point to the same inode, so this is mounted
        return true;
    }

    
    return false;
}

#endif
