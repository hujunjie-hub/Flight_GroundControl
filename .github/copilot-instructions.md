# Flight_GroundControl — GitHub Copilot instructions

Before changing this repository, read and follow:

1. [Root AGENTS.md](../AGENTS.md)
2. [AI execution and task-closure contract](../spec/AI-执行与任务闭环规范.md)
3. [Project specification index and current phase](../spec/README.md)
4. [Agent operation guide](../spec/AGENT-操作指南.md)
5. The task-specific specification.

The root AGENTS.md and the central AI execution contract are authoritative. Do not invent a separate workflow or override the user's latest explicit instructions.

Current phase (2026-11-16): complete the four Flight Diagnostics dashboard pages and visual alignment. The user has explicitly requested cleanup of all dashboard-specific test files and their build registrations; do not delete tests for unrelated product areas. Do not trigger CI, Unit, Integration, UI automation tests or full product acceptance during this development phase. Keep unrun checks as NOT_RUN. Only enter centralized validation when the user explicitly authorizes it.

Execute the complete requested scope in one continuous task. Do not return a plan instead of code changes, stop after a small subset, ask the user to repeat settled decisions, or claim completion without reviewing the actual diff. Preserve existing telemetry, data, behavior, safety boundaries and approved design decisions.
