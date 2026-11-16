# AGENTS.md

Repository-wide instructions for AI coding agents working on Flight_GroundControl (QGroundControl).

## 0. Mandatory instruction entrypoint

Before editing any file, read and follow:

1. [AI execution and task-closure contract](spec/AI-执行与任务闭环规范.md) — instruction priority, autonomous execution, phase gates, process safety and definition of done.
2. [Project specification index](spec/README.md) — current project phase and authoritative specification map.
3. [Agent operation guide](spec/AGENT-操作指南.md) — repository workflow, issue handling and delivery.
4. The task-specific specification(s) referenced by the index.

`CLAUDE.md` delegates to this file. GitHub Copilot and Cursor have thin adapter files that point to the same contract. Do not create competing agent workflows.

## 1. Instruction priority

The latest explicit user instruction for the current task takes precedence over generic defaults in this repository. Earlier user decisions remain in force unless superseded. Follow the central AI execution contract for all other behavior.

If the user has already made a clear decision, execute it; do not reopen the decision or ask the user to repeat it. Stop only for a genuine ambiguity, a destructive action requiring authorization, or a safety-critical conflict.

## 2. Current execution phase

The active phase is recorded in [spec/README.md](spec/README.md). As of 2026-11-16, the active work is the four-page Flight Diagnostics dashboard rebuild and visual alignment.

During this development phase:

- Do not trigger GitHub Actions, CI, workflow_dispatch, Unit, Integration or UI automation tests.
- Do not run `ctest` or dashboard UI test executables.
- Do not turn a focused UI change into a full build/test/package acceptance cycle.
- Lightweight source review, Git diff inspection and non-test static checks are allowed when useful.
- Report tests and CI that were not run as `NOT_RUN`; never imply they passed.
- Preserve existing tests by default. Remove dashboard-scoped test files and their build registrations only when the user's explicit task requires that cleanup; do not touch unrelated tests.

When the user explicitly switches to centralized validation, follow the full Windows Release acceptance guide.

## 3. Required execution behavior

- Treat each user request as an end-to-end delivery task, not a request for a plan.
- Inspect current HEAD, branch, working tree and existing implementation before editing.
- Convert every explicit requirement into an internal checklist; complete all items in scope.
- Implement the requested change in the actual repository. Do not substitute explanations, pseudo-code or partial styling changes for implementation.
- Complete necessary related changes across pages, shared components, data bindings, styles, resources, CMake registration and documentation.
- Preserve existing behavior, telemetry, data fields, safety semantics and user-approved design decisions unless the user explicitly asks to change them.
- Do not stop after one subtask while other in-scope work remains. “Continue” means resume from the latest repository state and finish the outstanding checklist.
- Do not repeatedly ask for confirmation on decisions already made.
- Review the final diff and report only changes and checks that actually occurred.

## 4. Quick references

- [AI execution contract](spec/AI-执行与任务闭环规范.md)
- [Agent operation guide](spec/AGENT-操作指南.md)
- [Project specifications](spec/README.md)
- [Coding style](CODING_STYLE.md)
- [Architecture and contribution patterns](.github/CONTRIBUTING.md)
- [Tools](tools/README.md)
- [Test framework](test/README.md) — consult when the user authorizes testing.
- [CI overview](.github/ci-overview.md) — consult when the user authorizes CI or centralized validation.
- [Pre-commit configuration](.pre-commit-config.yaml)

## 5. Golden rules

These QGroundControl architecture rules remain mandatory:

- **Fact System** — all vehicle parameters flow through Facts; never create parallel custom parameter storage.
- **Multi-Vehicle** — null-check `activeVehicle()` and `Vehicle*` before dereferencing.
- **Firmware Plugin** — use `vehicle->firmwarePlugin()` for firmware-specific behavior, not scattered `if (px4)` branches.
- **QML Integration** — register types with `QML_ELEMENT`, `QML_SINGLETON` or `QML_UNCREATABLE`; expose state via `Q_PROPERTY`.
- **No production `Q_ASSERT`** — use defensive checks and early returns.
- **Tests** — do not use `QTest::ignoreMessage` or fixed-delay `QTest::qWait(<n>)`; use the repository's log and asynchronous wait helpers when testing is authorized.

See [CONTRIBUTING.md](.github/CONTRIBUTING.md#architecture-patterns) and [CODING_STYLE.md](CODING_STYLE.md#common-pitfalls) for examples.

## 6. Product build and validation boundaries

When validation is authorized, the only product build target is **Windows x64 / Visual Studio 2022 MSVC / Release**. Do not configure, build, test or package Linux, macOS, Android, iOS, ARM64, Windows Debug, Docker or Flatpak product targets.

The complete validation sequence, test-count gates, executable verification and installer verification are defined in [spec/Windows-Release验收与交付.md](spec/Windows-Release验收与交付.md). That guide applies to the centralized acceptance phase, not automatically to every development change.

## 7. Definition of done

A task may be reported complete only when:

1. Every explicit in-scope requirement has an implemented result or a clearly documented genuine blocker.
2. All affected pages/components and required related files have been handled; no easy subset is presented as the whole task.
3. Existing user data, telemetry fields, interactions and safety semantics have not been silently removed.
4. The final diff has been reviewed for omissions and unrelated changes.
5. Visual tasks have been compared against the supplied reference at the available target viewport; otherwise visual verification is `UNVERIFIED`.
6. Tests/CI are reported according to the active phase. During dashboard development they are `NOT_RUN`, not PASS.

A plan, promise, partial edit, successful tool call or test process that is still running is not task completion.

## 8. Commit conventions

Use Conventional Commits when a commit is requested or required by the active task workflow. Do not create a commit for every small intermediate edit. Never force-push, rewrite another contributor's history or overwrite user changes. Follow the active phase's CI restrictions before pushing.
