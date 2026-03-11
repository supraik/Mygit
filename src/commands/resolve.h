#ifndef RESOLVE_H
#define RESOLVE_H

#include <string>

class Resolve {
private:
    bool findRepoRoot(std::string& repoPath);
    bool hasConflictMarkers(const std::string& filePath);

public:
    bool execute();
};

#endif
