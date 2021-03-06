/*=========================================================================

  Program:   Robarts Visualization Toolkit
  Module:    vtkCudaHierarchicalMaxFlowSegmentation2Worker.cxx

  Copyright (c) John SH Baxter, Robarts Research Institute

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/** @file vtkCudaHierarchicalMaxFlowSegmentation2Worker.cxx
 *
 *  @brief Implementation file with class that runs each individual GPU for GHMF.
 *
 *  @author John Stuart Haberl Baxter (Dr. Peters' Lab (VASST) at Robarts Research Institute)
 *
 *  @note August 27th 2013 - Documentation first compiled.
 *
 *  @note This is not a front-end class. Header details are in vtkCudaMaxFlowSegmentation.h
 *
 */

#include "CUDA_hierarchicalmaxflow.h"
#include "CudaObject.h"
#include "vtkCudaMaxFlowSegmentationScheduler.h"
#include "vtkCudaMaxFlowSegmentationWorker.h"
#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>

//-----------------------------------------------------------------
vtkCudaMaxFlowSegmentationWorker::vtkCudaMaxFlowSegmentationWorker(int g, double usage, vtkCudaMaxFlowSegmentationScheduler* p )
  : Parent(p)
  , GPU(g)
  , CudaObject(g)
{
  //Get GPU buffers
  int BuffersAcquired = 0;
  double PercentAcquired = 0.0;
  UnusedGPUBuffers.clear();
  CPU2GPUMap.clear();
  GPU2CPUMap.clear();
  CPU2GPUMap.insert(std::pair<float*,float*>((float*)0,(float*)0));
  GPU2CPUMap.insert(std::pair<float*,float*>((float*)0,(float*)0));
  while(true)
  {
    //try acquiring some new buffers
    float* NewAcquiredBuffers = 0;
    int NewNumberAcquired = 0;
    int Pad = Parent->VX*Parent->VY;
    double NewPercentAcquired = 0;
    CUDA_GetGPUBuffers( Parent->TotalNumberOfBuffers-BuffersAcquired, usage-PercentAcquired, &NewAcquiredBuffers,
                        Pad, Parent->VolumeSize, &NewNumberAcquired, &NewPercentAcquired );
    BuffersAcquired += NewNumberAcquired;
    PercentAcquired += NewPercentAcquired;

    //if no new buffers were acquired, exit the loop
    if( NewNumberAcquired == 0 )
    {
      break;
    }

    //else, load the new buffers into the list of unused buffers
    AllGPUBufferBlocks.push_back(NewAcquiredBuffers);
    NewAcquiredBuffers += Pad;
    for(int i = 0; i < NewNumberAcquired; i++)
    {
      UnusedGPUBuffers.push_back(NewAcquiredBuffers);
      NewAcquiredBuffers += Parent->VolumeSize;
    }
  }
  NumBuffers = BuffersAcquired;
}

vtkCudaMaxFlowSegmentationWorker::~vtkCudaMaxFlowSegmentationWorker()
{
  this->CallSyncThreads();

  //Return all GPU buffers
  ReturnLeafLabels();
  while( AllGPUBufferBlocks.size() > 0 )
  {
    CUDA_ReturnGPUBuffers( AllGPUBufferBlocks.front() );
    AllGPUBufferBlocks.pop_front();
  }


  //take down stack structure
  TakeDownPriorityStacks();

  //clear remaining mappings
  CPU2GPUMap.clear();
  GPU2CPUMap.clear();
  UnusedGPUBuffers.clear();
  AllGPUBufferBlocks.clear();
  CPUInUse.clear();
}

void vtkCudaMaxFlowSegmentationWorker::ReturnLeafLabels()
{
  //Copy back any uncopied leaf label buffers (others don't matter anymore)
  for( int i = 0; i < Parent->NumLeaves; i++ )
    if( CPU2GPUMap.find(Parent->leafLabelBuffers[i]) != CPU2GPUMap.end() )
    {
      Parent->ReturnBufferGPU2CPU(this,Parent->leafLabelBuffers[i], CPU2GPUMap[Parent->leafLabelBuffers[i]],GetStream());
      GPU2CPUMap.erase(GPU2CPUMap.find(CPU2GPUMap[Parent->leafLabelBuffers[i]]));
      CPU2GPUMap.erase(CPU2GPUMap.find(Parent->leafLabelBuffers[i]));
    }
}

void vtkCudaMaxFlowSegmentationWorker::ReturnBuffer(float* CPUBuffer)
{
  if( !CPUBuffer )
  {
    return;
  }
  if( CPU2GPUMap.find(CPUBuffer) != CPU2GPUMap.end() )
  {
    Parent->ReturnBufferGPU2CPU(this,CPUBuffer, CPU2GPUMap[CPUBuffer],GetStream());
    UnusedGPUBuffers.push_front(CPU2GPUMap[CPUBuffer]);
    GPU2CPUMap.erase(GPU2CPUMap.find(CPU2GPUMap[CPUBuffer]));
    CPU2GPUMap.erase(CPU2GPUMap.find(CPUBuffer));
    RemoveFromStack(CPUBuffer);
  }
}

void vtkCudaMaxFlowSegmentationWorker::UpdateBuffersInUse()
{
  for( std::set<float*>::iterator iterator = CPUInUse.begin();
       iterator != CPUInUse.end(); iterator++ )
  {

    //check if this buffer needs to be assigned
    if( !(*iterator) )
    {
      continue;
    }
    if( CPU2GPUMap.find( *iterator ) != CPU2GPUMap.end() )
    {
      continue;
    }

    //start assigning from the list of unused buffers
    if( UnusedGPUBuffers.size() > 0 )
    {
      float* NewGPUBuffer = UnusedGPUBuffers.front();
      UnusedGPUBuffers.pop_front();
      CPU2GPUMap.insert( std::pair<float*,float*>(*iterator, NewGPUBuffer) );
      GPU2CPUMap.insert( std::pair<float*,float*>(NewGPUBuffer, *iterator) );
      Parent->MoveBufferCPU2GPU(this,*iterator,NewGPUBuffer,GetStream());

      //update the priority stacks
      AddToStack(*iterator);
      continue;
    }

    //see if there is some garbage we can deallocate first
    bool flag = false;
    for( std::set<float*>::iterator iterator2 = Parent->NoCopyBack.begin();
         iterator2 != Parent->NoCopyBack.end(); iterator2++ )
    {
      if( CPUInUse.find(*iterator2) != CPUInUse.end() )
      {
        continue;
      }
      if( CPU2GPUMap.find(*iterator2) == CPU2GPUMap.end() )
      {
        continue;
      }
      float* NewGPUBuffer = CPU2GPUMap[*iterator2];
      CPU2GPUMap.erase( CPU2GPUMap.find(*iterator2) );
      GPU2CPUMap.erase( GPU2CPUMap.find(NewGPUBuffer) );
      CPU2GPUMap.insert( std::pair<float*,float*>(*iterator, NewGPUBuffer) );
      GPU2CPUMap.insert( std::pair<float*,float*>(NewGPUBuffer, *iterator) );
      Parent->MoveBufferCPU2GPU(this,*iterator,NewGPUBuffer,GetStream());

      //update the priority stacks
      RemoveFromStack(*iterator2);
      AddToStack(*iterator);
      flag = true;
      break;
    }
    if( flag )
    {
      continue;
    }

    //else, we have to move something in use back to the CPU
    flag = false;
    std::vector< std::list< float* > >::iterator stackIterator = PriorityStacks.begin();
    for( ; !flag && stackIterator != PriorityStacks.end(); stackIterator++ )
    {
      for(std::list< float* >::iterator subIterator = stackIterator->begin(); subIterator != stackIterator->end(); subIterator++ )
      {

        //can't remove this one because it is in use or null
        if( !(*subIterator) )
        {
          continue;
        }
        if( CPUInUse.find( *subIterator ) != CPUInUse.end() )
        {
          continue;
        }

        //else, find it and move it back to the CPU
        float* NewGPUBuffer = CPU2GPUMap.find(*subIterator)->second;

        CPU2GPUMap.erase( CPU2GPUMap.find(*subIterator) );
        GPU2CPUMap.erase( GPU2CPUMap.find(NewGPUBuffer) );
        CPU2GPUMap.insert( std::pair<float*,float*>(*iterator, NewGPUBuffer) );
        GPU2CPUMap.insert( std::pair<float*,float*>(NewGPUBuffer, *iterator) );
        Parent->ReturnBufferGPU2CPU(this,*subIterator,NewGPUBuffer,GetStream());
        Parent->MoveBufferCPU2GPU(this,*iterator,NewGPUBuffer,GetStream());

        //update the priority stack and leave immediately since our iterators
        //no longer have a valid contract (changed container)
        RemoveFromStack(*subIterator);
        AddToStack(*iterator);
        flag = true;
        break;

      }
      if( flag )
      {
        break;
      }
    }
  }
}

//Add a CPU-GPU buffer pair from this workers collection
void vtkCudaMaxFlowSegmentationWorker::AddToStack( float* CPUBuffer )
{
  int neededPriority = Parent->CPU2PriorityMap.find(CPUBuffer)->second;
  BuildStackUpToPriority( neededPriority );
  std::vector< std::list< float* > >::iterator stackIterator = PriorityStacks.begin();
  for(int count = 1; count < neededPriority; count++, stackIterator++);
  stackIterator->push_front(CPUBuffer);
}

//Remove a CPU-GPU buffer pair from this workers collection
void vtkCudaMaxFlowSegmentationWorker::RemoveFromStack( float* CPUBuffer )
{
  int neededPriority = Parent->CPU2PriorityMap.find(CPUBuffer)->second;
  std::vector< std::list< float* > >::iterator stackIterator = PriorityStacks.begin();
  for(int count = 1; count < neededPriority; count++, stackIterator++);
  for(std::list< float* >::iterator subIterator = stackIterator->begin(); subIterator != stackIterator->end(); subIterator++ )
  {
    if( *subIterator == CPUBuffer )
    {
      stackIterator->erase(subIterator);
      return;
    }
  }
}

//Make sure that this buffers collection can handle the stack size
void vtkCudaMaxFlowSegmentationWorker::BuildStackUpToPriority( unsigned int priority )
{
  while( PriorityStacks.size() < priority )
  {
    PriorityStacks.push_back( std::list<float*>() );
  }
}

//take down the stacks
void vtkCudaMaxFlowSegmentationWorker::TakeDownPriorityStacks()
{
  std::vector< std::list< float* > >::iterator stackIterator = PriorityStacks.begin();
  for( ; stackIterator != PriorityStacks.end(); stackIterator++ )
  {
    stackIterator->clear();
  }
  PriorityStacks.clear();
}

int vtkCudaMaxFlowSegmentationWorker::LowestBufferShift(unsigned int n)
{
  int retVal = 0;
  n -= (int) this->UnusedGPUBuffers.size();
  std::vector< std::list< float* > >::iterator stackIterator = PriorityStacks.begin();
  for(int count = 1; stackIterator != PriorityStacks.end(); count++, stackIterator++)
  {
    if( n > (*stackIterator).size() )
    {
      n -= (int) (*stackIterator).size();
      retVal += count * (int) (*stackIterator).size();
    }
    else
    {
      retVal += count*n;
      break;
    }
  }
  return (n>0) ? n: 0;
}
