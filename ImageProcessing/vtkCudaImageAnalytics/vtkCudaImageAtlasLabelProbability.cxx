/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkCudaImageAtlasLabelProbability.cxx

Copyright (c) John SH Baxter, Robarts Research Institute

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/** @file vtkCudaImageAtlasLabelProbability.cxx
*
*  @brief Implementation file with definitions for the CUDA accelerated label agreement data term. This
*      generates entropy or probability data terms based on how many label maps agree with a particular
*      labelling of each voxel.
*
*  @author John Stuart Haberl Baxter (Dr. Peters' Lab (VASST) at Robarts Research Institute)
*
*  @note August 27th 2013 - Documentation first compiled.
*
*/

#include "CUDA_atlasprobability.h"
#include "vtkCudaImageAtlasLabelProbability.h"
#include "vtkDataObject.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include <limits.h>
#include <math.h>

vtkStandardNewMacro(vtkCudaImageAtlasLabelProbability);

//----------------------------------------------------------------------------
vtkCudaImageAtlasLabelProbability::vtkCudaImageAtlasLabelProbability()
{
  this->NormalizeDataTerm = 0;
  this->LabelID = 1.0;
  this->Entropy = false;
  this->GaussianBlurOn = false;
  this->SetNumberOfInputPorts(1);
  this->MaxValueToGive = 100.0;
  this->GaussianDevs[0] = this->GaussianDevs[1] = this->GaussianDevs[2] = 1.0f;
}

vtkCudaImageAtlasLabelProbability::~vtkCudaImageAtlasLabelProbability()
{

}

//----------------------------------------------------------------------------
void vtkCudaImageAtlasLabelProbability::SetNormalizeDataTermOn()
{
  this->NormalizeDataTerm = 1;
}

//----------------------------------------------------------------------------
void vtkCudaImageAtlasLabelProbability::SetNormalizeDataTermOff()
{
  this->NormalizeDataTerm = 0;
}

//----------------------------------------------------------------------------
int vtkCudaImageAtlasLabelProbability::GetNormalizeDataTerm()
{
  return (this->NormalizeDataTerm);
}

//----------------------------------------------------------------------------
void vtkCudaImageAtlasLabelProbability::SetOutputToEntropy()
{
  this->SetEntropy(true);
}

//----------------------------------------------------------------------------
void vtkCudaImageAtlasLabelProbability::SetOutputToProbability()
{
  this->SetEntropy(false);
}

//----------------------------------------------------------------------------
void vtkCudaImageAtlasLabelProbability::SetStDevX(double val)
{
  this->GaussianDevs[0] = (val > 0.0f ? val: 0.0f);
}

void vtkCudaImageAtlasLabelProbability::SetStDevY(double val)
{
  this->GaussianDevs[1] = (val > 0.0f ? val: 0.0f);
}

void vtkCudaImageAtlasLabelProbability::SetStDevZ(double val)
{
  this->GaussianDevs[2] = (val > 0.0f ? val: 0.0f);
}

double vtkCudaImageAtlasLabelProbability::GetStDevX()
{
  return this->GaussianDevs[0];
}

double vtkCudaImageAtlasLabelProbability::GetStDevY()
{
  return this->GaussianDevs[1];
}

double vtkCudaImageAtlasLabelProbability::GetStDevZ()
{
  return this->GaussianDevs[2];
}

//----------------------------------------------------------------------------
// The output extent is the intersection.
int vtkCudaImageAtlasLabelProbability::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);

  int numLabelMaps = 0;
  for(int i = 0; i < inputVector[0]->GetNumberOfInformationObjects(); i++)
  {
    numLabelMaps++;
  }

  int ext[6], ext2[6], idx;

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),ext);

  for(int i = 0; i < inputVector[0]->GetNumberOfInformationObjects(); i++)
  {
    vtkInformation *inInfo2 = inputVector[0]->GetInformationObject(i);
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),ext2);
    for (idx = 0; idx < 3; ++idx)
    {
      if (ext2[idx*2] > ext[idx*2])
      {
        ext[idx*2] = ext2[idx*2];
      }
      if (ext2[idx*2+1] < ext[idx*2+1])
      {
        ext[idx*2+1] = ext2[idx*2+1];
      }
    }
  }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),ext,6);

  return 1;
}

//----------------------------------------------------------------------------
// This method is passed a input and output datas, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the datas data types.
int vtkCudaImageAtlasLabelProbability::RequestData(
  vtkInformation * vtkNotUsed( request ),
  vtkInformationVector ** inputVector,
  vtkInformationVector * outputVector )
{

  vtkImageData** inData = new vtkImageData*[inputVector[0]->GetNumberOfInformationObjects()];

  int numLabelMaps = 0;
  for(int i = 0; i < inputVector[0]->GetNumberOfInformationObjects(); i++)
  {
    inData[i] = vtkImageData::SafeDownCast(inputVector[0]->GetInformationObject(i)->Get(vtkDataObject::DATA_OBJECT()));
    if(inData[i])
    {
      numLabelMaps++;
    }
  }

  if (numLabelMaps == 0)
  {
    vtkErrorMacro( "At least one label map is required." );
    return -1;
  }

  vtkImageData* outData = vtkImageData::SafeDownCast(outputVector->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));
  outData->SetExtent(inData[0]->GetExtent());
  outData->AllocateScalars(VTK_FLOAT, 1);

  // this filter expects the output datatype to be float.
  if (outData->GetScalarType() != VTK_FLOAT)
  {
    vtkErrorMacro( "Output data type must be float." );
    return -1;
  }

  // this filter expects the label map to be of type char
  int LabelType = -1;
  for(int i = 0; i < inputVector[0]->GetNumberOfInformationObjects(); i++)
  {
    if( !inData[i] )
    {
      continue;
    }
    if( LabelType == -1 )
    {
      LabelType = inData[i]->GetScalarType();
    }
    if (inData[i]->GetScalarType() != LabelType)
    {
      vtkErrorMacro( "Label maps must be of same type." );
      return -1;
    }
    if ( inData[i]->GetNumberOfScalarComponents() != 1 )
    {
      vtkErrorMacro( "Label map can only have 1 component." );
      return -1;

    }
  }

  //get the volume size
  int VolumeSize = outData->GetDimensions()[0] * outData->GetDimensions()[1] *  outData->GetDimensions()[2];

  //allocate some working buffers
  short* agreementGPU;
  float* outputGPU;
  CUDA_GetRelevantBuffers(&agreementGPU,&outputGPU,VolumeSize,this->GetStream());

  //figure out the agreement
  short Number = 0;
  for(int i = 0; i < inputVector[0]->GetNumberOfInformationObjects(); i++)
  {
    if( !inData[i] )
    {
      continue;
    }
    Number++;
    this->ReserveGPU();
    switch (inData[i]->GetScalarType())
    {
      vtkTemplateMacro(
        CUDA_IncrementInformation((VTK_TT *) inData[i]->GetScalarPointer(), (VTK_TT) LabelID, agreementGPU, VolumeSize,this->GetStream()));
    default:
      vtkErrorMacro( "Execute: Unknown ScalarType");
      return -1;
    }
  }

  //finish the results
  short flags = this->Entropy ? 1 : 0;
  flags += this->NormalizeDataTerm ? 2 : 0;
  flags += this->GaussianBlurOn ? 4 : 0;
  int GaussWidth[3] = {this->GaussianDevs[0]*this->GaussianDevs[0] * 4.0, this->GaussianDevs[1]*this->GaussianDevs[1] * 4.0, this->GaussianDevs[2]*this->GaussianDevs[2] * 4.0 };
  this->ReserveGPU();
  CUDA_ConvertInformation(agreementGPU, outputGPU, this->MaxValueToGive, VolumeSize, Number, flags, GaussWidth, outData->GetDimensions(), this->GetStream() );
  CUDA_CopyBackResult(outputGPU, (float*) outData->GetScalarPointer(), VolumeSize, this->GetStream() );

  //deallocate memory
  delete[] inData;

  return 1;
}

//----------------------------------------------------------------------------
int vtkCudaImageAtlasLabelProbability::FillInputPortInformation(
  int port, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(),1);
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkCudaImageAtlasLabelProbability::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}

//----------------------------------------------------------------------------
void vtkCudaImageAtlasLabelProbability::SetInputLabelMapConnection(vtkAlgorithmOutput *in, int number)
{
  if(number >= 0)
  {
    this->SetNthInputConnection(0,number,in);
  }
}
