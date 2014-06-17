/*
   Copyright (c) 2009-2014, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef EL_APPLYPACKEDREFLECTORS_LUHF_HPP
#define EL_APPLYPACKEDREFLECTORS_LUHF_HPP



namespace El {
namespace apply_packed_reflectors {

//
// Since applying Householder transforms from vectors stored top-to-bottom
// implies that we will be forming a generalization of 
//
//  (I - tau_1 v_1^T conj(v_1)) (I - tau_0 v_0^T conj(v_0)) = 
//  I - [ v_0^T, v_1^T ] [  tau_0,                       0     ] [ conj(v_0) ]
//                       [ -tau_0 tau_1 conj(v_0) v_1^T, tau_1 ] [ conj(v_1) ],
//
// which has a lower-triangular center matrix, say S, we will form S as 
// the inverse of a matrix T, which can easily be formed as
// 
//   tril(T) = tril( conj(V V^H) ),  diag(T) = 1/t or 1/conj(t),
//
// where V is the matrix of Householder vectors and t is the vector of scalars.
// V is stored row-wise in the matrix.
//

template<typename F>
inline void
LUHF
( Conjugation conjugation, Int offset, 
  const Matrix<F>& H, const Matrix<F>& t, Matrix<F>& A )
{
    DEBUG_ONLY(
        CallStackEntry cse("apply_packed_reflectors::LUHF");
        if( H.Width() != A.Height() )
            LogicError("H's width must match A's height");
    )
    const Int nA = A.Width();
    const Int nH = H.Width();
    const Int diagLength = H.DiagonalLength(offset); 
    DEBUG_ONLY(
        if( t.Height() != diagLength )
            LogicError("t must be the same length as H's offset diag");
    )
    Matrix<F> HPanConj, SInv, Z;

    const Int iOff = ( offset>=0 ? 0      : -offset );
    const Int jOff = ( offset>=0 ? offset : 0       );

    const Int bsize = Blocksize();
    for( Int k=0; k<diagLength; k+=bsize )
    {
        const Int nb = Min(bsize,diagLength-k);
        const Int ki = k+iOff;
        const Int kj = k+jOff;

        auto HPan = LockedViewRange( H, ki, kj, ki+nb, nH );
        auto ABot = ViewRange( A, kj, 0, nH, nA );
        auto t1 = LockedView( t, k, 0, nb, 1 );

        Conjugate( HPan, HPanConj );
        MakeTriangular( UPPER, HPanConj );
        SetDiagonal( HPanConj, F(1) );

        Herk( LOWER, NORMAL, F(1), HPanConj, SInv );
        FixDiagonal( conjugation, t1, SInv );

        Gemm( NORMAL, NORMAL, F(1), HPanConj, ABot, Z );
        Trsm( LEFT, LOWER, NORMAL, NON_UNIT, F(1), SInv, Z );
        Gemm( ADJOINT, NORMAL, F(-1), HPanConj, Z, F(1), ABot );
    }
}

template<typename F>
inline void
LUHF
( Conjugation conjugation, Int offset, 
  const DistMatrix<F>& H, const DistMatrix<F,MD,STAR>& t, DistMatrix<F>& A )
{
    DEBUG_ONLY(
        CallStackEntry cse("apply_packed_reflectors::LUHF");
        if( H.Grid() != t.Grid() || t.Grid() != A.Grid() )
            LogicError("{H,t,A} must be distributed over the same grid");
    )
    const Int nA = A.Width();
    const Int nH = H.Width();
    const Int diagLength = H.DiagonalLength(offset); 
    DEBUG_ONLY(
        if( t.Height() != diagLength )
            LogicError("t must be the same length as H's offset diag");
        if( !H.DiagonalAlignedWith( t, offset ) )
            LogicError("t must be aligned with H's offset diagonal");
    )
    const Grid& g = H.Grid(); 
    DistMatrix<F> HPanConj(g);
    DistMatrix<F,STAR,VR  > HPan_STAR_VR(g);
    DistMatrix<F,STAR,MC  > HPan_STAR_MC(g);
    DistMatrix<F,STAR,STAR> t1_STAR_STAR(g);
    DistMatrix<F,STAR,STAR> SInv_STAR_STAR(g);
    DistMatrix<F,STAR,MR  > Z_STAR_MR(g);
    DistMatrix<F,STAR,VR  > Z_STAR_VR(g);

    const Int iOff = ( offset>=0 ? 0      : -offset );
    const Int jOff = ( offset>=0 ? offset : 0       );

    const Int bsize = Blocksize();
    for( Int k=0; k<diagLength; k+=bsize )
    {
        const Int nb = Min(bsize,diagLength-k);
        const Int ki = k+iOff;
        const Int kj = k+jOff;

        auto HPan = LockedViewRange( H, ki, kj, ki+nb, nH );
        auto ABot = ViewRange( A, kj, 0, nH, nA );
        auto t1 = LockedView( t, k, 0, nb, 1 );

        Conjugate( HPan, HPanConj );
        MakeTriangular( UPPER, HPanConj );
        SetDiagonal( HPanConj, F(1) );

        HPan_STAR_VR = HPanConj;
        Zeros( SInv_STAR_STAR, nb, nb );
        Herk
        ( LOWER, NORMAL,
          F(1), HPan_STAR_VR.LockedMatrix(),
          F(0), SInv_STAR_STAR.Matrix() );
        SInv_STAR_STAR.SumOver( HPan_STAR_VR.RowComm() );
        t1_STAR_STAR = t1;
        FixDiagonal( conjugation, t1_STAR_STAR, SInv_STAR_STAR );

        HPan_STAR_MC.AlignWith( ABot );
        HPan_STAR_MC = HPan_STAR_VR;
        Z_STAR_MR.AlignWith( ABot );
        LocalGemm( NORMAL, NORMAL, F(1), HPan_STAR_MC, ABot, Z_STAR_MR );
        Z_STAR_VR.AlignWith( ABot );
        Z_STAR_VR.PartialRowSumScatterFrom( Z_STAR_MR );

        LocalTrsm
        ( LEFT, LOWER, NORMAL, NON_UNIT, F(1), SInv_STAR_STAR, Z_STAR_VR );

        Z_STAR_MR = Z_STAR_VR;
        LocalGemm
        ( ADJOINT, NORMAL, F(-1), HPan_STAR_MC, Z_STAR_MR, F(1), ABot );
    }
}

} // namespace apply_packed_reflectors
} // namespace El

#endif // ifndef EL_APPLYPACKEDREFLECTORS_LUHF_HPP
