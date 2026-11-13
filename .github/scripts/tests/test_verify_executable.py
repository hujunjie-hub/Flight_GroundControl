"""Tests for verify_executable.py."""

from __future__ import annotations

import os
import sys
from pathlib import Path
from subprocess import CompletedProcess

import pytest
import verify_executable
from verify_executable import _setup_gstreamer_env


def test_boot_runner_resolves_from_checkout_without_github_workspace(tmp_path, monkeypatch):
    monkeypatch.delenv("GITHUB_WORKSPACE", raising=False)
    monkeypatch.chdir(tmp_path)
    binary_dir = tmp_path / "Release"
    binary_dir.mkdir()
    binary = binary_dir / "QGroundControl.exe"
    binary.touch()
    monkeypatch.setattr(
        sys, "argv", ["verify_executable.py", "--binary-path", str(binary), "--type", "exe"]
    )

    def run(command, *, cwd, check):
        runner = Path(command[1])
        assert runner.is_absolute()
        assert (
            runner
            == Path(verify_executable.__file__).resolve().parents[2] / "tools" / "run_tests.py"
        )
        assert runner.is_file()
        assert Path(cwd) == binary_dir
        return CompletedProcess(command, 0)

    monkeypatch.setattr(verify_executable.subprocess, "run", run)
    with pytest.raises(SystemExit) as result:
        verify_executable.main()
    assert result.value.code == 0


class TestSetupGstreamerEnv:
    """Tests for _setup_gstreamer_env function."""

    def test_no_cache_file(self, tmp_path, monkeypatch):
        """Returns without setting env if CMakeCache.txt is missing."""
        monkeypatch.delenv("GST_PLUGIN_PATH", raising=False)
        _setup_gstreamer_env(tmp_path)
        assert "GST_PLUGIN_PATH" not in os.environ

    def test_no_gstreamer_in_cache(self, tmp_path, monkeypatch):
        """Returns without setting env if no GStreamer_ROOT_DIR in cache."""
        cache = tmp_path / "CMakeCache.txt"
        cache.write_text("CMAKE_BUILD_TYPE:STRING=Release\n")
        monkeypatch.delenv("GST_PLUGIN_PATH", raising=False)
        _setup_gstreamer_env(tmp_path)
        assert "GST_PLUGIN_PATH" not in os.environ

    def test_sets_plugin_path(self, tmp_path, monkeypatch):
        """Sets GST_PLUGIN_PATH when plugin dir exists."""
        gst_root = tmp_path / "gstreamer"
        plugin_dir = gst_root / "lib" / "gstreamer-1.0"
        plugin_dir.mkdir(parents=True)

        cache = tmp_path / "CMakeCache.txt"
        cache.write_text(f"GStreamer_ROOT_DIR:PATH={gst_root}\n")

        monkeypatch.delenv("GST_PLUGIN_PATH", raising=False)
        _setup_gstreamer_env(tmp_path)

        assert os.environ["GST_PLUGIN_PATH"] == str(plugin_dir)
        assert os.environ["GST_PLUGIN_SYSTEM_PATH"] == str(plugin_dir)
