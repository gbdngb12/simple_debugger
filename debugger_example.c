#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#define MASK ULONG_MAX

struct opcode_address {
    char* instruction;
    size_t inst_size;
    unsigned long address;
};

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

/**
 * @brief 다음 명령어 실행
 *
 * @param pid
 * @param regs
 * @return struct opcode_address 실행한 명령어의 opcode, 주소
 */
struct opcode_address next_instruction(pid_t pid, struct user_regs_struct* regs, int* wait_status);

/**
 * @brief 레지스터 정보를 가져온다.
 *
 * @param pid thread/process id
 * @param regs 저장할 레지스터 정보
 */
void get_register(pid_t pid, struct user_regs_struct* regs);

void usage(const char* program) {
    printf("%s <target>", program);
}

void get_register(pid_t pid, struct user_regs_struct* regs) {
    if (ptrace(PTRACE_GETREGS, pid, 0, regs) == -1) {
        perror("ptrace(PTRACE_GETREGS)");
        exit(-1);
    }
}

struct opcode_address next_instruction(pid_t pid, struct user_regs_struct* regs, int* wait_status) {
    unsigned long prev_instr = regs->rip;
    struct opcode_address ret = {
        .instruction = NULL,
        .inst_size = 0,
        .address = prev_instr

    };
    // 2. 아직 까지도 wait된 상태에서 PTRACE_PEEKTEXT(opcode를 가져온다.)
    unsigned long instr = ptrace(PTRACE_PEEKTEXT, pid, regs->rip, 0);

    // 4. 단일 명령 실행후 다시 멈춘다.
    long ret_addr = ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
    if (ret_addr == -1) {
        perror("ptrace(PTRACE_SINGLESTEP)");
        exit(-1);
    }
    // 3. child process가 wait될때까지 기다린다.
    if (wait(wait_status) == -1) {
        perror("wait");
        exit(-1);
    }
    if (WIFSTOPPED(*wait_status)) {
        // 여기서 실제 명령어의 오프셋을 계산한다.
        get_register(pid, regs);

        return ret;
    }
    ret.address = 0;
    return ret;
}

void debugger(pid_t pid) {
    int wait_status;
    struct user_regs_struct regs;
    unsigned long base_address = 0;
    waitpid(pid, &wait_status, 0);

    while (WIFSTOPPED(wait_status)) {
        // 1. wait된 child process에서 PTRAGE_GETREGS를 수행한다.
        get_register(pid, &regs);

        // 2. 다음 명령어의 opcode를 들고오고 한줄 실행한다.
        struct opcode_address instr = next_instruction(pid, &regs, &wait_status);
        if (instr.address == 0) {
            // 끝
        } else {
            // 끝이 아니므로 출력

            if (instr.address == 0x401273) {
                // rip 조작..
                regs.rip = 0x40129e;
                ptrace(PTRACE_SETREGS, pid, 0, &regs);
            } 
            if(instr.address == 0x4012a2) {
                // rdi에는 문자열 주소가 존재함!!
                // 이제 이 값을 수정
                ptrace(PTRACE_POKEDATA, pid, regs.rdi, 0xb1ea998feb80b9ea /*김동건*/);//8바이트 복사
                ptrace(PTRACE_POKEDATA, pid, regs.rdi + 8, 0x00000000000000b4);//8바이트 복사
            }
        }
    }
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