#include "core/Core.h"

#include <memory>
#include <string>

static bool parseDatabaseFlag(int argc, char *argv[]) {
    bool enable = true;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);

        if (arg == "--database=true") {
            enable = true;
        } else if (arg == "--database=false") {
            enable = false;
        }
    }

    return enable;
}

int main(int argc, char *argv[]) {
    bool enableDb = parseDatabaseFlag(argc, argv);

    std::unique_ptr <Core> core = std::make_unique<Core>();
    core->run();

    core->shutdown();
    return 0;
}
