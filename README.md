<img src="Docs/readme/images/robartsvtk.png" alt="RobartsVTK" height="125px"/>

RobartsVTK is a software package for image processing and visualization.

# Download RobartsVTK

For now, RobartsVTK must be built. 

# Building RobartsVTK
For convenient building, please see the RobartsVTK [super build](http://git.imaging.robarts.ca/repos/RobartsVTKBuild) project.

## Known Configurations
RobartsVTK has been built on the following configurations:
* Windows 10 x64, Visual Studio 2012, 32bit build
* Windows 10 x64, Visual Studio 2012, 64bit build

## Dependencies
Dependencies must be built or installed before RobartsVTK can be built. Please visit the respective links to download the appropriate packages.
* [CMake 3.4](https://cmake.org/download/) - installed
* [VTK 6.2](http://www.vtk.org/download/) - built
 * If Python wrapping of RobartsVTK is desired, VTK must be built with WRAP_PYTHON enabled.
* [QT 4/5](http://download.qt.io/archive/qt/) - built (optional)
* [ITK 4.7.2](http://www.itk.org/ITK/resources/software.html) - built (optional)
* [CUDA 7](https://developer.nvidia.com/cuda-downloads) - installed (optional)
* [PLUS 2.3](http://plustoolkit.org) -built (optional)

## CMake Configuration
The following variables should be set when configuring RobartsVTK
* BUILD_SHARED_LIBS:BOOL = `OFF` (ON is experimental)
* RobartsVTK_Include_Outdated_Registration:BOOL = `OFF`
* ITK_DIR:PATH = `<path/to/your/itk-bin/dir>`
* PlusLib_DIR:PATH = `<path/to/your/plus-bin/dir>`
* QT4 - QT_QMAKE_EXECUTABLE:FILEPATH = `<path/to/your/qt-build>/bin/qmake.exe`
* QT5 - as above OR - Qt5_DIR:PATH = `<path/to/your/qt-build>/lib/cmake/Qt5`
* VTK_DIR:PATH = `<path/to/your/vtk-bin/dir>`
    * If you're wrapping with python:
        * PYTHON_INCLUDE_DIR:PATH = `<path/to/python-install>/include`
        * PYTHON_LIBRARY:PATH = `<path/to/python-install>/libs/python27.lib`

## License
Please see the [license](LICENSE.md) file.

## Acknowledgments
RobartsVTK would like to thank the creator and maintainers of [GitLab](https://about.gitlab.com/).