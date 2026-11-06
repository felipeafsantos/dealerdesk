/*
 * test_calc.c - unit tests for calc.c
 *
 * No framework: a CHECK macro, a counter, and a non-zero exit on failure.
 * Build and run with "make test".
 *
 * The expected values are the real invoices in data/invoices.dat. They were
 * produced by the current code, so they pin down the pricing rules as they
 * are, not as the README describes them (see docs/CODEBASE.md section 6).
 */
#include <stdio.h>
#include <math.h>
#include "calc.h"

static int checks = 0;
static int failures = 0;

#define CHECK(cond) do { \
    checks++; \
    if (!(cond)) { \
        printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        failures++; \
    } \
} while (0)

/* money is compared to the penny */
#define CHECK_MONEY(actual, expected) do { \
    double a_ = (actual), e_ = (expected); \
    checks++; \
    if (fabs(a_ - e_) >= 0.005) { \
        printf("FAIL %s:%d: %s = %.2f, expected %.2f\n", \
               __FILE__, __LINE__, #actual, a_, e_); \
        failures++; \
    } \
} while (0)

/* invoice 50230: retail, 5% salesman discount, 1500 trade-in */
static void test_invoice_50230_retail_with_discount_and_tradein(void)
{
    QUOTE q;
    calc_quote(&q, 9295.00, 'R', 0, 5, 1500.00);
    CHECK_MONEY(q.list,       9295.00);
    CHECK_MONEY(q.trade_disc,    0.00);
    CHECK_MONEY(q.fleet_disc,    0.00);
    CHECK_MONEY(q.disc_pct,      5.00);
    CHECK_MONEY(q.disc,        464.75);
    CHECK_MONEY(q.net,        8830.25);
    CHECK_MONEY(q.delivery,    150.00);
    CHECK_MONEY(q.vat,        1796.05);
    CHECK_MONEY(q.reg_fee,      55.00);
    CHECK_MONEY(q.tradein,    1500.00);
    CHECK_MONEY(q.total,      9331.30);
}

/* invoice 50231: trade customer, no salesman discount */
static void test_invoice_50231_trade(void)
{
    QUOTE q;
    calc_quote(&q, 13495.00, 'T', 0, 0, 0);
    CHECK_MONEY(q.trade_disc,  1349.50);   /* 10%, not the 5% the old README said */
    CHECK_MONEY(q.fleet_disc,     0.00);
    CHECK_MONEY(q.disc,           0.00);
    CHECK_MONEY(q.net,        12145.50);
    CHECK_MONEY(q.vat,         2459.10);
    CHECK_MONEY(q.total,      14809.60);
}

/* invoice 50232: retail, 8% salesman discount (Greenway Taxis by hand) */
static void test_invoice_50232_retail_8pct(void)
{
    QUOTE q;
    calc_quote(&q, 10495.00, 'R', 0, 8, 0);
    CHECK_MONEY(q.fleet_disc, 0.00);   /* given by hand then, not a fleet customer yet */
    CHECK_MONEY(q.disc,    839.60);
    CHECK_MONEY(q.net,    9655.40);
    CHECK_MONEY(q.vat,    1961.08);
    CHECK_MONEY(q.total, 11821.48);
}

static void test_salesman_discount_is_applied_after_trade_discount(void)
{
    QUOTE q;
    calc_quote(&q, 10000.00, 'T', 0, 10, 0);
    CHECK_MONEY(q.trade_disc, 1000.00);
    CHECK_MONEY(q.disc,        900.00);   /* 10% of 9000, not of 10000 */
    CHECK_MONEY(q.net,        8100.00);
}

static void test_discount_pct_is_clamped_to_0_15(void)
{
    QUOTE q;
    calc_quote(&q, 10000.00, 'R', 0, 20, 0);
    CHECK_MONEY(q.disc_pct, 15.00);
    CHECK_MONEY(q.disc,   1500.00);

    calc_quote(&q, 10000.00, 'R', 0, -5, 0);
    CHECK_MONEY(q.disc_pct, 0.00);
    CHECK_MONEY(q.disc,     0.00);
}

static void test_fees_are_fixed_and_never_discounted(void)
{
    QUOTE q;
    calc_quote(&q, 10000.00, 'T', 0, 15, 2000.00);
    CHECK_MONEY(q.delivery, 150.00);
    CHECK_MONEY(q.reg_fee,   55.00);

    calc_quote(&q, 500.00, 'R', 0, 0, 0);
    CHECK_MONEY(q.delivery, 150.00);
    CHECK_MONEY(q.reg_fee,   55.00);
}

static void test_retail_gets_no_trade_discount(void)
{
    QUOTE q;
    calc_quote(&q, 10000.00, 'R', 0, 0, 0);
    CHECK_MONEY(q.trade_disc, 0.00);
    CHECK_MONEY(q.net,    10000.00);
    /* any character other than 'T' is treated as retail */
    calc_quote(&q, 10000.00, 'X', 0, 0, 0);
    CHECK_MONEY(q.trade_disc, 0.00);
}

static void test_vat_is_20pct_of_net_plus_delivery(void)
{
    QUOTE q;
    calc_quote(&q, 10000.00, 'R', 0, 0, 0);
    CHECK_MONEY(q.vat, 2030.00);   /* (10000 + 150) * 0.20 */
}

static void test_tradein_is_taken_off_the_total(void)
{
    QUOTE q;
    calc_quote(&q, 10000.00, 'R', 0, 0, 2500.00);
    CHECK_MONEY(q.total, 10000.00 + 150.00 + 2030.00 + 55.00 - 2500.00);
}

/* TICKET-142: fleet customers get 4% of the list price */
static void test_fleet_customer_gets_4pct_of_list(void)
{
    QUOTE q;
    calc_quote(&q, 10000.00, 'R', 1, 0, 0);
    CHECK_MONEY(q.fleet_disc,  400.00);
    CHECK_MONEY(q.disc,          0.00);
    CHECK_MONEY(q.net,        9600.00);
    CHECK_MONEY(q.vat,        1950.00);   /* (9600 + 150) * 0.20 */
    CHECK_MONEY(q.total,     11755.00);
}

static void test_fleet_and_trade_both_apply_then_salesman(void)
{
    QUOTE q;
    calc_quote(&q, 10000.00, 'T', 1, 10, 0);
    CHECK_MONEY(q.trade_disc, 1000.00);
    CHECK_MONEY(q.fleet_disc,  400.00);
    CHECK_MONEY(q.disc,        860.00);   /* 10% of 8600, after both */
    CHECK_MONEY(q.net,        7740.00);
    CHECK_MONEY(q.vat,        1578.00);
    CHECK_MONEY(q.total,      9523.00);
}

static void test_fleet_discount_rounds_to_penny(void)
{
    QUOTE q;
    calc_quote(&q, 1234.56, 'R', 1, 0, 0);
    CHECK_MONEY(q.fleet_disc,  49.38);   /* 49.3824 */
    CHECK_MONEY(q.net,       1185.18);
    CHECK_MONEY(q.vat,        267.04);   /* 267.036 */
    CHECK_MONEY(q.total,     1657.22);
}

static void test_fees_unchanged_for_fleet(void)
{
    QUOTE q;
    calc_quote(&q, 10000.00, 'R', 1, 15, 500.00);
    CHECK_MONEY(q.delivery, 150.00);
    CHECK_MONEY(q.reg_fee,   55.00);
}

/* Greenway Taxis quoting vehicle 1002; the screen tests use the same numbers */
static void test_greenway_on_vehicle_1002(void)
{
    QUOTE q;
    calc_quote(&q, 10995.00, 'R', 1, 0, 0);
    CHECK_MONEY(q.fleet_disc,   439.80);
    CHECK_MONEY(q.net,        10555.20);
    CHECK_MONEY(q.vat,         2141.04);
    CHECK_MONEY(q.total,      12901.24);
}

static void test_rnd2_rounds_half_up_to_pennies(void)
{
    /* 0.125 and 0.124 are chosen because they are exact in binary */
    CHECK_MONEY(rnd2(0.125), 0.13);
    CHECK_MONEY(rnd2(0.124), 0.12);
    CHECK_MONEY(rnd2(1796.05), 1796.05);
    CHECK(rnd2(0.0) == 0.0);
}

int main(void)
{
    test_invoice_50230_retail_with_discount_and_tradein();
    test_invoice_50231_trade();
    test_invoice_50232_retail_8pct();
    test_salesman_discount_is_applied_after_trade_discount();
    test_discount_pct_is_clamped_to_0_15();
    test_fees_are_fixed_and_never_discounted();
    test_retail_gets_no_trade_discount();
    test_vat_is_20pct_of_net_plus_delivery();
    test_tradein_is_taken_off_the_total();
    test_fleet_customer_gets_4pct_of_list();
    test_fleet_and_trade_both_apply_then_salesman();
    test_fleet_discount_rounds_to_penny();
    test_fees_unchanged_for_fleet();
    test_greenway_on_vehicle_1002();
    test_rnd2_rounds_half_up_to_pennies();

    printf("test_calc: %d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
