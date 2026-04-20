// DebugProtocol.cpp
#ifdef WHISKERS_DEBUG_PROTOCOL

#include "DebugProtocol.h"

#include <cstdio>
#include <iostream>
#include <sstream>

#ifndef _WIN32
#include <poll.h>
#include <unistd.h>
#endif

DebugProtocol::DebugProtocol() = default;

DebugProtocol::~DebugProtocol() { shutdown(); }

void DebugProtocol::start() {
  if (running_.exchange(true)) return;
  readerExit_ = false;
  reader_ = std::thread([this] { readerLoop(); });
}

void DebugProtocol::shutdown() {
  if (!running_.exchange(false)) return;
  readerExit_ = true;
#ifndef _WIN32
  if (reader_.joinable()) reader_.join();
#else
  if (reader_.joinable()) reader_.detach();
#endif
}

void DebugProtocol::drainIncoming(std::vector<std::string>& out) {
  std::lock_guard<std::mutex> lock(mutex_);
  out.insert(out.end(), std::make_move_iterator(lines_.begin()),
             std::make_move_iterator(lines_.end()));
  lines_.clear();
}

void DebugProtocol::readerLoop() {
#ifndef _WIN32
  std::string buf;
  while (!readerExit_.load()) {
    pollfd pfd;
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;
    int pr = poll(&pfd, 1, 50);
    if (pr < 0) break;
    if (pr == 0) continue;
    if (!(pfd.revents & POLLIN)) continue;

    char chunk[1024];
    ssize_t n = read(STDIN_FILENO, chunk, sizeof(chunk));
    if (n <= 0) break;
    buf.append(chunk, chunk + n);
    for (;;) {
      size_t nl = buf.find('\n');
      if (nl == std::string::npos) break;
      std::string line = buf.substr(0, nl);
      buf.erase(0, nl + 1);
      {
        std::lock_guard<std::mutex> lock(mutex_);
        lines_.push_back(std::move(line));
      }
    }
  }
#else
  std::string line;
  while (!readerExit_.load() && std::getline(std::cin, line)) {
    std::lock_guard<std::mutex> lock(mutex_);
    lines_.push_back(std::move(line));
  }
#endif
}

#endif  // WHISKERS_DEBUG_PROTOCOL
