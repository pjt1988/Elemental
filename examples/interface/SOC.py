#
#  Copyright (c) 2009-2015, Jack Poulson
#  All rights reserved.
#
#  This file is part of Elemental and is under the BSD 2-Clause License, 
#  which can be found in the LICENSE file in the root directory, or at 
#  http://opensource.org/licenses/BSD-2-Clause
#
import El, math, time

n = 5
cutoff = 1000
output = True

worldRank = El.mpi.WorldRank()
worldSize = El.mpi.WorldSize()

# Construct s and z in the (product) cone
# =======================================
s = El.DistMultiVec()
z = El.DistMultiVec()
orders = El.DistMultiVec(El.iTag)
firstInds = El.DistMultiVec(El.iTag)
sampleRad = 1./math.sqrt(1.*n)
El.Uniform( s, 3*n, 1, 0, sampleRad )
El.Uniform( z, 3*n, 1, 0, sampleRad )
s.Set( 0, 0, 2. )
s.Set( n, 0, 3. )
s.Set( 2*n, 0, 4. )
z.Set( 0, 0, 5. )
z.Set( n, 0, 6. )
z.Set( 2*n, 0, 7. )
El.Zeros( orders, 3*n, 1 )
El.Zeros( firstInds, 3*n, 1 )
for i in xrange(n):
  orders.Set( i, 0, n )
  orders.Set( i+n, 0, n )
  orders.Set( i+2*n, 0, n )
  firstInds.Set( i, 0, 0 )
  firstInds.Set( i+n, 0, n )
  firstInds.Set( i+2*n, 0, 2*n )
if output:
  El.Print( s, "s" )
  El.Print( z, "z" )
  El.Print( orders, "orders" )
  El.Print( firstInds, "firstInds" )

# Compute the (Jordan) determinants and number of non-positive SOC members
# ========================================================================
sDets = El.SOCDets( s, orders, firstInds, cutoff )
zDets = El.SOCDets( z, orders, firstInds, cutoff )
sDetsBcast = El.DistMultiVec()
zDetsBcast = El.DistMultiVec()
El.Copy( sDets, sDetsBcast )
El.Copy( zDets, zDetsBcast )
El.SOCBroadcast( sDetsBcast, orders, firstInds, cutoff )
El.SOCBroadcast( zDetsBcast, orders, firstInds, cutoff )
sNumNonPos = El.NumNonSOC( s, orders, firstInds, cutoff )
zNumNonPos = El.NumNonSOC( z, orders, firstInds, cutoff )
if output:
  El.Print( sDets, "det(s)" )
  El.Print( zDets, "det(z)" )
  El.Print( sDetsBcast, "Broadcasted det(s)" )
  El.Print( zDetsBcast, "Broadcasted det(z)" )
  if worldRank == 0:
    print "# non-SOC in s:", sNumNonPos
    print "# non-SOC in z:", zNumNonPos

# Compute the square-roots of s and z
# ===================================
sRoot = El.SOCSquareRoot( s, orders, firstInds, cutoff )
zRoot = El.SOCSquareRoot( z, orders, firstInds, cutoff )
sRootSquared = El.SOCApply( sRoot, sRoot, orders, firstInds, cutoff )
zRootSquared = El.SOCApply( zRoot, zRoot, orders, firstInds, cutoff )
if output:
  El.Print( sRoot, "sqrt(s)" )
  El.Print( zRoot, "sqrt(z)" )
  El.Print( sRootSquared, "(sqrt(s))^2" )
  El.Print( zRootSquared, "(sqrt(z))^2" )

# Compute the inverses of s and z
# ===============================
sInv = El.SOCInverse( s, orders, firstInds, cutoff )
zInv = El.SOCInverse( z, orders, firstInds, cutoff )
sInv_s = El.SOCApply( sInv, s, orders, firstInds, cutoff )
zInv_z = El.SOCApply( zInv, z, orders, firstInds, cutoff )
s_sInv = El.SOCApply( s, sInv, orders, firstInds, cutoff )
z_zInv = El.SOCApply( z, zInv, orders, firstInds, cutoff )
if output:
  El.Print( sInv, "inv(s)" )
  El.Print( zInv, "inv(z)" )
  El.Print( sInv_s, "s o inv(s)" )
  El.Print( zInv_z, "z o inv(z)" )
  El.Print( s_sInv, "inv(s) o s" )
  El.Print( z_zInv, "inv(z) o z" )

# Compute the Nesterov-Todd scaling point of (s,z)
# ================================================
w = El.SOCNesterovTodd( s, z, orders, firstInds, cutoff )
wRoot = El.SOCSquareRoot( w, orders, firstInds, cutoff )
wRootInv = El.SOCInverse( wRoot, orders, firstInds, cutoff )
sNT = El.SOCApplyQuadratic( wRoot, s, orders, firstInds, cutoff )
zNT = El.SOCApplyQuadratic( wRootInv, z, orders, firstInds, cutoff )
if output:
  El.Print( w, "w" )
  El.Print( sNT, "s_NT" )
  El.Print( zNT, "z_NT" )

# Compute the minimum non-negative step length, alpha, such that s + alpha z
# touches the boundary of the product cone
upperBound = 100.
alpha = El.MaxStepInSOC( s, z, orders, firstInds, upperBound, cutoff )
p = El.DistMultiVec()
El.Copy( s, p )
El.Axpy( alpha, z, p )
pDets = El.SOCDets( p, orders, firstInds, cutoff )
if output:
  if worldRank == 0: 
    print "maximum step in cone is:", alpha
  El.Print( p, "s + alpha z" )
  El.Print( pDets, "det(s + alpha z)" )

# Require the user to press a button before the figures are closed
El.Finalize()
if worldSize == 1:
  raw_input('Press Enter to exit')