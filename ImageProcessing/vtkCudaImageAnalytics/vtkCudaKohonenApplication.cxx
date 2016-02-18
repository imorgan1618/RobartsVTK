#include "vtkCudaKohonenApplication.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"

#include <vtkVersion.h> // For VTK_MAJOR_VERSION

vtkStandardNewMacro(vtkCudaKohonenApplication);

vtkCudaKohonenApplication::vtkCudaKohonenApplication()
{
  //configure the input ports
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfInputConnections(0,1);
  this->SetNumberOfInputConnections(1,1);

  //initialize the scale to 1
  this->Scale = 1.0;
}

vtkCudaKohonenApplication::~vtkCudaKohonenApplication()
{
}

//------------------------------------------------------------
//Commands for CudaObject compatibility

void vtkCudaKohonenApplication::Reinitialize(int withData)
{
  //TODO
}

void vtkCudaKohonenApplication::Deinitialize(int withData)
{
}


//------------------------------------------------------------
int vtkCudaKohonenApplication::FillInputPortInformation(int i, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 0);
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
  return this->Superclass::FillInputPortInformation(i,info);
}
//----------------------------------------------------------------------------

void vtkCudaKohonenApplication::SetScale(double s)
{
  if( this->Scale != s && s >= 0.0 )
  {
    this->Scale = s;
    this->Modified();
  }
}
#if (VTK_MAJOR_VERSION < 6)
void vtkCudaKohonenApplication::SetDataInput(vtkImageData* d)
{
  this->SetInput(0,d);
}

void vtkCudaKohonenApplication::SetMapInput(vtkImageData* d)
{
  this->SetInput(1,d);
}
#else
void vtkCudaKohonenApplication::SetDataInputData(vtkImageData* d)
{
  this->SetInputData(0,d);
}

void vtkCudaKohonenApplication::SetMapInputData(vtkImageData* d)
{
  this->SetInputData(1,d);
}
void vtkCudaKohonenApplication::SetDataInputConnection(vtkAlgorithmOutput* d)
{
  this->vtkImageAlgorithm::SetInputConnection(0,d);
}

void vtkCudaKohonenApplication::SetMapInputConnection(vtkAlgorithmOutput* d)
{
  this->vtkImageAlgorithm::SetInputConnection(1,d);
}
#endif
vtkImageData* vtkCudaKohonenApplication::GetDataInput()
{
  return (vtkImageData*) this->GetInput(0);
}

vtkImageData* vtkCudaKohonenApplication::GetMapInput()
{
  return (vtkImageData*) this->GetInput(1);
}
//------------------------------------------------------------
int vtkCudaKohonenApplication::RequestInformation(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);
  vtkImageData* outData = vtkImageData::SafeDownCast(outputInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataObject::SetPointDataActiveScalarInfo(outputInfo, VTK_FLOAT, 2);
  return 1;
}

int vtkCudaKohonenApplication::RequestUpdateExtent(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* kohonenInfo = (inputVector[1])->GetInformationObject(0);
  vtkInformation* inputInfo = (inputVector[0])->GetInformationObject(0);
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);
  vtkImageData* kohonenData = vtkImageData::SafeDownCast(kohonenInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* inData = vtkImageData::SafeDownCast(inputInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* outData = vtkImageData::SafeDownCast(outputInfo->Get(vtkDataObject::DATA_OBJECT()));

  kohonenInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),kohonenData->GetExtent(),6);
  inputInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),inData->GetExtent(),6);

  return 1;
}

int vtkCudaKohonenApplication::RequestData(vtkInformation *request,
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector)
{

  vtkInformation* kohonenInfo = (inputVector[1])->GetInformationObject(0);
  vtkInformation* inputInfo = (inputVector[0])->GetInformationObject(0);
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);
  vtkImageData* kohonenData = vtkImageData::SafeDownCast(kohonenInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* inData = vtkImageData::SafeDownCast(inputInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* outData = vtkImageData::SafeDownCast(outputInfo->Get(vtkDataObject::DATA_OBJECT()));

  outData->ShallowCopy(inData);
#if (VTK_MAJOR_VERSION < 6)
  outData->SetScalarTypeToFloat();
  outData->SetNumberOfScalarComponents(2);
  outData->SetExtent(inData->GetExtent());
  outData->SetWholeExtent(inData->GetExtent());
  outData->AllocateScalars();
#else
  outData->SetExtent(inData->GetExtent());
  outData->AllocateScalars(VTK_FLOAT, 2);
#endif

  //update information container
  this->info.NumberOfDimensions = inData->GetNumberOfScalarComponents();
  inData->GetDimensions( this->info.VolumeSize );
  kohonenData->GetDimensions( this->info.KohonenMapSize );

  //update scale
  this->info.Scale = this->Scale;

  //pass it over to the GPU
  this->ReserveGPU();
  CUDAalgo_applyKohonenMap( (float*) inData->GetScalarPointer(),
                            (float*) kohonenData->GetScalarPointer(),
                            (float*) outData->GetScalarPointer(), this->info, this->GetStream() );

  return 1;
}