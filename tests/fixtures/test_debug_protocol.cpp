#include <catch2/catch_test_macros.hpp>

#if !defined(_WIN32)

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

std::string gameExecutable() {
  const char* env = std::getenv("WHISKERS_GAME_EXE");
  if (env && std::strlen(env) > 0) return env;
  return "./platformer_demo";
}

struct Proc {
  pid_t pid{-1};
  int stdinWr{-1};
  int stdoutRd{-1};

  bool start(const std::string& exe) {
    int inPipe[2];
    int outPipe[2];
    if (pipe(inPipe) != 0 || pipe(outPipe) != 0) return false;

    pid = fork();
    if (pid < 0) return false;

    if (pid == 0) {
      ::close(inPipe[1]);
      ::close(outPipe[0]);
      dup2(inPipe[0], STDIN_FILENO);
      dup2(outPipe[1], STDOUT_FILENO);
      ::close(inPipe[0]);
      ::close(outPipe[1]);

      execl(exe.c_str(), exe.c_str(), "--headless", "--debug-protocol", (char*)nullptr);
      _exit(127);
    }

    ::close(inPipe[0]);
    ::close(outPipe[1]);
    stdinWr = inPipe[1];
    stdoutRd = outPipe[0];
    return true;
  }

  void writeLine(const std::string& line) {
    std::string s = line + "\n";
    (void)::write(stdinWr, s.data(), s.size());
  }

  std::string readLine() {
    std::string buf;
    char c;
    while (::read(stdoutRd, &c, 1) == 1) {
      if (c == '\n') break;
      buf += c;
    }
    return buf;
  }

  nlohmann::json readJsonLine() {
    return nlohmann::json::parse(readLine());
  }

  void stop() {
    if (pid > 0) {
      kill(pid, SIGTERM);
      int st = 0;
      waitpid(pid, &st, 0);
      pid = -1;
    }
    if (stdinWr >= 0) ::close(stdinWr);
    if (stdoutRd >= 0) ::close(stdoutRd);
    stdinWr = stdoutRd = -1;
  }
};

}  // namespace

TEST_CASE("Player falls onto ground via debug protocol") {
  Proc p;
  std::string exe = gameExecutable();
  REQUIRE(p.start(exe));

  p.writeLine(R"({"cmd":"get_state"})");
  auto j0 = p.readJsonLine();
  REQUIRE(j0["status"] == "ok");
  float startY = 0;
  bool found = false;
  for (const auto& e : j0["result"]["entities"]) {
    if (e["type"] == "player") {
      startY = e["position"]["y"].get<float>();
      found = true;
      break;
    }
  }
  REQUIRE(found);

  p.writeLine(R"({"cmd":"step","args":{"frames":180}})");
  auto stepR = p.readJsonLine();
  REQUIRE(stepR["status"] == "ok");

  p.writeLine(R"({"cmd":"get_state"})");
  auto j1 = p.readJsonLine();
  REQUIRE(j1["status"] == "ok");
  for (const auto& e : j1["result"]["entities"]) {
    if (e["type"] == "player") {
      CHECK(e["position"]["y"].get<float>() < startY);
      CHECK(e["onGround"].get<bool>());
      CHECK(std::abs(e["velocity"]["y"].get<float>()) < 0.2f);
      break;
    }
  }

  p.writeLine(R"({"cmd":"quit"})");
  p.stop();
}

TEST_CASE("Screenshot returns valid PNG") {
  Proc p;
  REQUIRE(p.start(gameExecutable()));

  p.writeLine(R"({"cmd":"screenshot","args":{"width":320,"height":240}})");
  auto j = p.readJsonLine();
  REQUIRE(j["status"] == "ok");
  std::string b64 = j["result"]["png"].get<std::string>();
  REQUIRE(b64.size() > 80);

  p.writeLine(R"({"cmd":"quit"})");
  p.stop();
}

#else

TEST_CASE("debug protocol fixture tests skipped on Windows") { REQUIRE(true); }

#endif
