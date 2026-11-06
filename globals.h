/*
 * globals.h - shared records and tables
 */
#ifndef GLOBALS_H
#define GLOBALS_H

#define MAX_VEH   500
#define MAX_CUST  500
#define MAX_INV   2000

/* vehicle status */
#define ST_STOCK    0
#define ST_RESERVED 1
#define ST_SOLD     2

typedef struct {
    int    id;
    char   plate[12];
    char   make[24];
    char   model[40];
    int    year;
    long   mileage;
    double cost;
    double price;
    int    status;
} VEHICLE;

typedef struct {
    int  id;
    char name[48];
    char phone[20];
    char email[64];
    char type;          /* R = retail, T = trade */
    char fleet;         /* Y = fleet customer, N = not */
} CUSTOMER;

typedef struct {
    int    no;
    char   date[11];
    int    veh_id;
    int    cust_id;
    double list;
    double disc;        /* all discounts together */
    double net;
    double vat;
    double fees;
    double tradein;
    double total;
    double fleet_disc;  /* fleet discount, already included in disc */
} INVOICE;

extern VEHICLE  veh[MAX_VEH];
extern int      nveh;
extern CUSTOMER cust[MAX_CUST];
extern int      ncust;
extern INVOICE  inv[MAX_INV];
extern int      ninv;

extern int next_veh_id;
extern int next_cust_id;
extern int next_inv_no;

#endif
