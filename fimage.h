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

#ifndef CLCV_FIMAGE_H
#define CLCV_FIMAGE_H

#include <FreeImage.h>
#include <clcv/image2d.h>

void init_freeimage();
void deinit_freeimage();
FIBITMAP * load_freeimage(FREE_IMAGE_FORMAT fif, const char *image_path);
void save_image_8bit(const char *image_path, const clcv::image2d<int> & img);
void save_image_bin(const char *image_path, const clcv::image2d<int> & img);

template<class T>
clcv::image2d<T> freeimage2image2d(FIBITMAP * bitmap,
                                   const int row_round, const int col_round,
                                   const T init_val)
{
  const unsigned nrows = FreeImage_GetHeight(bitmap);
  const unsigned ncols = FreeImage_GetWidth(bitmap);
  const unsigned nrows_round = ((nrows - 1) / row_round + 1) * row_round;
  const unsigned ncols_round = ((ncols - 1) / col_round + 1) * col_round;
  
  clcv::image2d<T> img(nrows_round, ncols_round);
  for (unsigned r = 0; r < nrows_round; ++r)
    for (unsigned c = 0; c < ncols_round; ++c)
      img(r, c) = init_val;

  for (unsigned r = 0; r < nrows; ++r) {
    BYTE *data = (BYTE *)FreeImage_GetScanLine(bitmap, nrows - r - 1);
    for (unsigned c = 0; c < ncols; ++c)
      img(r, c) = data[c];
  }
  
  return img;
}

#endif
