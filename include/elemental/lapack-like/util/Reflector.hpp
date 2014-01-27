/*
   Copyright (C) 1992-2008 The University of Tennessee
   All rights reserved.

   Copyright (c) 2009-2014, Jack Poulson
   All rights reserved.

   This file is loosely based upon the LAPACK routines dlarfg.f and zlarfg.f.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef ELEM_REFLECTOR_HPP
#define ELEM_REFLECTOR_HPP

#include ELEM_NRM2_INC
#include ELEM_SCALE_INC

#include "./Reflector/Col.hpp"
#include "./Reflector/Row.hpp"

namespace elem {

//
// Follows the LAPACK convention of defining tau such that
//
//   H = I - tau [1; v] [1, v'],
//
// but adjoint(H) [chi; x] = [beta; 0]. 
//
// Note that the adjoint of H is applied. 
//
// On exit, chi is overwritten with beta, and x is overwritten with v.
//
// The major difference from LAPACK is in the treatment of the special case 
// of x=0, where LAPACK would put H := I, which is not a valid Householder 
// reflector. We instead follow the FLAME convention of defining H such that 
//    adjoint(H) [chi; 0] = [-chi; 0],
// which is accomplished by setting tau=2, and v=0.
//

template<typename F>
inline F
Reflector( Matrix<F>& chi, Matrix<F>& x )
{
    DEBUG_ONLY(CallStackEntry cse("Reflector"))
    typedef Base<F> Real;

    Real norm = Nrm2( x );
    F alpha = chi.Get(0,0);

    if( norm == Real(0) && ImagPart(alpha) == Real(0) )
    {
        chi.Set(0,0,-chi.Get(0,0));
        return F(2);
    }

    Real beta;
    if( RealPart(alpha) <= 0 )
        beta = lapack::SafeNorm( alpha, norm );
    else
        beta = -lapack::SafeNorm( alpha, norm );

    // Rescale if the vector is too small
    const Real safeMin = lapack::MachineSafeMin<Real>();
    const Real epsilon = lapack::MachineEpsilon<Real>();
    const Real safeInv = safeMin/epsilon;
    Int count = 0;
    if( Abs(beta) < safeInv )
    {
        Real invOfSafeInv = Real(1)/safeInv;
        do
        {
            ++count;
            Scale( invOfSafeInv, x );
            alpha *= invOfSafeInv;
            beta *= invOfSafeInv;
        } while( Abs(beta) < safeInv );

        norm = Nrm2( x );
        if( RealPart(alpha) <= 0 )
            beta = lapack::SafeNorm( alpha, norm );
        else
            beta = -lapack::SafeNorm( alpha, norm );
    }

    F tau = (beta-alpha) / beta;
    Scale( Real(1)/(alpha-beta), x );

    // Undo the scaling
    for( Int j=0; j<count; ++j )
        beta *= safeInv;

    chi.Set(0,0,beta);
    return tau;
}

template<typename F>
inline F
Reflector( F& chi, Int m, F* x, Int incx )
{
    DEBUG_ONLY(CallStackEntry cse("Reflector"))
    typedef Base<F> Real;

    Real norm = blas::Nrm2( m, x, incx );
    F alpha = chi;

    if( norm == Real(0) && ImagPart(alpha) == Real(0) )
    {
        chi = -chi;
        return F(2);
    }

    Real beta;
    if( RealPart(alpha) <= 0 )
        beta = lapack::SafeNorm( alpha, norm );
    else
        beta = -lapack::SafeNorm( alpha, norm );

    // Rescale if the vector is too small
    const Real safeMin = lapack::MachineSafeMin<Real>();
    const Real epsilon = lapack::MachineEpsilon<Real>();
    const Real safeInv = safeMin/epsilon;
    Int count = 0;
    if( Abs(beta) < safeInv )
    {
        Real invOfSafeInv = Real(1)/safeInv;
        do
        {
            ++count;
            blas::Scal( m, invOfSafeInv, x, incx );
            alpha *= invOfSafeInv;
            beta *= invOfSafeInv;
        } while( Abs(beta) < safeInv );

        norm = blas::Nrm2( m, x, incx );
        if( alpha.real() <= 0 )
            beta = lapack::SafeNorm( alpha.real(), alpha.imag(), norm );
        else
            beta = -lapack::SafeNorm( alpha.real(), alpha.imag(), norm );
    }

    F tau = (beta-alpha) / beta;
    blas::Scal( m, Real(1)/(alpha-beta), x, incx );

    // Undo the scaling
    for( Int j=0; j<count; ++j )
        beta *= safeInv;

    chi = beta;
    return tau;
}

template<typename F,Dist U,Dist V>
inline F
Reflector( DistMatrix<F,U,V>& chi, DistMatrix<F,U,V>& x )
{
    DEBUG_ONLY(
        CallStackEntry cse("Reflector");
        if( chi.Grid() != x.Grid() )
            LogicError("chi and x must be distributed over the same grid");
        if( chi.Height() != 1 || chi.Width() != 1 )
            LogicError("chi must be a scalar");
        if( x.Height() != 1 && x.Width() != 1 )
            LogicError("x must be a vector");
    )
    F tau;
    if( x.Width() == 1 && x.RowAlign() == chi.RowAlign() )
    {
        if( x.RowRank() == x.RowAlign() )
            tau = reflector::Col( chi, x );
        mpi::Broadcast( tau, x.RowAlign(), x.RowComm() );
    }
    else
    {
        if( x.ColRank() == x.ColAlign() )
            tau = reflector::Row( chi, x );
        mpi::Broadcast( tau, x.ColAlign(), x.ColComm() );
    }
    return tau;
}

} // namespace elem

#endif // ifndef ELEM_REFLECTOR_HPP