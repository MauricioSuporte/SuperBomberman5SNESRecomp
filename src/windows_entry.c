/* ROM-independent desktop entry point, shared by development and setup packs. */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <wchar.h>

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR unused, int show) {
  wchar_t root[32768], executable[32768];
  (void)instance; (void)previous; (void)unused;
  DWORD length = GetModuleFileNameW(NULL, root, ARRAYSIZE(root));
  if (!length || length >= ARRAYSIZE(root)) return 1;
  wchar_t *slash = wcsrchr(root, L'\\');
  if (!slash) return 1;
  *slash = 0;
  const wchar_t *paths[] = {
    L"%s\\SuperBomberman5SNESRecomp.exe",
    L"%s\\build-lle\\SuperBomberman5SNESRecomp.exe"
  };
  unsigned i;
  for (i = 0; i < ARRAYSIZE(paths); ++i) {
    int written = _snwprintf(executable, ARRAYSIZE(executable), paths[i], root);
    if (written < 0 || written >= ARRAYSIZE(executable)) continue;
    DWORD attributes = GetFileAttributesW(executable);
    if (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY)) break;
  }
  if (i == ARRAYSIZE(paths)) {
    MessageBoxW(NULL, L"Game executable not found. Extract the complete release ZIP, or build the project with tools/build-windows.ps1.",
                L"Super Bomberman 5", MB_OK | MB_ICONERROR);
    return 1;
  }
  /* Preserve the original Unicode command line after the executable name. */
  const wchar_t *arguments = GetCommandLineW();
  if (*arguments == L'\"') {
    ++arguments;
    while (*arguments && *arguments != L'\"') ++arguments;
    if (*arguments) ++arguments;
  } else {
    while (*arguments && *arguments != L' ' && *arguments != L'\t') ++arguments;
  }
  while (*arguments == L' ' || *arguments == L'\t') ++arguments;
  wchar_t command[32768];
  int size = _snwprintf(command, ARRAYSIZE(command), L"\"%s\" %s",
                        executable, *arguments ? arguments : L"--launcher");
  if (size < 0 || size >= ARRAYSIZE(command)) return 1;
  STARTUPINFOW startup = {0};
  PROCESS_INFORMATION child = {0};
  startup.cb = sizeof(startup);
  if (!CreateProcessW(executable, command, NULL, NULL, FALSE,
                      CREATE_NO_WINDOW, NULL, root, &startup, &child)) {
    MessageBoxW(NULL, L"Unable to start the game. Check that the complete package and its DLLs are present.",
                L"Super Bomberman 5", MB_OK | MB_ICONERROR);
    return 1;
  }
  CloseHandle(child.hThread);
  WaitForSingleObject(child.hProcess, INFINITE);
  DWORD result = 1;
  GetExitCodeProcess(child.hProcess, &result);
  CloseHandle(child.hProcess);
  return (int)result;
}
