/*
   Copyright (c) 2009-2015, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El.hpp"

// See Chapter 5 of Nicholas J. Higham's "Functions of Matrices: Theory and
// Computation", which is currently available at:
// http://www.siam.org/books/ot104/OT104HighamChapter5.pdf

namespace El {

namespace sign {

template<typename F>
inline void
NewtonStep
( const Matrix<F>& X, Matrix<F>& XNew, SignScaling scaling=SIGN_SCALE_FROB )
{
    DEBUG_ONLY(CSE cse("sign::NewtonStep"))
    typedef Base<F> Real;

    // Calculate mu while forming XNew := inv(X)
    Real mu=1;
    Matrix<Int> p;
    XNew = X;
    LU( XNew, p );
    if( scaling == SIGN_SCALE_DET )
    {
        SafeProduct<F> det = det::AfterLUPartialPiv( XNew, p );
        mu = Real(1)/Exp(det.kappa);
    }
    inverse::AfterLUPartialPiv( XNew, p );
    if( scaling == SIGN_SCALE_FROB )
        mu = Sqrt( FrobeniusNorm(XNew)/FrobeniusNorm(X) );

    // Overwrite XNew with the new iterate
    const Real halfMu = mu/Real(2);
    const Real halfMuInv = Real(1)/(2*mu); 
    XNew *= halfMuInv;
    Axpy( halfMu, X, XNew );
}

template<typename F>
inline void
NewtonStep
( const DistMatrix<F>& X, DistMatrix<F>& XNew, 
  SignScaling scaling=SIGN_SCALE_FROB )
{
    DEBUG_ONLY(CSE cse("sign::NewtonStep"))
    typedef Base<F> Real;

    // Calculate mu while forming B := inv(X)
    Real mu=1;
    DistMatrix<Int,VC,STAR> p( X.Grid() );
    XNew = X;
    LU( XNew, p );
    if( scaling == SIGN_SCALE_DET )
    {
        SafeProduct<F> det = det::AfterLUPartialPiv( XNew, p );
        mu = Real(1)/Exp(det.kappa);
    }
    inverse::AfterLUPartialPiv( XNew, p );
    if( scaling == SIGN_SCALE_FROB )
        mu = Sqrt( FrobeniusNorm(XNew)/FrobeniusNorm(X) );

    // Overwrite XNew with the new iterate
    const Real halfMu = mu/Real(2);
    const Real halfMuInv = Real(1)/(2*mu); 
    XNew *= halfMuInv;
    Axpy( halfMu, X, XNew );
}

template<typename F>
inline void
NewtonSchulzStep( const Matrix<F>& X, Matrix<F>& XTmp, Matrix<F>& XNew )
{
    DEBUG_ONLY(CSE cse("sign::NewtonSchulzStep"))
    typedef Base<F> Real;
    const Int n = X.Height();
 
    // XTmp := 3I - X^2
    Identity( XTmp, n, n );
    Gemm( NORMAL, NORMAL, Real(-1), X, X, Real(3), XTmp );

    // XNew := 1/2 X XTmp
    Gemm( NORMAL, NORMAL, Real(1)/Real(2), X, XTmp, XNew );
}

template<typename F>
inline void
NewtonSchulzStep
( const DistMatrix<F>& X, DistMatrix<F>& XTmp, DistMatrix<F>& XNew )
{
    DEBUG_ONLY(CSE cse("sign::NewtonSchulzStep"))
    typedef Base<F> Real;
    const Int n = X.Height();

    // XTmp := 3I - X^2
    Identity( XTmp, n, n );
    Gemm( NORMAL, NORMAL, Real(-1), X, X, Real(3), XTmp );

    // XNew := 1/2 X XTmp
    Gemm( NORMAL, NORMAL, Real(1)/Real(2), X, XTmp, XNew );
}

// Please see Chapter 5 of Higham's 
// "Functions of Matrices: Theory and Computation" for motivation behind
// the different choices of p, which are usually in {0,1,2}
template<typename F>
inline Int
Newton( Matrix<F>& A, const SignCtrl<Base<F>>& ctrl )
{
    DEBUG_ONLY(CSE cse("sign::Newton"))
    typedef Base<F> Real;
    Real tol = ctrl.tol;
    if( tol == Real(0) )
        tol = A.Height()*Epsilon<Real>();

    Int numIts=0;
    Matrix<F> B;
    Matrix<F> *X=&A, *XNew=&B;
    while( numIts < ctrl.maxIts )
    {
        // Overwrite XNew with the new iterate
        NewtonStep( *X, *XNew, ctrl.scaling );

        // Use the difference in the iterates to test for convergence
        Axpy( Real(-1), *XNew, *X );
        const Real oneDiff = OneNorm( *X );
        const Real oneNew = OneNorm( *XNew );

        // Ensure that X holds the current iterate and break if possible
        ++numIts;
        std::swap( X, XNew );
        if( ctrl.progress )
            cout << "after " << numIts << " Newton iter's: " 
                 << "oneDiff=" << oneDiff << ", oneNew=" << oneNew 
                 << ", oneDiff/oneNew=" << oneDiff/oneNew << ", tol=" 
                 << tol << endl;
        if( oneDiff/oneNew <= Pow(oneNew,ctrl.power)*tol )
            break;
    }
    if( X != &A )
        A = *X;
    return numIts;
}

template<typename F>
inline Int
Newton( DistMatrix<F>& A, const SignCtrl<Base<F>>& ctrl )
{
    DEBUG_ONLY(CSE cse("sign::Newton"))
    typedef Base<F> Real;
    Real tol = ctrl.tol;
    if( tol == Real(0) )
        tol = A.Height()*Epsilon<Real>();

    Int numIts=0;
    DistMatrix<F> B( A.Grid() );
    DistMatrix<F> *X=&A, *XNew=&B;
    while( numIts < ctrl.maxIts )
    {
        // Overwrite XNew with the new iterate
        NewtonStep( *X, *XNew, ctrl.scaling );

        // Use the difference in the iterates to test for convergence
        Axpy( Real(-1), *XNew, *X );
        const Real oneDiff = OneNorm( *X );
        const Real oneNew = OneNorm( *XNew );

        // Ensure that X holds the current iterate and break if possible
        ++numIts;
        std::swap( X, XNew );
        if( ctrl.progress && A.Grid().Rank() == 0 )
            cout << "after " << numIts << " Newton iter's: "
                 << "oneDiff=" << oneDiff << ", oneNew=" << oneNew
                 << ", oneDiff/oneNew=" << oneDiff/oneNew << ", tol=" 
                 << tol << endl;
        if( oneDiff/oneNew <= Pow(oneNew,ctrl.power)*tol )
            break;
    }
    if( X != &A )
        A = *X;
    return numIts;
}

// TODO: NewtonSchulzHybrid which estimates when || X^2 - I ||_2 < 1

} // namespace sign

template<typename F>
void Sign( Matrix<F>& A, const SignCtrl<Base<F>> ctrl )
{
    DEBUG_ONLY(CSE cse("Sign"))
    sign::Newton( A, ctrl );
}

template<typename F>
void Sign( Matrix<F>& A, Matrix<F>& N, const SignCtrl<Base<F>> ctrl )
{
    DEBUG_ONLY(CSE cse("Sign"))
    Matrix<F> ACopy( A );
    sign::Newton( A, ctrl );
    Gemm( NORMAL, NORMAL, F(1), A, ACopy, N );
}

template<typename F>
void Sign( ElementalMatrix<F>& APre, const SignCtrl<Base<F>> ctrl )
{
    DEBUG_ONLY(CSE cse("Sign"))

    auto APtr = ReadWriteProxy<F,MC,MR>( &APre );
    auto& A = *APtr;

    sign::Newton( A, ctrl );
}

template<typename F>
void Sign
( ElementalMatrix<F>& APre, ElementalMatrix<F>& NPre, 
  const SignCtrl<Base<F>> ctrl )
{
    DEBUG_ONLY(CSE cse("Sign"))

    auto APtr = ReadWriteProxy<F,MC,MR>( &APre ); auto& A = *APtr;
    auto NPtr = WriteProxy<F,MC,MR>( &NPre );     auto& N = *NPtr;

    DistMatrix<F> ACopy( A );
    sign::Newton( A, ctrl );
    Gemm( NORMAL, NORMAL, F(1), A, ACopy, N );
}

// The Hermitian sign decomposition is equivalent to the Hermitian polar
// decomposition... A = (U sgn(Lambda) U') (U sgn(Lambda)Lambda U')
//                    = (U sgn(Lambda) U') (U |Lambda| U')

// Even though sgn(lambda) isn't well-defined when lambda=0, we will extend it
// from the right so that the sign decomposition of a singular Hermitian matrix
// is a polar decomposition (which always exists).

template<typename F>
void HermitianSign
( UpperOrLower uplo, Matrix<F>& A, const HermitianEigCtrl<F>& ctrl )
{
    DEBUG_ONLY(CSE cse("HermitianSign"))
    typedef Base<F> Real;

    // Get the EVD of A
    Matrix<Real> w;
    Matrix<F> Z;
    HermitianEigSubset<Real> subset;
    HermitianEig( uplo, A, w, Z, UNSORTED, subset, ctrl );

    const Int n = A.Height();
    for( Int i=0; i<n; ++i )
    {
        const Real omega = w.Get(i,0);
        if( omega >= 0 )
            w.Set(i,0,Real(1));
        else
            w.Set(i,0,Real(-1));
    }

    // Reform the Hermitian matrix with the modified eigenvalues
    HermitianFromEVD( uplo, A, w, Z );
}

template<typename F>
void HermitianSign
( UpperOrLower uplo, Matrix<F>& A, Matrix<F>& N, 
  const HermitianEigCtrl<F>& ctrl )
{
    DEBUG_ONLY(CSE cse("HermitianSign"))
    typedef Base<F> Real;

    // Get the EVD of A
    Matrix<Real> w;
    Matrix<F> Z;
    HermitianEigSubset<Real> subset;
    HermitianEig( uplo, A, w, Z, UNSORTED, subset, ctrl );

    const Int n = A.Height();
    Matrix<Real> wSgn( n, 1 ), wAbs( n, 1 );
    for( Int i=0; i<n; ++i )
    {
        const Real omega = w.Get(i,0);
        if( omega >= 0 )
        {
            wSgn.Set(i,0,Real(1));
            wAbs.Set(i,0,omega);
        }
        else
        {
            wSgn.Set(i,0,Real(-1));
            wAbs.Set(i,0,-omega);
        }
    }

    // Form the Hermitian matrices with modified eigenvalues
    HermitianFromEVD( uplo, A, wSgn, Z );
    HermitianFromEVD( uplo, N, wAbs, Z );
}

template<typename F>
void HermitianSign
( UpperOrLower uplo, ElementalMatrix<F>& APre, 
  const HermitianEigCtrl<F>& ctrl )
{
    DEBUG_ONLY(CSE cse("HermitianSign"))

    auto APtr = ReadWriteProxy<F,MC,MR>( &APre );
    auto& A = *APtr;

    // Get the EVD of A
    typedef Base<F> Real;
    const Grid& g = A.Grid();
    DistMatrix<Real,VR,STAR> w(g);
    DistMatrix<F> Z(g);
    HermitianEigSubset<Real> subset;
    HermitianEig( uplo, A, w, Z, UNSORTED, subset, ctrl );

    const Int numLocalEigs = w.LocalHeight();
    for( Int iLoc=0; iLoc<numLocalEigs; ++iLoc )
    {
        const Real omega = w.GetLocal(iLoc,0);
        if( omega >= 0 )
            w.SetLocal(iLoc,0,Real(1));
        else
            w.SetLocal(iLoc,0,Real(-1));
    }

    // Reform the Hermitian matrix with the modified eigenvalues
    HermitianFromEVD( uplo, A, w, Z );
}

template<typename F>
void HermitianSign
( UpperOrLower uplo, ElementalMatrix<F>& APre, ElementalMatrix<F>& NPre,
  const HermitianEigCtrl<F>& ctrl )
{
    DEBUG_ONLY(CSE cse("HermitianSign"))

    auto APtr = ReadWriteProxy<F,MC,MR>( &APre ); auto& A = *APtr;
    auto NPtr = WriteProxy<F,MC,MR>( &NPre );     auto& N = *NPtr;

    // Get the EVD of A
    typedef Base<F> Real;
    const Grid& g = A.Grid();
    DistMatrix<Real,VR,STAR> w(g);
    DistMatrix<F> Z(g);
    HermitianEigSubset<Real> subset;
    HermitianEig( uplo, A, w, Z, UNSORTED, subset, ctrl );

    const Int n = A.Height();
    const Int numLocalEigs = w.LocalHeight();
    DistMatrix<Real,VR,STAR> wSgn(g), wAbs(g);
    wSgn.AlignWith( w );
    wAbs.AlignWith( w );
    wSgn.Resize( n, 1 );
    wAbs.Resize( n, 1 );
    for( Int iLoc=0; iLoc<numLocalEigs; ++iLoc )
    {
        const Real omega = w.GetLocal(iLoc,0);
        if( omega >= 0 )
        {
            wSgn.SetLocal(iLoc,0,Real(1));
            wAbs.SetLocal(iLoc,0,omega);
        }
        else
        {
            wSgn.SetLocal(iLoc,0,Real(-1));
            wAbs.SetLocal(iLoc,0,-omega);
        }
    }

    // Form the Hermitian matrix with the modified eigenvalues
    HermitianFromEVD( uplo, A, wSgn, Z );
    HermitianFromEVD( uplo, N, wAbs, Z );
}

#define PROTO(F) \
  template void Sign \
  ( Matrix<F>& A, const SignCtrl<Base<F>> ctrl ); \
  template void Sign \
  ( ElementalMatrix<F>& A, const SignCtrl<Base<F>> ctrl ); \
  template void Sign \
  ( Matrix<F>& A, Matrix<F>& N, const SignCtrl<Base<F>> ctrl ); \
  template void Sign \
  ( ElementalMatrix<F>& A, ElementalMatrix<F>& N, \
    const SignCtrl<Base<F>> ctrl ); \
  template void HermitianSign \
  ( UpperOrLower uplo, Matrix<F>& A, \
    const HermitianEigCtrl<F>& ctrl ); \
  template void HermitianSign \
  ( UpperOrLower uplo, ElementalMatrix<F>& A, \
    const HermitianEigCtrl<F>& ctrl ); \
  template void HermitianSign \
  ( UpperOrLower uplo, Matrix<F>& A, Matrix<F>& N, \
    const HermitianEigCtrl<F>& ctrl ); \
  template void HermitianSign \
  ( UpperOrLower uplo, ElementalMatrix<F>& A, ElementalMatrix<F>& N, \
    const HermitianEigCtrl<F>& ctrl );

#define EL_NO_INT_PROTO
#include "El/macros/Instantiate.h"

} // namespace El
