#include "resolve.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

bool Resolve::execute() {
    std::string repoPath;
    if (!findRepoRoot(repoPath)) {
        std::cerr << "fatal: not a mygit repository\n";
        return false;
    }

    // Check if we're in a merge state
    std::string mergeHeadPath = repoPath + "/MERGE_HEAD";
    if (!fs::exists(mergeHeadPath)) {
        std::cerr << "fatal: no merge in progress\n";
        return false;
    }

    // Read conflict list
    std::string conflictsPath = repoPath + "/MERGE_CONFLICTS";
    if (!fs::exists(conflictsPath)) {
        std::cout << "No conflicts recorded. You can commit the merge.\n";
        return true;
    }

    std::ifstream conflictsFile(conflictsPath);
    if (!conflictsFile) {
        std::cerr << "fatal: failed to read conflict list\n";
        return false;
    }

    std::vector<std::string> unresolvedFiles;
    std::string line;
    while (std::getline(conflictsFile, line)) {
        if (!line.empty()) {
            // Check if file still has conflict markers
            if (hasConflictMarkers(line)) {
                unresolvedFiles.push_back(line);
            }
        }
    }
    conflictsFile.close();

    if (unresolvedFiles.empty()) {
        // All conflicts resolved — remove the conflicts file
        fs::remove(conflictsPath);
        std::cout << "All conflicts resolved. Use 'mygit add <file>' then 'mygit commit -m <message>' to complete the merge.\n";
        return true;
    }

    std::cerr << "The following files still have conflict markers:\n";
    for (const auto& f : unresolvedFiles) {
        std::cerr << "\t" << f << "\n";
    }
    std::cerr << "\nResolve the conflicts by editing the files, removing the conflict markers,\n";
    std::cerr << "then run 'mygit add <file>' and 'mygit commit -m <message>'.\n";
    return false;
}

bool Resolve::findRepoRoot(std::string& repoPath) {
    fs::path current = fs::current_path();
    while (true) {
        fs::path mygitPath = current / ".mygit";
        if (fs::exists(mygitPath) && fs::is_directory(mygitPath)) {
            repoPath = mygitPath.string();
            return true;
        }
        if (!current.has_parent_path() || current == current.parent_path()) {
            return false;
        }
        current = current.parent_path();
    }
}

bool Resolve::hasConflictMarkers(const std::string& filePath) {
    if (!fs::exists(filePath)) {
        return false;
    }

    std::ifstream file(filePath);
    if (!file) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("<<<<<<<") == 0 || line.find(">>>>>>>") == 0) {
            return true;
        }
    }
    return false;
}
