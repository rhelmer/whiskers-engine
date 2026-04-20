#!/usr/bin/env python3
"""
Optional MCP bridge: exposes debug-protocol helpers for LLM playtesting.

Requires: pip install mcp
Run: python3 tools/mcp_server.py  (stdio MCP transport)

If the `mcp` package is not installed, this script prints setup hints and exits.
"""

from __future__ import annotations

import json
import os
import subprocess
import sys
from typing import Any


def _find_game_exe() -> list[str]:
    env = os.environ.get("WHISKERS_GAME_EXE", "").strip()
    if env:
        return [env]
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.normpath(os.path.join(here, ".."))
    for name in ("platformer_demo", "platformer_demo.exe"):
        p = os.path.join(root, "build", name)
        if os.path.isfile(p):
            return [p]
    return ["./build/platformer_demo"]


class GameSession:
    def __init__(self) -> None:
        self.proc: subprocess.Popen[str] | None = None
        self._buf: str = ""

    def start(self) -> dict[str, Any]:
        if self.proc and self.proc.poll() is None:
            return {"pid": self.proc.pid, "status": "already_running"}
        exe = _find_game_exe()
        self.proc = subprocess.Popen(
            exe + ["--headless", "--debug-protocol"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            bufsize=1,
            env={**os.environ, "WHISKERS_DEBUG_PROTOCOL": "1"},
        )
        assert self.proc.stdin and self.proc.stdout
        return {"pid": self.proc.pid, "exe": exe[0]}

    def stop(self) -> dict[str, Any]:
        if self.proc is None:
            return {"ok": True}
        self.proc.terminate()
        try:
            self.proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            self.proc.kill()
        self.proc = None
        return {"ok": True}

    def _write_cmd(self, obj: dict[str, Any]) -> None:
        if not self.proc or not self.proc.stdin:
            raise RuntimeError("game not started")
        line = json.dumps(obj, separators=(",", ":")) + "\n"
        self.proc.stdin.write(line)
        self.proc.stdin.flush()

    def _read_json_line(self) -> dict[str, Any]:
        if not self.proc or not self.proc.stdout:
            raise RuntimeError("game not started")
        while True:
            line = self.proc.stdout.readline()
            if not line:
                raise EOFError("game stdout closed")
            line = line.strip()
            if not line:
                continue
            try:
                return json.loads(line)
            except json.JSONDecodeError:
                continue

    def get_state(self) -> dict[str, Any]:
        self._write_cmd({"cmd": "get_state"})
        return self._read_json_line()

    def step_frames(self, frames: int) -> dict[str, Any]:
        self._write_cmd({"cmd": "step", "args": {"frames": int(frames)}})
        return self._read_json_line()

    def press_keys(self, keys: list[str], frames: int) -> dict[str, Any]:
        self._write_cmd(
            {"cmd": "input_frame", "args": {"keys": keys, "frames": int(frames)}}
        )
        return self._read_json_line()

    def screenshot(self, width: int = 640, height: int = 480) -> dict[str, Any]:
        self._write_cmd({"cmd": "screenshot", "args": {"width": width, "height": height}})
        return self._read_json_line()

    def load_level(self, path: str) -> dict[str, Any]:
        self._write_cmd({"cmd": "load_level", "args": {"path": path}})
        return self._read_json_line()

    def set_gravity(self, x: float, y: float) -> dict[str, Any]:
        self._write_cmd({"cmd": "set_gravity", "args": {"x": x, "y": y}})
        return self._read_json_line()

    def teleport_entity(self, entity: int, x: float, y: float) -> dict[str, Any]:
        self._write_cmd(
            {"cmd": "teleport", "args": {"entity": entity, "x": x, "y": y}}
        )
        return self._read_json_line()


def main() -> None:
    try:
        from mcp.server.fastmcp import FastMCP  # type: ignore
    except ImportError:
        print(
            "Install MCP: pip install mcp\n"
            "Then configure this script as an MCP server in Cursor/Claude.",
            file=sys.stderr,
        )
        sys.exit(1)

    sess = GameSession()
    mcp = FastMCP("whiskers-playtest")

    @mcp.tool()
    def start_game() -> dict[str, Any]:
        """Launch platformer_demo with debug protocol (headless)."""
        return sess.start()

    @mcp.tool()
    def stop_game() -> dict[str, Any]:
        """Terminate the game process."""
        return sess.stop()

    @mcp.tool()
    def get_state() -> dict[str, Any]:
        """Return latest JSON game state (entities, positions, velocities)."""
        return sess.get_state()

    @mcp.tool()
    def step_frames(frames: int = 1) -> dict[str, Any]:
        """Advance the simulation by N fixed physics frames."""
        return sess.step_frames(frames)

    @mcp.tool()
    def press_keys(keys: list[str], frames: int = 30) -> dict[str, Any]:
        """Hold named keys (e.g. D, Space) for N frames (debug input_frame)."""
        return sess.press_keys(keys, frames)

    @mcp.tool()
    def screenshot(width: int = 640, height: int = 480) -> dict[str, Any]:
        """Capture framebuffer as base64 PNG JSON result."""
        return sess.screenshot(width, height)

    @mcp.tool()
    def load_level(path: str) -> dict[str, Any]:
        """Load a level JSON file by path (relative to cwd)."""
        return sess.load_level(path)

    @mcp.tool()
    def set_gravity(x: float = 0.0, y: float = -25.0) -> dict[str, Any]:
        """Set world gravity."""
        return sess.set_gravity(x, y)

    @mcp.tool()
    def teleport_entity(entity: int = 0, x: float = 0.0, y: float = 0.0) -> dict[str, Any]:
        """Teleport entity by index."""
        return sess.teleport_entity(entity, x, y)

    mcp.run()


if __name__ == "__main__":
    main()
