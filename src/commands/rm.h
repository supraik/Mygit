#ifndef RM_H
#define RM_H

#include <string>

class Rm {
public:
    bool execute(const std::string& filePath);
    
private:
    // TODO: Add your helper methods here
    // Hint: You'll need to:
    // 1. Find the repository root
    // 2. Remove the file from the index
    // 3. Optionally delete the file from working directory
};

#endif
