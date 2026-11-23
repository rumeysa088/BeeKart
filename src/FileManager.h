#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <string>
#include <vector>

class FileManager {
public:
    static std::vector<std::string> split(const std::string& s, char delim);
    static std::string trim(const std::string& s);
    static std::vector<std::string> readLines(const std::string& path);
    static bool writeLines(const std::string& path,
                           const std::vector<std::string>& lines);
    static bool appendLine(const std::string& path,
                           const std::string& line);
    static bool fileExists(const std::string& path);
    static void ensureDir(const std::string& path);
};




#endif
