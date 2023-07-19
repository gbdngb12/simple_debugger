#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * @brief pid process(thread)를 추적(trace)한다.
 *
 * @param pid process/thread의 id
 */
void debugger(pid_t pid);

/**
 * @brief 도구의 사용법을 출력한다.
 *
 * @param program 파일 이름
 */
void usage(const char* program);


void usage(const char* program) {
    printf("%s <target>", program);
}

void debugger(pid_t pid) {
    int wait_status;    
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        usage(argv[0]);
        return -1;
    }
    pid_t pid = fork();

    if (pid == 0) {
        // Child process
        if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
            perror("ptrace(PTRACE_TRACEME)");
            exit(-1);
        }
        execl(argv[1], argv[1], NULL);
    } else if (pid != 0) {
        // Parent process
        debugger(pid);
    } else {
        // Error Handling
        perror("fork");
        return -1;
    }

    return 0;
}