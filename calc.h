#ifndef CALC_H
#define CALC_H

typedef struct {
    double list;
    double trade_disc;
    double fleet_disc;
    double disc_pct;
    double disc;
    double net;
    double delivery;
    double vat;
    double reg_fee;
    double tradein;
    double total;
} QUOTE;

double rnd2(double x);
void   calc_quote(QUOTE *q, double list, char cust_type, int fleet,
                  double disc_pct, double tradein);

#endif
