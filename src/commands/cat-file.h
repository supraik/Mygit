#ifndef CAT_FILE_H
#define CAT_FILE_H

#include <string>

class CatFile {
public:
    // Execute with option flag and hash
    bool execute(const std::string& option, const std::string& hash);
    
private:
    // Find .mygit directory
    bool findRepoRoot(std::string& repoPath);
    
    // Read object content from .mygit/objects/
    std::string readObject(const std::string& repoPath, const std::string& hash);
    
    // Parse header and content from object
    void parseObject(const std::string& objectData, 
                     std::string& type, 
                     int& size, 
                     std::string& content);
};

#endif
