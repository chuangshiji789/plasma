/**
 *
 * @file core_blas_z.h
 *
 *  PLASMA header.
 *  PLASMA is a software package provided by Univ. of Tennessee,
 *  Univ. of California Berkeley, Univ. of Colorado Denver and
 *  Univ. of Manchester.
 *
 * @version 3.0.0
 * @author Jakub Kurzak
 * @date 2016-01-01
 * @precisions normal z -> c d s
 *
 **/
#ifndef ICL_CORE_BLAS_Z_H
#define ICL_CORE_BLAS_Z_H

#include "plasma_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
void CORE_zgemm(
    PLASMA_enum transA, PLASMA_enum transB,
    int m, int n, int k,
    PLASMA_Complex64_t alpha, const PLASMA_Complex64_t *A, int lda,
                              const PLASMA_Complex64_t *B, int ldb,
    PLASMA_Complex64_t beta,        PLASMA_Complex64_t *C, int ldc);

void CORE_zherk(
    PLASMA_enum uplo, PLASMA_enum trans,
    int n, int k,
    double alpha, 
    const PLASMA_Complex64_t *A, int lda,
    double beta,        
    PLASMA_Complex64_t *C, int ldc);

void CORE_zsyrk(
    PLASMA_enum uplo, PLASMA_enum trans, 
    int n, int k,
    PLASMA_Complex64_t alpha, 
    const PLASMA_Complex64_t *A, int lda,
    PLASMA_Complex64_t beta,  
    PLASMA_Complex64_t *C, int ldc);

void CORE_zher2k(
    PLASMA_enum uplo, PLASMA_enum trans,
    int n, int k,
    PLASMA_Complex64_t alpha, const PLASMA_Complex64_t *A, int lda,
    const PLASMA_Complex64_t *B, int ldb,
    double beta,  PLASMA_Complex64_t *C, int ldc);

void CORE_zlacpy(
    PLASMA_enum uplo,
    int m, int n,
    const PLASMA_Complex64_t *A, int lda,
          PLASMA_Complex64_t *B, int ldb);

/******************************************************************************/

void CORE_OMP_zgemm(
    PLASMA_enum transA, PLASMA_enum transB,
    int m, int n, int k,
    PLASMA_Complex64_t alpha, const PLASMA_Complex64_t *A, int lda,
                              const PLASMA_Complex64_t *B, int ldb,
    PLASMA_Complex64_t beta,        PLASMA_Complex64_t *C, int ldc);

void CORE_OMP_zherk(
    PLASMA_enum uplo, PLASMA_enum trans,
    int n, int k,
    double alpha, 
    const PLASMA_Complex64_t *A, int lda,
    double beta,        
    PLASMA_Complex64_t *C, int ldc);

void CORE_OMP_zsyrk(
    PLASMA_enum uplo, PLASMA_enum trans, 
    int n, int k,
    PLASMA_Complex64_t alpha, 
    const PLASMA_Complex64_t *A, int lda,
    PLASMA_Complex64_t beta,  
    PLASMA_Complex64_t *C, int ldc);

void CORE_OMP_zher2k(
    PLASMA_enum uplo, PLASMA_enum trans,
    int n, int k,
    PLASMA_Complex64_t alpha, const PLASMA_Complex64_t *A, int lda,
    const PLASMA_Complex64_t *B, int ldb,
    double beta,  PLASMA_Complex64_t *C, int ldc);


void CORE_OMP_zlacpy(
    PLASMA_enum uplo,
    int m, int n, int nb,
    const PLASMA_Complex64_t *A, int lda,
          PLASMA_Complex64_t *B, int ldb);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif // ICL_CORE_BLAS_Z_H
