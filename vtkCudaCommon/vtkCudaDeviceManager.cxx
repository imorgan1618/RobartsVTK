/** @file vtkCudaDeviceManager.cxx
 *
 *  @brief A singleton class to manage cards and stream interleaving
 *
 *  @author John Stuart Haberl Baxter (Dr. Peter's Lab at Robarts Research Institute)
 *  @note First documented on June 12, 2012
 *
 */

#include "vtkCudaDeviceManager.h"
#include "vtkObjectFactory.h"
#include "cuda_runtime_api.h"
#include "vtkCudaObject.h"
#include <set>

vtkCudaDeviceManager* vtkCudaDeviceManager::singletonManager = 0;

vtkCudaDeviceManager* vtkCudaDeviceManager::Singleton(){
	if( !singletonManager )
		singletonManager = new vtkCudaDeviceManager();
	return singletonManager;
}

vtkCudaDeviceManager::vtkCudaDeviceManager(){

	//create the locks
	this->regularLock = vtkMutexLock::New();
	int n = this->GetNumberOfDevices();

}

vtkCudaDeviceManager::~vtkCudaDeviceManager(){

	//define a list to collect the used device ID's in
	std::set<int> devicesInUse;
	this->regularLock->Lock();

	//synchronize and end all streams
	for( std::map<cudaStream_t*,int>::iterator it = this->StreamToDeviceMap.begin();
		 it != this->StreamToDeviceMap.end(); it++ ){
		this->SynchronizeStream( (*it).first );
		cudaStreamDestroy( *(it->first) );
		devicesInUse.insert( it->second );
	}

	//decommission the devices
	for( std::set<int>::iterator it = devicesInUse.begin();
		 it != devicesInUse.end(); it++ ){
		cudaSetDevice( *it );
		cudaDeviceReset( );
	}

	//clean up variables
	this->StreamToDeviceMap.clear();
	this->StreamToObjectMap.clear();
	this->ObjectToDeviceMap.clear();
	this->regularLock->Unlock();
	this->regularLock->Delete();

}

int vtkCudaDeviceManager::GetNumberOfDevices(){
	
	int numberOfDevices = 0;
	cudaError_t result = cudaGetDeviceCount (&numberOfDevices);
	
	if( result != 0 ){
		vtkErrorMacro(<<"Catostrophic CUDA error - cannot count number of devices.");
		return -1;
	}
	return numberOfDevices;

}

bool vtkCudaDeviceManager::GetDevice(vtkCudaObject* caller, int device){

	//check that the device supplied is a valid device
	if( device < 0 || device >= this->GetNumberOfDevices() ){
		vtkErrorMacro(<<"Invalid device identifier.");
		return true;
	}

	//lock critical section
	this->regularLock->Lock();

	//figure out if that pair already exists
	bool needToAdd = true;
	std::pair<std::multimap<vtkCudaObject*,int>::iterator,std::multimap<vtkCudaObject*,int>::iterator> ret =
		this->ObjectToDeviceMap.equal_range(caller);
	for( std::multimap<vtkCudaObject*,int>::iterator it = ret.first; it != ret.second; it++){
		if( it->second == device ){
			needToAdd = false;
			break;
		}
	}

	//if it does not, add it
	if( needToAdd ){
		this->ObjectToDeviceMap.insert( std::pair<vtkCudaObject*,int>(caller, device) );
		this->DeviceToObjectMap.insert( std::pair<int, vtkCudaObject*>(device, caller) );
	}

	//unlock critical section and return
	this->regularLock->Unlock();
	return false;
}

bool vtkCudaDeviceManager::ReturnDevice(vtkCudaObject* caller, int device){
	this->regularLock->Lock();

	//find if there is a valid mapping]
	bool found = false;
	std::multimap<vtkCudaObject*,int>::iterator otdmEraseIt = this->ObjectToDeviceMap.begin();
	std::multimap<int,vtkCudaObject*>::iterator dtomEraseIt = this->DeviceToObjectMap.begin();
	std::pair<std::multimap<vtkCudaObject*,int>::iterator,std::multimap<vtkCudaObject*,int>::iterator> ret =
		this->ObjectToDeviceMap.equal_range(caller);
	for( std::multimap<vtkCudaObject*,int>::iterator it = ret.first; it != ret.second; it++){
		if( it->second == device ){
			found = true;
			otdmEraseIt = it;
			
			//find associated backward mapping
			std::pair<std::multimap<int,vtkCudaObject*>::iterator,std::multimap<int,vtkCudaObject*>::iterator> innerRet =
				this->DeviceToObjectMap.equal_range(device);
			for( std::multimap<int,vtkCudaObject*>::iterator it2 = innerRet.first; it2 != innerRet.second; it2++){
				if( it2->second == caller ){
					dtomEraseIt = it2;
					break;
				}
			}

			break;
		}
	}

	//bool found = false;
	//bool emptyDevice = true;
	//std::multimap<vtkCudaObject*,int>::iterator it = this->ObjectToDeviceMap.begin();
	//std::multimap<vtkCudaObject*,int>::iterator eraseIt = this->ObjectToDeviceMap.begin();
	//for( ; it != this->ObjectToDeviceMap.end(); it++ ){
	//	if( it->first == caller && it->second == device ){
	//		found = true;
	//		eraseIt = it;
	//	}else if(it->second == device){
	//		emptyDevice = false;
	//	}
	//}

	//if said mapping is not found, then this was called in error and nothing changes
	if( !found ){
		vtkErrorMacro(<<"Could not locate supplied caller-device pair.");
		this->regularLock->Unlock();
		return true;
	}

	//if found, remove mappings
	this->DeviceToObjectMap.erase(dtomEraseIt);
	this->ObjectToDeviceMap.erase(otdmEraseIt);

	//if found remove streams associated with this caller on this device
	std::set<cudaStream_t*> streamsToReturn;
	std::multimap<cudaStream_t*,vtkCudaObject*>::iterator it2 = this->StreamToObjectMap.begin();
	for( ; it2 != this->StreamToObjectMap.end(); it2++ ){
		if( this->StreamToDeviceMap[it2->first] == device ){
			cudaStream_t* tempStreamPointer = it2->first;
		}
	}
	for( std::set<cudaStream_t*>::iterator it = streamsToReturn.begin();
		 it != streamsToReturn.end(); it++ )
			this->ReturnStream(caller, *it, device );

	//clear a device if it is no longer used by any object
	if( this->DeviceToObjectMap.count(device) == 0 ){
		int oldDevice = 0;
		cudaGetDevice( &oldDevice );
		if( device != oldDevice ) cudaSetDevice( device );
		cudaDeviceReset();
		if( device != oldDevice ) cudaSetDevice( oldDevice );
	}

	//unlock the critical section
	this->regularLock->Unlock();

	return false;
}

bool vtkCudaDeviceManager::GetStream(vtkCudaObject* caller, cudaStream_t** stream, int device){
	
	//check device identifier for consistency
	if( device < 0 || device >= this->GetNumberOfDevices() ){
		vtkErrorMacro(<<"Invalid device identifier.");
		return true;
	}

	//if stream is provided, check for stream-device consistancy
	this->regularLock->Lock();
	if( *stream && this->StreamToDeviceMap.count(*stream) == 1 && 
		this->StreamToDeviceMap[*stream] != device ){
		this->regularLock->Unlock();
		vtkErrorMacro(<<"Stream already assigned to particular device.");
		return true;
	}

	//find if this call is redundant
	bool found = false;
	std::multimap<cudaStream_t*,vtkCudaObject*>::iterator it = this->StreamToObjectMap.begin();
	for( ; it != this->StreamToObjectMap.end(); it++ )
		if( it->first == *stream && it->second == caller )
			found = true;
	if( found ){
		this->regularLock->Unlock();
		return false;
	}

	//create the new stream and mapping
	cudaSetDevice(device);
	if( *stream == 0 ){
		cudaStream_t* temp = new cudaStream_t();
		cudaStreamCreate( temp );
		*stream = temp;
	}
	this->StreamToDeviceMap.insert( std::pair<cudaStream_t*,int>(*stream,device) );
	this->StreamToObjectMap.insert( std::pair<cudaStream_t*,vtkCudaObject*>(*stream,caller) );

	this->regularLock->Unlock();

	return false;

}

bool vtkCudaDeviceManager::ReturnStream(vtkCudaObject* caller, cudaStream_t* stream, int device){
	this->regularLock->Lock();

	//find if that is a valid three-tuple (stream, object, device)
	bool found = false;
	std::multimap<cudaStream_t*,vtkCudaObject*>::iterator it = this->StreamToObjectMap.begin();
	for( ; it != this->StreamToObjectMap.end(); it++ ){
		if( it->first == stream && it->second == caller &&
			this->StreamToDeviceMap[it->first] == device ){
			found = true;
			break;
		}
	}
	if( !found ){
		vtkErrorMacro(<<"Could not locate supplied caller-device pair.");
		this->regularLock->Unlock();
		return true;
	}

	//remove that part of the mapping
	this->StreamToObjectMap.erase(it);
	if( this->StreamToObjectMap.count(stream) == 0 ){
		this->StreamToDeviceMap.erase( stream );
		this->DestroyEmptyStream( stream );
	}

	this->regularLock->Unlock();
	return false;

}

bool vtkCudaDeviceManager::SynchronizeStream( cudaStream_t* stream ){
	
	//find mapped result and device
	this->regularLock->Lock();
	if( this->StreamToDeviceMap.count(stream) != 1 ){
		vtkErrorMacro(<<"Cannot synchronize unused stream.");
		this->regularLock->Unlock();
		return true;
	}
	int device = this->StreamToDeviceMap[stream];
	this->regularLock->Unlock();

	//synchronize the stream and return the success value
	int oldDevice = 0;
	cudaGetDevice( &oldDevice );
	cudaSetDevice( device );
	cudaStreamSynchronize( *stream );
	cudaSetDevice( oldDevice );

	return cudaGetLastError() != cudaSuccess;

}

bool vtkCudaDeviceManager::ReserveGPU( cudaStream_t* stream ){
	
	//find mapped result and device
	this->regularLock->Lock();
	if( this->StreamToDeviceMap.count(stream) != 1 ){
		vtkErrorMacro(<<"Cannot synchronize unused stream.");
		this->regularLock->Unlock();
		return true;
	}
	int device = this->StreamToDeviceMap[stream];
	this->regularLock->Unlock();

	//synchronize the stream and return the success value
	cudaSetDevice( device );
	return cudaGetLastError() != cudaSuccess;

}

int vtkCudaDeviceManager::QueryDeviceForObject( vtkCudaObject* object ){
	this->regularLock->Lock();
	int device = -1;
	if( this->ObjectToDeviceMap.count(object) == 1 )
		device = this->ObjectToDeviceMap.find(object)->second;
	else
		vtkErrorMacro(<<"No unique mapping exists.");
	this->regularLock->Unlock();
	return device;
}

int vtkCudaDeviceManager::QueryDeviceForStream( cudaStream_t* stream ){
	this->regularLock->Lock();
	int device = -1;
	if( this->StreamToDeviceMap.count(stream) == 1 )
		device = this->StreamToDeviceMap[stream];
	else
		vtkErrorMacro(<<"No mapping exists.");
	this->regularLock->Unlock();
	return device;
}

void vtkCudaDeviceManager::DestroyEmptyStream( cudaStream_t* stream ){

	this->SynchronizeStream( stream );

	bool found = false;
	std::map<cudaStream_t*,int>::iterator it = this->StreamToDeviceMap.begin();
	for( ; it != this->StreamToDeviceMap.end(); it++ ){
		if( it->first == stream ){
			found = true;
			break;
		}
	}
	if( !found ){
		vtkErrorMacro(<<"Could not locate supplied caller-device pair.");
		return;
	}
	this->StreamToDeviceMap.erase(it);

}