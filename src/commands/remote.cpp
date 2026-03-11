#include "remote.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

bool Remote::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        return listRemotes(false);
    }
    
    std::string subcommand = args[0];
    
    if (subcommand == "add") {
        if (args.size() < 3) {
            std::cerr << "Usage: mygit remote add <name> <url>\n";
            return false;
        }
        return addRemote(args[1], args[2]);
    }
    else if (subcommand == "remove" || subcommand == "rm") {
        if (args.size() < 2) {
            std::cerr << "Usage: mygit remote remove <name>\n";
            return false;
        }
        return removeRemote(args[1]);
    }
    else if (subcommand == "-v" || subcommand == "--verbose") {
        return listRemotes(true);
    }
    else {
        std::cerr << "Unknown remote subcommand: " << subcommand << "\n";
        std::cerr << "Usage: mygit remote [add|remove|-v]\n";
        return false;
    }
}

bool Remote::addRemote(const std::string& name, const std::string& url) {
    std::string configPath = ".mygit/config";
    
    // Read existing config
    std::vector<std::string> lines;
    std::ifstream configFile(configPath);
    if (configFile) {
        std::string line;
        while (std::getline(configFile, line)) {
            lines.push_back(line);
        }
        configFile.close();
    }
    
    // Check if remote already exists
    std::map<std::string, std::string> remotes = getRemotes();
    if (remotes.find(name) != remotes.end()) {
        std::cerr << "Remote '" << name << "' already exists\n";
        return false;
    }
    
    // Append new remote
    std::ofstream outFile(configPath, std::ios::app);
    if (!outFile) {
        std::cerr << "Failed to open config file\n";
        return false;
    }
    
    outFile << "[remote \"" << name << "\"]\n";
    outFile << "\turl = " << url << "\n";
    
    std::cout << "Added remote '" << name << "': " << url << "\n";
    return true;
}

bool Remote::removeRemote(const std::string& name) {
    std::string configPath = ".mygit/config";
    
    // Read existing config
    std::vector<std::string> lines;
    std::ifstream configFile(configPath);
    if (!configFile) {
        std::cerr << "Config file not found\n";
        return false;
    }
    
    std::string line;
    while (std::getline(configFile, line)) {
        lines.push_back(line);
    }
    configFile.close();
    
    // Remove remote section
    std::vector<std::string> newLines;
    bool inRemoteSection = false;
    bool found = false;
    
    for (const auto& line : lines) {
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        
        if (trimmed[0] == '[' && trimmed.back() == ']') {
            std::string section = trimmed.substr(1, trimmed.length() - 2);
            if (section == "remote \"" + name + "\"") {
                inRemoteSection = true;
                found = true;
                continue;
            } else {
                inRemoteSection = false;
            }
        }
        
        if (!inRemoteSection) {
            newLines.push_back(line);
        }
    }
    
    if (!found) {
        std::cerr << "Remote '" << name << "' not found\n";
        return false;
    }
    
    // Write back
    std::ofstream outFile(configPath);
    if (!outFile) {
        std::cerr << "Failed to write config file\n";
        return false;
    }
    
    for (const auto& line : newLines) {
        outFile << line << "\n";
    }
    
    std::cout << "Removed remote '" << name << "'\n";
    return true;
}

bool Remote::listRemotes(bool verbose) {
    std::map<std::string, std::string> remotes = getRemotes();
    
    if (remotes.empty()) {
        return true;
    }
    
    for (const auto& [name, url] : remotes) {
        if (verbose) {
            std::cout << name << "\t" << url << " (fetch)\n";
            std::cout << name << "\t" << url << " (push)\n";
        } else {
            std::cout << name << "\n";
        }
    }
    
    return true;
}

std::map<std::string, std::string> Remote::getRemotes() {
    std::map<std::string, std::string> remotes;
    std::string configPath = ".mygit/config";
    
    std::ifstream configFile(configPath);
    if (!configFile) {
        return remotes;
    }
    
    std::string line;
    bool inRemoteSection = false;
    std::string currentRemote;
    
    while (std::getline(configFile, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty()) continue;
        
        if (line[0] == '[' && line.back() == ']') {
            std::string section = line.substr(1, line.length() - 2);
            if (section.substr(0, 7) == "remote ") {
                currentRemote = section.substr(8, section.length() - 9);
                inRemoteSection = true;
            } else {
                inRemoteSection = false;
            }
        } else if (inRemoteSection) {
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = line.substr(0, eqPos);
                std::string value = line.substr(eqPos + 1);
                
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                if (key == "url") {
                    remotes[currentRemote] = value;
                }
            }
        }
    }
    
    return remotes;
}
