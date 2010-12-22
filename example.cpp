// Copyright (c) 2010 iZsh - izsh at fail0verflow.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <mach/mach_time.h>
#include <assert.h>
#include <iostream>
#include <sstream>
#include <clcv/clcv.h>
#include <clcv/clinfo.h>
#include "fimage.h"

using namespace std;
using namespace clcv;

// For coalescing, BLOCKX should be a multiple of 16 (half wrap)
#define BLOCKX 16
#define BLOCKY 16

const int niter = 10;

uint64_t mach_deltat(uint64_t endTime, uint64_t startTime)
{  
  uint64_t difference = endTime - startTime;
  uint64_t value = 0.0;
  
  mach_timebase_info_data_t info;
  mach_timebase_info( &info );
  value = difference * info.numer / info.denom;

  return value;
}

void BenchmarkCL(FIBITMAP * bitmap, const char * outpath)
{
  cout << endl;
  cout << "=================================================================================" << endl;
  cout << "Benchmarking GPU" << endl;
  cout << "=================================================================================" << endl;
  
  image2d<cl_int> imgin = freeimage2image2d(bitmap, BLOCKX, BLOCKY, 255);
  cout << "Rounded to: " << imgin.ncols() << "x" << imgin.nrows() << endl;  

  try {
    CLCV<cl_int> clcv;
    cout << "Connected to " << get_device_name(clcv.get_device_id()) << endl;
    // Build and load the SE
    win2d<cl_int> se = win_rect<cl_int>(5, 7);
    clcv_se_id se_id = clcv.load_se(se);
    // Execute and benchmark
    {
      cl::NDRange local_work_size(BLOCKX, BLOCKY);
      clcv.set_local_work_size(local_work_size);
      uint64_t mbeg;
      mbeg = mach_absolute_time();
      cl::Event event;
      cout << "Benchmarking on " << niter << " images..." << endl;
      for (int i = 0; i < niter; ++i)
      {
        clcv_image_id image_id = clcv.open(imgin);
        event = clcv.push_binarize(128, 1, 0);
        event = clcv.push_naiveopening(se_id);
        clcv.fetch().wait();
        // Save the last image
        if (i == niter - 1)
        {
          image2d<cl_int> imgout = clcv.save(image_id);
          ostringstream os;
          os << outpath << "/" << "imageout.png";
          save_image_bin(os.str().c_str(), imgout);
        }
        clcv.close(image_id);
      }
      clcv.finish();
      uint64_t mend = mach_absolute_time();
      cout << "Opening operator time = " << get_elapsed_time(event) / 1e6 << "ms" << endl;
      cout << "Overall time = " << mach_deltat(mend, mbeg) / 1e6 << "ms" << endl;
    }
  } catch (cl::Error error) {
    cerr << "OpenCL error: " << error.what() << '(' << error.err() << ')' << endl;
  }

}

int main (int argc, char * const argv[])
{
  cout << "===================================" << endl;
  cout << "CLCV -- Copyright 2010 iZsh" << endl;
  cout << "Mathematical Morphology" << endl;
  cout << "Benchmarking Example" << endl;
  cout << "===================================" << endl;
  cout << endl << endl;
  
  if (argc != 3)
  {
    cout << "Usage: " << argv[0] << " <image path> <output directory path>" << endl;
    exit(1);
  }

  init_freeimage();
  FIBITMAP * bitmap = load_freeimage(FIF_JPEG, argv[1]);
  if (bitmap == NULL) {
    cout << "Couldn't load file" << endl;
    deinit_freeimage();
    return 0;
  }
  cout << "Image Loading OK" << endl;

  unsigned nrows = FreeImage_GetHeight(bitmap);
  unsigned ncols = FreeImage_GetWidth(bitmap);
  cout << "Image Resolution: " << ncols << "x" << nrows << endl;

  BenchmarkCL(bitmap, argv[2]);

  FreeImage_Unload(bitmap);
  deinit_freeimage();
  return 0;
}
