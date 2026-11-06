# TICKET-142 Fleet Discount Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fleet customers get 4% off the vehicle list price, shown as its own "Fleet discount" line on quotes and invoices, with everything else unchanged to the penny.

**Architecture:** A `fleet` flag is appended to the customer record and a `fleet_disc` amount to the invoice record (both trailing fields, so old files load unchanged). `calc_quote()` gains a `fleet` parameter and a `fleet_disc` output; `print_quote()` and `view_invoice()` print the line only when non-zero. Staff mark customers through a new Customers menu option and a Y/N question on Add customer.

**Tech Stack:** C89-style C, Makefile, plain-C unit tests (`tests/test_calc.c`), piped-stdin screen tests (`tests/e2e/run.sh`).

**Spec:** `TICKET-142.md` plus decisions recorded below (user, 2026-11-06). Context: `docs/CODEBASE.md` sections 5, 6, 9.


## Global Constraints

- Branch: new `feature/ticket-142` from up-to-date `master` (delete the stale local branch of that name first). User commits by hand; each task ends with a paste-ready message. No push by me.
- Staff notice nothing except the feature: existing prompts, menu numbers, layouts and totals byte-identical; `quote_retail` and `quote_trade` expected files must pass unchanged until the version bump.
- Data files compatible both ways: new fields appended only; old files load without conversion; the v2.3 binary must still load files v2.4 wrote (it drops the new fields on save, stated in NOTES.md).
- Money: doubles, `rnd2()` at the same points as today; one penny off is a regression.
- No `.c` in the repo root besides the four sources; tests under `tests/`; C89 declarations at block top; `-Wall -Wextra` clean.
- Decisions: (1) customer field 6 `fleet` = `Y`/`N`, type `R`/`T` untouched, trade+fleet allowed; (2) fleet 4% of list, trade 10% of list, fleet line after trade line, salesman % applied to `list - trade - fleet`; (3) Customers menu "4. Mark / unmark fleet customer" + "Fleet customer (Y/N)?" after the trade question on Add; TYPE shows Retail / Trade / Fleet / Trade+Fleet; (4) invoice field 12 `fleet_disc`, `disc` still holds all discounts, view prints "Fleet discount" only when non-zero and "Discounts" shows `disc - fleet_disc`; (5) version v2.4 in the last task; (6) today's date masked as `<TODAY>` in screen tests.
- Help screen (`main.c:495-505`) untouched.

## Review Focus

1. A customer row with an empty phone or email (pre-existing `strtok` shift, CODEBASE §5): the shifted type lands in `fleet` and is not `Y`, so the customer is not fleet. Pinned in Task 2's unit-free check: documented in NOTES.md as unchanged behaviour, not fixed.
2. Marking an ID that does not exist: "Customer not found." and no save. Pinned in `customers_fleet.in` (Task 2).
3. A fleet customer unmarked after a sale: the stored invoice still shows the fleet line, because invoices display stored amounts. Pinned in `sell_fleet.in` (Task 4).
4. Fleet flag letter other than `Y` in the file (e.g. `y`, `1`): treated as not fleet. Pinned in Task 2 loader test via a screen case? No: pinned by the rule `c->fleet == 'Y'` and a NOTES.md line; covered implicitly by `customers_fleet` which writes `Y`/`N` only.
5. The v2.3 binary reading files written by v2.4: loads all rows, totals unchanged for R/T customers, drops fields 6 and 12 on save. Verified by hand in Task 4 step 9 and recorded in NOTES.md.

---

### Task 1: Mask today's date in screen tests

**Files:**
- Modify: `tests/e2e/run.sh` (the filter pipeline, lines 48-52)
- Modify: `docs/CODEBASE.md` §10 (one sentence)

**Interfaces:** Produces: screen-test output where every occurrence of `$(date +%F)` reads `<TODAY>`.

- [ ] **Step 1:** In `run.sh`, after `ROOT=` add `today=$(date +%F)`; in the output pipeline add `| sed "s/$today/<TODAY>/g"` after the existing `sed`. Apply the same `sed` to the expected-side `tr` line for symmetry.
- [ ] **Step 2:** Run `mingw32-make test` → `e2e: 2 case(s), 0 failed` (no case prints a date yet).
- [ ] **Step 3:** Prove the mask: `printf '5\n1\n\n0\n' | ./dealerdesk.exe | tr -d '\033\r' | sed "s/$(date +%F)/<TODAY>/g" | grep -c 2021` → `3` (old dates untouched); then `date +%F` does not appear in a quick manual sell run in a temp copy (optional).
- [ ] **Step 4:** §10 of `docs/CODEBASE.md`: add "Today's date is replaced by `<TODAY>` so sale cases are deterministic." to the screen-tests paragraph.
- [ ] **Step 5:** Hand over commit: files `tests/e2e/run.sh`, `docs/CODEBASE.md`; subject "Mask today's date in screen tests".

---

### Task 2: Customer fleet flag, marking screen, type label

**Files:**
- Modify: `globals.h:28-34` (CUSTOMER), `dbfile.c:62-93` (load_customers), `dbfile.c:182-197` (save_customers), `main.c:211-216` (print_cust_row), `main.c:231-258` (add_customer), `main.c:278-299` (customer_menu), `main.c:336` (print_quote label)
- Create: `tests/e2e/customers_fleet.in`, `tests/e2e/customers_fleet.expected`

**Interfaces:**
- Produces: `CUSTOMER.fleet` (`char`, `'Y'` or `'N'`); `static const char *cust_type_txt(const CUSTOMER *c)` in `main.c` returning `"Retail"`, `"Trade"`, `"Fleet"`, `"Trade+Fleet"`; `static void mark_fleet(void)`.
- File format: `id|name|phone|email|type|fleet`; a missing sixth field loads as `'N'`.

- [ ] **Step 1: Write the screen test first.** `tests/e2e/customers_fleet.in`:
  ```
  2
  4
  2008

  4
  9999

  1

  4
  2008

  1

  0
  0
  ```
  (mark Greenway, try unknown ID 9999, list, unmark, list, back, exit).
- [ ] **Step 2:** Run `mingw32-make test` → `FAIL customers_fleet` (no expected file yet, and option 4 is "Invalid option"). Expected: the diff shows "Invalid option".
- [ ] **Step 3:** `globals.h`: append `char fleet;          /* Y = fleet customer, N = not */` to `CUSTOMER` after `type`.
- [ ] **Step 4:** `dbfile.c` `load_customers`: after the type token add `t = strtok(NULL, "|"); c->fleet = (t && t[0] == 'Y') ? 'Y' : 'N';`. `save_customers`: format `"%d|%s|%s|%s|%c|%c\n"` with `cust[i].fleet` as the last argument.
- [ ] **Step 5:** `main.c`: add `cust_type_txt()` above `print_cust_row` (`'T'`+`'Y'` → "Trade+Fleet", `'T'` → "Trade", `'Y'` → "Fleet", else "Retail"); use it in `print_cust_row` (`main.c:215`) and `print_quote` (`main.c:336`) in place of the `c->type == 'T' ? "Trade" : "Retail"` expressions.
- [ ] **Step 6:** `main.c` `add_customer`: after `c.type = ...` add `c.fleet = confirm("  Fleet customer (Y/N)? ") ? 'Y' : 'N';`.
- [ ] **Step 7:** `main.c`: add `mark_fleet()` before `customer_menu`: `header("Fleet Customer")`; `c = find_cust(read_int("  Customer ID : "))`; if `!c` print `"\n  Customer not found.\n"`; else toggle `c->fleet` between `'Y'` and `'N'`, `save_customers()`, print `"\n  Customer %d is now a fleet customer.\n"` or `"\n  Customer %d is no longer a fleet customer.\n"`; `pause_enter()`. In `customer_menu` add `printf("  4. Mark / unmark fleet customer\n");` before "0. Back" and `case 4: mark_fleet(); break;`.
- [ ] **Step 8:** Build: `mingw32-make` → no warnings. Run the script by hand in a temp copy of the repo and read the output: Greenway shows `Fleet` in the first list and `Retail` in the second; 9999 gives "Customer not found."; the temp `customers.dat` has six fields per row with `N` everywhere except `Y` on 2008 while marked. Save the cleaned output as `tests/e2e/customers_fleet.expected` only after that check.
- [ ] **Step 9:** `mingw32-make test` → 39 unit checks, `e2e: 3 case(s), 0 failed`; `quote_retail` and `quote_trade` unchanged (`git status` shows their expected files unmodified).
- [ ] **Step 10:** Hand over commit: `globals.h dbfile.c main.c tests/e2e/customers_fleet.in tests/e2e/customers_fleet.expected`; subject "Let staff mark customers as fleet".

---

### Task 3: Fleet discount in pricing (tests first)

**Files:**
- Modify: `calc.h:4-18`, `calc.c:23-45`, `tests/test_calc.c`, `main.c:326` (call site)

**Interfaces:**
- Produces: `void calc_quote(QUOTE *q, double list, char cust_type, int fleet, double disc_pct, double tradein);` and `QUOTE.fleet_disc` (double, after `trade_disc`).
- Rule: `fleet_disc = fleet ? rnd2(list * 0.04) : 0`; `disc = rnd2((list - trade_disc - fleet_disc) * disc_pct / 100.0)`; `net = list - trade_disc - fleet_disc - disc`; everything after `net` unchanged.

- [ ] **Step 1: Update existing calls and add failing tests** in `tests/test_calc.c`: every existing `calc_quote(&q, L, T, P, TI)` becomes `calc_quote(&q, L, T, 0, P, TI)` (values unchanged); add in each of the three invoice tests `CHECK_MONEY(q.fleet_disc, 0.00)`; add functions:
  - `test_fleet_customer_gets_4pct_of_list`: `(10000.00,'R',1,0,0)` → fleet_disc 400.00, disc 0, net 9600.00, vat 1950.00, total 11755.00.
  - `test_fleet_and_trade_both_apply_then_salesman`: `(10000.00,'T',1,10,0)` → trade 1000.00, fleet 400.00, disc 860.00, net 7740.00, vat 1578.00, total 9523.00.
  - `test_fleet_discount_rounds_to_penny`: `(1234.56,'R',1,0,0)` → fleet 49.38, net 1185.18, vat 267.04, total 1657.22.
  - `test_fees_unchanged_for_fleet`: `(10000.00,'R',1,15,500)` → delivery 150.00, reg_fee 55.00.
  - `test_greenway_on_vehicle_1002`: `(10995.00,'R',1,0,0)` → fleet 439.80, net 10555.20, vat 2141.04, total 12901.24.
  Call them from `main`.
- [ ] **Step 2:** `mingw32-make test` → compile error (`calc_quote` takes 5 arguments; no `fleet_disc` member). That is the failing state.
- [ ] **Step 3:** `calc.h`: add `double fleet_disc;` after `trade_disc`; change the prototype to the 6-parameter form. `calc.c`: implement per the rule above; rewrite the header comment's price build-up to include "- fleet discount (fleet customers get 4% of list)" and correct its stale lines (10% trade, no PIN, VAT on net + delivery) since the comment is not staff-facing.
- [ ] **Step 4:** `main.c:326`: `calc_quote(q, v->price, c->type, c->fleet == 'Y', pct, ti);`.
- [ ] **Step 5:** `mingw32-make test` → `test_calc: 61 checks, 0 failed` (39 + 3 + 19 new; exact count printed), `e2e: 3 case(s), 0 failed`.
- [ ] **Step 6:** Hand over commit: `calc.h calc.c main.c tests/test_calc.c`; subject "Calculate the fleet discount".

---

### Task 4: Fleet line on quotes and invoices, stored on invoices

**Files:**
- Modify: `main.c:332-352` (print_quote), `globals.h:36-48` (INVOICE), `dbfile.c:95-138` (load_invoices), `dbfile.c:199-215` (save_invoices), `main.c:392-406` (sell_vehicle), `main.c:455-468` (view_invoice)
- Create: `tests/e2e/quote_fleet.in/.expected`, `tests/e2e/sell_fleet.in/.expected`

**Interfaces:**
- Produces: `INVOICE.fleet_disc` (double, last member); file format gains field 12 `fleet_disc` (`%.2f`); missing field loads as 0.
- Screen lines: quote `"  Fleet discount           %12.2f\n"` with `-q->fleet_disc`, printed after the trade line (`main.c:341`) only if `> 0`; invoice view `"  Fleet discount           %12.2f\n"` after List price only if `> 0`, and the Discounts line prints `-(p->disc - p->fleet_disc)`.

- [ ] **Step 1: Write the two scripts.** `quote_fleet.in`: `2,4,2008,<blank>,0,3,1002,2008,0,0,<blank>,0` (one per line). `sell_fleet.in`: `2,4,2008,<blank>,0,4,1002,2008,0,0,Y,<blank>,5,2,50233,<blank>,2,50232,<blank>,0,2,4,2008,<blank>,0,5,2,50233,<blank>,0,0` (mark; sell 1002 to 2008 with no discount; view the new invoice 50233; view old invoice 50232; unmark Greenway; view 50233 again; exit).
- [ ] **Step 2:** `mingw32-make test` → both new cases FAIL (no expected files).
- [ ] **Step 3:** `print_quote`: insert the fleet line after the trade-discount `if` block.
- [ ] **Step 4:** `globals.h`: append `double fleet_disc;  /* fleet discount, part of disc */` to `INVOICE`. `dbfile.c`: `load_invoices` reads a 12th token into `i->fleet_disc` (`t ? atof(t) : 0`); `save_invoices` format gains `|%.2f` with `inv[i].fleet_disc`.
- [ ] **Step 5:** `sell_vehicle`: `i->disc = q.trade_disc + q.fleet_disc + q.disc;` and `i->fleet_disc = q.fleet_disc;`. `view_invoice`: after the List price line add the conditional fleet line; change the Discounts line's value to `-(p->disc - p->fleet_disc)`.
- [ ] **Step 6:** Build clean. Run both scripts by hand in a temp copy and check: quote shows `Customer : Greenway Taxis (Fleet)`, `Fleet discount -439.80`, no `Discount (` line, `Net price 10555.20`, `VAT 20% 2141.04`, `TOTAL 12901.24`. Sell: preview identical to the quote, `Invoice 50233 created.`; view 50233 shows `Date: <TODAY>`, `Fleet discount -439.80`, `Discounts -0.00` (same `-0.00` the program already prints for a zero discount), `Fees 205.00`, `VAT 2141.04`, `TOTAL 12901.24`; view 50232 identical to today's output (no fleet line, `Discounts -839.60`); the second view of 50233 after unmarking is identical to the first. In the temp `invoices.dat` the new row is `50233|<date>|1002|2008|10995.00|439.80|10555.20|2141.04|205.00|0.00|12901.24|439.80` and the three old rows are unchanged. Save the expected files only after this check.
- [ ] **Step 7:** `mingw32-make test` → `e2e: 5 case(s), 0 failed`, `quote_retail`/`quote_trade`/`customers_fleet` expected files unmodified.
- [ ] **Step 8:** Rebuild and confirm `-Wall -Wextra` clean; `git diff --stat` touches only the listed files.
- [ ] **Step 9: Old-binary compatibility check.** In the scratchpad: `git show master:calc.c` (and `.h`, `main.c`, `dbfile.c`, `screen.c`, `globals.h`, `calc.h`, `dbfile.h`, `screen.h`) into `old/`, compile `gcc -o old/dealerdesk old/*.c -lm`; copy the temp data from step 6 (written by the new binary) next to it; run `printf '5\n2\n50233\n\n2\n1\n\n3\n1003\n2002\n6\n500\n\n0\n' | old/dealerdesk`: invoice 50233 shows TOTAL 12901.24 with one `Discounts -439.80` line; customer list shows Greenway as Retail (flag unknown to it); the trade quote prints exactly `quote_trade.expected`'s numbers. Then make it save (`2,4` is Invalid there; use `1,6,1004,<blank>` to toggle a reservation, which saves stock, then `2,2` add a customer to save customers): confirm `customers.dat` is back to 5 fields and loads in the new binary with every fleet flag `N`. Record the outcome in NOTES.md (Task 5).
- [ ] **Step 10:** Hand over commit: `globals.h dbfile.c main.c tests/e2e/quote_fleet.* tests/e2e/sell_fleet.*`; subject "Show and store the fleet discount on quotes and invoices".

---

### Task 5: Docs, NOTES.md, version v2.4

**Files:**
- Modify: `screen.c:18`, `README.md` (title, PRICING RULES), `docs/CODEBASE.md` (§5 formats, §6 table, §9 rewritten as implemented, §10 cases), `NOTES.md` (TICKET-142 section), all five `tests/e2e/*.expected`

- [ ] **Step 1:** `README.md` PRICING RULES: add after the trade bullet "* Fleet customers get a 4% fleet discount off the list price. A customer can be both trade and fleet, and then gets both." `docs/CODEBASE.md`: §5 customers format `id|name|phone|email|type|fleet` and invoices format with `fleet_disc`; §6 table gains the fleet row and the salesman base becomes `list - trade - fleet`; §9 becomes "TICKET-142 as implemented" listing the six decisions and the old-binary behaviour; §10 lists the three new cases.
- [ ] **Step 2:** `NOTES.md` TICKET-142 section (replace the placeholder): the six decisions with one-line reasons; what v2.3 does with v2.4 files (result of Task 4 step 9); anything the agent got wrong during the ticket, if it did; the manual checks done before each expected file was saved. Keep the whole file under two pages.
- [ ] **Step 3:** `screen.c:18`: `v2.3` → `v2.4`; README title `DEALERDESK v2.4`. `mingw32-make -s version` → `v2.4`; `sh .github/scripts/check-version.sh origin/master` → `version v2.3 -> v2.4`.
- [ ] **Step 4:** Re-record the five expected files by running each script by hand in a temp copy with the new binary (same procedure as before); `git diff tests/e2e` must show only `DEALERDESK v2.3` → `v2.4` header lines, nothing else. If anything else differs, stop and investigate.
- [ ] **Step 5:** `mingw32-make test` → all green. Run the `reviewer` agent on the branch; fix or record findings.
- [ ] **Step 6:** Hand over commit: `screen.c README.md docs/CODEBASE.md NOTES.md tests/e2e/*.expected`; subject "Bump to v2.4 and document the fleet discount". Then the PR text; CI's version step must print `version v2.3 -> v2.4`; merging runs the gated release for v2.4.

## Verification (whole branch)
- Unit: three real invoices still reproduce; new fleet cases pass; count printed by `test_calc`.
- Screens: five cases green; the two original ones changed only on the version line.
- Compatibility: Task 4 step 9 result recorded.
- Reviewer agent: no blocking findings.
- After merge: v2.4 release approved and published; then Rollback to v2.3 and back to v2.4 to prove that workflow.
