/* ============================================================
 * serial_analytics.c
 * Sequential Commerce Analytics Benchmark
 * 
 * Reads a CSV dataset of retail transactions and computes:
 *   - Total Revenue & Average Transaction Value (ATV)
 *   - Revenue by Category  (Electronics, Fashion, Groceries)
 *   - Revenue by Region    (Johor, KL, Penang, Sabah, Sarawak)
 *   - Payment Distribution (Cash, Card, eWallet)
 *   - Product Demand       (Laptop, Phone, Shoes)
 *
 * Results are printed to stdout and appended to benchmark.csv.
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_LINE 512

/* ---- Field indices in the CSV (0-based) ---- */
enum {
    F_TRANSACTION_ID = 0,
    F_TIMESTAMP,
    F_CUSTOMER_ID,
    F_MEMBERSHIP,
    F_PRODUCT_ID,
    F_PRODUCT_NAME,     /* index 5 */
    F_CATEGORY,         /* index 6 */
    F_REGION,           /* index 7 */
    F_PAYMENT_METHOD,   /* index 8 */
    F_QUANTITY,
    F_UNIT_PRICE,
    F_DISCOUNT,
    F_TOTAL_PRICE,      /* index 12 */
    NUM_FIELDS
};

/* ---- Parse one CSV line and extract the fields we need ---- */
static int parse_line(char *line,
                      char **product, char **category,
                      char **region, char **payment,
                      double *total_price)
{
    char *fields[NUM_FIELDS];
    char *rest = line;
    int i;

    for (i = 0; i < NUM_FIELDS; i++) {
        fields[i] = strtok_r((i == 0) ? rest : NULL, ",\r\n", &rest);
        if (!fields[i]) return -1;  /* malformed row */
    }

    *product     = fields[F_PRODUCT_NAME];
    *category    = fields[F_CATEGORY];
    *region      = fields[F_REGION];
    *payment     = fields[F_PAYMENT_METHOD];
    *total_price = atof(fields[F_TOTAL_PRICE]);

    return 0;
}

/* ============================================================ */
int main(void)
{
    FILE *file = fopen("data/dataset.csv", "r");
    if (!file) {
        fprintf(stderr, "Error: cannot open data/dataset.csv\n");
        return 1;
    }

    char line[MAX_LINE];

    /* Skip header */
    if (!fgets(line, MAX_LINE, file)) {
        fprintf(stderr, "Error: empty file\n");
        fclose(file);
        return 1;
    }

    /* ---- Accumulators ---- */
    double total_revenue = 0.0;
    int    count = 0;

    double electronics = 0.0, fashion = 0.0, groceries = 0.0;
    double johor = 0.0, kl = 0.0, penang = 0.0, sabah = 0.0, sarawak = 0.0;
    double cash = 0.0, card = 0.0, ewallet = 0.0;
    double laptop = 0.0, phone = 0.0, shoes = 0.0;

    /* ---- Timed section ---- */
    struct timespec ts_start, ts_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    while (fgets(line, MAX_LINE, file)) {

        char *product, *category, *region, *payment;
        double value;

        if (parse_line(line, &product, &category, &region, &payment, &value) != 0)
            continue;

        total_revenue += value;
        count++;

        /* Revenue by Category */
        if      (strcmp(category, "Electronics") == 0) electronics += value;
        else if (strcmp(category, "Fashion")     == 0) fashion     += value;
        else if (strcmp(category, "Groceries")   == 0) groceries   += value;

        /* Revenue by Region */
        if      (strcmp(region, "Johor")   == 0) johor   += value;
        else if (strcmp(region, "KL")      == 0) kl      += value;
        else if (strcmp(region, "Penang")  == 0) penang  += value;
        else if (strcmp(region, "Sabah")   == 0) sabah   += value;
        else if (strcmp(region, "Sarawak") == 0) sarawak += value;

        /* Payment Distribution */
        if      (strcmp(payment, "Cash")    == 0) cash    += value;
        else if (strcmp(payment, "Card")    == 0) card    += value;
        else if (strcmp(payment, "eWallet") == 0) ewallet += value;

        /* Product Demand (revenue-based) */
        if      (strcmp(product, "Laptop") == 0) laptop += value;
        else if (strcmp(product, "Phone")  == 0) phone  += value;
        else if (strcmp(product, "Shoes")  == 0) shoes  += value;
    }

    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    fclose(file);

    double time_sec = (ts_end.tv_sec  - ts_start.tv_sec)
                    + (ts_end.tv_nsec - ts_start.tv_nsec) / 1e9;

    double atv = (count > 0) ? total_revenue / count : 0.0;

    /* ---- Console output ---- */
    printf("\n========== SERIAL ANALYTICS ==========\n");
    printf("Transactions : %d\n",    count);
    printf("Total Revenue: %.2f\n",  total_revenue);
    printf("ATV          : %.2f\n",  atv);
    printf("Time         : %.6f s\n", time_sec);
    printf("\n--- Revenue by Category ---\n");
    printf("Electronics  : %.2f\n", electronics);
    printf("Fashion      : %.2f\n", fashion);
    printf("Groceries    : %.2f\n", groceries);
    printf("\n--- Revenue by Region ---\n");
    printf("Johor        : %.2f\n", johor);
    printf("KL           : %.2f\n", kl);
    printf("Penang       : %.2f\n", penang);
    printf("Sabah        : %.2f\n", sabah);
    printf("Sarawak      : %.2f\n", sarawak);
    printf("\n--- Payment Distribution ---\n");
    printf("Cash         : %.2f\n", cash);
    printf("Card         : %.2f\n", card);
    printf("eWallet      : %.2f\n", ewallet);
    printf("\n--- Product Demand ---\n");
    printf("Laptop       : %.2f\n", laptop);
    printf("Phone        : %.2f\n", phone);
    printf("Shoes        : %.2f\n", shoes);
    printf("=======================================\n\n");

    /* ---- Append to benchmark.csv ---- */
    mkdir("results", 0755);

    /* Write header if file is new */
    FILE *check = fopen("results/benchmark.csv", "r");
    int need_header = (check == NULL);
    if (check) fclose(check);

    FILE *out = fopen("results/benchmark.csv", "a");
    if (!out) {
        fprintf(stderr, "Error: cannot open results/benchmark.csv\n");
        return 1;
    }

    if (need_header) {
        fprintf(out,
            "Implementation,Dataset,Processes,ExecutionTime,"
            "Revenue,ATV,"
            "Electronics,Fashion,Groceries,"
            "Johor,KL,Penang,Sabah,Sarawak,"
            "Cash,Card,eWallet,"
            "Laptop,Phone,Shoes\n");
    }

    fprintf(out,
        "serial,%d,1,%.6f,"
        "%.2f,%.2f,"
        "%.2f,%.2f,%.2f,"
        "%.2f,%.2f,%.2f,%.2f,%.2f,"
        "%.2f,%.2f,%.2f,"
        "%.2f,%.2f,%.2f\n",
        count, time_sec,
        total_revenue, atv,
        electronics, fashion, groceries,
        johor, kl, penang, sabah, sarawak,
        cash, card, ewallet,
        laptop, phone, shoes);

    fclose(out);

    printf("Results appended to results/benchmark.csv\n");

    return 0;
}
