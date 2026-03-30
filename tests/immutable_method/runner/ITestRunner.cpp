#include "ITestRunner.h"
#include <windows.h>
#include <string>
#include <filesystem>

// Helper to run an external executable and return its exit code
static int run_external(const std::wstring &exePath, const std::wstring &args) {
  STARTUPINFOW si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};

  // Build a robust command line: "<exePath>" <args>
  std::wstring cmdLine = L"\"" + exePath + L"\"";
  if (!args.empty()) {
    cmdLine += L" " + args;
  }
  // CreateProcess with NULL lpApplicationName and a writable command line
  std::wstring cmdLineMutable = cmdLine; // CreateProcess may modify this buffer
  BOOL ok = CreateProcessW(
      nullptr,                   // lpApplicationName
      &cmdLineMutable[0],        // lpCommandLine
      NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
  if (!ok) {
    return -1; // Could not start process
  }

  // Wait for completion
  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exitCode = 0;
  GetExitCodeProcess(pi.hProcess, &exitCode);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return static_cast<int>(exitCode);
}

// Locate test binary from environment or a set of common build locations
static std::wstring locate_test_bin() {
  // 1) Environment variable override
  wchar_t envBuf[MAX_PATH] = {0};
  DWORD len = GetEnvironmentVariableW(L"IMMUTABLE_METHOD_TEST_BIN", envBuf, MAX_PATH);
  if (len > 0) {
    return std::wstring(envBuf);
  }

  // 2) Common candidate paths (prioritize newer/local builds)
  auto exists = [](const std::wstring &p) {
    DWORD attrs = GetFileAttributesW(p.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
  };
  const std::wstring candidates[] = {
    L".\\tests\\immutable_method\\Debug\\immutable_method_catch2_test.exe",
    L".\\build\\tests\\immutable_method\\Debug\\immutable_method_catch2_test.exe",
    L".\\Debug\\immutable_method_catch2_test.exe",
    L"..\\..\\build\\tests\\immutable_method\\Debug\\immutable_method_catch2_test.exe",
    L"D:\\projects\\CppProjs\\c-hat\\build\\tests\\immutable_method\\Debug\\immutable_method_catch2_test.exe",
    L"D:\\projects\\CppProjs\\c-hat\\Debug\\immutable_method_catch2_test.exe",
  };

  for (const auto &p : candidates) {
    if (exists(p)) {
      // Debug: reveal which candidate is selected
      wprintf(L"[Debugger] Found candidate test binary: %s\n", p.c_str());
      return p;
    }
  }

  // 3) Final fallback: try a path relative to repo root if detection fails
  std::wstring fallback = L".\\tests\\immutable_method\\Debug\\immutable_method_catch2_test.exe";
  return fallback;
}

extern "C" int run_immutable_method_tests() {
  // Optional verbose debug: enable via IMMUTABLE_METHOD_TEST_DEBUG (non-zero)
  bool verbose = false;
  wchar_t dbgBuf[8] = {0};
  DWORD dbgLen = GetEnvironmentVariableW(L"IMMUTABLE_METHOD_TEST_DEBUG", dbgBuf, 8);
  if (dbgLen > 0 && dbgBuf[0] != L'0') verbose = true;
  std::wstring exePath = locate_test_bin();
  // Debug: log which test binary path is chosen
  if (!exePath.empty()) {
    wprintf(L"Using immutable_method test binary: %s\n", exePath.c_str());
  } else {
    wprintf(L"[Warning] No test binary path resolved for immutable_method tests.\n");
  }
  if (verbose) {
    wprintf(L"[Verbose] Executing with -s argument.\n");
  }
  // If the path is invalid, return a non-zero indicative code
  DWORD attrs = GetFileAttributesW(exePath.c_str());
  if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
    return -1;
  }

  // Execute test binary with standard arguments
  int code = run_external(exePath, L"-s");
  return code;
}
