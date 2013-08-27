/** @file vtkHierarchicalMaxFlowSegmentation.h
 *
 *  @brief Header file with definitions of CPU-based solver for generalized hierarchical max-flow
 *			segmentation problems.
 *
 *  @author John Stuart Haberl Baxter (Dr. Peter's Lab at Robarts Research Institute)
 *	
 *	@note August 27th 2013 - Documentation first compiled.
 *
 *  @note This is the base class for GPU accelerated max-flow segmentors in vtkCudaImageAnalytics
 *
 */

#ifndef __VTKHIERARCHICALMAXFLOWSEGMENTATION_H__
#define __VTKHIERARCHICALMAXFLOWSEGMENTATION_H__

#include "vtkImageAlgorithm.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkTree.h"

#include <map>
#include <list>
#include <limits.h>
#include <float.h>

class vtkHierarchicalMaxFlowSegmentation : public vtkImageAlgorithm
{
public:
	vtkTypeMacro( vtkHierarchicalMaxFlowSegmentation, vtkImageAlgorithm );
	static vtkHierarchicalMaxFlowSegmentation *New();
	
	// Description:
	// Set the hierarchical model used in the segmentation. Leaves in the tree correspond
	// to disjoint labels in the output image. Branches correspond to super-objects,
	// collections of these labels
	vtkSetObjectMacro(Hierarchy,vtkTree)
	vtkGetObjectMacro(Hierarchy,vtkTree)
	
	// Description:
	// Weight the smoothness term. If no scalar is provided, it is assumed to be 1. If
	// no smoothness term is provided, it is assumed to be the unit function.
	void AddSmoothnessScalar( vtkIdType node, double alpha );
	
	// Description:
	// Get and Set the number of iterations used by the algorithm (computing convergence term
	// is too slow).
	vtkSetClampMacro(NumberOfIterations,int,0,INT_MAX);
	vtkGetMacro(NumberOfIterations,int);
	
	// Description:
	// Get and Set the labeling constant, CC, of the algorithm. The default value is 0.25
	// and is unlikely to require modification.
	vtkSetClampMacro(CC,float,0.0f,1.0f);
	vtkGetMacro(CC,float);
	
	// Description:
	// Get and Set the step size of the algorithm for updating spatial flows. The default
	// value is 0.1 and is unlikely to require modification.
	vtkSetClampMacro(StepSize,float,0.0f,1.0f);
	vtkGetMacro(StepSize,float);
	
	// Description:
	// Get and Set the data cost for the objects. The algorithm only uses those which
	// correspond to leaf nodes due to the data term pushdown theorem. These must be
	// supplied for every leaf node for the algorithm to run.
	// The hierarchy must be supplied prior to invoking these methods.
	vtkDataObject* GetDataInput(int idx);
	void SetDataInput(int idx, vtkDataObject *input);
	
	// Description:
	// Get and Set the smoothness for the objects. The algorithm only uses those which
	// correspond to non-root nodes. If not supplied, they are assumed to be the unit
	// field (ie: S(x)=1 for all voxels x).
	// The hierarchy must be supplied prior to invoking these methods.
	vtkDataObject* GetSmoothnessInput(int idx);
	void SetSmoothnessInput(int idx, vtkDataObject *input);
	
	// Description:
	// Get the final probabilistic labelling, assuming idx refers to a leaf
	// node in the supplied hierarchy
	vtkDataObject* GetOutput(int idx);
	
	// Description:
	// If the subclass does not define an Execute method, then the task
	// will be broken up, multiple threads will be spawned, and each thread
	// will call this method. It is public so that the thread functions
	// can call this method.
	virtual int RequestData(vtkInformation *request, 
							 vtkInformationVector **inputVector, 
							 vtkInformationVector *outputVector);
	virtual int RequestInformation( vtkInformation* request,
							 vtkInformationVector** inputVector,
							 vtkInformationVector* outputVector);
	virtual int RequestUpdateExtent( vtkInformation* request,
							 vtkInformationVector** inputVector,
							 vtkInformationVector* outputVector);
	virtual int RequestDataObject( vtkInformation* request,
							 vtkInformationVector** inputVector,
							 vtkInformationVector* outputVector);
	virtual int FillInputPortInformation(int i, vtkInformation* info);

protected:
	vtkHierarchicalMaxFlowSegmentation();
	virtual ~vtkHierarchicalMaxFlowSegmentation();

	int CheckInputConsistancy( vtkInformationVector** inputVector, int* Extent, int& NumNodes, int& NumLeaves, int& NumEdges );
	
	virtual int InitializeAlgorithm();
	virtual int RunAlgorithm();

	void PropogateLabels( vtkIdType currNode );
	void SolveMaxFlow( vtkIdType currNode );
	void UpdateLabel( vtkIdType node );
	
	vtkTree* Hierarchy;
	std::map<vtkIdType,double> SmoothnessScalars;
	std::map<vtkIdType,int> LeafMap;
	std::map<vtkIdType,int> BranchMap;
	int		NumLeaves;
	int		NumBranches;
	int		NumNodes;
	int		NumEdges;

	int NumberOfIterations;
	float CC;
	float StepSize;
	int VolumeSize;
	int VX, VY, VZ;
	
	std::map<vtkIdType,int> InputDataPortMapping;
	std::map<int,vtkIdType> BackwardsInputDataPortMapping;
	int FirstUnusedDataPort;
	std::map<vtkIdType,int> InputSmoothnessPortMapping;
	std::map<int,vtkIdType> BackwardsInputSmoothnessPortMapping;
	int FirstUnusedSmoothnessPort;

	//pointers to variable structures, easier to keep as part of the class definition
	std::list<float*> CPUBuffersAcquired;
	std::list<int> CPUBuffersSize;
	int TotalNumberOfBuffers;
	float**	branchFlowXBuffers;
	float**	branchFlowYBuffers;
	float**	branchFlowZBuffers;
	float**	branchDivBuffers;
	float**	branchSinkBuffers;
	float**	branchIncBuffers;
	float**	branchLabelBuffers;
	float**	branchSmoothnessTermBuffers;
	float**	branchWorkingBuffers;
	float*	branchSmoothnessConstants;

	float**	leafFlowXBuffers;
	float**	leafFlowYBuffers;
	float**	leafFlowZBuffers;
	float**	leafDivBuffers;
	float**	leafSinkBuffers;
	float**	leafIncBuffers;
	float**	leafLabelBuffers;
	float**	leafDataTermBuffers;
	float**	leafSmoothnessTermBuffers;
	float*	leafSmoothnessConstants;

	float*	sourceFlowBuffer;
	float*	sourceWorkingBuffer;

private:
	vtkHierarchicalMaxFlowSegmentation operator=(const vtkHierarchicalMaxFlowSegmentation&){} //not implemented
	vtkHierarchicalMaxFlowSegmentation(const vtkHierarchicalMaxFlowSegmentation&){} //not implemented

};

#endif
