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

// check the first line of the template file for a matching version
bool CloudfuseMngr::templateValid()
{
    std::string firstLine;
    bool readFailed = false;
    // open the template file
    std::ifstream templateFileStream(templateFile);
    // check if the file doesn't exist (or failed to open)
    if (!templateFileStream.is_open())
    {
        // we need to write the file
        return false;
    }
    // read the first line and close the file
    readFailed = !getline(templateFileStream, firstLine);
    templateFileStream.close();
    if (readFailed)
    {
        // reading the line failed, so we need to overwrite the file
        return false;
    }
    // if the versions don't match, we need to overwrite the template
    return templateVersionString.compare(firstLine) == 0;
}

bool CloudfuseMngr::writeTemplate()
{
    // write the template file
    std::ofstream out(templateFile, std::ios::trunc);
    if (!out.is_open())
    {
        // failed to open template file for writing
        printf("Failed to open config template (%s).\n", templateFile);
        return false;
    }
    out << config_template;
    out.close();
    if (out.fail())
    {
        // failed to write template file
        printf("Failed to write config template (%s).\n", templateFile);
        return false;
    }
    return true;
}