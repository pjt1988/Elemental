/*
   Copyright (c) 2009-2014, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef EL_CONDENSE_HPP
#define EL_CONDENSE_HPP

namespace El {

// Bidiag
// ======

// Return the packed reduction to bidiagonal form
// ----------------------------------------------
template<typename F>
void Bidiag( Matrix<F>& A, Matrix<F>& tP, Matrix<F>& tQ );

template<typename F>
void Bidiag
( DistMatrix<F>& A, DistMatrix<F,STAR,STAR>& tP, DistMatrix<F,STAR,STAR>& tQ );

// Only return the condensed bidiagonal matrix
// -------------------------------------------
template<typename F>
void Bidiag( Matrix<F>& A ); 

template<typename F>
void Bidiag( DistMatrix<F>& A );

namespace bidiag {

template<typename F>
void ApplyQ
( LeftOrRight side, Orientation orientation,
  const Matrix<F>& A, const Matrix<F>& t, Matrix<F>& B );

template<typename F>
void ApplyQ
( LeftOrRight side, Orientation orientation,
  const DistMatrix<F>& A, const DistMatrix<F,MD,STAR>& t, DistMatrix<F>& B );

template<typename F>
void ApplyQ
( LeftOrRight side, Orientation orientation,
  const DistMatrix<F>& A, const DistMatrix<F,STAR,STAR>& t, DistMatrix<F>& B );

template<typename F>
void ApplyP
( LeftOrRight side, Orientation orientation,
  const Matrix<F>& A, const Matrix<F>& t, Matrix<F>& B );

template<typename F>
void ApplyP
( LeftOrRight side, Orientation orientation,
  const DistMatrix<F>& A, const DistMatrix<F,MD,STAR>& t, DistMatrix<F>& B );

template<typename F>
void ApplyP
( LeftOrRight side, Orientation orientation,
  const DistMatrix<F>& A, const DistMatrix<F,STAR,STAR>& t, DistMatrix<F>& B );

} // namespace bidiag

// HermitianTridiag
// ================

namespace HermitianTridiagApproachNS {
enum HermitianTridiagApproach
{
    HERMITIAN_TRIDIAG_NORMAL, // Keep the current grid
    HERMITIAN_TRIDIAG_SQUARE, // Drop to a square process grid
    HERMITIAN_TRIDIAG_DEFAULT // Square grid algorithm only if already square
};
}
using namespace HermitianTridiagApproachNS;

struct HermitianTridiagCtrl {
    HermitianTridiagApproach approach;
    GridOrder order;

    HermitianTridiagCtrl()
    : approach(HERMITIAN_TRIDIAG_SQUARE), order(ROW_MAJOR)
    { }
};

template<typename F>
void HermitianTridiag( UpperOrLower uplo, Matrix<F>& A, Matrix<F>& t );
template<typename F>
void HermitianTridiag
( UpperOrLower uplo, DistMatrix<F>& A, DistMatrix<F,STAR,STAR>& t,
  const HermitianTridiagCtrl ctrl=HermitianTridiagCtrl() );

template<typename F>
void HermitianTridiag( UpperOrLower uplo, Matrix<F>& A );
template<typename F>
void HermitianTridiag
( UpperOrLower uplo, DistMatrix<F>& A,
  const HermitianTridiagCtrl ctrl=HermitianTridiagCtrl() );

namespace herm_tridiag {

template<typename F>
void ApplyQ
( LeftOrRight side, UpperOrLower uplo, Orientation orientation,
  const Matrix<F>& A, const Matrix<F>& t, Matrix<F>& B );
template<typename F>
void ApplyQ
( LeftOrRight side, UpperOrLower uplo, Orientation orientation,
  const DistMatrix<F>& A, const DistMatrix<F,MD,STAR>& t, DistMatrix<F>& B );
template<typename F>
void ApplyQ
( LeftOrRight side, UpperOrLower uplo, Orientation orientation,
  const DistMatrix<F>& A, const DistMatrix<F,STAR,STAR>& t, DistMatrix<F>& B );

} // namespace herm_tridiag

// Hessenberg
// ==========
template<typename F>
void Hessenberg( UpperOrLower uplo, Matrix<F>& A );
template<typename F>
void Hessenberg( UpperOrLower uplo, DistMatrix<F>& A );

template<typename F>
void Hessenberg( UpperOrLower uplo, Matrix<F>& A, Matrix<F>& t );
template<typename F>
void Hessenberg
( UpperOrLower uplo, DistMatrix<F>& A, DistMatrix<F,STAR,STAR>& t );

namespace hessenberg {

template<typename F>
void ApplyQ
( LeftOrRight side, UpperOrLower uplo, Orientation orientation,
  const Matrix<F>& A, const Matrix<F>& t, Matrix<F>& B );
template<typename F>
void ApplyQ
( LeftOrRight side, UpperOrLower uplo, Orientation orientation,
  const DistMatrix<F>& A, const DistMatrix<F,MD,STAR>& t, DistMatrix<F>& B );
template<typename F>
void ApplyQ
( LeftOrRight side, UpperOrLower uplo, Orientation orientation,
  const DistMatrix<F>& A, const DistMatrix<F,STAR,STAR>& t, DistMatrix<F>& B );

} // namespace hessenberg

} // namespace El

#endif // ifndef EL_CONDENSE_HPP
