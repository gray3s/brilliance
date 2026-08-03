#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

#ifndef SCRIPT_REL_PATH
#error "SCRIPT_REL_PATH must be defined at compile time"
#endif

static std::string dirnameOf(const std::string &path) {
    const std::string::size_type pos = path.find_last_of('/');
    if (pos == std::string::npos) {
        return ".";
    }
    if (pos == 0) {
        return "/";
    }
    return path.substr(0, pos);
}

static std::string selfPath(const char *argv0) {
    char buffer[PATH_MAX + 1];
    const ssize_t len = readlink("/proc/self/exe", buffer, PATH_MAX);
    if (len > 0) {
        buffer[len] = '\0';
        return std::string(buffer);
    }

    char resolved[PATH_MAX + 1];
    if (argv0 != nullptr && realpath(argv0, resolved) != nullptr) {
        return std::string(resolved);
    }

    return argv0 == nullptr ? std::string(".") : std::string(argv0);
}

static bool executableFile(const std::string &path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode) && access(path.c_str(), X_OK) == 0;
}

int main(int argc, char **argv) {
    const std::string executable = selfPath(argc > 0 ? argv[0] : nullptr);
    const std::string binDir = dirnameOf(executable);
    const std::string repoRoot = dirnameOf(binDir);
    const std::string scriptPath = repoRoot + "/" + SCRIPT_REL_PATH;

    if (getenv("AICHESS_BINARY_TRACE") != nullptr) {
        std::cerr << "script launcher: exe=" << executable << "\n";
        std::cerr << "script launcher: repo_root=" << repoRoot << "\n";
        std::cerr << "script launcher: script=" << scriptPath << "\n";
    }

    if (!executableFile(scriptPath)) {
        std::cerr << "script launcher: target is not executable: " << scriptPath << "\n";
        return 127;
    }

    std::vector<char *> execArgs;
    execArgs.reserve(static_cast<size_t>(argc) + 1);
    execArgs.push_back(const_cast<char *>(scriptPath.c_str()));
    for (int i = 1; i < argc; ++i) {
        execArgs.push_back(argv[i]);
    }
    execArgs.push_back(nullptr);

    execv(scriptPath.c_str(), execArgs.data());
    std::cerr << "script launcher: exec failed for " << scriptPath << ": " << strerror(errno) << "\n";
    return 126;
}
