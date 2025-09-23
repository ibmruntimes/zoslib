#include "sys/ptrace.h" 
#include "gtest/gtest.h"
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

namespace {

TEST(PtraceTest, TraceMeAndDetach) {
  const int child_exit_code = 42;
  pid_t pid = fork();

  if (pid == 0) {
    if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
      _exit(EXIT_FAILURE);
    }
    // Stop so the parent knows we are ready.
    raise(SIGSTOP);

    _exit(child_exit_code);

  } else if (pid > 0) {
    int status;
    pid_t child_pid;

    // 1. Wait for the initial SIGSTOP from the child.
    child_pid = waitpid(pid, &status, 0);
    ASSERT_EQ(child_pid, pid);
    ASSERT_TRUE(WIFSTOPPED(status))
        << "Child should be stopped after initial raise(SIGSTOP)";

    // 2. Detach from the child process. This allows the child to resume
    //    normal execution as if it were never being traced.
    //    The 'data' argument (fourth param) is a signal to deliver upon detach.
    //    NULL (0) means deliver no signal.
    long ret = ptrace(PTRACE_DETACH, pid, NULL, NULL);
    ASSERT_EQ(ret, 0) << "PTRACE_DETACH failed. errno: " << errno;

    // 3. Now that the child is running freely, wait for it to terminate.
    //    There will be no more ptrace-stops.
    child_pid = waitpid(pid, &status, 0);
    ASSERT_EQ(child_pid, pid);

    // 4. Verify the child exited normally with the correct code.
    ASSERT_TRUE(WIFEXITED(status))
        << "Child should have exited normally after being detached";
    EXPECT_EQ(WEXITSTATUS(status), child_exit_code)
        << "Child exited with an unexpected status code";

  } else {
    FAIL() << "Failed to fork a child process. errno: " << errno;
  }
}

TEST(PtraceTest, ReadBlockFromSharedMemory) {
  // 1. Create a shared memory segment for an 8-byte long.
  int shm_id = shmget(IPC_PRIVATE, sizeof(long), IPC_CREAT | 0666);
  ASSERT_NE(shm_id, -1) << "shmget failed. errno: " << errno;

  void *shared_addr = shmat(shm_id, NULL, 0);
  ASSERT_NE(shared_addr, (void *)-1) << "shmat failed. errno: " << errno;

  long* shared_long = static_cast<long*>(shared_addr);
  const long secret_value = 0xFEEDBEEFDEADBEEFL;
  pid_t pid = fork();

  if (pid == 0) {
    // --- Child Process ---
    *shared_long = secret_value;

    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    raise(SIGSTOP);
    _exit(EXIT_SUCCESS);

  } else if (pid > 0) {
    // --- Parent Process ---
    int status;
    long buffer_for_result = 0;

    waitpid(pid, &status, 0);
    ASSERT_TRUE(WIFSTOPPED(status));

    // Use PT_READ_BLOCK to read the 8-byte long.
    long ret = ptrace(PT_READ_BLOCK, pid, shared_addr, (void*)sizeof(long), &buffer_for_result);
    
    ASSERT_EQ(ret, sizeof(long)) << "PTRACE_READ_BLOCK should return the length read.";
    ASSERT_EQ(errno, 0) << "PTRACE_READ_BLOCK failed. errno: " << errno;

    // Verify the value placed in our local buffer is correct.
    EXPECT_EQ(buffer_for_result, secret_value);

    // Clean up.
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    waitpid(pid, &status, 0);
    
    shmdt(shared_addr);
    shmctl(shm_id, IPC_RMID, NULL);

  } else {
    FAIL() << "Fork failed";
    shmdt(shared_addr);
    shmctl(shm_id, IPC_RMID, NULL);
  }
}

} // namespace
