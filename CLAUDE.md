# CLAUDE.md

This file is for Claude Code. The rules shared with every coding agent live in
`AGENTS.md`, imported here so there is one source of truth:

@AGENTS.md

## Claude Code specifics

- The `reviewer` subagent in `.claude/agents/reviewer.md` runs the code review
  protocol from `AGENTS.md`. Use it to review a branch: it builds, runs the
  tests and drives the binary, and it never edits, commits or pushes.
- The `/code-review` command is local-only. Nothing it produces is posted to
  GitHub.
