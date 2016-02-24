/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkImageSMIPVIManipulator.h,v $
  Language:  C++
  Date:      $Date: 2007/05/04 14:34:35 $
  Version:   $Revision: 1.1 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageSMIPVIManipulator - Returns the normalized mutual information
// of 2 images.
// .SECTION Description
// vtkImageSMIPVIManipulator calculates the normalized mutual information of 2 images.
// Both images can be cropped to the same extent; image 1 can be translated
// with respect to image 2 using trilinear interpolation.  This filter assumes
// that both inputs are of the same size, and that you want the SMIPVI to be 0,
// if image 1 is translated outside of image 2.

#ifndef __vtkImageSMIPVIManipulator_h
#define __vtkImageSMIPVIManipulator_h

#include "vtkRobartsRegistrationModule.h"

#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkImageShiftScale.h"

class VTKROBARTSREGISTRATION_EXPORT vtkImageSMIPVIManipulator : public vtkObject
{
public:
  static vtkImageSMIPVIManipulator *New();
  vtkTypeMacro(vtkImageSMIPVIManipulator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/get the 2 input images.
  virtual void SetInput1(vtkImageData *input);
  virtual void SetInput2(vtkImageData *input);
#if (VTK_MAJOR_VERSION < 6)
  virtual void UpdateScaled();
#endif
  vtkImageData *GetInput1();
  vtkImageData *GetInput2();

  // Description:
  // Select NMI (Metric = 0), MI (Metric = 1), or ECR (Metric = 2)
  vtkSetMacro(Metric, int);
  vtkGetMacro(Metric, int);
  int Metric;

  // Description:
  // Numbers of bins to use, and number of intensities per bin.
  // I assume images have intensities >= 0.
  // will create bins of width
  virtual void SetBinNumber(int numS, int numT);
  int BinNumber[2];
  virtual void SetMaxIntensities(int maxS, int maxT);
  int MaxIntensities[2];
  double BinWidth[2];

  // Description:
  // Set/get the extent to calculate the SMIPVI over.
  virtual void SetExtent(int ext[6]);
  int Extent[6];

  // Description:
  // Set the translation of input 1 in 3D (mm)
  virtual void SetTranslation(double Translation[3]);

  // Description:
  // Get the absolute difference
  double GetResult();
  double Result;

  // Source, target, and join histograms
  double *HistS;
  double *HistT;
  double *HistST;

  // Entropy of the target image, can be calculated less often.
  double entropyT;

  // Trilinear coefficients updated on SetTranslation.  Keep these
  // global and public to speed up execution (If they were in
  // protected would have to pass them using TemplateMacroXX where
  // XX is big)
  double F000;
  double F100;
  double F010;
  double F110;
  double F001;
  double F101;
  double F011;
  double F111;

protected:
  vtkImageSMIPVIManipulator();
  ~vtkImageSMIPVIManipulator();

  // Globals used to speed up repeated execution:

  // Increments to go through the data (calculate on SetExtent)
  vtkIdType inc[3];
  int inc2[2];

  // Information about inputs (calculate on SetInput1)
  int inExt[6];
  double inSpa[3];

  // Information on translation of image 1 (calculate on SetTranslation)
  int loc000[3];
  int loc111[3];

  // Number of voxels in Extent
  double count;

  // Input data
  vtkImageData *inData[2];
  vtkImageData *inDataScl[2];
  void *inPtr[2];

};

#endif













