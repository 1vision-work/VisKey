// SPDX-License-Identifier: GPL-3.0-or-later
// Generates the engine snapshot from the ORIGINAL engine. The original engine keeps state in
// globals, so every case runs in a fresh forked process to guarantee independence.
// Usage: gen_snapshot <output-file>
#include "driver_original.h"
#include "../common/cases.h"
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <sys/wait.h>
#include <unistd.h>

using namespace vktest;

static std::string inChild(const std::function<std::string()>& fn) {
    int fd[2];
    if (pipe(fd) != 0) { perror("pipe"); exit(2); }
    pid_t pid = fork();
    if (pid == 0) {
        close(fd[0]);
        std::string out;
        try { out = fn(); } catch (...) { out = "!! exception\n"; }
        size_t off = 0;
        while (off < out.size()) { ssize_t w = write(fd[1], out.data() + off, out.size() - off); if (w <= 0) break; off += size_t(w); }
        close(fd[1]);
        _exit(0);
    }
    close(fd[1]);
    std::string res; char buf[4096]; ssize_t r;
    while ((r = read(fd[0], buf, sizeof buf)) > 0) res.append(buf, size_t(r));
    close(fd[0]);
    int st = 0; waitpid(pid, &st, 0);
    if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) res += "!! crashed\n\n";
    return res;
}

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s <out>\n", argv[0]); return 2; }
    std::string all = "# VisKey engine snapshot. Generated from the original OpenKey engine (see Engine/tests/README.md).\n"
                      "# Do not edit by hand; changes require a reason in the commit message.\n\n";
    for (const auto& c : allCases())
        all += inChild([&] { OriginalDriver d; return renderCase(d, c); });
    for (const auto& c : allConvertCases())
        all += inChild([&] { OriginalDriver d; d.begin(Config{}); return renderConvertCase(d, c); });
    all += inChild([] { OriginalDriver d; d.begin(Config{}); return renderSmartSwitch(d); });
    all += inChild([] { OriginalDriver d; d.begin(Config{}); return renderMacroBlob(d); });
    FILE* f = fopen(argv[1], "wb");
    if (!f) { perror("open"); return 2; }
    fwrite(all.data(), 1, all.size(), f);
    fclose(f);
    printf("wrote %s (%zu cases + %zu convert + 2 misc)\n", argv[1], allCases().size(), allConvertCases().size());
    return 0;
}
