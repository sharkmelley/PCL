// ****************************************************************************
// PixInsight Class Library - PCL 02.00.13.0689
// ****************************************************************************
// pcl/LanczosInterpolation.cpp - Released 2014/10/29 07:34:20 UTC
// ****************************************************************************
// This file is part of the PixInsight Class Library (PCL).
// PCL is a multiplatform C++ framework for development of PixInsight modules.
//
// Copyright (c) 2003-2014, Pleiades Astrophoto S.L. All Rights Reserved.
//
// Redistribution and use in both source and binary forms, with or without
// modification, is permitted provided that the following conditions are met:
//
// 1. All redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
// 2. All redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// 3. Neither the names "PixInsight" and "Pleiades Astrophoto", nor the names
//    of their contributors, may be used to endorse or promote products derived
//    from this software without specific prior written permission. For written
//    permission, please contact info@pixinsight.com.
//
// 4. All products derived from this software, in any form whatsoever, must
//    reproduce the following acknowledgment in the end-user documentation
//    and/or other materials provided with the product:
//
//    "This product is based on software from the PixInsight project, developed
//    by Pleiades Astrophoto and its contributors (http://pixinsight.com/)."
//
//    Alternatively, if that is where third-party acknowledgments normally
//    appear, this acknowledgment must be reproduced in the product itself.
//
// THIS SOFTWARE IS PROVIDED BY PLEIADES ASTROPHOTO AND ITS CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL PLEIADES ASTROPHOTO OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, BUSINESS
// INTERRUPTION; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; AND LOSS OF USE,
// DATA OR PROFITS) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
// ****************************************************************************

#include <pcl/AutoLock.h>
#include <pcl/LanczosInterpolation.h>

namespace pcl
{

// ----------------------------------------------------------------------------

PCL_DATA float* __Lanczos3_LUT = 0;
PCL_DATA float* __Lanczos4_LUT = 0;
PCL_DATA float* __Lanczos5_LUT = 0;

// ----------------------------------------------------------------------------

/*
 * Sinc function for x >= 0
 */
static double Sinc( double x )
{
   x *= Const<double>::pi();
   return (x > 1.0e-07) ? Sin( x )/x : 1.0;
}

/*
 * Evaluate Lanczos function of nth order at 0 <= x < n
 */
static double Lanczos( double x, int n )
{
   return Sinc( x ) * Sinc( x/n );
}

// ----------------------------------------------------------------------------

// Thread-safe routine
void PCL_FUNC __InitializeLanczosLUT( float*& LUT, int n )
{
   static Mutex mutex;
   volatile AutoLock lock( mutex );

   if ( LUT == 0 )
   {
      LUT = new float[ n*__PCL_LANCZOS_LUT_RESOLUTION + 1 ];
      for ( int i = 0, k = 0; i < n; ++i )
         for ( int j = 0; j < __PCL_LANCZOS_LUT_RESOLUTION; ++j, ++k )
            LUT[k] = Lanczos( i + double( j )/__PCL_LANCZOS_LUT_RESOLUTION, n );
      LUT[n*__PCL_LANCZOS_LUT_RESOLUTION] = 0;
   }
}

// ----------------------------------------------------------------------------

} // pcl

// ****************************************************************************
// EOF pcl/LanczosInterpolation.cpp - Released 2014/10/29 07:34:20 UTC