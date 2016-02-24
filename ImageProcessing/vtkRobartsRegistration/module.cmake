vtk_module(vtkRobartsRegistration
  GROUPS
    Imaging
  DEPENDS
    vtkImagingHybrid 
    vtkImagingCore
    vtkImagingStatistics
    vtkFiltersGeneral
    vtkFiltersCore
    vtkImagingMath
    vtkCommonDataModel
    vtkFiltersGeneral
  KIT
    vtkRobartsImageProcessing
  )