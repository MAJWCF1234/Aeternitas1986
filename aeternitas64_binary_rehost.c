/*
 * Thin launcher for the recovered Windows build: runs aeternitas64.exe from
 * the same directory as this program (the directory of the running module).
 *
 * The fat embedded-PE version (~600k+ bytes of hex in one .c file) is optional.
 * Regenerate it with: py tools/generate_binary_rehost_c.py
 *
 * Normal workflow: place or build aeternitas64.exe next to aeternitas64_binary_rehost.exe.
 */

#include <errno.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32)
#error This rehost launcher targets Windows (spawn of PE).
#else
#include <windows.h>
#endif

static int peer_exe_path(char *out, size_t cap) {
  char module[MAX_PATH];
  char *slash;

  if (GetModuleFileNameA(NULL, module, MAX_PATH) == 0)
    return -1;
  slash = strrchr(module, '\\');
  if (!slash)
    slash = strrchr(module, '/');
  if (!slash)
    return -1;
  *slash = '\0';
  if ((strlen(module) + 1 + strlen("aeternitas64.exe") + 1) > cap)
    return -1;
  snprintf(out, cap, "%s\\aeternitas64.exe", module);
  return 0;
}

static int spawn_peer(const char *exe, int argc, char **argv) {
  char **av;
  intptr_t rc;
  int i;

  av = (char **)calloc((size_t)argc + 1, sizeof(char *));
  if (!av) {
    fputs("aeternitas64_binary_rehost: out of memory\n", stderr);
    return 125;
  }
  av[0] = (char *)exe;
  for (i = 1; i < argc; i++)
    av[i] = argv[i];
  av[argc] = NULL;

  rc = _spawnv(_P_WAIT, exe, (const char *const *)av);
  free(av);
  if (rc == -1) {
    fprintf(stderr, "aeternitas64_binary_rehost: spawn failed: %s\n", strerror(errno));
    return 127;
  }
  return (int)rc;
}

int main(int argc, char **argv) {
  char path[MAX_PATH];
  FILE *fp;

  if (peer_exe_path(path, sizeof path) != 0) {
    fputs("aeternitas64_binary_rehost: could not resolve peer path\n", stderr);
    return 124;
  }

  fp = fopen(path, "rb");
  if (!fp) {
    fprintf(stderr,
            "aeternitas64_binary_rehost: missing %s\n"
            "  Build aeternitas64.exe, copy it next to this binary, or run:\n"
            "    py tools/generate_binary_rehost_c.py\n",
            path);
    return 2;
  }
  fclose(fp);

  return spawn_peer(path, argc, argv);
}
