# DealerDesk codebase reference

This document describes DealerDesk **as the code actually behaves**, with
`file:line` references into the source. Where the README, the help screen or
code comments say something different, the code wins: the invoices in
`data/invoices.dat` were produced by this code and must keep matching it to the
penny. See [Docs vs code](#7-docs-vs-code-discrepancies) for the list.

Line numbers are correct as of the commit that added this file. If they drift,
search for the function name.

## 1. What it is and how to run it

A single-process, menu-driven terminal application in portable C (no
platform-specific headers). It manages vehicle stock, customers, quotes and
invoices for a car showroom.

```
gcc -o dealerdesk *.c -lm      # -lm: calc.c uses floor()
./dealerdesk
```

* **Run it from the repository root.** The data directory is the relative path
  `data/` (`dbfile.c:13`), not `C:\DEALERDESK\DATA` as the README and the
  comment at `dbfile.c:4` claim.
* **Every change is written to `data/*.dat` immediately** (see section 5).
  After experimenting, restore the sample data with `git checkout -- data`.
* There is exactly one `main()` (`main.c:509`), so `*.c` is a valid build. Any
  extra `.c` file placed in the root would be swept into the build, which is
  why tests live under `tests/`.
* Version string "v2.3" is printed by `header()` at `screen.c:18`.

## 2. Source layout

| File | Lines | Role |
|---|---|---|
| `main.c` | 540 | All menus and screens. Defines the global tables and ID counters (`main.c:18-27`). |
| `calc.c`, `calc.h` | 46 / 21 | Pricing. `QUOTE` struct (`calc.h:4-15`), `rnd2()` (`calc.c:18`), `calc_quote()` (`calc.c:23`). Pure functions, no globals, no I/O. |
| `dbfile.c`, `dbfile.h` | 229 / 10 | Load and save the four flat files. `load_all()` (`dbfile.c:156`), `save_stock/customers/invoices/ids()` (`dbfile.c:164-229`). |
| `screen.c`, `screen.h` | 70 / 12 | Terminal helpers: `cls`, `header`, `read_line`, `read_int`, `read_money`, `pause_enter`, `confirm`. |
| `globals.h` | 62 | `VEHICLE`, `CUSTOMER`, `INVOICE` structs, table limits, status codes, `extern` declarations. |

Limits (`globals.h:7-9`): 500 vehicles, 500 customers, 2000 invoices. Each
"add" screen refuses when the table is full (`main.c:87`, `:236`, `:376`).

Vehicle status codes (`globals.h:12-14`): `0` in stock, `1` reserved, `2` sold.

Functions in `main.c`:

| Area | Functions |
|---|---|
| Lookup | `status_txt` :29, `find_veh` :40, `find_cust` :49 (linear scans by id) |
| Stock | `list_stock` :60, `add_vehicle` :81, `find_plate` :116, `change_price` :138, `reserve_vehicle` :162, `stock_menu` :180 |
| Customers | `print_cust_row` :211, `list_customers` :218, `add_customer` :231, `find_customer` :260, `customer_menu` :278 |
| Quotes and sales | `build_quote` :303, `print_quote` :332, `quote_screen` :354, `sell_vehicle` :367 |
| Invoices | `list_invoices` :418, `view_invoice` :436, `invoice_menu` :472 |
| Other | `help_screen` :495, `main` :509 |

## 3. Menu map

```
Main (main.c:516)
  1 Stock (stock_menu, main.c:180)
      1 List stock                 list_stock(0)   hides sold vehicles
      2 List all incl. sold        list_stock(1)
      3 Add vehicle                add_vehicle     plate + list price required
      4 Find by plate              find_plate      exact, case-sensitive strcmp
      5 Change price               change_price    ignores price <= 0
      6 Reserve / release          reserve_vehicle toggles 0 <-> 1, refuses sold
      0 Back
  2 Customers (customer_menu, main.c:278)
      1 List customers             list_customers
      2 Add customer               add_customer    name required; "Trade (Y/N)?" sets type
      3 Find customer              find_customer   substring match on name (strstr)
      0 Back
  3 Quote                          quote_screen    prints quote, saves nothing
  4 Sell vehicle                   sell_vehicle    quote -> preview -> Y/N -> invoice
  5 Invoices (invoice_menu, main.c:472)
      1 List invoices              list_invoices
      2 View invoice               view_invoice
      0 Back
  9 Help                           help_screen     (not listed in README; its text is outdated)
  0 Exit
```

Quote and Sell share `build_quote()` (`main.c:303`): vehicle ID, customer ID,
discount %, trade-in. A sold vehicle cannot be quoted or sold (`main.c:314`).
A reserved vehicle can be.

## 4. Input model

All input passes through `read_line()` (`screen.c:22-43`), which wraps `fgets`
on `stdin`. There is no `scanf`, no raw terminal mode, no `conio`. This matters
because it makes the program fully scriptable: `./dealerdesk < script.txt`.

* **EOF on stdin calls `exit(0)`** (`screen.c:29-31`). A script that runs out
  of lines ends the program cleanly, with exit code 0.
* **The exit code is always 0.** Wrong input never changes it, so automated
  checks must inspect stdout or the data files, not the exit status.
* `read_int` is `atoi` (`screen.c:45`), `read_money` is `atof` (`screen.c:52`).
  Non-numeric input becomes `0`. At a menu prompt `0` means Back, and on the
  main menu it means Exit, so a typo or blank line silently backs out.
* `confirm()` is true only if the first character is `Y` or `y` (`screen.c:69`).
* Input longer than the buffer is truncated and the rest of the line discarded
  (`screen.c:36-38`). Any `|` is replaced with `/` because `|` is the data
  file separator (`screen.c:39-42`).
* Every screen ends with `pause_enter()` (`screen.c:59`), which consumes one
  more line. Scripts must include that blank line.
* Each screen starts with `header()`, which prints the ANSI clear sequence
  `ESC[2J ESC[H` (`screen.c:11`). Captured output contains those bytes.
* Prompts are flushed before reading (`screen.c:28`), so captured output is in
  a stable order.

Example script: quote vehicle 1003 for customer 2003, no discount, no trade-in,
then exit.

```
3
1003
2003
0
0

0
```

## 5. Data files

All four files are plain text, UTF-8/ASCII, stored with LF line endings in git.
Three are pipe-separated records, one per line, read with `fgets` + `strtok`
and rewritten **in full** by `fprintf` on every save. Lines that are empty or
start with `#` are skipped on load (`dbfile.c:34`, `:75`, `:108`). Both CR
and LF are stripped on load (`dbfile.c:33`, `:74`, `:107`); saves write the
platform's native line ending.

### `data/stock.dat`: `VEHICLE` (`globals.h:16-26`)

```
id|plate|make|model|year|mileage|cost|price|status
1003|BD68 KWP|Volkswagen|Golf 1.5 TSI Life|2018|52300|11200.00|15995.00|0
```

Load `dbfile.c:21-60`, save `dbfile.c:164-180`. Field widths: plate 12, make
24, model 40 (including the terminator). Plates are stored exactly as typed;
the sample data has mixed case (`yf20 hbn`).

### `data/customers.dat`: `CUSTOMER` (`globals.h:28-34`)

```
id|name|phone|email|type
2008|Greenway Taxis|01632 960702|fleet@greenwaytaxis.example|R
```

Load `dbfile.c:62-93`, save `dbfile.c:182-197`. `type` is one character: `R`
retail or `T` trade. A missing type field defaults to `'R'` (`dbfile.c:89`).
Only `'T'` is ever tested for (`main.c:215`, `:336`, `calc.c:28`); any other
character behaves as retail.

### `data/invoices.dat`: `INVOICE` (`globals.h:36-48`)

```
no|date|veh_id|cust_id|list|disc|net|vat|fees|tradein|total
50230|2021-02-11|1009|2003|9295.00|464.75|8830.25|1796.05|205.00|1500.00|9331.30
```

Load `dbfile.c:95-138`, save `dbfile.c:199-215`. Written by `sell_vehicle()`
(`main.c:392-406`):

* `disc` is **all discounts added together** (`main.c:400`:
  `trade_disc + disc`). The saved invoice cannot tell a trade discount from a
  salesman discount.
* `fees` is delivery plus registration (`main.c:403`), 205.00 today.
* `date` is `YYYY-MM-DD` from `time(NULL)` (`main.c:395-396`).
* `view_invoice()` prints one combined "Discounts" line (`main.c:461`). The
  per-discount lines exist only on the quote and the invoice preview
  (`print_quote`, `main.c:340-343`).

### `data/next_ids.dat`

```
vehicle=1013
customer=2009
invoice=50233
```

`key=value` lines read with `sscanf` (`dbfile.c:148-152`), written at
`dbfile.c:226`. Defaults if the file is missing: 1000 / 2000 / 50000
(`main.c:25-27`). IDs are never reused.

### Compatibility behaviour of the loaders

* **Missing trailing fields are safe.** Every field after the first is read
  as `t ? value : 0` (or `'R'` for customer type). So a record format can
  gain a field at the end and old files still load without conversion.
* **Older binaries ignore extra fields on load but drop them on save**,
  because `save_*` writes a fixed format.
* **Empty fields shift the following ones.** `strtok` treats `||` as a single
  separator, so a customer saved with an empty phone loads with the email in
  `phone` and the type in `email`, and `type` falls back to `'R'`.
  `add_customer` allows empty phone and email (`main.c:243-244`), so existing
  data may already contain such rows. This is a pre-existing bug, not fixed
  here, and it affects any new trailing field in the same way.
* The whole file is rewritten on each save. If a save fails, an error line is
  printed (`dbfile.c:171`, `:189`, `:207`, `:223`) and the program carries on.

## 6. Pricing rules as implemented

Everything is in `calc_quote()` (`calc.c:23-45`), called only from
`build_quote()` (`main.c:326`) with the vehicle's list price, the customer type
character, the typed discount percentage and the typed trade-in value.

| Step | Code | Rule |
|---|---|---|
| Trade discount | `calc.c:27-29` | `type == 'T'` gets **10%** of list price, rounded to pennies. Retail gets 0. |
| Salesman discount % | `calc.c:31-35` | Clamped to **0..15**. No manager PIN. |
| Salesman discount | `calc.c:36` | `(list - trade_disc) * pct / 100`, rounded. Applied **after** the trade discount. |
| Net price | `calc.c:38` | `list - trade_disc - disc`. Not rounded (its inputs already are). |
| Delivery | `calc.c:39` | Fixed 150.00. |
| VAT | `calc.c:40` | **20% of (net + delivery)**, rounded. Delivery is VATable. |
| Registration fee | `calc.c:41` | Fixed 55.00, outside VAT. |
| Trade-in | `calc.c:42` | Taken as typed, not validated, may be negative or exceed the total. |
| Total | `calc.c:44` | `net + delivery + vat + reg_fee - tradein`. |

Rounding: `rnd2(x) = floor(x * 100 + 0.5) / 100` (`calc.c:18-21`), i.e. round
half up at the penny.

Relevant history (`git log`): `9dc21bb` "vat on the discounted price not list
price (accountant)", `3a46905` "delivery is vatable", `defe6c4` "round to
pennies, invoices 1p out sometimes", `a42d118` "no more manager pin, showrooms
kept ringing for it".

### Worked example: invoice 50230

Retail customer 2003, vehicle 1009 (list 9295.00), 5% discount, trade-in 1500.

```
list        9295.00
trade_disc     0.00   retail
disc         464.75   rnd2(9295.00 * 5 / 100)
net         8830.25   9295.00 - 464.75
delivery     150.00
vat         1796.05   rnd2((8830.25 + 150.00) * 0.20)
reg_fee       55.00
tradein     1500.00
total       9331.30   8830.25 + 150.00 + 1796.05 + 55.00 - 1500.00
```

Saved as `50230|...|9295.00|464.75|8830.25|1796.05|205.00|1500.00|9331.30`,
which is the row in `data/invoices.dat`. The other two sample invoices also
reproduce: 50231 is a trade sale (customer 2002) with the 10% trade discount
and nothing else, 50232 is Greenway Taxis (retail) with an 8% salesman
discount, the "by hand" fleet discount that TICKET-142 describes.

## 7. Docs vs code discrepancies

| Topic | Docs say | Code does | Where |
|---|---|---|---|
| Trade discount | 5% | 10% | `README.md:66`, `main.c:500`, `calc.c:6` vs `calc.c:29`; confirmed by invoice 50231 |
| Salesman discount cap | 10%, manager PIN above | clamped at 15%, no PIN | `README.md:67`, `main.c:501`, `calc.c:7` vs `calc.c:31-34` |
| VAT base | list price | net + delivery | `README.md:64-65`, `calc.c:10` vs `calc.c:40` |
| Delivery and VAT | delivery VAT exempt | delivery is VATable | `README.md:68-69` vs `calc.c:40` |
| Plates | upper-cased, spaces stripped | stored as typed; lookup is case-sensitive | `main.c:5-6`, `main.c:499` vs `main.c:93`, `:124` (commit `76169d5`) |
| Version | v2.1 | v2.3 | `README.md:3` vs `screen.c:18`, `RELEASE.txt:12` |
| Main menu | has "6. Reports" | no Reports; has "9. Help" | `README.md:52-58` vs `main.c:516-523` (commit `399a544`) |
| Build line | `main.c dbfile.c screen.c report.c` | `report.c` does not exist, `calc.c` and `-lm` are needed | `README.md:29` |
| Data location | `C:\DEALERDESK\DATA` | `./data/` relative to the working directory | `README.md:39`, `dbfile.c:4` vs `dbfile.c:13` |

The README and help screen are staff-facing and are **not** changed by this
document; the brief says staff should notice nothing apart from the new
feature. Correcting them is a separate decision.

## 8. Known quirks and risks

* Exit code is always 0, including on EOF (`screen.c:31`).
* Invoice dates come from the wall clock (`main.c:395`); `localtime()` is not
  NULL-checked (`main.c:396`).
* ANSI clear-screen codes are printed before every screen (`screen.c:11`),
  including when stdout is a pipe.
* `find_plate` is an exact, case-sensitive match (`main.c:124`), so
  `yf20 hbn` is not found by `YF20 HBN`.
* `view_invoice` scans all invoices without breaking (`main.c:445-447`); with
  duplicate numbers the last one wins.
* Trade-in is unvalidated (`calc.c:42`); a negative trade-in increases the
  total.
* Table limits are hard (500 / 500 / 2000); the files are rewritten in full.
* `ctype.h` is included in `main.c` but unused (`main.c:11`).
* The pipe-separator bug with empty fields (section 5).

## 9. Notes for TICKET-142 (fleet discount)

Facts the implementation has to respect. Decisions are not made here.

* **Customer flag.** `CUSTOMER` (`globals.h:28-34`) has only `type` (`R`/`T`).
  A fleet marker can be a new sixth field in `customers.dat`; the loader
  (`dbfile.c:88-89`) already tolerates a missing trailing field, so existing
  files load unchanged. Reusing `type = 'F'` would also load, but older code
  would print it as "Retail" and it cannot express a customer who is both
  trade and fleet.
* **Pricing.** The only place to add the 4% is `calc_quote()` (`calc.c:23`),
  and the `QUOTE` struct (`calc.h:4-15`) needs a `fleet_disc` field so
  `print_quote()` (`main.c:332`) can show it as its own line, like
  `trade_disc` at `main.c:340-341`. Delivery and registration must stay
  untouched.
* **Invoices.** The saved invoice holds one combined `disc` (`main.c:400`) and
  `view_invoice` prints one "Discounts" line (`main.c:461`). Showing a separate
  "Fleet discount" line on **stored** invoices needs a new invoice field,
  appended at the end so old rows load with it as 0 and display as today.
* **Open questions to decide and write down:** whether fleet and trade stack
  when a customer is both; whether the 4% is taken from the list price or
  after the trade discount; order of the fleet line relative to the salesman
  discount; whether the salesman discount is then applied to the price net of
  the fleet discount (as it is for trade, `calc.c:36`).
* **Regression guard.** Non-fleet quotes and invoices must be unchanged to
  the penny. The three sample invoices and the worked example above are the
  reference values.
