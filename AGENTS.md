# AGENTS.md

Instructions for AI coding agents (Codex, Claude Code, etc.) working on QGroundControl.

## Quick References

- [CODING_STYLE.md](CODING_STYLE.md) — Naming, formatting, C++20 features, QML style, logging
- [.github/CONTRIBUTING.md](.github/CONTRIBUTING.md) — Architecture patterns (Fact System, Multi-Vehicle, FirmwarePlugin)
- [tools/README.md](tools/README.md) — Development scripts and tooling
- [test/README.md](test/README.md) — Test framework, base classes, CTest labels, MultiSignalSpy, coverage
- [.github/ci-overview.md](.github/ci-overview.md) — CI workflow/action/script layout and conventions
- [.pre-commit-config.yaml](.pre-commit-config.yaml) — All enforced linters (clang-format, clang-tidy, ruff, pyright, shellcheck, actionlint, zizmor, qmllint, clazy, vehicle-null-check, check-no-qassert, check-no-qtest-ignore-message)

## Golden Rules (enforced — violations fail CI)

These are the non-negotiables. The first four are QGC's core architecture patterns; the rest are
enforced by pre-commit hooks, so ignoring them wastes a build cycle. Full list with code examples:
[.github/CONTRIBUTING.md#architecture-patterns](.github/CONTRIBUTING.md#architecture-patterns) and
[CODING_STYLE.md#common-pitfalls](CODING_STYLE.md#common-pitfalls).

- **Fact System** — ALL vehicle parameters flow through Facts; never create custom parameter storage.
- **Multi-Vehicle** — ALWAYS null-check `activeVehicle()` / `Vehicle*` before dereferencing (`vehicle-null-check`).
- **Firmware Plugin** — use `vehicle->firmwarePlugin()` for firmware-specific behavior, not `if (px4)` branches.
- **QML Integration** — register types with `QML_ELEMENT`/`QML_SINGLETON`/`QML_UNCREATABLE`; expose state via `Q_PROPERTY`.
- **No `Q_ASSERT` in production code** — use defensive checks with early returns (`check-no-qassert`).
- **No `QTest::ignoreMessage`** in tests — use `expectLogMessage`/`ignoreLogMessage` (`check-no-qtest-ignore-message`).
- **No fixed-delay `QTest::qWait(<n>)`** — use `QTRY_*_WITH_TIMEOUT` or `QSignalSpy::wait` (`check-no-fixed-qwait`).

## Critical Files (Read First!)

1. `src/FactSystem/Fact.h` — Parameter system foundation
2. `src/Vehicle/Vehicle.h` — Core vehicle model
3. `src/FirmwarePlugin/FirmwarePlugin.h` — Firmware abstraction

## Code Structure

Key modules (full tree under `src/` — ~33 subdirectories):

```text
src/
├── Vehicle/          # Vehicle state/comms
├── Comms/            # Link layer (serial, UDP, TCP, Bluetooth)
├── FactSystem/       # Parameter management
├── FirmwarePlugin/   # PX4/ArduPilot abstraction
├── AutoPilotPlugins/ # Vehicle setup UI
├── MissionManager/   # Mission planning
├── MAVLink/          # Protocol handling
├── VideoManager/     # Video pipeline (GStreamer)
├── FlyView/          # In-flight UI
├── PlanView/         # Mission planning UI
├── QmlControls/      # Reusable QML components
└── Settings/         # Persistent settings
```

## Build & Test Scope (Mandatory)

**This repository's active validation target is Windows x64 MSVC 2022 Release only.**
Do not build, configure, run tests, package, or otherwise validate any other operating
system, architecture, compiler, or build configuration.

In particular, do not run Linux, macOS, Android, iOS, ARM64, Docker, Flatpak, Windows
Debug, or other non-Windows-x64-Release build/test jobs. Do not spend time fixing or
reporting failures from those environments as part of this task.

The required validation sequence is:

1. Configure with the Windows x64 MSVC 2022 toolchain in **Release** mode.
2. Build the Release target successfully.
3. Run the **Unit** test suite and record the actual executed test count.
4. Run the **Integration** test suite and record the actual executed test count.
5. Treat missing test reports, CTest errors, skipped/cancelled test steps, and zero executed
   tests as failures. A successful build alone is not a successful validation.
6. Verify the generated Windows x64 Release executable.
7. Build the Windows installer, install it in a clean temporary directory, and verify the
   installed executable and required runtime/plugin dependencies.
8. Run applicable platform-independent static checks (for example, formatting and lint)
   when relevant to the changed files. These checks do not authorize builds or tests on
   another platform.

Use the repository's `just` recipes and scripts only where they select the required
Windows x64 Release toolchain. See [tools/README.md](tools/README.md) and
[.github/ci-overview.md](.github/ci-overview.md) for command details; do not follow
generic examples that would invoke another platform or configuration.

- **Build incrementally** — rebuild after focused groups of C++/Qt changes and fix errors
  before continuing.
- **Tight test loops** — use a focused test filter while iterating, then run both complete
  Unit and Integration labels for final Windows Release validation.
- **Test-count gate** — Unit and Integration must each execute at least one test; zero is
  always a failure, even if the test command exits successfully.
- **No platform expansion** — do not add or restore CI jobs for other operating systems,
  architectures, or build configurations for this validation scope.

## Definition of Done

Before considering a change complete:

1. Windows x64 MSVC 2022 **Release** configuration and build succeed.
2. Unit tests execute and pass with a non-zero recorded test count.
3. Integration tests execute and pass with a non-zero recorded test count.
4. Windows x64 Release executable verification succeeds.
5. Windows installer creation, clean installation, and installed-runtime verification succeed.
6. Applicable platform-independent lint/format checks pass.
7. Commit message follows Conventional Commits (below).

## Commit & Review Conventions

Commit messages follow **Conventional Commits** — the type drives release automation
(`.releaserc.json` → semantic-release). Use: `feat`, `fix`, `perf`, `revert` (release-triggering);
`docs`, `style`, `chore`, `refactor`, `test`, `build`, `ci` (no release). Example: `fix(Vehicle): guard null activeVehicle in telemetry handler`.

Your output will be reviewed by another AI agent before being accepted. Keep changes focused and
minimal, use clear naming, and leave explanatory commit messages. Avoid unrelated changes,
commented-out code, or ambiguous TODOs.

---

**Key Principle**: Match the style of code you're editing. See [CODING_STYLE.md](CODING_STYLE.md) for conventions and [CODING_STYLE.md#examples](CODING_STYLE.md#examples) for canonical Vehicle/Fact/QML snippets.
