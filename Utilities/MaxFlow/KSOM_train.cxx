/*------------------------------------------------------------------------------//
KSOM_train.exe

Description:
This file is a utility for applying the GHMF solver. It will take a brief description
of the GHMF hierarchy with associated smoothness/data terms and alphas as well as
identifiers and output values

Usage:\t OutputFilename DeviceNumber MapSize NumberOfIterations ScheduleFilename DataFileList [LabelFileList Label]

OutputFilename: The file root to save the KSOM to
DeviceNumber: The device to use
MapSize: The size of the (square) map to train
NumberOfIterations: The number of iterations to train over
ScheduleFilename: The file containing the schedule
DataFileList: The file containing filenames for the data to train over
LabelFileList: The file containing files for the labels to restrict training to
Label: The label to restrict training to

The Schedule file contains a list of lines each with a 7-tuple:
  (Iteration, MeanAlpha, VarAlpha, WeightAlpha, MeanWidth, VarWidth, WeightWidth)
where widths are expressed by ratio of map size.
//------------------------------------------------------------------------------*/

#include "vtkCudaKohonenGenerator.h"
#include "vtkPiecewiseFunction.h"
#include "vtkMetaImageReader.h"
#include "vtkMetaImageWriter.h"
#include "vtkImageThreshold.h"
#include "vtkImageCast.h"

#include <string>
#include <algorithm>
#include <fstream>

#include <vtkVersion.h> //for VTK_MAJOR_VERSION

void showHelpMessage()
{
  std::cerr << "Usage:\t OutputFilename DeviceNumber MapSize NumberOfIterations ScheduleFilename DataFileList [LabelFileList Label]" << std::endl;
}

void showLongHelpMessage()
{
  std::cerr <<
            "Description:" << std::endl <<
            "This file is a utility for applying the GHMF solver. It will take a brief description " <<
            "of the GHMF hierarchy with associated smoothness/data terms and alphas as well as " <<
            "identifiers and output values" << std::endl <<
            std::endl <<
            "Usage:\t OutputFilename DeviceNumber MapSize NumberOfIterations ScheduleFilename DataFileList [LabelFileList Label]" << std::endl <<
            std::endl <<
            "OutputFilename: The file root to save the KSOM to" << std::endl <<
            "DeviceNumber: The device to use" << std::endl <<
            "MapSize: The size of the (square) map to train" << std::endl <<
            "NumberOfIterations: The number of iterations to train over" << std::endl <<
            "ScheduleFilename: The file containing the schedule" << std::endl <<
            "DataFileList: The file containing filenames for the data to train over" << std::endl <<
            "LabelFileList: The file containing files for the labels to restrict training to" << std::endl <<
            "Label: The label to restrict training to" << std::endl <<
            std::endl <<
            "The Schedule file contains a list of lines each with a 7-tuple: " << std::endl <<
            "\t(Iteration, MeanAlpha, VarAlpha, WeightAlpha, MeanWidth, VarWidth, WeightWidth)" << std::endl <<
            "where widths are expressed by ratio of map size." << std::endl;
}

int main(int argc, char** argv)
{

  if(argc < 2)
  {
    showHelpMessage();
    return 0;
  }

  //check for explicit help message
  std::string FirstArgument = std::string(argv[1]);
  std::transform(FirstArgument.begin(), FirstArgument.end(), FirstArgument.begin(), ::tolower);
  int found = (int) FirstArgument.find(std::string("help"));
  if(found!=std::string::npos)
  {
    showLongHelpMessage();
    return 0;
  }

  //make sure there are enough arguments
  if(argc < 7)
  {
    showHelpMessage();
    return 0;
  }

  //get numeric parameters
  int DeviceNumber = std::atoi(argv[2]);
  int MapSize = std::atoi(argv[3]);
  int NumIts = std::atoi(argv[4]);

  //get filenames
  std::string OutputFilename = std::string(argv[1]);
  std::string ScheduleFilename = std::string(argv[5]);
  std::string DataFileList = std::string(argv[6]);
  std::string LabelFileList = (argc > 7) ? std::string(argv[7]) : "";
  int Label = (argc > 8) ? std::atoi(argv[8]) : -1;

  //create schedule
  vtkPiecewiseFunction* MeansAlphaSchedule = vtkPiecewiseFunction::New();
  vtkPiecewiseFunction* VarsAlphaSchedule = vtkPiecewiseFunction::New();
  vtkPiecewiseFunction* WeightsAlphaSchedule = vtkPiecewiseFunction::New();
  vtkPiecewiseFunction* MeansWidthSchedule = vtkPiecewiseFunction::New();
  vtkPiecewiseFunction* VarsWidthSchedule = vtkPiecewiseFunction::New();
  vtkPiecewiseFunction* WeightsWidthSchedule = vtkPiecewiseFunction::New();
  std::ifstream ScheduleStream = std::ifstream();
  ScheduleStream.open(ScheduleFilename);
  while(!ScheduleStream.eof())
  {
    double Iteration;
    double mA, vA, wA, mW, vW, wW;
    ScheduleStream >> Iteration;
    ScheduleStream >> mA >> vA >> wA >> mW >> vW >> wW;
    MeansAlphaSchedule->AddPoint(Iteration,mA);
    VarsAlphaSchedule->AddPoint(Iteration,vA);
    WeightsAlphaSchedule->AddPoint(Iteration,wA);
    MeansWidthSchedule->AddPoint(Iteration,mW);
    VarsWidthSchedule->AddPoint(Iteration,vW);
    WeightsWidthSchedule->AddPoint(Iteration,wW);
  }
  ScheduleStream.close();

  //create generator
  vtkCudaKohonenGenerator* Generator = vtkCudaKohonenGenerator::New();
  Generator->SetDevice(DeviceNumber);
  Generator->SetNumberOfIterations(NumIts);
  Generator->SetKohonenMapSize(MapSize,MapSize);
  Generator->SetMeansAlphaSchedule(MeansAlphaSchedule);
  Generator->SetVarsAlphaSchedule(VarsAlphaSchedule);
  Generator->SetWeightsAlphaSchedule(WeightsAlphaSchedule);
  Generator->SetMeansWidthSchedule(MeansWidthSchedule);
  Generator->SetVarsWidthSchedule(VarsWidthSchedule);
  Generator->SetWeightsWidthSchedule(WeightsWidthSchedule);

  //load data into generator
  if(LabelFileList.empty())
  {
    Generator->SetUseMaskFlag(false);
    std::ifstream DataFileStream = std::ifstream();
    DataFileStream.open(DataFileList);
    int i = 0;
    while(!DataFileStream.eof())
    {
      std::string DataFileName;
      std::getline(DataFileStream,DataFileName);
      vtkMetaImageReader* DataReader = vtkMetaImageReader::New();
      DataReader->SetFileName(DataFileName.c_str());
      vtkImageCast* Caster = vtkImageCast::New();
#if (VTK_MAJOR_VERSION < 6)
      Caster->SetInput(DataReader->GetOutput());
#else
      Caster->SetInputConnection(DataReader->GetOutputPort());
#endif
      Caster->SetOutputScalarTypeToFloat();
      Caster->Update();
#if (VTK_MAJOR_VERSION < 6)
      Generator->SetInput(i,Caster->GetOutput());
#else
      Generator->SetInputConnection(i, Caster->GetOutputPort());
#endif
      std::cout << "Read data file:  " << DataFileName << std::endl;
      DataReader->Delete();
      Caster->Delete();
      i++;
    }
    DataFileStream.close();
  }
  else
  {
    Generator->SetUseMaskFlag(true);
    std::ifstream DataFileStream = std::ifstream();
    std::ifstream LabelFileStream = std::ifstream();
    DataFileStream.open(DataFileList);
    LabelFileStream.open(LabelFileList);
    int i = 0;
    while(!DataFileStream.eof())
    {
      std::string DataFileName;
      std::getline(DataFileStream,DataFileName);
      vtkMetaImageReader* DataReader = vtkMetaImageReader::New();
      DataReader->SetFileName(DataFileName.c_str());
      vtkImageCast* Caster = vtkImageCast::New();
#if (VTK_MAJOR_VERSION < 6)
      Caster->SetInput(DataReader->GetOutput());
#else
      Caster->SetInputConnection(DataReader->GetOutputPort());
#endif
      Caster->SetOutputScalarTypeToFloat();
      Caster->Update();
#if (VTK_MAJOR_VERSION < 6)
      Generator->SetInput(i,Caster->GetOutput());
#else
      Generator->SetInputConnection(i, Caster->GetOutputPort());
#endif
      std::cout << "Read data file: " << DataFileName << std::endl;
      DataReader->Delete();
      Caster->Delete();


      std::string LabelFileName;
      std::getline(LabelFileStream,LabelFileName);
      vtkMetaImageReader* LabelReader = vtkMetaImageReader::New();
      LabelReader->SetFileName(LabelFileName.c_str());
      if( Label < 0 )
      {
        vtkImageCast* Caster = vtkImageCast::New();
#if (VTK_MAJOR_VERSION < 6)
        Caster->SetInput(LabelReader->GetOutput());
#else
        Caster->SetInputConnection(LabelReader->GetOutputPort());
#endif
        Caster->SetOutputScalarTypeToChar();
        Caster->Update();
#if (VTK_MAJOR_VERSION < 6)
        Generator->SetInput(2*i+1, Caster->GetOutput());
#else
        Generator->SetInputConnection(2*i+1, Caster->GetOutputPort());
#endif
        Caster->Delete();
      }
      else
      {
        vtkImageThreshold* Threshold = vtkImageThreshold::New();
#if (VTK_MAJOR_VERSION < 6)
        Threshold->SetInput(LabelReader->GetOutput());
#else
        Threshold->SetInputConnection(LabelReader->GetOutputPort());
#endif
        Threshold->SetInValue(1);
        Threshold->SetOutValue(0);
        Threshold->ThresholdBetween(Label,Label);
        Threshold->SetOutputScalarTypeToChar();
        Threshold->Update();
#if (VTK_MAJOR_VERSION < 6)
        Generator->SetInput(2*i+1, Threshold->GetOutput());
#else
        Generator->SetInputConnection(2*i+1, Threshold->GetOutputPort());
#endif
        Threshold->Delete();
      }
      std::cout << "Read label file: " << LabelFileName << std::endl;
      LabelReader->Delete();

      i++;
    }
    DataFileStream.close();
    LabelFileStream.close();
  }

  //run generator
  Generator->Update();

  //save output
  std::string OutputFilenameMHD = OutputFilename + ".mhd";
  std::string OutputFilenameRAW = OutputFilename + ".raw";
  vtkMetaImageWriter* Writer = vtkMetaImageWriter::New();
#if (VTK_MAJOR_VERSION < 6)
  Writer->SetInput(Generator->GetOutput());
#else
  Writer->SetInputConnection(Generator->GetOutputPort());
#endif
  Writer->SetFileName(OutputFilenameMHD.c_str());
  Writer->SetRAWFileName(OutputFilenameRAW.c_str());
  Writer->Write();
  Writer->Delete();

  //clean up generator
  Generator->Delete();

  //clean up schedules
  MeansAlphaSchedule->Delete();
  VarsAlphaSchedule->Delete();
  WeightsAlphaSchedule->Delete();
  MeansWidthSchedule->Delete();
  VarsWidthSchedule->Delete();
  WeightsWidthSchedule->Delete();
}