/*
 * DEALERDESK - showroom dealer management
 * main.c - menus and screens
 *
 * NOTE: plates are always stored in upper case with no spaces
 *       (normalised when the vehicle is added) so lookups are simple.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "globals.h"
#include "screen.h"
#include "dbfile.h"
#include "calc.h"

VEHICLE  veh[MAX_VEH];
int      nveh = 0;
CUSTOMER cust[MAX_CUST];
int      ncust = 0;
INVOICE  inv[MAX_INV];
int      ninv = 0;

int next_veh_id  = 1000;
int next_cust_id = 2000;
int next_inv_no  = 50000;

static const char *status_txt(int s)
{
    if (s == ST_STOCK)
        return "IN STOCK";
    if (s == ST_RESERVED)
        return "RESERVED";
    if (s == ST_SOLD)
        return "SOLD";
    return "?";
}

static VEHICLE *find_veh(int id)
{
    int i;
    for (i = 0; i < nveh; i++)
        if (veh[i].id == id)
            return &veh[i];
    return NULL;
}

static CUSTOMER *find_cust(int id)
{
    int i;
    for (i = 0; i < ncust; i++)
        if (cust[i].id == id)
            return &cust[i];
    return NULL;
}

/* ------------------------------------------------------------ STOCK */

static void list_stock(int show_sold)
{
    int i, n = 0;

    header(show_sold ? "All Vehicles" : "Stock List");
    printf("  %-5s %-10s %-12s %-22s %4s %8s %10s  %s\n",
           "ID", "PLATE", "MAKE", "MODEL", "YEAR", "MILES", "PRICE", "STATUS");
    printf("  ----------------------------------------------------------------------------------\n");
    for (i = 0; i < nveh; i++) {
        if (!show_sold && veh[i].status == ST_SOLD)
            continue;
        printf("  %-5d %-10s %-12s %-22.22s %4d %8ld %10.2f  %s\n",
               veh[i].id, veh[i].plate, veh[i].make, veh[i].model,
               veh[i].year, veh[i].mileage, veh[i].price,
               status_txt(veh[i].status));
        n++;
    }
    printf("\n  %d vehicle(s)\n", n);
    pause_enter();
}

static void add_vehicle(void)
{
    VEHICLE v;
    char b[32];

    header("Add Vehicle");
    if (nveh >= MAX_VEH) {
        printf("  Stock file is full!\n");
        pause_enter();
        return;
    }
    memset(&v, 0, sizeof v);
    read_line("  Plate      : ", v.plate, sizeof v.plate);
    read_line("  Make       : ", v.make, sizeof v.make);
    read_line("  Model      : ", v.model, sizeof v.model);
    v.year = read_int("  Year       : ");
    read_line("  Mileage    : ", b, sizeof b);
    v.mileage = atol(b);
    v.cost = read_money("  Cost price : ");
    v.price = read_money("  List price : ");

    if (v.plate[0] == 0 || v.price <= 0) {
        printf("\n  Plate and list price are required.\n");
        pause_enter();
        return;
    }
    v.id = next_veh_id++;
    v.status = ST_STOCK;
    veh[nveh++] = v;
    save_stock();
    save_ids();
    printf("\n  Vehicle %d added.\n", v.id);
    pause_enter();
}

static void find_plate(void)
{
    char b[16];
    int i, found = 0;

    header("Find Vehicle");
    read_line("  Plate : ", b, sizeof b);
    for (i = 0; i < nveh; i++) {
        if (strcmp(veh[i].plate, b) == 0) {
            printf("\n  ID       : %d\n", veh[i].id);
            printf("  Vehicle  : %s %s (%d)\n", veh[i].make, veh[i].model, veh[i].year);
            printf("  Mileage  : %ld\n", veh[i].mileage);
            printf("  Price    : %.2f\n", veh[i].price);
            printf("  Status   : %s\n", status_txt(veh[i].status));
            found = 1;
        }
    }
    if (!found)
        printf("\n  No vehicle with plate %s\n", b);
    pause_enter();
}

static void change_price(void)
{
    VEHICLE *v;
    double p;

    header("Change Price");
    v = find_veh(read_int("  Vehicle ID : "));
    if (!v) {
        printf("\n  Vehicle not found.\n");
        pause_enter();
        return;
    }
    printf("  %s %s %s, current price %.2f\n", v->plate, v->make, v->model, v->price);
    p = read_money("  New price  : ");
    if (p > 0) {
        v->price = p;
        save_stock();
        printf("\n  Price updated.\n");
    } else {
        printf("\n  Price not changed.\n");
    }
    pause_enter();
}

static void reserve_vehicle(void)
{
    VEHICLE *v;

    header("Reserve / Release");
    v = find_veh(read_int("  Vehicle ID : "));
    if (!v) {
        printf("\n  Vehicle not found.\n");
    } else if (v->status == ST_SOLD) {
        printf("\n  Vehicle is already sold.\n");
    } else {
        v->status = (v->status == ST_STOCK) ? ST_RESERVED : ST_STOCK;
        save_stock();
        printf("\n  Vehicle %d is now %s.\n", v->id, status_txt(v->status));
    }
    pause_enter();
}

static void stock_menu(void)
{
    int opt;

    for (;;) {
        header("Stock");
        printf("  1. List stock\n");
        printf("  2. List all vehicles (incl. sold)\n");
        printf("  3. Add vehicle\n");
        printf("  4. Find by plate\n");
        printf("  5. Change price\n");
        printf("  6. Reserve / release vehicle\n");
        printf("  0. Back\n\n");
        opt = read_int("  Option: ");
        switch (opt) {
        case 1: list_stock(0); break;
        case 2: list_stock(1); break;
        case 3: add_vehicle(); break;
        case 4: find_plate(); break;
        case 5: change_price(); break;
        case 6: reserve_vehicle(); break;
        case 0: return;
        default:
            printf("  Invalid option\n");
            pause_enter();
        }
    }
}

/* -------------------------------------------------------- CUSTOMERS */

static const char *cust_type_txt(const CUSTOMER *c)
{
    if (c->type == 'T')
        return c->fleet == 'Y' ? "Trade+Fleet" : "Trade";
    return c->fleet == 'Y' ? "Fleet" : "Retail";
}

static void print_cust_row(CUSTOMER *c)
{
    printf("  %-5d %-26.26s %-14s %-28.28s %s\n",
           c->id, c->name, c->phone, c->email, cust_type_txt(c));
}

static void list_customers(void)
{
    int i;

    header("Customers");
    printf("  %-5s %-26s %-14s %-28s %s\n", "ID", "NAME", "PHONE", "EMAIL", "TYPE");
    printf("  -----------------------------------------------------------------------------------\n");
    for (i = 0; i < ncust; i++)
        print_cust_row(&cust[i]);
    printf("\n  %d customer(s)\n", ncust);
    pause_enter();
}

static void add_customer(void)
{
    CUSTOMER c;

    header("Add Customer");
    if (ncust >= MAX_CUST) {
        printf("  Customer file is full!\n");
        pause_enter();
        return;
    }
    memset(&c, 0, sizeof c);
    read_line("  Name   : ", c.name, sizeof c.name);
    read_line("  Phone  : ", c.phone, sizeof c.phone);
    read_line("  Email  : ", c.email, sizeof c.email);
    c.type = confirm("  Trade customer (Y/N)? ") ? 'T' : 'R';
    c.fleet = confirm("  Fleet customer (Y/N)? ") ? 'Y' : 'N';

    if (c.name[0] == 0) {
        printf("\n  Name is required.\n");
        pause_enter();
        return;
    }
    c.id = next_cust_id++;
    cust[ncust++] = c;
    save_customers();
    save_ids();
    printf("\n  Customer %d added.\n", c.id);
    pause_enter();
}

static void find_customer(void)
{
    char b[48];
    int i, n = 0;

    header("Find Customer");
    read_line("  Name contains : ", b, sizeof b);
    printf("\n");
    for (i = 0; i < ncust; i++) {
        if (strstr(cust[i].name, b)) {
            print_cust_row(&cust[i]);
            n++;
        }
    }
    printf("\n  %d match(es)\n", n);
    pause_enter();
}

static void mark_fleet(void)
{
    CUSTOMER *c;

    header("Fleet Customer");
    c = find_cust(read_int("  Customer ID : "));
    if (!c) {
        printf("\n  Customer not found.\n");
    } else {
        c->fleet = (c->fleet == 'Y') ? 'N' : 'Y';
        save_customers();
        if (c->fleet == 'Y')
            printf("\n  Customer %d is now a fleet customer.\n", c->id);
        else
            printf("\n  Customer %d is no longer a fleet customer.\n", c->id);
    }
    pause_enter();
}

static void customer_menu(void)
{
    int opt;

    for (;;) {
        header("Customers");
        printf("  1. List customers\n");
        printf("  2. Add customer\n");
        printf("  3. Find customer\n");
        printf("  4. Mark / unmark fleet customer\n");
        printf("  0. Back\n\n");
        opt = read_int("  Option: ");
        switch (opt) {
        case 1: list_customers(); break;
        case 2: add_customer(); break;
        case 3: find_customer(); break;
        case 4: mark_fleet(); break;
        case 0: return;
        default:
            printf("  Invalid option\n");
            pause_enter();
        }
    }
}

/* ----------------------------------------------------------- QUOTES */

static int build_quote(QUOTE *q, VEHICLE **pv, CUSTOMER **pc)
{
    VEHICLE *v;
    CUSTOMER *c;
    double pct, ti;

    v = find_veh(read_int("  Vehicle ID  : "));
    if (!v) {
        printf("\n  Vehicle not found.\n");
        return 0;
    }
    if (v->status == ST_SOLD) {
        printf("\n  Vehicle is already sold.\n");
        return 0;
    }
    c = find_cust(read_int("  Customer ID : "));
    if (!c) {
        printf("\n  Customer not found.\n");
        return 0;
    }
    pct = read_money("  Discount %  : ");
    ti = read_money("  Trade-in    : ");

    calc_quote(q, v->price, c->type, c->fleet == 'Y', pct, ti);
    *pv = v;
    *pc = c;
    return 1;
}

static void print_quote(const char *title, QUOTE *q, VEHICLE *v, CUSTOMER *c)
{
    printf("\n  %s\n", title);
    printf("  ----------------------------------------------\n");
    printf("  Customer : %s (%s)\n", c->name, cust_type_txt(c));
    printf("  Vehicle  : %s %s %s\n", v->plate, v->make, v->model);
    printf("  ----------------------------------------------\n");
    printf("  List price               %12.2f\n", q->list);
    if (q->trade_disc > 0)
        printf("  Trade discount           %12.2f\n", -q->trade_disc);
    if (q->fleet_disc > 0)
        printf("  Fleet discount           %12.2f\n", -q->fleet_disc);
    if (q->disc > 0)
        printf("  Discount (%4.1f%%)         %12.2f\n", q->disc_pct, -q->disc);
    printf("  Net price                %12.2f\n", q->net);
    printf("  Delivery                 %12.2f\n", q->delivery);
    printf("  VAT 20%%                  %12.2f\n", q->vat);
    printf("  Registration fee         %12.2f\n", q->reg_fee);
    if (q->tradein != 0)
        printf("  Trade-in                 %12.2f\n", -q->tradein);
    printf("  ==============================================\n");
    printf("  TOTAL                    %12.2f\n", q->total);
}

static void quote_screen(void)
{
    QUOTE q;
    VEHICLE *v;
    CUSTOMER *c;

    header("Quote");
    if (build_quote(&q, &v, &c)) {
        print_quote("QUOTATION (valid 7 days)", &q, v, c);
    }
    pause_enter();
}

static void sell_vehicle(void)
{
    QUOTE q;
    VEHICLE *v;
    CUSTOMER *c;
    INVOICE *i;
    time_t now;

    header("Sell Vehicle");
    if (ninv >= MAX_INV) {
        printf("  Invoice file is full!\n");
        pause_enter();
        return;
    }
    if (!build_quote(&q, &v, &c)) {
        pause_enter();
        return;
    }
    print_quote("INVOICE PREVIEW", &q, v, c);
    if (!confirm("\n  Confirm sale (Y/N)? ")) {
        printf("\n  Sale cancelled.\n");
        pause_enter();
        return;
    }

    i = &inv[ninv];
    memset(i, 0, sizeof *i);
    i->no = next_inv_no++;
    now = time(NULL);
    strftime(i->date, sizeof i->date, "%Y-%m-%d", localtime(&now));
    i->veh_id = v->id;
    i->cust_id = c->id;
    i->list = q.list;
    i->disc = q.trade_disc + q.fleet_disc + q.disc;
    i->fleet_disc = q.fleet_disc;
    i->net = q.net;
    i->vat = q.vat;
    i->fees = q.delivery + q.reg_fee;
    i->tradein = q.tradein;
    i->total = q.total;
    ninv++;

    v->status = ST_SOLD;
    save_invoices();
    save_stock();
    save_ids();
    printf("\n  Invoice %d created.\n", i->no);
    pause_enter();
}

/* --------------------------------------------------------- INVOICES */

static void list_invoices(void)
{
    int i;
    CUSTOMER *c;

    header("Invoices");
    printf("  %-6s %-10s %-6s %-26s %12s\n", "NO", "DATE", "VEH", "CUSTOMER", "TOTAL");
    printf("  ----------------------------------------------------------------\n");
    for (i = 0; i < ninv; i++) {
        c = find_cust(inv[i].cust_id);
        printf("  %-6d %-10s %-6d %-26.26s %12.2f\n",
               inv[i].no, inv[i].date, inv[i].veh_id,
               c ? c->name : "(unknown)", inv[i].total);
    }
    printf("\n  %d invoice(s)\n", ninv);
    pause_enter();
}

static void view_invoice(void)
{
    int no, i;
    INVOICE *p = NULL;
    VEHICLE *v;
    CUSTOMER *c;

    header("View Invoice");
    no = read_int("  Invoice no : ");
    for (i = 0; i < ninv; i++)
        if (inv[i].no == no)
            p = &inv[i];
    if (!p) {
        printf("\n  Invoice not found.\n");
        pause_enter();
        return;
    }
    v = find_veh(p->veh_id);
    c = find_cust(p->cust_id);
    printf("\n  INVOICE %d    Date: %s\n", p->no, p->date);
    printf("  ----------------------------------------------\n");
    printf("  Customer : %s\n", c ? c->name : "(unknown)");
    printf("  Vehicle  : %s %s %s\n", v ? v->plate : "?", v ? v->make : "", v ? v->model : "");
    printf("  ----------------------------------------------\n");
    printf("  List price               %12.2f\n", p->list);
    if (p->fleet_disc > 0)
        printf("  Fleet discount           %12.2f\n", -p->fleet_disc);
    printf("  Discounts                %12.2f\n", -(p->disc - p->fleet_disc));
    printf("  Net price                %12.2f\n", p->net);
    printf("  Fees                     %12.2f\n", p->fees);
    printf("  VAT                      %12.2f\n", p->vat);
    if (p->tradein != 0)
        printf("  Trade-in                 %12.2f\n", -p->tradein);
    printf("  ==============================================\n");
    printf("  TOTAL                    %12.2f\n", p->total);
    pause_enter();
}

static void invoice_menu(void)
{
    int opt;

    for (;;) {
        header("Invoices");
        printf("  1. List invoices\n");
        printf("  2. View invoice\n");
        printf("  0. Back\n\n");
        opt = read_int("  Option: ");
        switch (opt) {
        case 1: list_invoices(); break;
        case 2: view_invoice(); break;
        case 0: return;
        default:
            printf("  Invalid option\n");
            pause_enter();
        }
    }
}

/* ------------------------------------------------------------- HELP */

static void help_screen(void)
{
    header("Help");
    printf("  DEALERDESK QUICK HELP\n\n");
    printf("  - Plates are converted to upper case automatically.\n");
    printf("  - Trade customers receive a 5%% trade discount.\n");
    printf("  - Salesman discounts above 10%% require a manager PIN.\n");
    printf("  - Quotes are valid for 7 days.\n");
    printf("  - A sold vehicle cannot be reserved or sold again.\n");
    pause_enter();
}

/* ------------------------------------------------------------- MAIN */

int main(void)
{
    int opt;

    load_all();

    for (;;) {
        header("Main Menu");
        printf("  1. Stock\n");
        printf("  2. Customers\n");
        printf("  3. Quote\n");
        printf("  4. Sell vehicle\n");
        printf("  5. Invoices\n");
        printf("  9. Help\n");
        printf("  0. Exit\n\n");
        opt = read_int("  Option: ");
        switch (opt) {
        case 1: stock_menu(); break;
        case 2: customer_menu(); break;
        case 3: quote_screen(); break;
        case 4: sell_vehicle(); break;
        case 5: invoice_menu(); break;
        case 9: help_screen(); break;
        case 0:
            printf("\n  Goodbye.\n");
            return 0;
        default:
            printf("  Invalid option\n");
            pause_enter();
        }
    }
}
