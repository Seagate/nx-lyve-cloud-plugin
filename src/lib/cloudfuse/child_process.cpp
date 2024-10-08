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

#include "child_process.h"
#include <fstream>

std::string CloudfuseMngr::getMountDir()
{
    return mountDir;
}

std::string CloudfuseMngr::getFileCacheDir()
{
    return fileCacheDir;
}

bool CloudfuseMngr::templateOutdated(std::string templateFilePath)
{
    // check the first line of the template file for a matching version
    std::string firstLine;
    // read the first line of the template file
    std::ifstream templateFileStream(templateFilePath);
    // if the file doesn't exist (or can't be opened), we need to write it
    if (templateFileStream.fail())
    {
        return true;
    }
    getline(templateFileStream, firstLine);
    templateFileStream.close();
    // if the versions don't match, we need to overwrite the template
    return templateVersionString.compare(firstLine) != 0;
}