//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 02.01.01.0784
// ----------------------------------------------------------------------------
// Standard ArcsinhStretch Process Module Version 00.00.01.0104
// ----------------------------------------------------------------------------
// ArcsinhStretchInstance.cpp 
// ----------------------------------------------------------------------------
// This file is part of the standard ArcsinhStretch PixInsight module.
//
// Copyright (c) 2003-2017 Pleiades Astrophoto S.L. All Rights Reserved.
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
// ----------------------------------------------------------------------------

#include "ArcsinhStretchInstance.h"
#include "ArcsinhStretchParameters.h"

#include <pcl/AutoViewLock.h>
#include <pcl/Console.h>
#include <pcl/StdStatus.h>
#include <pcl/View.h>

namespace pcl
{

// ----------------------------------------------------------------------------

ArcsinhStretchInstance::ArcsinhStretchInstance( const MetaProcess* m ) :
   ProcessImplementation( m ),
   p_stretch(TheArcsinhStretchParameter->DefaultValue()),
   p_blackpoint(TheArcsinhStretchBlackPointParameter->DefaultValue()),
   p_protectHighlights(TheArcsinhStretchProtectHighlightsParameter->DefaultValue()),
   p_useRgbws( TheArcsinhStretchUseRgbwsParameter->DefaultValue() ),
   p_previewClipped(TheArcsinhStretchPreviewClippedParameter->DefaultValue()),
   p_coarse(0.0),
   p_fine(0.0)
{
}

ArcsinhStretchInstance::ArcsinhStretchInstance( const ArcsinhStretchInstance& x ) :
   ProcessImplementation( x )
{
   Assign( x );
}

void ArcsinhStretchInstance::Assign( const ProcessImplementation& p )
{
   const ArcsinhStretchInstance* x = dynamic_cast<const ArcsinhStretchInstance*>( &p );
   if ( x != nullptr )
   {
	  p_stretch = x->p_stretch;
	  p_blackpoint = x->p_blackpoint;
      p_protectHighlights = x->p_protectHighlights;
	  p_useRgbws = x->p_useRgbws;
      p_previewClipped  = x->p_previewClipped;
   }
}

bool ArcsinhStretchInstance::CanExecuteOn( const View& view, String& whyNot ) const
{
   if ( view.Image().IsComplexSample() )
   {
      whyNot = "ArcsinhStretch cannot be executed on images with complex numbers.";
      return false;
   }

   whyNot.Clear();
   return true;
}

class ArcsinhStretchEngine
{
public:

   template <class P>
   static void Apply(GenericImage<P>& image, const ArcsinhStretchInstance& instance, bool preview_mode)
   {

	   size_type numChannels = image.NumberOfNominalChannels();
	   int xDim = image.Width();
	   int yDim = image.Height();

	   double MaxRepresentableValue = 1.0;  //MaxRepresentableValue is used to scale image data into range [0.0,1.0] whatever numerical representation is used
	   if (!image.IsFloatSample())
	   {
		   switch (image.BitsPerSample())
		   {
				case  8: MaxRepresentableValue = 256.0 - 1.0; break;
				case 16: MaxRepresentableValue = 65536.0 - 1.0; break;
				case 32: MaxRepresentableValue = 65536.0*65536.0 - 1.0; break;
		   }
	   }

	   double asinh_parm = 0.0;  //ArcsinhStretch parameter
	   if (instance.p_stretch != 1.0)
	   {
		   //Binary search for parameter "a" that gives the right stretch
		   double low = 0;
		   double high = 10000;  
		   //double multiplier_low = 1;
		   //double multiplier_high = high / ArcSinh(high);
		   double mid;
		   for (int i = 0; i < 20; i++)
		   {
			   mid = (low + high) / 2.0;
			   double multiplier_mid = mid / ArcSinh(mid);
			   if (instance.p_stretch <= multiplier_mid)
				   high = mid;
			   else
				   low = mid;
		   }
		   asinh_parm = mid;
	   }

	   double maxStretchedVal = 0;
	   double maxActual = image.MaximumSampleValue();
	   double denominator = ArcSinh(asinh_parm);


	   //Set for RGB coefficients for calculating luminance and normalise them
	   FVector lumCoeffs((float)(1.0) / (float)(3.0), 3);  //Default values are equally weighted 1/3 each
	   if (instance.p_useRgbws)
	   {
           RGBColorSystem rgbws = image.RGBWorkingSpace();
	       lumCoeffs = rgbws.LuminanceCoefficients();
       }
	   //Normalise luminance coeffs
	   double sumLumCoeffs = 0;
	   for (int ich = 0; ich < numChannels; ich++)
		   sumLumCoeffs += lumCoeffs[ich];
	   for (int ich = 0; ich < numChannels; ich++)
		   lumCoeffs[ich]/= sumLumCoeffs;

	   //MDSTODO
	   Console().WriteLn("Normalised luminance coeffs: " + String(lumCoeffs[0]) + "  "+String(lumCoeffs[1]) + "  " + String(lumCoeffs[2])  );

	   //If scaling to protect highlights is switched on then pre-process the data to calculate the required scaling factor
	   //For speed use a sub-sampled image to estimate the maximum post-stretch image value
	   //The maximum post-stretch image value is then used in the second iteration as a multiplier to prevent highlight clipping
	   double multiplier = 1.0;
	   if (instance.p_protectHighlights)
	   {
		   for (int ix = 0; ix < xDim; ix += 2)  // Subsample by 2
		   {
			   for (int iy = 0; iy < yDim; iy += 2)  // Subsample by 2
			   {
				   Point point = Point(ix, iy);
				   double intensity = 0;
				   for (int ich = 0; ich < numChannels; ich++)
				   {
					   intensity += image.Pixel(point, ich)*lumCoeffs[ich];
				   }
				   double rescaledintensity = (intensity / MaxRepresentableValue - instance.p_blackpoint) / (maxActual / MaxRepresentableValue - instance.p_blackpoint);
				   double arcsinhScale = 1.0;
				   if (asinh_parm != 0.0 && rescaledintensity != 0.0) arcsinhScale = ArcSinh(asinh_parm*rescaledintensity) / denominator / rescaledintensity;
				   for (int ich = 0; ich < numChannels; ich++)
				   {
					   double newval = (image.Pixel(point, ich) / MaxRepresentableValue - instance.p_blackpoint) / (maxActual / MaxRepresentableValue - instance.p_blackpoint)*arcsinhScale*MaxRepresentableValue;
					   maxStretchedVal = Max(maxStretchedVal, newval);
				   }
			   }
		   }

		   //MDSTODO
		   Console().WriteLn("MaxStretchedVal: " + String(maxStretchedVal));

		   multiplier = MaxRepresentableValue / maxStretchedVal;
	   }

	   //Now treat the whole image using the multiplier just calculated
	   for (int ix = 0; ix < xDim; ix++)
	   {
		   for (int iy = 0; iy < yDim; iy++)
		   {
			   Point point = Point(ix, iy);
			   double intensity = 0;
			   for (int ich = 0; ich < numChannels; ich++)
			   {
				   intensity += image.Pixel(point, ich)*lumCoeffs[ich];
			   }
			   double rescaledintensity = (intensity / MaxRepresentableValue - instance.p_blackpoint) / (maxActual / MaxRepresentableValue - instance.p_blackpoint);
			   double arcsinhScale = 1.0;
			   if (asinh_parm != 0.0 && rescaledintensity != 0.0) arcsinhScale = ArcSinh(asinh_parm*rescaledintensity) / denominator / rescaledintensity;
			   for (int ich = 0; ich < numChannels; ich++)
			   {
				   double newval = (image.Pixel(point, ich) / MaxRepresentableValue - instance.p_blackpoint) / (maxActual / MaxRepresentableValue - instance.p_blackpoint)*arcsinhScale*MaxRepresentableValue;
				   newval *= multiplier;
				   if (preview_mode)  // Preview will always be 16 bit integer UInt16Image (we hope!)
				   {
					   newval = Min(newval, MaxRepresentableValue - 1);  //In preview the value MaxRepresentableValue is reserved for highlighting values clipped to zero
					   if (instance.p_previewClipped && newval < 0.0) newval = MaxRepresentableValue;
				   }
				   image.Pixel(point, ich) = Min(Max(newval, 0.0), MaxRepresentableValue);
			   }
		   }
	   }

	   image.ResetSelections();
	   
   }
};

void ArcsinhStretchInstance::Preview(UInt16Image& image) const
{
	try
	{
		bool preview_mode = true;
		ArcsinhStretchEngine::Apply(image, *this, preview_mode);

	}
	catch (...)
	{
	}
}
bool ArcsinhStretchInstance::ExecuteOn(View& view)
{
   AutoViewLock lock( view );

   ImageVariant image = view.Image();
   if ( image.IsComplexSample() )
      return false;

   StandardStatus status;
   image.SetStatusCallback( &status );

   Console().EnableAbort();

   bool preview_mode = false;
   if ( image.IsFloatSample() )
      switch ( image.BitsPerSample() )
      {
	  case 32: ArcsinhStretchEngine::Apply(static_cast<Image&>(*image), *this, preview_mode); break;
	  case 64: ArcsinhStretchEngine::Apply(static_cast<DImage&>(*image), *this, preview_mode); break;
      }
   else
      switch ( image.BitsPerSample() )
      {
	  case  8: ArcsinhStretchEngine::Apply(static_cast<UInt8Image&>(*image), *this, preview_mode); break;
	  case 16: ArcsinhStretchEngine::Apply(static_cast<UInt16Image&>(*image), *this, preview_mode); break;
	  case 32: ArcsinhStretchEngine::Apply(static_cast<UInt32Image&>(*image), *this, preview_mode); break;
      }

   return true;
}

void* ArcsinhStretchInstance::LockParameter( const MetaParameter* p, size_type /*tableRow*/ )
{
	if (p == TheArcsinhStretchParameter)
		return &p_stretch;
	if (p == TheArcsinhStretchBlackPointParameter)
		return &p_blackpoint;
	if (p == TheArcsinhStretchProtectHighlightsParameter)
		return &p_protectHighlights;
	if ( p == TheArcsinhStretchUseRgbwsParameter )
      return &p_useRgbws;
   if ( p == TheArcsinhStretchPreviewClippedParameter )
      return &p_previewClipped;
   return nullptr;
}


// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF ArcsinhStretchInstance.cpp 
