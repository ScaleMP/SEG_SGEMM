#include <stdio.h>
#include <stdlib.h>
#include <mkl.h>

#define TYPE float
#define FUNC cblas_sgemm
#define CHAR 'S'

long int max_c_rows = 128*1024, max_c_cols = 128*1024, max_b_rows = 16*1024;
long int seg_c_rows, seg_c_cols, seg_b_rows, count;
TYPE *a, *b, *c;    // matrices

struct coordinates {
  long int c_row, c_col, b_row;
} *plan;

double timenow(void)
{
        static int first = 1;
        static time_t base;
        struct timeval now;
        gettimeofday(&now, NULL);
        if (first) {
            base = now.tv_sec;
            first = 0;
        }
        return (double)(now.tv_sec - base) + 1e-6 * now.tv_usec;
}

void makeplan(void)
{
        long int i,j,k;

        if (seg_c_rows == 0 || max_c_rows % seg_c_rows != 0 || max_c_rows < seg_c_rows) {
            printf("Error: Target segmented row count (%ld) not a divisor of target row count (%ld)\n", seg_c_rows, max_c_rows);
        } else if (seg_c_cols == 0 || max_c_cols % seg_c_cols != 0 || max_c_cols < seg_c_cols) {
            printf("Error: Target segmented column count (%ld) not a divisor of target column count (%ld)\n", seg_c_cols, max_c_cols);
        } else if (seg_b_rows == 0 || max_b_rows % seg_b_rows != 0 || max_b_rows < seg_b_rows) {
            printf("Error: Hidden segmented row count (%ld) not a divisor of hidden row count (%ld)\n", seg_b_rows, max_b_rows);
        } else {
            count = (max_c_rows / seg_c_rows) * (max_c_cols / seg_c_cols) * (max_b_rows / seg_b_rows);
            plan = calloc(count, sizeof(struct coordinates));
            for (i = 0; i < (max_c_rows / seg_c_rows); i++)
                for (j = 0; j < (max_c_cols / seg_c_cols); j++)
                    for (k = 0; k < (max_b_rows / seg_b_rows); k++) {
                        plan[(i*(max_c_cols / seg_c_cols)+j)*(max_b_rows / seg_b_rows)+k].c_row = i * seg_c_rows;
                        plan[(i*(max_c_cols / seg_c_cols)+j)*(max_b_rows / seg_b_rows)+k].c_col = j * seg_c_cols;
                        plan[(i*(max_c_cols / seg_c_cols)+j)*(max_b_rows / seg_b_rows)+k].b_row = k * seg_b_rows;
                    }
            return;
        }
        exit(1);
}

void allocate(void)
{
        a = mkl_malloc(sizeof(TYPE) * max_c_rows * max_b_rows, 4096);
        b = mkl_malloc(sizeof(TYPE) * max_b_rows * max_c_cols, 4096);
        c = mkl_malloc(sizeof(TYPE) * max_c_rows * max_c_cols, 4096);
        if (NULL == a || NULL == b || NULL == c) {
            printf("Error: Unable to allocate matrix data.\n");
            exit(1);
        }
}

main(int argc, char **argv)
{
        int arg = 1;
        int loops = 1;
        long int i,j,k,l;
        double memsize, srun, init1, init2, init3, scalc, ecalc, fcalc, before, after;

        setlinebuf(stdout);

        if (argc > arg) max_c_rows = atoi(argv[arg++]);
        seg_c_rows = max_c_rows;
        if (argc > arg) max_c_cols = atoi(argv[arg++]);
        seg_c_cols = max_c_cols;
        if (argc > arg) max_b_rows = atoi(argv[arg++]);
        seg_b_rows = max_b_rows;
        if (argc > arg) seg_c_rows = atoi(argv[arg++]);
        if (argc > arg) seg_c_cols = atoi(argv[arg++]);
        if (argc > arg) seg_b_rows = atoi(argv[arg++]);

        if (argc > arg) loops      = atoi(argv[arg++]);

        makeplan();
        allocate();

        memsize = (1.0 * max_c_rows * max_c_cols + 1.0 * max_c_rows * max_b_rows + 1.0 * max_c_cols * max_b_rows) * sizeof(TYPE) / 1024.0 / 1024.0 / 1024.0;
        printf("\n%cGEMM : Performing C[%ld][%ld] = A[%ld][%ld] x B[%ld][%ld]  (%.1fGB)\n",CHAR,max_c_rows,max_c_cols,max_c_rows,max_b_rows,max_b_rows,max_c_cols,memsize);

        srun = timenow();              // Start recording matrix initialization time (Init)

#pragma omp parallel for private(i,j)
        for (i=0; i<max_c_rows; i++)    // Initialize the multiplicand matrix
            for (j=0; j<max_b_rows; j++)
                a[i*max_b_rows+j] = (TYPE)(i+j+1);
        init1 = timenow();

#pragma omp parallel for private(i,j)
        for (i=0; i<max_c_cols; i++)    // Initialize the multiplier matrix
            for (j=0; j<max_b_rows; j++)
                b[i*max_b_rows+j] = (TYPE)((i+1)*(j+1));
        init2 = timenow();

#pragma omp parallel for private(i,j)
        for (i=0; i<max_c_rows; i++)    // Initialize the result matrix
            for (j=0; j<max_c_cols; j++)
                c[i*max_c_cols+j] = 0;
        init3 = timenow();

        printf("%cGEMM : Init time: A = %3.02f , B = %3.02f , C = %3.02f\n",CHAR, init1 - srun, init2 - init1, init3 - init2);
        for (j = 0; j < loops; j++) {
            printf("%cGEMM : Iter %d : Performing %ld sub-calculations.\n", CHAR, j+1, count);

            after = scalc = timenow();      // Start recording matrix multiplication (computation) time (Calc)
            for (i=0; i<count; i++) {       // Calling MKL matrix multiplication routine to multiply matrix A and B
                before = after;
                FUNC(CblasRowMajor, CblasNoTrans, CblasTrans,
                    seg_c_rows, seg_c_cols, seg_b_rows, 1,
                    a + max_b_rows * plan[i].c_row + plan[i].b_row, max_b_rows,
                    b + max_b_rows * plan[i].c_col + plan[i].b_row, max_b_rows, 1,
                    c + max_c_cols * plan[i].c_row + plan[i].c_col, max_c_cols );
                after = timenow();
                printf("%cGEMM :        Part %4ld    Calc times: %3.02f      GFlop/s: %3.02f\n", CHAR, i+1, after - before, 1e-9 * 2.0 * seg_c_rows * seg_c_cols * seg_b_rows / (after-before));
            }
            ecalc = timenow();        // Stop recording computation time

            printf("%cGEMM :   Init time: %3.02f   Calc times: %3.02f      GFlop/s: %3.02f\n", CHAR, init3 - srun, ecalc - scalc, 1e-9 * 2.0 * max_c_rows * max_c_cols * max_b_rows / (ecalc - scalc));
        }
        mkl_free(c); mkl_free(b); mkl_free(a); free(plan);

        exit(0);
}


