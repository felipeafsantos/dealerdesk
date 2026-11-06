# NOTES

Felipe Santos, DealerDesk take-home.

## What changed, and why in that order

Reading the code first set the priorities: pricing is one pure function
(`calc_quote`), all input goes through `fgets` so the app can be driven from a
text file, and the three invoices in `data/invoices.dat` reproduce the current
code to the penny. So the work went after trust and repeatability, not
tooling breadth.

1. **Know the real behaviour.** `docs/CODEBASE.md` describes the code as it
   is, with line references, and lists nine places where the README and
   comments were wrong (trade discount 10% not 5%, VAT on net plus delivery,
   no manager PIN, and so on). Everything after was built on the code.
2. **One-command build.** `Makefile` with `-Wall -Wextra`; the code compiles
   clean.
3. **Regression safety net.** `tests/test_calc.c`, plain C with no
   framework, pinned to the three real invoices plus the clamp, fixed fees,
   VAT base, trade-in and rounding. "Unchanged to the penny" became provable.
4. **CI on every push and pull request**, including a smoke run that proves
   starting the program does not rewrite `data/`.
5. **Screen tests.** `tests/e2e/run.sh` pipes a script into the real binary in
   a temporary copy of `data/` and diffs the screens. Unit tests cannot see a
   missing line on a quote; the fleet discount is exactly that.
6. **Release.** Merging a version bump to master builds, tests, tags and
   publishes a GitHub Release after one human approval; a Rollback workflow
   marks an earlier release as latest; CI refuses a program change without a
   version bump. `RELEASE.txt` describes it. The README was then corrected
   (a developer document); the in-app help screen was left alone (staff see
   it).

## How I worked with the agent

Claude Code, driven by `AGENTS.md` (rules any tool reads; `CLAUDE.md`
imports it) and a committed `reviewer` agent. My rules: never commit or push
(I do those by hand, so I read every message); the code, not the README, is
the source of truth; staff must notice nothing; when unclear, ask rather than
assume. That last rule replaced the agent's instinct to "make a reasonable
call and write it down": every decision below was a question put to me with
a recommendation.

The agent explored with a subagent and reported the docs-vs-code gaps before
any tooling existed. Each branch was reviewed by the `reviewer` agent, which
rebuilds, re-runs the tests and recomputes every pinned number.

**Where it got things wrong, and how I caught it.**

- The first `Makefile` used `CC ?= gcc`. GNU make's built-in default `cc`
  wins over `?=`, so the first Windows build failed with "cannot find the
  file specified". Caught by running the build; fixed with `CC = gcc`.
- The CI version gate was a no-op on GitHub: on a shallow pull-request
  checkout `git diff A...B` has no merge base, git fails, and the pipe into
  grep hid it, so the step passed. The reviewer agent reproduced this with a
  simulated checkout. Fixed with a two-dot diff and an explicit exit.
- The reviewer also caught that `make test` needs `sh` on PATH (fails from
  plain cmd on Windows) and stale line numbers in the codebase doc after
  the README was corrected. Same lesson each time: run it, do not trust the
  diff.

## Decisions

- Tests pin what the code does, not the README, because production invoices
  came from the code.
- Showrooms run the Linux binary (2021 "runs on the linux box" commit), so
  the Linux-only release matches deployment; Windows builds are noted as not
  covered.
- One real approval gate on the release rather than simulated
  dev/test/qa/prd stages: the real deployment is a person copying a binary
  to the fileserver, and a workflow can gate that honestly but cannot do it.
  Rollback marks an earlier release as latest and says the copy is manual.
- Tagging and publishing share one workflow run because a tag pushed by a
  workflow with the built-in token does not trigger other workflows. A
  version bump is required only for pull requests that change `.c`/`.h`
  files, so docs do not force a staff-visible version change. With no tag
  present, the first merge offered v2.3, the intended baseline release.
- Plain C assertions rather than a framework: one function under test.
- After reading another candidate's approach, two rules were adopted into
  `AGENTS.md`: data files must stay loadable by the previous binary
  (rollback), and a written order of what to trust. Its spec framework,
  simulated deploy stages and source-tree move were left out on purpose.

## TICKET-142 (v2.4)

Five commits that each build and pass: date mask for sale screens, customer
flag and marking screen, pricing with tests written first, the line on
quotes and invoices, docs and version. Plan: `docs/TICKET-142/TICKET-142_PLAN.md`.

Decisions the ticket left open, answered before code was written:

- A sixth `fleet` column on customers rather than a new type letter, so a
  trade customer can also be fleet and old files load unchanged.
- Trade and fleet both come off the list price; the salesman discount
  applies to the remainder, as it already did after the trade discount.
- Customers menu option 4 to mark or unmark by ID, plus a Y/N question on
  Add customer: the one staff-visible change, which the ticket asks for.
- A twelfth invoice column holding the fleet part of the discount; `disc`
  still holds the whole discount, so totals and existing rows are untouched
  and old invoices display as before.

Rollback checked by building v2.3 from master and running it on v2.4 files:
it loads everything, shows a fleet invoice with one Discounts line and the
same total, and drops the fleet column when it saves, so after a rollback
nobody is fleet and no money is lost. The pre-existing empty-phone
field-shift bug is unchanged.

Every expected screen was read against `calc.c` before being recorded; the
pricing tests were seen failing to compile before `calc.c` changed.

## Next, with more time

`-Werror` in CI; a Windows CI job; fix the `strtok` empty-field bug; correct
the in-app help text once the product owner agrees the wording; a Windows
build in the release if the showrooms ever need one.
