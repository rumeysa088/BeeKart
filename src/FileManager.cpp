#include "FileManager.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include <sstream>
#include <direct.h>

std::vector<std::string> FileManager::split(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) out.push_back(item);
    return out;
}

std::string FileManager::trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    return s.substr(a, b - a + 1);
}

std::vector<std::string> FileManager::readLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    if (!file.is_open()) return lines;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

bool FileManager::writeLines(const std::string& path,
                             const std::vector<std::string>& lines) {
    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) return false;
    for (const auto& l : lines) file << l << "\n";
    return true;
}

bool FileManager::appendLine(const std::string& path,
                             const std::string& line) {
    std::ofstream file(path, std::ios::app);
    if (!file.is_open()) return false;
    file << line << "\n";
    return true;
}

bool FileManager::fileExists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

/*
void FileManager::ensureDir(const std::string& path) {
    mkdir(path.c_str(), 0755);
}*/


void FileManager::ensureDir(const std::string& path) {
    #ifdef _WIN32
        _mkdir(path.c_str());
    #else
        mkdir(path.c_str(), 0755);
    #endif
}
