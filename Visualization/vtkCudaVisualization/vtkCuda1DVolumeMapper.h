/** @file vtkCudaVolumeMapper.h
 *
 *  @brief Header file defining a volume mapper (ray caster) using CUDA kernels for parallel ray calculation
 *
 *  @author John Stuart Haberl Baxter (Dr. Peters' Lab (VASST) at Robarts Research Institute)
 *  @note First documented on May 12, 2012
 *
 */

#ifndef __VTKCUDA1DVOLUMEMAPPER_H
#define __VTKCUDA1DVOLUMEMAPPER_H

#include "vtkCudaVisualizationModule.h"

#include "vtkCudaVolumeMapper.h"

class CUDA_container1DTransferFunctionInformation;
class CUDA_containerOutputImageInformation;
class CUDA_containerRendererInformation;
class CUDA_containerVolumeInformation;
class vtkCuda1DTransferFunctionInformationHandler;
struct cudaArray;

/** @brief vtkCuda1DVolumeMapper is a volume mapper, taking a set of 3D image data objects, volume and renderer as input and creates a 2D ray casted projection of the scene which is then displayed to screen
 *
 */
class VTKCUDAVISUALIZATION_EXPORT vtkCuda1DVolumeMapper : public vtkCudaVolumeMapper
{
public:

  vtkTypeMacro( vtkCuda1DVolumeMapper, vtkCudaVolumeMapper );

  /** @brief VTK compatible constructor method
   *
   */
  static vtkCuda1DVolumeMapper *New();

  virtual void SetInputInternal( vtkImageData * image, int frame);
  virtual void ClearInputInternal();
  virtual void ChangeFrameInternal(int frame);
  virtual void InternalRender (  vtkRenderer* ren, vtkVolume* vol,
                                 const cudaRendererInformation& rendererInfo,
                                 const cudaVolumeInformation& volumeInfo,
                                 const cudaOutputImageInformation& outputInfo );

protected:
  /** @brief Constructor which initializes the number of frames, rendering type and other constants to safe initial values, and creates the required information handlers
   *
   */
  vtkCuda1DVolumeMapper();

  /** @brief Destructor which deallocates the various information handlers and matrices
   *
   */
  ~vtkCuda1DVolumeMapper();
  virtual void Reinitialize(int withData = 0);
  virtual void Deinitialize(int withData = 0);

  vtkCuda1DTransferFunctionInformationHandler* transferFunctionInfoHandler;

  static vtkMutexLock* tfLock;

  cudaArray* SourceData[ VTKCUDAVOLUMEMAPPER_UPPER_BOUND ];

private:
  vtkCuda1DVolumeMapper operator=(const vtkCuda1DVolumeMapper&); /**< not implemented */
  vtkCuda1DVolumeMapper(const vtkCuda1DVolumeMapper&); /**< not implemented */

};

#endif