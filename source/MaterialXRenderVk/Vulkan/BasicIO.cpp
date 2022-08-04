#include "BasicIO.h"
#include <iostream>
#include <cmath>

std::ifstream::pos_type QueryFileSize(const std::string &filename)
{
    std::ifstream infile(filename.c_str(), std::ifstream::ate | std::ifstream::binary);
    auto sz = infile.tellg();
    infile.close();
    return sz;
}

uint32_t *ReadBinaryFile(uint32_t& length, const char* filename)
{
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        // this fallback is for hwk-local, where our working directory isn't
        // what either the deployed application or local developer setups expect
        std::string alternate = "/";
        alternate += std::string(filename);
        fp = fopen(alternate.c_str(), "rb");
        if (fp == NULL) {
            printf("Could not find or open file: %s\n", filename);
            return nullptr;
        }
    }

    // get file size.
    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    long filesizepadded = long(std::ceil(filesize / 4.0)) * 4;

    // read file contents.
    //auto str = std::allocate_shared<uint32_t>(std::allocator<char>(), filesizepadded);
    uint32_t *str = new uint32_t[filesizepadded];
    fread(str, filesize, sizeof(char), fp);
    fclose(fp);

    length = filesizepadded;
    return str;
}

std::vector<std::string> tokenize(const std::string& str, std::string delim)
{
    std::string s(str);
    if (s.find(delim) == std::string::npos)
        return { s };
    std::vector<std::string> tokens;
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delim)) != std::string::npos) {
        token = s.substr(0, pos);
        tokens.push_back(token);
        s.erase(0, pos + delim.length());
    }
    token = s.substr(0, s.length());
    tokens.push_back(token);
    return tokens;
}

#if !defined(WIN32)
#include <errno.h>
errno_t fopen_s(
   FILE** pFile,
   const char *filename,
   const char *mode
)
{
    *pFile = fopen(filename, mode);
    return errno;
}
#endif