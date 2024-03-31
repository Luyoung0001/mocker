#include <cstring>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>

static void run(int argc, char *argv[]);
static std::string cmd(int argc, char *argv[]);
static void run_child(int argc, char *argv[]);

int main(int argc, char *argv[]) {

    if (argc < 3) {
        std::cerr << "Too few arguments" << std::endl;
        exit(-1);
    }
    if (!strcmp(argv[1], "run")) {
        run(argc - 2, &argv[2]);
    }
    return 0;
}
static void run(int argc, char *argv[]) {
    // 合成命令
    std::cout << "parent running " << cmd(argc, argv) << " as " << getpid()
              << std::endl;

    if (unshare(CLONE_NEWPID) < 0) {
        std::cerr << "failed to unshare in child: PID" << std::endl;
        exit(-1);
    }

    // 用子进程运行命令,同时父进程保持

    pid_t child_pid = fork();
    if (child_pid < 0) {
        std::cerr << "failed to fork" << std::endl;
        return;
    }
    if (child_pid)

    {
        if (waitpid(child_pid, NULL, 0) < 0) {
            std::cerr << "failed to wait for child" << std::endl;
        } else {
            std::cout << "child terminited" << std::endl;
        }
    } else {
        run_child(argc, argv);
    }
}

const char *child_hostname = "container";

static void run_child(int argc, char *argv[]) {
    std::cout << "child running " << cmd(argc, argv) << " as " << getpid()
              << std::endl;
    int flags = CLONE_NEWUTS | CLONE_NEWNS;
    if (unshare(flags) < 0) {
        std::cerr << "failed to share in child: UTS" << std::endl;
        exit(-1);
    }
    if (mount(NULL, "/", NULL, MS_SLAVE | MS_REC, NULL) < 0) {
        std::cerr << "failed to mount /" << std::endl;
    }

    // 修改 filesystem 的 view,在制定目录下
    if (chroot("./ubuntu_rootfs") < 0) {
        std::cerr << "failed to chroot" << std::endl;
        exit(-1);
    }
    if (chdir("/") < 0) {
        std::cerr << "failed to chdir to /" << std::endl;
        exit(-1);
    }

    // 挂载
    if (mount("proc", "proc", "proc", 0, NULL) < 0) {
        std::cerr << "failed to mount /proc" << std::endl;
    }

    // 修改 host name
    if (sethostname(child_hostname, strlen(child_hostname)) < 0) {
        std::cerr << "failed to changge hostname" << std::endl;
    }

    if (execvp(argv[0], argv)) {
        perror("execvp failed");
    }
}

static std::string cmd(int argc, char *argv[]) {
    std::string cmd = "";
    for (int i = 0; i < argc; i++) {
        cmd.append(argv[i] + std::string(" "));
    }
    return cmd;
}