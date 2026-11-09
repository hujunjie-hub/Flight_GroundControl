# \`.github/\` — CI, Workflows, and Repo Metadata

> See [\`AGENTS.md\`](../AGENTS.md) for the canonical agent guide. This document describes the
> supported CI target, active workflows, composite actions, Python helpers, and build configuration.

## CI target

**The active platform builds are Linux x64 (GCC) and Windows x64 (MSVC 2022).**

- Linux x64 Release and Windows x64 Release are the default builds.
- Windows Debug is available only through manual workflow dispatch.
- Linux uses the Ubuntu x64 runner and distro dependencies; Windows uses x64 Qt and GStreamer SDKs.
- ARM64, macOS, iOS, Android, Docker, Flatpak, and VM build workflows are not part of this CI configuration.
- Non-build checks remain enabled: workflow linting, CI script tests, pre-commit, CodeQL, docs, Doxygen, link checks, and repository automation.

This is a CI/build-target restriction. Shared QGroundControl source code and platform-specific source files are retained where required by the upstream codebase; their presence does not mean those platforms are built or packaged by this CI.

## Contents

- [Layout](#layout)
- [Workflows](#workflows)
- [Linux and Windows x64 builds](#linux-and-windows-x64-build)
- [Active composite actions](#active-composite-actions)
- [CI scripts](#ci-scripts)
- [Build configuration](#build-configuration)
- [Dependency management](#dependency-management)
- [CI conventions](#ci-conventions)
- [Tests](#tests)

## Layout

\`\`\`text
.github/
├── workflows/                 # Linux/Windows x64 builds, quality checks, and repo automation
├── actions/                   # Composite actions used by active workflows
├── scripts/                   # Python helpers invoked by workflows and actions
│   ├── templates/             # Jinja2 templates
│   └── tests/                 # pytest suite for scripts/
├── build-config.json          # Linux/Windows x64 toolchain versions and settings
├── build-config.schema.json   # JSON Schema for build-config.json
├── dependabot.yml             # Dependabot config (GitHub Actions only)
└── renovate.json              # Renovate config (npm, Python, pre-commit)
\`\`\`

## Workflows

| Workflow | Purpose |
| --- | --- |
| \`linux.yml\` | Linux x64 GCC Release build, unit/integration tests, and AppImage verification |
| \`windows.yml\` | Windows x64 MSVC Release build, unit/integration tests, and installer verification |
| \`_detect-changes.yml\` | Reusable Linux/Windows change detector; skips platform builds on unrelated PR changes |
| \`build-results.yml\` | Aggregate Linux, Windows, and pre-commit results for a PR; save successful main-branch baselines |
| \`pre-commit.yml\` | Linting and formatting checks |
| \`ci-scripts.yml\` | actionlint and Python tests for CI scripts |
| \`check-links.yml\` | Markdown link validation |
| \`analysis.yml\` | Static analysis |
| \`codeql.yml\` | CodeQL security scanning |
| \`pr-checks.yml\` | PR validation checks |
| \`release.yml\` | Release automation; waits for Linux and Windows x64 release builds |
| \`docs.yml\`, \`doxygen.yml\` | Documentation build and deployment |
| \`cache-cleanup.yml\`, \`cache-cleanup-pr.yml\`, \`_cache-cleanup.yml\` | Cache maintenance |
| \`crowdin.yml\`, \`lupdate.yml\` | Translation workflows |
| \`dependency-review.yml\` | Dependency security review |
| \`scorecard.yml\` | OpenSSF Scorecard |
| \`px4-metadata.yml\` | PX4 metadata sync |
| \`welcome.yml\` | New contributor welcome |

There are no macOS, iOS, Android, Docker, Flatpak, or VM platform build workflows in the active workflow set.

## Linux and Windows x64 builds

The Linux and Windows workflows use:

- Linux: Ubuntu 24.04 x64, GCC, Linux Qt SDK, and distro-managed system dependencies.
- Windows: Windows x64, MSVC 2022, x64 Qt SDK, and x64 MSVC GStreamer SDK.
- Both use the shared CMake build actions and Release configuration by default; Windows Debug is a manual dispatch option.

The build pipeline performs:

1. Platform-specific Linux/Windows change detection for pull requests.
2. Build environment setup and dependency configuration.
3. CMake configure and compilation.
4. Unit and integration tests on both platforms.
5. Linux executable and AppImage verification; Windows executable and installer verification.
6. Release artifact attestation and upload when repository release credentials are configured.

A skipped or cancelled test stage is not a passing validation result. The merge gate requires both Linux x64 and Windows x64 builds and their configured tests to pass.

## Active composite actions

| Action | Purpose |
| --- | --- |
| \`build-config\` | Read toolchain versions from \`.github/build-config.json\` |
| \`build-prerequisites\` | Shared prerequisites: build config, CMake, Python, and runner preparation |
| \`build-setup\` | Prepare Linux x64 / Windows x64 build environments and Qt SDKs |
| \`cmake-configure\` | Configure QGroundControl with the CI options |
| \`cmake-build\` | Build with consistent logging, timing, and compiler-cache settings |
| \`cmake-install\` | Stage the Linux AppImage or Windows installer |
| \`run-unit-tests\` | Run CTest labels and generate test output |
| \`test-report\` | Publish and upload JUnit / CTest results |
| \`verify-executable\` | Boot-test the built or installed executable |
| \`detect-changes\` | Detect Linux x64 / Windows x64 build-relevant file changes |
| \`attest-and-upload\` | Attest and upload the Linux AppImage or Windows installer |
| \`attest-sbom\` | Generate SBOM and build provenance attestations |
| \`gate-platform-workflows\` | Gate combined reports on the configured platform workflow set |
| \`build-results-bootstrap\` | Sparse-checkout the scripts/actions used by build-results |
| \`download-all-artifacts\` | Download artifacts for the same commit |
| \`collect-artifact-sizes\` | Collect artifact sizes for build reports |
| \`replace-cache-entry\` | Replace a named GitHub Actions cache entry |
| \`setup-python\` | Install Python, uv, and selected dependency groups |
| \`cache\`, \`cache-cleanup\` | Build cache and cache maintenance |
| \`coverage\`, \`size-analysis\` | Quality and binary-size reporting |
| \`deploy-docs\` | Deploy documentation |
| \`install-dependencies\`, \`qt-install\` | Install Linux system dependencies or Windows build dependencies and Qt |

## CI scripts

Python helpers in \`.github/scripts/\` are invoked by active workflows and composite actions.

| Script | Purpose |
| --- | --- |
| \`ci_bootstrap.py\` | Make \`tools/common\` imports available to CI scripts |
| \`detect_changes.py\` | Linux x64 / Windows x64 change detection |
| \`cmake_helper.py\` | CMake configure, build, and CTest helpers |
| \`ccache_helper.py\` | Compiler-cache setup and reporting |
| \`cpm_helper.py\` | CPM dependency fingerprint and source-cache setup |
| \`collect_build_status.py\` | Collect Linux/Windows/pre-commit workflow status |
| \`collect_artifact_sizes.py\` | Collect artifact sizes for build reports |
| \`generate_build_results_comment.py\` | Generate consolidated PR build-result comments |
| \`check_baseline_ready.py\` | Verify baseline-cache update readiness |
| \`precommit_results.py\` | Normalize pre-commit results |
| \`attest_helper.py\`, \`generate_cpm_sbom.py\` | SBOM and artifact attestation helpers |
| \`find_artifact.py\`, \`download_artifacts.py\` | Locate and download build artifacts |
| \`verify_executable.py\` | Verify Linux and Windows executables / packages |
| \`deploy_docs.py\` | Documentation deployment |
| \`gh_cache_cleanup.py\` | GitHub Actions cache maintenance |
| \`gh_pr_size_label.py\` | Maintain PR size labels |
| \`coverage_comment.py\`, \`verify_coverage_thresholds.py\` | Coverage reporting and threshold checks |
| \`size_analysis.py\` | Binary size analysis |
| \`test_duration_report.py\` | Test duration reporting |
| \`xml_utils.py\` | Safe XML parsing |

## Build configuration

\`build-config.json\` is the source of truth for the supported CI toolchain:

- \`build.platform_workflows\`: exactly \`Linux,Windows\`.
- Qt target and minimum versions.
- CMake minimum version.
- GStreamer default/minimum and Windows x64 SDK versions.
- Linux/Windows plugin selections and the Windows x64 SDK SHA256 checksum.

Android and Apple SDK metadata are intentionally absent. \`cmake/BuildConfig.cmake\` treats non-Linux/Windows platform metadata as optional. The JSON Schema rejects additional platform build configuration keys.

Read values via \`common.build_config.get_build_config_value()\`, \`tools/setup/read_config.py\`, or the \`build-config\` composite action.

## Dependency management

Dependency updates are split between two bots:

- **Dependabot** (\`.github/dependabot.yml\`) owns GitHub Actions updates.
- **Renovate** (\`.github/renovate.json\`) owns npm, Python/uv, and pre-commit updates.

## CI conventions

- **Python dependencies**: CI scripts use \`httpx\` for GitHub API access and \`jinja2\` for templating. Dependencies are declared in \`tools/pyproject.toml\`.
- **Shared helpers**: \`tools/common/gh_actions.py\` provides GitHub API pagination with a \`gh\` CLI fallback. Import as \`from common.gh_actions import ...\`.
- **Bootstrap scripts**: \`install_dependencies_helper.py\` and \`ccache_helper.py\` use only the standard library because they may run before Python dependencies are installed.
- **Outputs**: use \`common.gh_actions.write_github_output()\` for \`$GITHUB_OUTPUT\` writes.
- **Platform scope**: keep automatic platform builds limited to Linux x64 and Windows x64 unless the project requirement and its schema/tests are explicitly changed.

## Tests

\`ci-scripts.yml\` runs two jobs on changes under \`.github/\` and \`tools/\`:

1. \`actionlint\` for workflow and composite-action syntax.
2. pytest for \`tools/tests\` and \`.github/scripts/tests\` on Python 3.10 and 3.12.

Run the CI script tests locally:

\`\`\`bash
pytest -q .github/scripts/tests/
\`\`\`

Run the complete CI Python test set:

\`\`\`bash
pytest -q tools/tests .github/scripts/tests
\`\`\`
