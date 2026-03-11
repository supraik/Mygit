#include <iostream>
#include <string>
#include <vector>
#include "commands/init.h"
#include "commands/add.h"
#include "commands/status.h"
#include "commands/ls-files.h"
#include "commands/cat-file.h"
#include "commands/rm.h"
#include "commands/commit.h"
#include "commands/log.h"
#include "commands/checkout.h"
#include "commands/branch.h"
#include "commands/merge.h"
#include "commands/push.h"
#include "commands/pull.h"
#include "commands/remote.h"
#include "commands/resolve.h"

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
    else if (command == "commit") {
        if (argc < 4 || std::string(argv[2]) != "-m") {
            std::cerr << "Usage: mygit commit -m <message>\n";
            return 1;
        }
        Commit commit;
        return commit.execute(argv[3]) ? 0 : 1;
    }
    else if (command == "log") {
        Log log;
        return log.execute() ? 0 : 1;
    }
    else if (command == "checkout") {
        if (argc < 3) {
            std::cerr << "Usage: mygit checkout <branch|commit>\n";
            return 1;
        }
        Checkout checkout;
        return checkout.execute(argv[2]) ? 0 : 1;
    }
    else if (command == "branch") {
        if (argc < 3) {
            std::cerr << "Usage: mygit branch [-d] <branch-name>\n";
            return 1;
        }
        Branch branch;
        if (argc == 4 && std::string(argv[2]) == "-d") {
            // Delete branch: mygit branch -d <branch-name>
            return branch.execute(argv[3], "-d") ? 0 : 1;
        } else {
            // Create branch: mygit branch <branch-name>
            return branch.execute(argv[2]) ? 0 : 1;
        }
    }
    else if (command == "merge") {
        if (argc < 3) {
            std::cerr << "Usage: mygit merge <branch-name>\n";
            return 1;
        }
        Merge merge;
        return merge.execute(argv[2]) ? 0 : 1;
    }
    else if (command == "push") {
        if (argc < 3) {
            std::cerr << "Usage: mygit push <remote> [branch]\n";
            return 1;
        }
        Push push;
        std::string branch = (argc > 3) ? argv[3] : "";
        return push.execute(argv[2], branch) ? 0 : 1;
    }
    else if (command == "pull") {
        if (argc < 3) {
            std::cerr << "Usage: mygit pull <remote> [branch]\n";
            return 1;
        }
        Pull pull;
        std::string branch = (argc > 3) ? argv[3] : "";
        return pull.execute(argv[2], branch) ? 0 : 1;
    }
    else if (command == "remote") {
        std::vector<std::string> args;
        for (int i = 2; i < argc; i++) {
            args.push_back(argv[i]);
        }
        Remote remote;
        return remote.execute(args) ? 0 : 1;
    }
    else if (command == "resolve") {
        Resolve resolve;
        return resolve.execute() ? 0 : 1;
    }
    std::cerr << "Unknown command: " << command << "\n";
    return 1;
}