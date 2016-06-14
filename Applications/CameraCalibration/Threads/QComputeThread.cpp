/*=========================================================================

Program:   Camera Calibration
Module:    $RCSfile: QComputeThread.cpp,v $
Creator:   Adam Rankin <arankin@robarts.ca>
Language:  C++
Author:    $Author: Adam Rankin $

==========================================================================

Copyright (c) Adam Rankin, arankin@robarts.ca

Use, modification and redistribution of the software, in source or
binary forms, are permitted provided that the following terms and
conditions are met:

1) Redistribution of the source code, in verbatim or modified
form, must retain the above copyright notice, this license,
the following disclaimer, and any notices that refer to this
license and/or the following disclaimer.

2) Redistribution in binary form must include the above copyright
notice, a copy of this license and the following disclaimer
in the documentation or with other materials provided with the
distribution.

3) Modified copies of the source code must be clearly marked as such,
and must not be misrepresented as verbatim copies of the source code.

THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS"
WITHOUT EXPRESSED OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  IN NO EVENT SHALL ANY COPYRIGHT HOLDER OR OTHER PARTY WHO MAY
MODIFY AND/OR REDISTRIBUTE THE SOFTWARE UNDER THE TERMS OF THIS LICENSE
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA OR DATA BECOMING INACCURATE
OR LOSS OF PROFIT OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF
THE USE OR INABILITY TO USE THE SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

=========================================================================*/

#include "QComputeThread.h"

// PLUS includes
#include <PlusCommon.h>

// Qt includes
#include <QImage>

// Open CV includes
#include <opencv2/imgproc.hpp>

//----------------------------------------------------------------------------
QComputeThread::QComputeThread(QObject *parent /*= 0*/)
  : QThread(parent)
  , Computation(COMPUTATION_NONE)
{

}

//----------------------------------------------------------------------------
QComputeThread::~QComputeThread()
{
  wait();
}

//----------------------------------------------------------------------------
void QComputeThread::run()
{
  ComputationType compType;
  {
    QMutexLocker locker(&Mutex);
    compType = Computation;
  }

  if( compType == COMPUTATION_MONO_CALIBRATE )
  {
    emit monoCalibrationComplete();
  }
  else if( compType == COMPUTATION_STEREO_CALIBRATE )
  {
    emit stereoCalibrationComplete();
  }

  return;
}

//----------------------------------------------------------------------------
bool QComputeThread::CalibrateCamera()
{
  if (!isRunning())
  {
    {
      QMutexLocker locker(&Mutex);
      Computation = COMPUTATION_MONO_CALIBRATE;
    }

    start(LowPriority);
  }

  return false;
}

//----------------------------------------------------------------------------
bool QComputeThread::StereoCalibrate()
{
  if (!isRunning())
  {
    {
      QMutexLocker locker(&Mutex);
      Computation = COMPUTATION_STEREO_CALIBRATE;
    }

    start(LowPriority);
  }

  return false;
}