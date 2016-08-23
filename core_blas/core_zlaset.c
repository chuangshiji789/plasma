/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @precisions normal z -> c d s
 *
 **/

#include "core_blas.h"
#include "plasma_types.h"
#include "plasma_internal.h"

#ifdef PLASMA_WITH_MKL
    #include <mkl.h>
#else
    #include <cblas.h>
    #include <lapacke.h>
#endif

/***************************************************************************//**
 *
 * @ingroup CORE_PLASMA_Complex64_t
 *
 *  Sets the elements of the matrix A on the diagonal
 *  to beta and on the off-diagonals to alpha
 *
 *******************************************************************************
 *
 * @param[in] uplo
 *          Specifies which elements of the matrix are to be set
 *          - PlasmaUpper: Upper part of A is set;
 *          - PlasmaLower: Lower part of A is set;
 *          - PlasmaUpperLower: ALL elements of A are set.
 *
 * @param[in] m
 *          The number of rows of the matrix A.  m >= 0.
 *
 * @param[in] n
 *         The number of columns of the matrix A.  n >= 0.
 *
 * @param[in] alpha
 *         The constant to which the off-diagonal elements are to be set.
 *
 * @param[in] beta
 *         The constant to which the diagonal elements are to be set.
 *
 * @param[in,out] A
 *         On entry, the m-by-n tile A.
 *         On exit, A has been set accordingly.
 *
 * @param[in] lda
 *         The leading dimension of the array A.  lda >= max(1,m).
 *
 ******************************************************************************/
void CORE_zlaset(PLASMA_enum uplo,
                 int m, int n,
                 PLASMA_Complex64_t alpha, PLASMA_Complex64_t beta,
                 PLASMA_Complex64_t *A, int lda)
{
    LAPACKE_zlaset_work(LAPACK_COL_MAJOR,
                        lapack_const(uplo),
                        m, n,
                        alpha, beta, A, lda);
}

/******************************************************************************/
void CORE_OMP_zlaset(PLASMA_enum uplo, int m, int n,
                     PLASMA_Complex64_t alpha, PLASMA_Complex64_t beta,
                     PLASMA_Complex64_t *A, int lda)
{
    // OpenMP depends on lda == m
    #pragma omp task depend(out:A[0:m*n])
    CORE_zlaset(uplo, m, n,
                alpha, beta,
                A, lda);
}