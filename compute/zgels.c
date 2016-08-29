/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @precisions normal z -> s d c
 *
 **/

#include "plasma_types.h"
#include "plasma_async.h"
#include "plasma_context.h"
#include "plasma_descriptor.h"
#include "plasma_internal.h"
#include "plasma_z.h"

/***************************************************************************//**
 *
 * @ingroup PLASMA_Complex64_t
 *
 *  Solves overdetermined or underdetermined linear systems
 *  involving an m-by-n matrix A using the QR or the LQ factorization of A.  It
 *  is assumed that A has full rank.  The following options are provided:
 *
 *  # trans = PlasmaNoTrans and m >= n: find the least squares solution of an
 *    overdetermined system, i.e., solve the least squares problem:
 *    minimize || B - A*X ||.
 *
 *  # trans = PlasmaNoTrans and m < n: find the minimum norm solution of an
 *    underdetermined system A * X = B.
 *
 *  Several right-hand side vectors B and solution vectors X can be handled in a
 *  single call; they are stored as the columns of the m-by-nrhs right-hand side
 *  matrix B and the n-by-nrhs solution matrix X.
 *
 *******************************************************************************
 *
 * @param[in] trans
 *          - PlasmaNoTrans:  the linear system involves A
 *                            (the only supported option for now).
 *
 * @param[in] m
 *          The number of rows of the matrix A. m >= 0.
 *
 * @param[in] n
 *          The number of columns of the matrix A. n >= 0.
 *
 * @param[in] nrhs
 *          The number of right hand sides, i.e., the number of columns of the
 *          matrices B and X.  nrhs >= 0.
 *
 * @param[in,out] A
 *          On entry, the m-by-n matrix A.
 *          On exit,
 *          if m >= n, A is overwritten by details of its QR factorization as
 *                     returned by PLASMA_zgeqrf;
 *          if m < n, A is overwritten by details of its LQ factorization as
 *                      returned by PLASMA_zgelqf.
 *
 * @param[in] lda
 *          The leading dimension of the array A. lda >= max(1,m).
 *
 * @param[out] descT
 *          On exit, auxiliary factorization data.
 *
 * @param[in,out] B
 *          On entry, the m-by-nrhs matrix B of right-hand side vectors, stored
 *          columnwise;
 *          On exit, if return value = 0, B is overwritten by the solution
 *          vectors, stored columnwise:
 *          if m >= n, rows 1 to N of B contain the least squares solution
 *          vectors; the residual sum of squares for the solution in each column
 *          is given by the sum of squares of the modulus of elements n+1 to m
 *          in that column;
 *          if m < n, rows 1 to n of B contain the minimum norm solution
 *          vectors;
 *
 * @param[in] ldb
 *          The leading dimension of the array B. ldb >= MAX(1,m,n).
 *
 *******************************************************************************
 *
 * @retval PLASMA_SUCCESS successful exit
 * @retval < 0 if -i, the i-th argument had an illegal value
 *
 *******************************************************************************
 *
 * @sa PLASMA_zgels_Tile_Async
 * @sa PLASMA_cgels
 * @sa PLASMA_dgels
 * @sa PLASMA_sgels
 * @sa PLASMA_zgeqrf
 * @sa PLASMA_zgeqrs
 *
 ******************************************************************************/
int PLASMA_zgels(PLASMA_enum trans, int m, int n, int nrhs,
                 PLASMA_Complex64_t *A, int lda,
                 PLASMA_desc *descT,
                 PLASMA_Complex64_t *B, int ldb)
{
    int nb;
    int retval;
    int status;

    PLASMA_desc descA, descB;

    // Get PLASMA context.
    plasma_context_t *plasma = plasma_context_self();
    if (plasma == NULL) {
        plasma_fatal_error("PLASMA not initialized");
        return PLASMA_ERR_NOT_INITIALIZED;
    }

    // Check input arguments
    if (trans != PlasmaNoTrans) {
        plasma_error("only PlasmaNoTrans supported");
        return PLASMA_ERR_NOT_SUPPORTED;
    }
    if (m < 0) {
        plasma_error("illegal value of m");
        return -2;
    }
    if (n < 0) {
        plasma_error("illegal value of n");
        return -3;
    }
    if (nrhs < 0) {
        plasma_error("illegal value of nrhs");
        return -4;
    }
    if (lda < imax(1, m)) {
        plasma_error("illegal value of lda");
        return -6;
    }
    if (ldb < imax(1, imax(m, n))) {
        plasma_error("illegal value of ldb");
        return -9;
    }

    // quick return
    if (imin(m, imin(n, nrhs)) == 0) {
        for (int i = 0; i < imax(m, n); i++)
            for (int j = 0; j < nrhs; j++)
                B[j*ldb+i] = 0.0;
        return PLASMA_SUCCESS;
    }

    // Tune NB & IB depending on M, N & NRHS; Set NBNB
    //status = plasma_tune(PLASMA_FUNC_ZGELS, M, N, NRHS);
    //if (status != PLASMA_SUCCESS) {
    //    plasma_error("plasma_tune() failed");
    //    return status;
    //}
    nb = plasma->nb;

    // Initialize tile matrix descriptors.
    descA = plasma_desc_init(PlasmaComplexDouble, nb, nb,
                             nb*nb, m, n, 0, 0, m, n);

    descB = plasma_desc_init(PlasmaComplexDouble, nb, nb,
                             nb*nb, m, nrhs, 0, 0, m, nrhs);

    // Allocate matrices in tile layout.
    retval = plasma_desc_mat_alloc(&descA);
    if (retval != PLASMA_SUCCESS) {
        plasma_error("plasma_desc_mat_alloc() failed");
        return retval;
    }
    retval = plasma_desc_mat_alloc(&descB);
    if (retval != PLASMA_SUCCESS) {
        plasma_error("plasma_desc_mat_alloc() failed");
        plasma_desc_mat_free(&descA);
        return retval;
    }

    // Create sequence.
    PLASMA_sequence *sequence = NULL;
    retval = plasma_sequence_create(&sequence);
    if (retval != PLASMA_SUCCESS) {
        plasma_error("plasma_sequence_create() failed");
        return retval;
    }
    // Initialize request.
    PLASMA_request request = PLASMA_REQUEST_INITIALIZER;

    // asynchronous block
    #pragma omp parallel
    #pragma omp master
    {
        // Translate to tile layout.
        PLASMA_zcm2ccrb_Async(A, lda, &descA, sequence, &request);
        PLASMA_zcm2ccrb_Async(B, ldb, &descB, sequence, &request);

        // Call the tile async function.
        PLASMA_zgels_Tile_Async(PlasmaNoTrans,
                                &descA, descT,
                                &descB,
                                sequence, &request);

        // Translate back to LAPACK layout.
        PLASMA_zccrb2cm_Async(&descA, A, lda, sequence, &request);
        PLASMA_zccrb2cm_Async(&descB, B, ldb, sequence, &request);
    }
    // implicit synchronization

    // Free matrices in tile layout.
    plasma_desc_mat_free(&descA);
    plasma_desc_mat_free(&descB);

    // Return status.
    status = sequence->status;
    plasma_sequence_destroy(sequence);
    return status;
}

/***************************************************************************//**
 *
 * @ingroup PLASMA_Complex64_t_Tile_Async
 *
 *  Solves overdetermined or underdetermined linear
 *  system of equations using the tile QR or the tile LQ factorization.
 *  May return before the computation is finished.
 *  Allows for pipelining of operations at runtime.
 *
 *******************************************************************************
 *
 * @param[in] trans
 *          - PlasmaNoTrans:  the linear system involves A
 *                            (the only supported option for now).
 *
 * @param[in,out] descA
 *          Descriptor of matrix A stored in the tile layout.
 *          On exit,
 *          if m >= n, descA is overwritten by details of its QR factorization
 *                     as returned by PLASMA_zgeqrf;
 *          if m < n,  descA is overwritten by details of its LQ factorization
 *                     as returned by PLASMA_zgelqf.
 *
 * @param[out] descT
 *          Descriptor of matrix T.
 *          Auxiliary factorization data, computed by
 *          PLASMA_zgeqrf or PLASMA_zgelqf.
 *
 * @param[in,out] descB
 *          Descriptor of matrix B.
 *          On entry, right-hand side matrix B in the tile layout.
 *          On exit, solution matrix X in the tile layout.
 *
 * @param[in] sequence
 *          Identifies the sequence of function calls that this call belongs to
 *          (for completion checks and exception handling purposes).
 *
 * @param[out] request
 *          Identifies this function call (for exception handling purposes).
 *
 * @retval void
 *          Errors are returned by setting sequence->status and
 *          request->status to error values.  The sequence->status and
 *          request->status should never be set to PLASMA_SUCCESS (the
 *          initial values) since another async call may be setting a
 *          failure value at the same time.
 *
 *******************************************************************************
 *
 * @sa PLASMA_zgels
 * @sa PLASMA_cgels_Tile_Async
 * @sa PLASMA_dgels_Tile_Async
 * @sa PLASMA_sgels_Tile_Async
 *
 ******************************************************************************/
void PLASMA_zgels_Tile_Async(PLASMA_enum trans,
                             PLASMA_desc *descA, PLASMA_desc *descT,
                             PLASMA_desc *descB,
                             PLASMA_sequence *sequence,
                             PLASMA_request *request)
{
    // Get PLASMA context.
    plasma_context_t *plasma = plasma_context_self();
    if (plasma == NULL) {
        plasma_error("PLASMA not initialized");
        plasma_request_fail(sequence, request, PLASMA_ERR_ILLEGAL_VALUE);
        return;
    }

    // Check input arguments.
    if (trans != PlasmaNoTrans) {
        plasma_error("only PlasmaNoTrans supported");
        plasma_request_fail(sequence, request, PLASMA_ERR_NOT_SUPPORTED);
        return;
    }
    if (plasma_desc_check(descA) != PLASMA_SUCCESS) {
        plasma_error("invalid descriptor A");
        plasma_request_fail(sequence, request, PLASMA_ERR_ILLEGAL_VALUE);
        return;
    }
    if (plasma_desc_check(descT) != PLASMA_SUCCESS) {
        plasma_error("invalid descriptor T");
        plasma_request_fail(sequence, request, PLASMA_ERR_ILLEGAL_VALUE);
        return;
    }
    if (plasma_desc_check(descB) != PLASMA_SUCCESS) {
        plasma_error("invalid descriptor B");
        plasma_request_fail(sequence, request, PLASMA_ERR_ILLEGAL_VALUE);
        return;
    }
    if (sequence == NULL) {
        plasma_error("NULL sequence");
        plasma_request_fail(sequence, request, PLASMA_ERR_ILLEGAL_VALUE);
        return;
    }
    if (request == NULL) {
        plasma_error("NULL request");
        plasma_request_fail(sequence, request, PLASMA_ERR_ILLEGAL_VALUE);
        return;
    }

    // quick return
    // TODO:
    // currently NOT equivalent to LAPACK's:
    // Jakub S.: Why was it commented out in version 2.8.0 ?
    //if (imin(m, imin(n, nrhs)) == 0) {
    //    for (int i = 0; i < imax(m, n); i++)
    //        for (int j = 0; j < nrhs; j++)
    //            B[j*ldb+i] = 0.0;
    //    return PLASMA_SUCCESS;
    // Jakub K.: Cannot return PLASMA_SUCCESS.
    //           In fact, cannot return anything - the function is void.
    //           Can we implement LAPACK-compliant quick return?
    //}

    if (descA->m >= descA->n) {
        // solution based on QR factorization
        plasma_pzgeqrf(*descA, *descT, sequence, request);

        // Plasma_ConjTrans will be converted to PlasmaTrans by the
        // automatic datatype conversion, which is what we want here.
        // Note that PlasmaConjTrans is protected from this conversion.
        plasma_pzunmqr(PlasmaLeft, Plasma_ConjTrans,
                       *descA, *descB, *descT,
                       sequence, request);

        plasma_pztrsm(PlasmaLeft, PlasmaUpper,
                      PlasmaNoTrans, PlasmaNonUnit,
                      1.0,
                      plasma_desc_submatrix(*descA, 0, 0, descA->n, descA->n),
                      plasma_desc_submatrix(*descB, 0, 0, descA->n, descB->n),
                      sequence, request);
    }
    else {
        // solution based on LQ factorization
        plasma_error("LQ factorization not supported yet");
        plasma_request_fail(sequence, request, PLASMA_ERR_NOT_SUPPORTED);

    //    plasma_pztile_zero(plasma_desc_submatrix(B, A->m, 0,
    //                                             A->n - A->m, B->n),
    //                       sequence, request);

    //    plasma_pzgelqf(A, T,
    //                   sequence, request);

    //    plasma_pztrsm(PlasmaLeft, PlasmaLower,
    //                  PlasmaNoTrans, PlasmaNonUnit,
    //                  1.0,
    //                  plasma_desc_submatrix(A, 0, 0, A->m, A->m),
    //                  plasma_desc_submatrix(B, 0, 0, A->m, B->n),
    //                  sequence, request);

    //    plasma_pzunmlq(PlasmaLeft, Plasma_ConjTrans,
    //                   A, B, T,
    //                   sequence, request);
    }
}
