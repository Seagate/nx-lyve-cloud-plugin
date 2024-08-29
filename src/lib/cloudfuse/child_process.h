/*
   Licensed under the MIT License <http://opensource.org/licenses/MIT>.

   Copyright © 2024 Seagate Technology LLC and/or its Affiliates

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE
*/

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
    processReturn dryRun(const std::string passphrase);
    processReturn mount(const std::string passphrase);
    processReturn genS3Config(const std::string accessKeyId, const std::string secretAccessKey,
                              const std::string endpoint, const std::string bucketName, const std::string passphrase);
#elif defined(__linux__) || defined(__APPLE__)
    processReturn dryRun(const std::string accessKeyId, const std::string secretAccessKey,
                         const std::string passphrase);
    processReturn mount(const std::string accessKeyId, const std::string secretAccessKey, const std::string passphrase);
    processReturn genS3Config(const std::string endpoint, const std::string bucketName, const std::string passphrase);
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
    std::string templateVersionString;
    bool templateOutdated(std::string templateFilePath);
#ifdef _WIN32
    processReturn spawnProcess(wchar_t *argv, std::wstring envp);
    processReturn encryptConfig(const std::string passphrase);
#elif defined(__linux__) || defined(__APPLE__)
    processReturn spawnProcess(char *const argv[], char *const envp[]);
#endif
};
