/* ============================================================
 * mpi_analytics.c
 * Hybrid MPI + OpenMP Commerce Analytics Benchmark
 *
 * Design (avoids unsafe MPI_Scatter on char**):
 *   1. Rank 0 counts total rows, broadcasts count.
 *   2. Every rank opens the CSV independently and seeks
 *      to its assigned row range — no MPI_Scatter needed.
 *   3. Each rank accumulates local metrics using OpenMP
 *      parallel for with reduction.
 *   4. MPI_Reduce sums the per-rank double arrays into
 *      rank 0's global array. Count is reduced separately
 *      as MPI_INT (never mix types in one Reduce call).
 *
 * Foster's Methodology:
 *   Partitioning  – rows divided among MPI ranks
 *   Communication – MPI_Reduce partial sums
 *   Agglomeration – contiguous row chunks per rank
 *   Mapping       – one MPI rank per cloud CPU core
 * ============================================================ */

#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_LINE 512

/* ---- Metric indices in the reduction array ---- */
enum {
    M_REVENUE = 0,
    M_ELECTRONICS, M_FASHION, M_GROCERIES,
    M_JOHOR, M_KL, M_PENANG, M_SABAH, M_SARAWAK,
    M_CASH, M_CARD, M_EWALLET,
    M_LAPTOP, M_PHONE, M_SHOES,
    NUM_METRICS   /* 15 doubles */
};

/* ---- CSV field indices (0-based) ---- */
enum {
    F_TRANSACTION_ID = 0,
    F_TIMESTAMP,
    F_CUSTOMER_ID,
    F_MEMBERSHIP,
    F_PRODUCT_ID,
    F_PRODUCT_NAME,
    F_CATEGORY,
    F_REGION,
    F_PAYMENT_METHOD,
    F_QUANTITY,
    F_UNIT_PRICE,
    F_DISCOUNT,
    F_TOTAL_PRICE,
    NUM_FIELDS
};

/* ---- Parse one CSV line ---- */
static int parse_line(char *line,
                      char **product, char **category,
                      char **region, char **payment,
                      double *total_price)
{
    char *fields[NUM_FIELDS];
    char *rest = line;

    for (int i = 0; i < NUM_FIELDS; i++) {
        fields[i] = strtok_r((i == 0) ? rest : NULL, ",\r\n", &rest);
        if (!fields[i]) return -1;
    }

    *product     = fields[F_PRODUCT_NAME];
    *category    = fields[F_CATEGORY];
    *region      = fields[F_REGION];
    *payment     = fields[F_PAYMENT_METHOD];
    *total_price = atof(fields[F_TOTAL_PRICE]);

    return 0;
}

/* ---- Accumulate one row into metric array ---- */
static void accumulate(double *m,
                       const char *product, const char *category,
                       const char *region,  const char *payment,
                       double value)
{
    m[M_REVENUE] += value;

    if      (strcmp(category, "Electronics") == 0) m[M_ELECTRONICS] += value;
    else if (strcmp(category, "Fashion")     == 0) m[M_FASHION]     += value;
    else if (strcmp(category, "Groceries")   == 0) m[M_GROCERIES]   += value;

    if      (strcmp(region, "Johor")   == 0) m[M_JOHOR]   += value;
    else if (strcmp(region, "KL")      == 0) m[M_KL]      += value;
    else if (strcmp(region, "Penang")  == 0) m[M_PENANG]  += value;
    else if (strcmp(region, "Sabah")   == 0) m[M_SABAH]   += value;
    else if (strcmp(region, "Sarawak") == 0) m[M_SARAWAK] += value;

    if      (strcmp(payment, "Cash")    == 0) m[M_CASH]    += value;
    else if (strcmp(payment, "Card")    == 0) m[M_CARD]    += value;
    else if (strcmp(payment, "eWallet") == 0) m[M_EWALLET] += value;

    if      (strcmp(product, "Laptop") == 0) m[M_LAPTOP] += value;
    else if (strcmp(product, "Phone")  == 0) m[M_PHONE]  += value;
    else if (strcmp(product, "Shoes")  == 0) m[M_SHOES]  += value;
}

/* ============================================================ */
int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* ---- Step 1: Rank 0 counts total data rows ---- */
    int total_rows = 0;

    if (rank == 0) {
        FILE *f = fopen("data/dataset.csv", "r");
        if (!f) {
            fprintf(stderr, "Error: cannot open data/dataset.csv\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        char buf[MAX_LINE];
        if (!fgets(buf, MAX_LINE, f)) { fclose(f); MPI_Abort(MPI_COMM_WORLD, 1); }
        while (fgets(buf, MAX_LINE, f))
            total_rows++;
        fclose(f);
        printf("[Rank 0] Total rows: %d, MPI ranks: %d, OMP threads: %d\n",
               total_rows, size, omp_get_max_threads());
    }

    MPI_Bcast(&total_rows, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (total_rows == 0) {
        if (rank == 0) fprintf(stderr, "Error: dataset is empty\n");
        MPI_Finalize();
        return 1;
    }

    /* ---- Step 2: Compute this rank's row range ---- */
    int chunk     = total_rows / size;
    int remainder = total_rows % size;

    /* Ranks 0..remainder-1 get (chunk+1) rows, the rest get chunk */
    int my_start, my_count;
    if (rank < remainder) {
        my_count = chunk + 1;
        my_start = rank * (chunk + 1);
    } else {
        my_count = chunk;
        my_start = remainder * (chunk + 1) + (rank - remainder) * chunk;
    }

    /* ---- Step 3: Each rank reads only its rows ---- */
    double start_time = MPI_Wtime();

    FILE *file = fopen("data/dataset.csv", "r");
    if (!file) {
        fprintf(stderr, "[Rank %d] Error: cannot open data/dataset.csv\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    char line[MAX_LINE];
    if (!fgets(line, MAX_LINE, file)) { fclose(file); MPI_Abort(MPI_COMM_WORLD, 1); }

    /* Skip to my_start */
    for (int i = 0; i < my_start; i++)
        if (!fgets(line, MAX_LINE, file)) break;

    /* Read assigned rows into a buffer for OpenMP */
    char (*rows)[MAX_LINE] = malloc((size_t)my_count * MAX_LINE);
    if (!rows) {
        fprintf(stderr, "[Rank %d] Error: malloc failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (int i = 0; i < my_count; i++) {
        if (!fgets(rows[i], MAX_LINE, file)) {
            /* Shouldn't happen; zero-fill remaining */
            rows[i][0] = '\0';
        }
    }
    fclose(file);

    /* ---- Step 4: Process rows with OpenMP ---- */
    double local_metrics[NUM_METRICS];
    memset(local_metrics, 0, sizeof(local_metrics));
    int local_count = 0;

    #pragma omp parallel
    {
        /* Per-thread private accumulators to avoid false sharing */
        double thread_m[NUM_METRICS];
        memset(thread_m, 0, sizeof(thread_m));
        int thread_count = 0;

        #pragma omp for schedule(static)
        for (int i = 0; i < my_count; i++) {

            char buf[MAX_LINE];
            strncpy(buf, rows[i], MAX_LINE);
            buf[MAX_LINE - 1] = '\0';

            char *product, *category, *region, *payment;
            double value;

            if (parse_line(buf, &product, &category, &region, &payment, &value) != 0)
                continue;

            accumulate(thread_m, product, category, region, payment, value);
            thread_count++;
        }

        /* Merge thread results into local totals */
        #pragma omp critical
        {
            for (int k = 0; k < NUM_METRICS; k++)
                local_metrics[k] += thread_m[k];
            local_count += thread_count;
        }
    }

    free(rows);

    /* ---- Step 5: MPI_Reduce — doubles and int separately ---- */
    double global_metrics[NUM_METRICS];
    memset(global_metrics, 0, sizeof(global_metrics));
    int    global_count = 0;

    MPI_Reduce(local_metrics, global_metrics, NUM_METRICS,
               MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    MPI_Reduce(&local_count, &global_count, 1,
               MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    double end_time = MPI_Wtime();
    double elapsed  = end_time - start_time;

    /* ---- Step 6: Rank 0 outputs results ---- */
    if (rank == 0) {

        double atv = (global_count > 0)
                   ? global_metrics[M_REVENUE] / global_count : 0.0;

        printf("\n========== MPI+OpenMP ANALYTICS ==========\n");
        printf("Transactions : %d\n",    global_count);
        printf("Total Revenue: %.2f\n",  global_metrics[M_REVENUE]);
        printf("ATV          : %.2f\n",  atv);
        printf("Processes    : %d\n",    size);
        printf("OMP Threads  : %d\n",    omp_get_max_threads());
        printf("Time         : %.6f s\n", elapsed);
        printf("\n--- Revenue by Category ---\n");
        printf("Electronics  : %.2f\n", global_metrics[M_ELECTRONICS]);
        printf("Fashion      : %.2f\n", global_metrics[M_FASHION]);
        printf("Groceries    : %.2f\n", global_metrics[M_GROCERIES]);
        printf("\n--- Revenue by Region ---\n");
        printf("Johor        : %.2f\n", global_metrics[M_JOHOR]);
        printf("KL           : %.2f\n", global_metrics[M_KL]);
        printf("Penang       : %.2f\n", global_metrics[M_PENANG]);
        printf("Sabah        : %.2f\n", global_metrics[M_SABAH]);
        printf("Sarawak      : %.2f\n", global_metrics[M_SARAWAK]);
        printf("\n--- Payment Distribution ---\n");
        printf("Cash         : %.2f\n", global_metrics[M_CASH]);
        printf("Card         : %.2f\n", global_metrics[M_CARD]);
        printf("eWallet      : %.2f\n", global_metrics[M_EWALLET]);
        printf("\n--- Product Demand ---\n");
        printf("Laptop       : %.2f\n", global_metrics[M_LAPTOP]);
        printf("Phone        : %.2f\n", global_metrics[M_PHONE]);
        printf("Shoes        : %.2f\n", global_metrics[M_SHOES]);
        printf("==========================================\n\n");

        /* ---- Append to benchmark.csv ---- */
        mkdir("results", 0755);

        FILE *check = fopen("results/benchmark.csv", "r");
        int need_header = (check == NULL);
        if (check) fclose(check);

        FILE *out = fopen("results/benchmark.csv", "a");
        if (!out) {
            fprintf(stderr, "Error: cannot open results/benchmark.csv\n");
            MPI_Finalize();
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
            "mpi,%d,%d,%.6f,"
            "%.2f,%.2f,"
            "%.2f,%.2f,%.2f,"
            "%.2f,%.2f,%.2f,%.2f,%.2f,"
            "%.2f,%.2f,%.2f,"
            "%.2f,%.2f,%.2f\n",
            global_count, size, elapsed,
            global_metrics[M_REVENUE], atv,
            global_metrics[M_ELECTRONICS], global_metrics[M_FASHION],
            global_metrics[M_GROCERIES],
            global_metrics[M_JOHOR], global_metrics[M_KL],
            global_metrics[M_PENANG], global_metrics[M_SABAH],
            global_metrics[M_SARAWAK],
            global_metrics[M_CASH], global_metrics[M_CARD],
            global_metrics[M_EWALLET],
            global_metrics[M_LAPTOP], global_metrics[M_PHONE],
            global_metrics[M_SHOES]);

        fclose(out);
        printf("Results appended to results/benchmark.csv\n");
    }

    MPI_Finalize();
    return 0;
}
