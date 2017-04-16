/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

/*
 * Name: Qin Jiarui
 * ID: 515030910475
 * loginID: ics515030910475
 */

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
    //I define 12 variables here
    //I use 4 of them in 32 * 32; 12 of them in 64 * 64, and 4 of them in 61 * 67
    int i, j, ii, jj, buf0, buf1, buf2, buf3, buf4, buf5, buf6, buf7;
    

    //In the case of 32 * 32, I just use 8 * 8 blocks to decrease misses
    if (M == 32) {
        for (ii = 0; ii < 32; ii = ii + 8) {
            for (jj = 0; jj < 32; jj = jj + 8) {
                for (i = jj; i < jj + 8; ++ i) {
                    for (j = ii; j < ii + 8; ++ j) {
                        if (i == j) {
                            //the element on the diagonal
                        }
                        else {
                            B[j][i] = A[i][j];
                        }   
                    }
                    if (ii == jj)
                        B[i][i] = A[i][i];
                }
            }
        }
    }


    //In the case of 64 * 64, I have to say, it's very hard for me, I spent almost 3 days to work on it.
    //I use 8 * 8 block, but split it into 4 area(4 * 4):right up, left up, right down, and left down
    //The most tricky part is dealing with right up and left down
    //I use B's right up as a buffer of A's right -- make full use of cache(8 ints in each set)
    //For the blocks on the diagonal, I specificly write a routine for their right up area to decrease misses.
    //For other blocks' right up and left down areas I have to use 8 buffers to decrease misses
    //Details are in my code, if you have time and patience, you can read it :)
    else if (M == 64) {
        for (ii = 0; ii < 64; ii += 8) {
            for (jj = 0; jj < 64; jj += 8) {

                if (ii == jj) {//for the 8 * 8 blocks on the diagonal

                    //left up area
                    for (i = jj; i < jj + 4; ++ i) {
                        buf0 = A[i][ii + 4];
                        buf1 = A[i][ii + 5];
                        buf2 = A[i][ii + 6];
                        buf3 = A[i][ii + 7];

                        for (j = ii; j < ii + 4; ++ j) {
                            if (i == j) {
                                //the element on the diagonal
                            }
                            else {
                                B[j][i] = A[i][j];
                            }   
                        }
                        if (ii == jj)
                            B[i][i] = A[i][i];
                        
                        //use B's right as a buffer
                        B[ii][i + 4] = buf0;
                        B[ii + 1][i + 4] = buf1;
                        B[ii + 2][i + 4] = buf2;
                        B[ii + 3][i + 4] = buf3;
                    }
                    //B's right up buffer to B's left down
                    for (i = 0; i < 4; ++ i) {
                        buf0 = B[ii + i][jj + 4];
                        buf1 = B[ii + i][jj + 5];
                        buf2 = B[ii + i][jj + 6];
                        buf3 = B[ii + i][jj + 7];

                        B[ii + 4 + i][jj] = buf0;
                        B[ii + 4 + i][jj + 1] = buf1;
                        B[ii + 4 + i][jj + 2] = buf2;
                        B[ii + 4 + i][jj + 3] = buf3;
                    }

                    //B's right down and use 4 buffers to store B's left down
                    for (i = jj + 4; i < jj + 8; ++ i) {
                        buf0 = A[i][ii];
                        buf1 = A[i][ii + 1];
                        buf2 = A[i][ii + 2];
                        buf3 = A[i][ii + 3];
                        for (j = ii + 4; j < ii + 8; ++ j) {
                            if (i == j) {
                                //the element on the diagonal
                            }
                            else {
                                B[j][i] = A[i][j];
                            }
                        }
                        if (ii == jj)
                            B[i][i] = A[i][i];

                        B[ii + i - jj - 4][jj + 4] = buf0;
                        B[ii + i - jj - 4][jj + 5] = buf1;
                        B[ii + i - jj - 4][jj + 6] = buf2;
                        B[ii + i - jj - 4][jj + 7] = buf3;
                    }

                    //specific routine on B's right up
                    buf0 = B[ii][jj + 4];
                    buf1 = B[ii + 1][jj + 4];
                    buf2 = B[ii + 2][jj + 4];
                    buf3 = B[ii + 2][jj + 5];
                    buf4 = B[ii + 3][jj + 4];
                    buf5 = B[ii + 3][jj + 5];
                    buf6 = B[ii + 3][jj + 6];

                    B[ii + 1][jj + 4] = B[ii][jj + 5];
                    B[ii + 2][jj + 4] = B[ii][jj + 6];
                    B[ii + 2][jj + 5] = B[ii + 1][jj + 6];
                    B[ii + 3][jj + 4] = B[ii][jj + 7];
                    B[ii + 3][jj + 5] = B[ii + 1][jj + 7];
                    B[ii + 3][jj + 6] = B[ii + 2][jj + 7];

                    B[ii][jj + 5] = buf1;
                    B[ii][jj + 6] = buf2;
                    B[ii + 1][jj + 6] = buf3;
                    B[ii][jj + 7] = buf4;
                    B[ii + 1][jj + 7] = buf5;
                    B[ii + 2][jj + 7] = buf6;
                }  



                else {//for other 8 * 8 blocks

                    //left up area
                    for (i = jj; i < jj + 4; ++ i) {
                        buf0 = A[i][ii + 4];
                        buf1 = A[i][ii + 5];
                        buf2 = A[i][ii + 6];
                        buf3 = A[i][ii + 7];

                        for (j = ii; j < ii + 4; ++ j) {
                            B[j][i] = A[i][j]; 
                        }
                        
                        //use B's right as a buffer
                        B[ii][i + 4] = buf0;
                        B[ii + 1][i + 4] = buf1;
                        B[ii + 2][i + 4] = buf2;
                        B[ii + 3][i + 4] = buf3;
                    }

                    //GREATEST tricky here
                    for (i = 0; i < 4; ++ i) {
                        buf0 = B[ii + i][jj + 4];
                        buf1 = B[ii + i][jj + 5];
                        buf2 = B[ii + i][jj + 6];
                        buf3 = B[ii + i][jj + 7];


                        buf4 = A[jj + 4][ii + i];
                        buf5 = A[jj + 5][ii + i];
                        buf6 = A[jj + 6][ii + i];
                        buf7 = A[jj + 7][ii + i];

                        B[ii + i][jj + 4] = buf4;
                        B[ii + i][jj + 5] = buf5;
                        B[ii + i][jj + 6] = buf6;
                        B[ii + i][jj + 7] = buf7;

                        B[ii + 4 + i][jj] = buf0;
                        B[ii + 4 + i][jj + 1] = buf1;
                        B[ii + 4 + i][jj + 2] = buf2;
                        B[ii + 4 + i][jj + 3] = buf3;             
                    }

                    //B's right down
                    for (i = jj + 4; i < jj + 8; ++ i) {
                        for (j = ii + 4; j < ii + 8; ++ j) {
                            B[j][i] = A[i][j];
                        }
                    }
                }
            }
        }
    }

    //In the case of 61 * 67, I found that I can just use 16 as block size and the result is good enough
    //Because the limit is not that tight as 64 * 64
    else {
        for (ii = 0; ii < 61; ii = ii + 16) {
            for (jj = 0; jj < 67; jj = jj + 16) {
                for (i = jj; (i < jj + 16 && i < 67); ++ i) {
                    for (j = ii; (j < ii + 16 && j < 61); ++ j) {
                        if (i == j) {
                            //The elements on the diagonal
                        }
                        else
                            B[j][i] = A[i][j];
                    }
                    if (ii == jj)
                        B[i][i] = A[i][i];
                }
            }
        }
    }    
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions() {
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
