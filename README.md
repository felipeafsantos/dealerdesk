This standard disclaimer states that the work is entirely fictional and any resemblance to real people, places, or events is coincidental.

DEALERDESK v2.4
===============

Dealer management for the showroom: stock, customers, quotes and invoices.


QUICK START (Linux, macOS, or WSL on Windows)
---------------------------------------------

You need a C compiler (gcc or clang). From the repository root:

    gcc -o dealerdesk *.c -lm
    ./dealerdesk

* Always run it from the repository root. It reads and writes the files in ./data
* Pick a menu option by typing its number and pressing ENTER. 0 goes back, and
  0 on the main menu exits.
* Every change (new vehicle, sale, ...) is saved to data/*.dat immediately.
  To put the sample data back:

      git checkout -- data


BUILD
-----

    make            equivalent to: gcc -o dealerdesk *.c -lm
    make test       builds and runs the unit tests and the screen tests
    make clean

(On Windows with the WinLibs/MinGW toolchain use mingw32-make, and run
make test from Git Bash: the screen tests need sh.)

Developers: docs/CODEBASE.md describes the code, the data files and the
pricing rules as the code implements them. CI builds and tests every push.

Windows releases used to be built with the Borland compiler on the old build
PC. Releases are now built by CI on Linux, see RELEASE.txt.


RUN
---

    ./dealerdesk

Data files are read from ./data, relative to the directory you run it from

    stock.dat       vehicles
    customers.dat   customers
    invoices.dat    invoices
    next_ids.dat    id counters

Do NOT edit the .dat files by hand while the program is running!


MENU
----

    1. Stock
    2. Customers
    3. Quote
    4. Sell vehicle
    5. Invoices
    9. Help
    0. Exit


PRICING RULES
-------------

* Trade customers get a 10% trade discount off the list price.
* Fleet customers get a 4% fleet discount off the list price. A customer can
  be both trade and fleet, and then gets both. Mark a customer as fleet from
  the Customers menu.
* Salesman discount is up to 15% and is taken off the price after the trade
  and fleet discounts. There is no manager PIN.
* VAT (20%) is calculated on the net price plus delivery.
* Delivery (150.00) is added to every sale and is VATable.
* First registration (55.00) is added to every sale and is outside VAT.
* Trade-in value is taken off the final total.
* Every amount is rounded to the penny.


RELEASE
-------

See RELEASE.txt


Problems: ask Phil (ext. 214).
