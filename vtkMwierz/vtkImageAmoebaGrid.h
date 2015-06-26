/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkImageAmoebaGrid.h,v $
  Language:  C++
  Date:      $Date: 2007/05/04 14:34:34 $
  Version:   $Revision: 1.1 $

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkImageAmoebaGrid - This deforms an input grid by maximizing the
//          normalized cross-correlation of two images.
// .SECTION Description
// vtkImageAmoebaGrid downsamples according to outputextent.
// KernelRadius is specified in "input extent" units.
// If a third input is specified (0, 1, 2), It is taken as the 'input grid'
// and used to start the optimization of each vector.
// This input grid also overrides any Output

#ifndef __vtkImageAmoebaGrid_h
#define __vtkImageAmoebaGrid_h


#include "vtkImageAlgorithm.h"
#include "vtkFunctionMinimizer.h"
#include "vtkImageStencilData.h"

#include <vtkVersion.h> //for VTK_MAJOR_VERSION




class VTK_EXPORT vtkImageAmoebaGrid : public vtkImageAlgorithm
{
public:
  static vtkImageAmoebaGrid *New();
  vtkTypeMacro(vtkImageAmoebaGrid,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // Set/Get the output extent
  vtkSetVector6Macro(OutputExtent, int);
  vtkGetVector6Macro(OutputExtent, int);


  // Description:
  // Set/Get the kernel size
  vtkSetVector3Macro(KernelRadius, float);
  vtkGetVector3Macro(KernelRadius, float);

  // Read the shrink factors computed from the ratio of the
  // input and output extents
  vtkGetVector3Macro(ShrinkFactors, float);

  // Description:
  // Set/Get the maximum vector displacement size
  vtkSetVector3Macro(VectorBounds, float);
  vtkGetVector3Macro(VectorBounds, float);

  // Description:
  // Set/Get the amoeba tolerance (default = 0.005)
  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);

  float GetMeanVectorLength();
  float GetMeanCost();
  int GetVectorsMinimized();
  
  // Description:
  // Specify the stencil to use.  The stencil can be created
  // from a vtkImplicitFunction or a vtkPolyData.
#if (VTK_MAJOR_VERSION <= 5)
  virtual void SetStencil(vtkImageStencilData *stencil);
#else
  virtual void SetStencilData(vtkImageStencilData *stencil);
#endif
  vtkImageStencilData *GetStencil();

  // Description:
  // Reverse the stencil.
  vtkSetMacro(ReverseStencil, int);
  vtkBooleanMacro(ReverseStencil, int);
  vtkGetMacro(ReverseStencil, int);
  

protected:

  vtkImageAmoebaGrid();
  ~vtkImageAmoebaGrid();
  vtkImageAmoebaGrid(const vtkImageAmoebaGrid&) {};
  void operator=(const vtkImageAmoebaGrid&) {};
  
  float ShrinkFactors[3];
  float KernelRadius[3];
  int OutputExtent[6];
  float VectorBounds[3];
  int ReverseStencil;
  double Tolerance; // tolerance value for the amoeba
    
  // arrays of dimension [LastThreadCount] used to compute statistics on each
  // warp iteration.
  int LastThreadCount; // since the number of threads could change...
  double *VectorLength;
  double *TotalCost;
  int *VectorsMinimized;


  void ExecuteInformation(vtkImageData **inDatas, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageAlgorithm::ExecuteInformation();};
  void ComputeInputUpdateExtents(vtkDataObject *output);
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6], 
        int vtkNotUsed(whichInput));
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
           int extent[6], int id);

};

#endif



