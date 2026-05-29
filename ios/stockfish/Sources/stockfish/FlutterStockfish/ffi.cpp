#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <atomic>

#include "../Engine/src/bitboard.h"
#include "../Engine/src/misc.h"
#include "../Engine/src/position.h"
#include "../Engine/src/types.h"
#include "../Engine/src/uci.h"
#include "../Engine/src/tune.h"

#include "ffi.h"

// https://jineshkj.wordpress.com/2006/12/22/how-to-capture-stdin-stdout-and-stderr-of-child-program/
#define NUM_PIPES 2
#define PARENT_WRITE_PIPE 0
#define PARENT_READ_PIPE 1
#define READ_FD 0
#define WRITE_FD 1
#define PARENT_READ_FD (pipes[PARENT_READ_PIPE][READ_FD])
#define PARENT_WRITE_FD (pipes[PARENT_WRITE_PIPE][WRITE_FD])
#define CHILD_READ_FD (pipes[PARENT_WRITE_PIPE][READ_FD])
#define CHILD_WRITE_FD (pipes[PARENT_READ_PIPE][WRITE_FD])

int stockfish_engine_main(int, char **);

static const char *QUITOK = "quitok\n";
static int pipes[NUM_PIPES][2];
static char buffer[4096];

// 引擎线程
static pthread_t engine_thread;

// 引擎运行状态 - 使用 atomic 确保线程安全
static std::atomic<bool> engine_running(false);
static std::atomic<bool> engine_initialized(false);

// 引擎线程函数
static void* engine_thread_func(void* arg) {
    // 重定向标准输入输出
    dup2(CHILD_READ_FD, STDIN_FILENO);
    dup2(CHILD_WRITE_FD, STDOUT_FILENO);

    int argc = 1;
    char arg0[] = "";
    char *argv[] = {arg0};
    stockfish_engine_main(argc, argv);

    std::cout << QUITOK << std::flush;
    engine_running = false;
    return nullptr;
}

int stockfish_init()
{
  if (engine_initialized) {
    // Send quit to the old thread to terminate it cleanly during Hot Restart
    if (engine_running) {
      write(PARENT_WRITE_FD, "quit\n", 5);
      int retry = 0;
      while (engine_running && retry < 20) {
        usleep(50000);  // 50ms
        retry++;
      }
    }

    // Clean up old pipes
    close(pipes[PARENT_READ_PIPE][READ_FD]);
    close(pipes[PARENT_READ_PIPE][WRITE_FD]);
    close(pipes[PARENT_WRITE_PIPE][READ_FD]);
    close(pipes[PARENT_WRITE_PIPE][WRITE_FD]);
  }

  pipe(pipes[PARENT_READ_PIPE]);
  pipe(pipes[PARENT_WRITE_PIPE]);

  // Set PARENT_READ_FD to non-blocking so stdout_read won't block the Dart isolate
  int flags = fcntl(PARENT_READ_FD, F_GETFL, 0);
  fcntl(PARENT_READ_FD, F_SETFL, flags | O_NONBLOCK);

  engine_initialized = true;
  return 0;
}

int stockfish_main()
{
  if (engine_running) return -1;

  engine_running = true;
  pthread_create(&engine_thread, nullptr, engine_thread_func, nullptr);
  pthread_detach(engine_thread);

  // 立即返回，让 Dart 代码可以轮询
  return 0;
}

bool stockfish_is_running()
{
  return engine_running;
}

void stockfish_reset_state()
{
  engine_running = false;
  engine_initialized = false;
}

ssize_t stockfish_stdin_write(char *data)
{
  return write(PARENT_WRITE_FD, data, strlen(data));
}

char *stockfish_stdout_read()
{
  ssize_t count = read(PARENT_READ_FD, buffer, sizeof(buffer) - 1);
  if (count < 0)
  {
    return NULL;
  }

  buffer[count] = 0;
  if (strcmp(buffer, QUITOK) == 0)
  {
    return NULL;
  }

  return buffer;
}
