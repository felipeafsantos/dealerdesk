# NOTES

Felipe Santos, DealerDesk take-home. Branch `delivery/docs-and-ci`.

## What changed, and why in that order

The brief says nobody trusts the docs, there are no tests, no CI, no reliable
build and releases are by hand. Before touching any of that I read the code
and found three things that set the priorities: the pricing is one pure
function (`calc_quote` in `calc.c`), every bit of input goes through `fgets`
so the whole app can be driven by a text file on stdin, and the three
invoices shipped in `data/invoices.dat` reproduce the current code to the
penny. So the cheapest, highest-value work was about trust and repeatability,
not tooling breadth.

1. **Know the real behaviour.** `docs/CODEBASE.md` describes the code as it
   is, with line references, and lists nine places where the README and
   comments are wrong (the trade discount is 10%, not 5%; VAT is on net plus
   delivery, not on list; there is no manager PIN; and so on). Everything
   after this was built on the code, not the docs.
2. **One-command build.** A `Makefile` with `-Wall -Wextra`. The code
   compiles clean, so this costs nothing and removes "build by hand with the
   right flags".
3. **Regression safety net.** `tests/test_calc.c`: 39 plain-C checks, no
   framework, pinned to the three real invoices plus the clamp, the fixed
   fees, the VAT base, the trade-in and the rounding. This is what makes
   "unchanged to the penny" provable rather than hoped.
4. **CI on every push and pull request.** GitHub Actions runs the build, the
   tests and a smoke run that proves starting the program does not rewrite
   `data/`.
5. **Screen tests.** `tests/e2e/run.sh` pipes a script into the real binary
   in a temporary copy of `data/` and diffs the quote screen for a retail and
   a trade customer. The unit tests cannot see a wrong or missing line on the
   screen, and the fleet discount is exactly that kind of change.
6. **Release by tag.** A second workflow builds, tests and publishes a GitHub
   Release on any `v*` tag. `RELEASE.txt` now describes that instead of
   "build on your machine and test it a bit".

The README was corrected after that, because it is a developer document and
the brief says to start with it. The in-app help screen is staff-facing and
was left alone.

## How I worked with the agent

Claude Code, driven through `CLAUDE.md` and a `reviewer` agent, both
committed. The rules I gave it: never commit or push (I do those by hand, so
every commit message was reviewed by me first); the code, not the README, is
the source of truth; staff must notice nothing; and when something is
unclear, ask rather than assume. The last rule replaced the agent's own
instinct to "make a reasonable call and write it down".

The order was deliberate: explore and document first, then tests pinned to
real data, then automation. The agent did the exploration with a subagent,
reported the docs-vs-code discrepancies, and I used that to decide what to
pin before any build tooling existed. The branch was then reviewed by the
`reviewer` agent, which rebuilt, re-ran the tests, recomputed every pinned
number and found no blocking issues. It did catch that `make test` needs
`sh` on PATH, which fails from plain cmd on Windows and only worked here
because Git Bash was on the PATH, plus stale README line numbers in the
codebase doc after the README was corrected. Both are fixed.

**Where it got something wrong.** The first `Makefile` set the compiler with
`CC ?= gcc`. GNU make has a built-in default of `cc`, so the `?=` never took
effect, and the first `mingw32-make` on Windows failed with "cannot find the
file specified". The agent caught it only because it ran the build instead
of declaring it done; the fix is a plain `CC = gcc`, which the command line
can still override. Smaller one: an HTML preview of the docs loaded a script
from a CDN and rendered blank in the app's panel. Both are the same lesson:
run the thing, do not trust the diff.

## Assumptions and decisions

Each of these was a question I answered, not an assumption the agent made.

- The tests pin what the code does, not what the README says, because the
  invoices in production were produced by the code.
- The showrooms run the Linux binary (the 2021 "runs on the linux box"
  commit), so the Linux-only release workflow matches deployment. Windows
  builds are noted as not covered.
- The README is corrected because it is for developers; the help screen
  inside the app is not, because staff see it.
- The trade screen test uses a 6% salesman discount so it is not confused
  with the 4% fleet rate coming in TICKET-142.
- One real approval gate on the release (a `production` environment with a
  required reviewer) rather than simulated dev/test/qa/prd stages. The real
  deployment is a person copying a binary to the fileserver; a workflow can
  gate the publish honestly, but it cannot deploy, so it should not pretend to.
- Plain C assertions rather than a test framework: the thing under test is
  one function, and vendoring Unity would be the largest dependency in the
  repo.

## TICKET-142

To be completed when the ticket ships. Questions still to be answered before
code is written (from `docs/CODEBASE.md` section 9): whether fleet and trade
stack when a customer is both; whether the 4% is taken from the list price or
after the trade discount; where the fleet line sits relative to the salesman
discount; and whether the salesman discount is then applied to the price net
of the fleet discount, as it is for trade.

## What I would do next with more time

- A sell-vehicle screen test with the date masked, covering the invoice
  preview and the saved invoice.
- `-Werror` in the CI build step.
- A Windows job in CI, since the developers are on Windows.
- Fix the pre-existing `strtok` bug where an empty phone or email shifts the
  following fields on load.
- Correct the in-app help text once the product owner agrees the wording.
