// DebugProtocol.h — JSON-over-stdio command queue (stdin reader thread).
#pragma once

#ifdef WHISKERS_DEBUG_PROTOCOL

#include <atomic>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class DebugProtocol {
 public:
  DebugProtocol();
  ~DebugProtocol();

  void start();
  void shutdown();

  // Pop all complete lines queued since last call (called from main thread).
  void drainIncoming(std::vector<std::string>& out);

 private:
  void readerLoop();

  std::mutex mutex_;
  std::deque<std::string> lines_;
  std::thread reader_;
  std::atomic<bool> running_{false};
  std::atomic<bool> readerExit_{false};
};

#endif  // WHISKERS_DEBUG_PROTOCOL
