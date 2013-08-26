#include "vtkCudaKohonenApplication.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"

vtkStandardNewMacro(vtkCudaKohonenApplication);

vtkCudaKohonenApplication::vtkCudaKohonenApplication(){
	//configure the input ports
	this->SetNumberOfInputPorts(3);
	this->SetNumberOfInputConnections(0,1);
	this->SetNumberOfInputConnections(1,1);
	this->SetNumberOfInputConnections(2,1);

	//initialize the scale to 1
	this->Scale = 1.0;
}

vtkCudaKohonenApplication::~vtkCudaKohonenApplication(){
}

//------------------------------------------------------------
//Commands for vtkCudaObject compatibility

void vtkCudaKohonenApplication::Reinitialize(int withData){
	//TODO
}

void vtkCudaKohonenApplication::Deinitialize(int withData){
}


//----------------------------------------------------------------------------

void vtkCudaKohonenApplication::SetScale(double s){
	if( this->Scale != s && s >= 0.0 ){
		this->Scale = s;
		this->Modified();
	}
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
							vtkInformationVector *outputVector){

	vtkInformation* kohonenInfo = (inputVector[1])->GetInformationObject(0);
	vtkInformation* inputInfo = (inputVector[0])->GetInformationObject(0);
	vtkInformation* maskInfo = (inputVector[2])->GetInformationObject(0);
	vtkInformation* outputInfo = outputVector->GetInformationObject(0);
	vtkImageData* kohonenData = vtkImageData::SafeDownCast(kohonenInfo->Get(vtkDataObject::DATA_OBJECT()));
	vtkImageData* inData = vtkImageData::SafeDownCast(inputInfo->Get(vtkDataObject::DATA_OBJECT()));
	vtkImageData* maskData = vtkImageData::SafeDownCast(maskInfo->Get(vtkDataObject::DATA_OBJECT()));
	vtkImageData* outData = vtkImageData::SafeDownCast(outputInfo->Get(vtkDataObject::DATA_OBJECT()));
	
	//figure out the extent of the output
    int updateExtent[6];
	outData->SetScalarTypeToFloat();
	outData->SetNumberOfScalarComponents(2);
	outData->SetExtent(inData->GetExtent());
	outData->SetWholeExtent(inData->GetExtent());
	outData->AllocateScalars();
	
	//update information container
	this->info.NumberOfDimensions = inData->GetNumberOfScalarComponents();
	inData->GetDimensions( this->info.VolumeSize );
	kohonenData->GetDimensions( this->info.KohonenMapSize );
	
	//update scale
	this->info.Scale = this->Scale;

	//pass it over to the GPU
	this->ReserveGPU();
	CUDAalgo_applyKohonenMap( (float*) inData->GetScalarPointer(), (char*) maskData->GetScalarPointer(), (float*) kohonenData->GetScalarPointer(),
							  (float*) outData->GetScalarPointer(), this->info, this->GetStream() );

	return 1;
}