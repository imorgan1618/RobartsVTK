/** @file CUDA_vtkCuda1DVolumeMapper_renderAlgo.h
 *
 *  @brief Header file with definitions for different CUDA functions for setting up and running the ray casting process
 *
 *  @author John Stuart Haberl Baxter (Dr. Peters' Lab (VASST) at Robarts Research Institute)
 *  @note First documented on May 12, 2011
 *
 *  @note This is primarily an internal file used by the vtkCuda1DVolumeMapper to manage the ray casting process
 *
 */

#ifndef CUDA_VTKCUDA1DVOLUMEMAPPER_RENDERALGO_H
#define CUDA_VTKCUDA1DVOLUMEMAPPER_RENDERALGO_H
#include "CUDA_containerRendererInformation.h"
#include "CUDA_containerVolumeInformation.h"
#include "CUDA_containerOutputImageInformation.h"
#include "CUDA_container1DTransferFunctionInformation.h"

/** @brief Compute the image of the volume taking into account occluding isosurfaces returning it in a image buffer
 *
 *  @param outputInfo Structure containing information for the rendering process describing the output image and how it is handled
 *  @param renderInfo Structure containing information for the rendering process taken primarily from the renderer, such as camera/shading properties
 *  @param volumeInfo Structure containing information for the rendering process taken primarily from the volume, such as dimensions and location in space
 *
 *  @pre The current frame is less than the number of frames, and is non-negative
 *  @pre CUDA-OpenGL interoperability is functional (ie. Only 1 OpenGL context which corresponds solely to the singular renderer/window)
 *
 */
bool CUDA_vtkCuda1DVolumeMapper_renderAlgo_doRender(const cudaOutputImageInformation& outputInfo,
               const cudaRendererInformation& rendererInfo,
               const cudaVolumeInformation& volumeInfo,
               const cuda1DTransferFunctionInformation& transInfo,
               cudaArray* frame, cudaStream_t* stream);

/** @brief Changes the current volume to be rendered to this particular frame, used in 4D visualization
 *
 *  @param frame The frame (starting with 0) that you want to change the currently rendering volume to
 *
 *  @pre frame is less than the total number of frames and is non-negative
 *
 */
bool CUDA_vtkCuda1DVolumeMapper_renderAlgo_changeFrame(cudaArray* frame, cudaStream_t* stream);

/** @brief Deallocates the frames and clears the container (needed for ray caster deallocation)
 *
 */
void CUDA_vtkCuda1DVolumeMapper_renderAlgo_clearImageArray(cudaArray** frame, cudaStream_t* stream);

/** @brief Loads the RGBA 2D transfer functions into texture memory
 *
 *  @param volumeInfo Structure containing information for the rendering process taken primarily from the volume, such as dimensions and location in space
 *  @param FunctionSize The size of each dimension of each transfer function
 *  @param redTF A floating point buffer containing the red transfer function
 *  @param greenTF A floating point buffer containing the green transfer function
 *  @param blueTF A floating point buffer containing the blue transfer function
 *  @param alphaTF A floating point buffer containing the opacity transfer function
 *
 *  @pre Each transfer function is square with the intensities separated by 1, and gradients by FunctionSize
 *
 */
bool CUDA_vtkCuda1DVolumeMapper_renderAlgo_loadTextures(cuda1DTransferFunctionInformation& transInfo,
                  float* redTF, float* greenTF, float* blueTF, float* alphaTF, float* galphaTF,
                  cudaStream_t* stream);
bool CUDA_vtkCuda1DVolumeMapper_renderAlgo_UnloadTextures(cuda1DTransferFunctionInformation& transInfo, cudaStream_t* stream);

/** @brief Loads an image into a 3D CUDA array which will be bound to a 3D texture for rendering
 *
 *  @param volumeInfo Structure containing information for the rendering process taken primarily from the volume, such as dimensions and location in space
 *  @param index The frame number of this image
 *
 *  @pre index is between 0 and 99 inclusive
 *
 */
bool CUDA_vtkCuda1DVolumeMapper_renderAlgo_loadImageInfo(const float* imageData, const cudaVolumeInformation& volumeInfo, cudaArray** frame,
                             cudaStream_t* stream);

#endif
