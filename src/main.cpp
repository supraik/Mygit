#include <iostream>
#include <string>
#include "commands/init.h"
#include "commands/add.h"
#include "commands/status.h"
#include "commands/ls-files.h"
#include "commands/cat-file.h"
#include "commands/rm.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: mygit <command> [options]\n";
        return 1;
    }
    
    std::string command = argv[1];
    
    if (command == "init") {
        Init init;
        return init.execute() ? 0 : 1;
    }
    else if (command == "add") {
        if (argc < 3) {
            std::cerr << "Usage: mygit add <file>\n";
            return 1;
        }
        Add add;
        return add.execute(argv[2]) ? 0 : 1;
    }
    else if (command == "status") {
        Status status;
        return status.execute() ? 0 : 1;
    }
    else if (command == "ls-files") {
        LsFiles lsFiles;
        return lsFiles.execute() ? 0 : 1;
    }
    else if (command == "cat-file") {
        if (argc < 4) {
            std::cerr << "Usage: mygit cat-file (-p|-s|-t) <object>\n";
            return 1;
        }
        CatFile catFile;
        return catFile.execute(argv[2], argv[3]) ? 0 : 1;
    }
    else if (command == "rm") {
        if (argc < 3) {
            std::cerr << "Usage: mygit rm <file>\n";
            return 1;
        }
        Rm rm;
        return rm.execute(argv[2]) ? 0 : 1;
    }
    
    std::cerr << "Unknown command: " << command << "\n";
    return 1;
}