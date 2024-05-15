#include "child_process.h"
#include <iostream>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>

CloudfuseMngr::CloudfuseMngr(std::string mountDir, std::string configFile) {
    this->mountDir = mountDir;
    this->configFile = configFile;
}

int CloudfuseMngr::spawnProcess(char *const argv[], char *const envp[]) {
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
        //std::cout << "Output from Child: ";
        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            //std::cout << buffer;
        }
        //std::cout << '\n';
        close(pipefd[0]); // Close read end of pipe
        
        // Wait for cloudfuse command to stop
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            return WTERMSIG(status);
        }
        
        return -1;
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

bool CloudfuseMngr::dryRun() {
    std::string configArg = "--config-file=" + configFile;
    char *const argv[] = {const_cast<char*>("/bin/cloudfuse"), const_cast<char*>("mount"), const_cast<char*>(mountDir.c_str()), const_cast<char*>(configArg.c_str()), const_cast<char*>("--dry-run"), NULL};
    
    int ret = spawnProcess(argv, NULL);
    return ret == 0;
}

bool CloudfuseMngr::isInstalled() {
    char *const argv[] = {const_cast<char*>("/bin/cloudfuse"), const_cast<char*>("version"), NULL};
    
    int ret = spawnProcess(argv, NULL);
    return ret == 0;
}

bool CloudfuseMngr::mount() {
    std::string configArg = "--config-file=" + configFile;
    char *const argv[] = {const_cast<char*>("/bin/cloudfuse"), const_cast<char*>("mount"), const_cast<char*>(mountDir.c_str()), const_cast<char*>(configArg.c_str()), NULL};
    
    int ret = spawnProcess(argv, NULL);
    return ret == 0;
}

bool CloudfuseMngr::unmount() {
    char *const argv[] = {const_cast<char*>("/bin/cloudfuse"), const_cast<char*>("unmount"), const_cast<char*>(mountDir.c_str()), const_cast<char*>("-z"), NULL};
    char *const envp[] = {const_cast<char*>("PATH=/bin"), NULL};

    int ret = spawnProcess(argv, envp);
    return ret == 0;
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

int main() {
    // Run test with non existent config file
    std::string mountDir = "/home/jfan/Desktop/mount";
    std::string configFile = "notExist";
    CloudfuseMngr errCf = CloudfuseMngr(mountDir, configFile);

    std::cout << "\n" << "Starting cloudfuse with incorrect config file" << '\n';
    std::cout << "Is cloudfuse installed: " << errCf.isInstalled() << '\n';
    std::cout << "Try dry run: " << errCf.dryRun() << '\n';
    std::cout << "Try mount: " << errCf.mount() << '\n';
    std::cout << "Is cloudfuse mounted: " << errCf.isMounted() << '\n';
    std::cout << "Try unmount: " << errCf.unmount() << '\n';

    // Run test with existing config file
    configFile = "config.yaml";
    CloudfuseMngr workingCf = CloudfuseMngr(mountDir, configFile);

    std::cout << "\n" << "Starting cloudfuse with correct config file" << '\n';
    std::cout << "Is cloudfuse installed: " << workingCf.isInstalled() << '\n';
    std::cout << "Try dry run: " << workingCf.dryRun() << '\n';
    std::cout << "Try mount: " << workingCf.mount() << '\n';
    std::cout << "Is cloudfuse mounted: " << workingCf.isMounted() << '\n';
    std::cout << "Try unmount: " << workingCf.unmount() << '\n';

    return 0;
}
