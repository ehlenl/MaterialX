#pragma once

#include <fstream>
#include <string>
#include <vector>

std::ifstream::pos_type QueryFileSize(const std::string &filename);
uint32_t *ReadBinaryFile(uint32_t& length, const char* filename);
std::vector<std::string> tokenize(const std::string& str, std::string delim);

#if !defined(WIN32)
errno_t fopen_s(FILE** pFile, const char *filename, const char *mode);
#endif