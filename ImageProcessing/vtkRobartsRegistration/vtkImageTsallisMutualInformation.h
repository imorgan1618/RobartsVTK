/*=========================================================================

Program:   Visualization Toolkit
Module:    $RCSfile: vtkImageTsallisMutualInformation.h,v $
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
// .NAME vtkImageTsallisMutualInformation - Returns the normalized mutual
// information of 2 images
// .SECTION Description
// vtkImageTsallisMutualInformation calculates the normalized mutual
// information of 2 images

#ifndef __vtkImageTsallisMutualInformation_h
#define __vtkImageTsallisMutualInformation_h

#include "vtkRobartsRegistrationModule.h"

#include "vtkThreadedImageAlgorithm.h"
#include "vtkObjectFactory.h"
#include "vtkImageStencilData.h"
#include "vtkImageData.h"
#include "vtkMultiThreader.h"
#include "math.h"
#include <vtkVersion.h> //for VTK_MAJOR_VERSION

// Constants used for array declaration.
#define MAX_THREADS 24
#define MAX_BINS_S  4096
#define MAX_BINS_T  4096

class VTKROBARTSREGISTRATION_EXPORT vtkImageTsallisMutualInformation : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageTsallisMutualInformation *New();
#if (VTK_MAJOR_VERSION < 6)
  vtkTypeRevisionMacro(vtkImageTsallisMutualInformation, vtkImageTwoInputFilter);
#else
  vtkTypeMacro(vtkImageTsallisMutualInformation, vtkThreadedImageAlgorithm);
#endif
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/get the 2 input images and stencil to specify which voxels to accumulate.
#if (VTK_MAJOR_VERSION < 6)
  virtual void SetInput1(vtkImageData *input);
  virtual void SetInput2(vtkImageData *input);
  void SetStencil(vtkImageStencilData *stencil);
#else
  virtual void SetInput1Data(vtkImageData *input);
  virtual void SetInput2Data(vtkImageData *input);
  void SetStencilData(vtkImageStencilData *stencil);
#endif

  vtkImageData *GetInput1();
  vtkImageData *GetInput2();
  vtkImageStencilData *GetStencil();

  // Description:
  // Overide GetOutput to allow summing of histogram over threads.
  vtkImageData *GetOutput();

  // Description:
  // Reverse the stencil.
  vtkSetMacro(ReverseStencil, int);
  vtkBooleanMacro(ReverseStencil, int);
  vtkGetMacro(ReverseStencil, int);

  // Description:
  // Numbers of bins to use, and number of intensities per bin.
  // I assume images are 0 to 4095, therefore combinations like
  // BinNumber = 256, BinWidth = 16 are OK.
  virtual void SetBinNumber(int numS, int numT);
  int BinNumber[2];
  virtual void SetMaxIntensities(int maxS, int maxT);
  int MaxIntensities[2];
  double BinWidth[2];
  vtkSetMacro(qValue, double);
  double qValue;

  // Description:
  // This is kept public instead of protected since it is called
  // from a non-member thread function.
  // ThreadedExecute1 creates histograms for each thread.
  // ThreadedExecute2 combines histograms into entropies for each thread.
  void ThreadedExecute1(vtkImageData **inDatas, vtkImageData *outData, int extent[6], int id);
  void ThreadedExecute2(int extentS[6], int extentST[6], int id);

  // Description:
  // Combine the threaded entropies into final entropies and return
  // normalized mutual information.
  double GetResult();

  // Description:
  // Source, target, and join histograms divided into threads.
  // Maximum of 2 processors can be handled.
  long int ThreadHistS[MAX_THREADS][MAX_BINS_S];
  long int ThreadHistT[MAX_THREADS][MAX_BINS_T];
  long int ThreadHistST[MAX_THREADS][MAX_BINS_S][MAX_BINS_T];

  // Description:
  // Entropies and voxel count divided into threads.
  double ThreadEntropyS1[MAX_THREADS];
  double ThreadEntropyT1[MAX_THREADS];
  double ThreadEntropyST1[MAX_THREADS];
  double ThreadEntropyS2[MAX_THREADS];
  double ThreadEntropyT2[MAX_THREADS];
  double ThreadEntropyST2[MAX_THREADS];
  double ThreadCount[MAX_THREADS];

protected:
  vtkImageTsallisMutualInformation();
  ~vtkImageTsallisMutualInformation();

  vtkMultiThreader *Threader;
  int NumberOfThreads;

  int ReverseStencil;

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6],int vtkNotUsed(whichInput));
  void ExecuteData(vtkDataObject *output);
  void ExecuteInformation(vtkImageData **inDatas, vtkImageData *outData);
#if (VTK_MAJOR_VERSION < 6)
  void ExecuteInformation()
  {
    this->vtkImageTwoInputFilter::ExecuteInformation();
  };
#endif

private:
  vtkImageTsallisMutualInformation(const vtkImageTsallisMutualInformation&);  // Not implemented.
  void operator=(const vtkImageTsallisMutualInformation&);  // Not implemented.
};

#endif