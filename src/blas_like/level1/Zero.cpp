/*
   Copyright (c) 2009-2014, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El.hpp"

namespace El {

template<typename T>
void Zero( Matrix<T>& A )
{
    DEBUG_ONLY(CallStackEntry cse("Zero"))
    const Int height = A.Height();
    const Int width = A.Width();
    EL_PARALLEL_FOR
    for( Int j=0; j<width; ++j )
        MemZero( A.Buffer(0,j), height );
}

template<typename T>
void Zero( AbstractDistMatrix<T>& A )
{
    DEBUG_ONLY(CallStackEntry cse("Zero"))
    Zero( A.Matrix() );
}

template<typename T>
void Zero( AbstractBlockDistMatrix<T>& A )
{
    DEBUG_ONLY(CallStackEntry cse("Zero"))
    Zero( A.Matrix() );
}

template<typename T>
void Zero( MultiVec<T>& X )
{
    DEBUG_ONLY(CallStackEntry cse("Zero"))
    const int height = X.Height();
    const int width = X.Width();
    for( int j=0; j<width; ++j )
        for( int i=0; i<height; ++i )
            X.Set( i, j, T(0) );
}

template<typename T>
void Zero( DistMultiVec<T>& X )
{
    DEBUG_ONLY(CallStackEntry cse("Zero"))
    const int localHeight = X.LocalHeight();
    const int width = X.Width();
    for( int j=0; j<width; ++j )
        for( int iLocal=0; iLocal<localHeight; ++iLocal )
            X.SetLocal( iLocal, j, T(0) );
}

#define PROTO(T) \
  template void Zero( Matrix<T>& A ); \
  template void Zero( AbstractDistMatrix<T>& A ); \
  template void Zero( AbstractBlockDistMatrix<T>& A ); \
  template void Zero( MultiVec<T>& A ); \
  template void Zero( DistMultiVec<T>& A );

#include "El/macros/Instantiate.h"

} // namespace El