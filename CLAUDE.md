# CLAUDE.md

DealerDesk is a menu-driven terminal application in portable C that a car
showroom uses daily for stock, customers, quotes and invoices. Read
`docs/CODEBASE.md` before changing anything: it describes the code as it
really behaves, with line references, and lists where the README and comments
are wrong.

## Working rules

- **Never run `git commit` or `git push` without asking first.** Each commit
  and each push needs its own explicit go-ahead. Prepare the diff and a
  drafted message, then ask.
- **The code, not the README or the help screen, is the source of truth for
  behaviour.** Production data was produced by this code. Pricing rules are in
  `calc.c`; see `docs/CODEBASE.md` section 6.
- **Staff must not notice any change apart from the feature being built.**
  Do not change prompts, screen layouts, menu numbers or data file formats
  unless the task requires it. New data fields go at the end of a record so
  existing files load without conversion.
- **Run the program from the repository root.** It reads and writes `data/`
  relative to the working directory, and every action saves immediately.
  After a manual run, restore the sample data with `git checkout -- data`.
- Keep the existing C style: C89-style declarations at the top of blocks,
  4-space indent, `static` helpers, no new dependencies. There is one
  `main()` in `main.c`; do not add `.c` files to the repository root because
  `*.c` is the documented build. Tests live under `tests/`.
- **Do not assume.** When the ticket, the code or the task is unclear, stop
  and ask for clarification before acting. Once a question is answered, record
  the decision in `NOTES.md` so reviewers can see it.

## Build and test

```
make            # builds ./dealerdesk  (same as: gcc -o dealerdesk *.c -lm)
make test       # builds and runs the unit tests in tests/
make clean
```

On Windows with the WinLibs toolchain use `mingw32-make` in place of `make`.
CI (`.github/workflows/ci.yml`) runs the same targets on Ubuntu.

## Code review protocol

When asked to review a branch:

1. Scope strictly to the branch's own diff
   (`git diff $(git merge-base origin/master HEAD)..HEAD`). Do **not** report
   pre-existing issues outside the diff; `docs/CODEBASE.md` section 8 already
   lists the known ones.
2. Gather context first: the ticket (`TICKET-*.md`), the PR description via
   `gh pr view <n> --repo felipeafsantos/dealerdesk` when there is one, and
   `docs/CODEBASE.md`.
3. Verify every claim empirically before writing it down: build with `make`,
   run `make test`, and drive the binary with piped input where a screen is
   affected. State the evidence next to each finding. Anything that touches
   `calc.c` must be checked against the three sample invoices to the penny.
4. Check what this codebase cares about most: non-fleet totals unchanged,
   existing `data/*.dat` files still load, new record fields appended at the
   end, no change to prompts or screens staff would notice, no `.c` files in
   the repository root, portable C only.
5. Deliver the review in the conversation. When a written review *is* asked
   for, it belongs in `docs/<TICKET-ID>/<TICKET-ID>_REVIEW_ACTIONS.md`
   (`_2`, `_3` ... for later rounds), never at the repository root. Supersede
   an earlier draft rather than leaving versions side by side.

The `reviewer` agent in `.claude/agents/` runs this protocol. It may build and
run tests; it must not commit, push or edit source files.

## Files

- `main.c` menus and screens, `calc.c` pricing, `dbfile.c` flat-file load and
  save, `screen.c` terminal input and output, `globals.h` records and limits.
- `data/*.dat` sample data, pipe-separated text.
- `BRIEF.md` the task, `TICKET-142.md` the feature to ship, `NOTES.md` the
  write-up for reviewers.
