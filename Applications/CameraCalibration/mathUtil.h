/*=========================================================================

  Program:   tracking with GUI
  Module:    $RCSfile: mathUtil.h,v $
  Creator:   Elvis C. S. Chen <chene@robarts.ca>
  Language:  C++
  Author:    $Author: Elvis Chen $  
  Date:      $Date: 2011/07/04 15:28:30 $
  Version:   $Revision: 0.99 $

  ==========================================================================

  Copyright (c) Elvis C. S. Chen, elvis.chen@gmail.com

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

#ifndef __MATHUTIL_H__
#define __MATHUTIL_H__

struct p3
{
  double x;
  double y;
  double z;
};

// x . y
double dot( p3 x, p3 y );

// x * y
p3 cross( p3 x, p3 y );

// euclidean distance between 2 points
double distance( p3 x, p3 y );

// vector between 2 points
// y-x
p3 pointVector( p3 x, p3 y );

#endif // of __MATHUTIL_H__