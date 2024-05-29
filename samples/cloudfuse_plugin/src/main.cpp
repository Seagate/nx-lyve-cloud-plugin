#include <cloudfuse/src/child_process.h>
#include <iostream>

int main() {
    // Run test with non existent config file
    std::string mountDir = "/home/jfan/Desktop/mount4";
    std::string configFile = "notExist";
    CloudfuseMngr errCf = CloudfuseMngr(mountDir, configFile);

    std::cout << "\n" << "Starting cloudfuse with incorrect config file" << '\n';
    std::cout << "Is cloudfuse installed: " << errCf.isInstalled() << '\n';
    std::cout << "Is cloudfuse mounted: " << errCf.isMounted() << '\n';
    std::cout << "Try unmount: " << errCf.unmount().errCode << '\n';

    // Run test with existing config file
    configFile = "config-no-auth.yaml";
    CloudfuseMngr workingCf = CloudfuseMngr(mountDir, configFile);

    std::cout << "\n" << "Starting cloudfuse with correct config file" << '\n';
    std::cout << "Is cloudfuse installed: " << workingCf.isInstalled() << '\n';
    std::cout << "Is cloudfuse mounted: " << workingCf.isMounted() << '\n';
    std::cout << "Try unmount: " << workingCf.unmount().errCode << '\n';

    return 0;
}
