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

#include <iostream>
#include "fimage.h"

using namespace std;

void FreeImageErrorHandler(FREE_IMAGE_FORMAT FIF, const char *Message)
{ 
  cout << endl << "*** " << flush;
  if (FIF != FIF_UNKNOWN)
  {
    cout << FreeImage_GetFormatFromFIF(FIF) << " Format" << endl;
  } 
  cout << Message << flush;
  cout << " ***" << endl;
} 

void init_freeimage()
{
  FreeImage_Initialise();
  FreeImage_SetOutputMessage(FreeImageErrorHandler); 
}

void deinit_freeimage()
{
  FreeImage_DeInitialise();
}

FIBITMAP * load_freeimage(FREE_IMAGE_FORMAT fif, const char *image_path)
{
  FIBITMAP *bitmap = FreeImage_Load(fif, image_path, JPEG_DEFAULT); 
  if (!bitmap)
    return NULL;
  
  if (FreeImage_GetColorsUsed(bitmap) != 256
      || FreeImage_GetBPP(bitmap) != 8
      || FreeImage_GetImageType(bitmap) != FIT_BITMAP)
  {
    FreeImage_Unload(bitmap);
    return NULL;
  }

  return bitmap;
}

void save_image_bin(const char *image_path, const clcv::image2d<int> & img)
{
  FIBITMAP *bitmap = FreeImage_Allocate(img.ncols(), img.nrows(), 8);
  for(unsigned r = 0; r < img.nrows(); ++r)
  {
    BYTE *bits = (BYTE *)FreeImage_GetScanLine(bitmap, r); 
    for(unsigned c = 0; c < img.ncols(); ++c)
      bits[c] = img(img.nrows() - r - 1, c);
  }
  // Build a binary palette 
  RGBQUAD *pal = FreeImage_GetPalette(bitmap);
  pal[0].rgbRed = 0, pal[0].rgbGreen = 0, pal[0].rgbBlue = 0;
  for (int i = 1; i < 256; ++i)
  {
    pal[i].rgbRed = 255; 
    pal[i].rgbGreen = 255; 
    pal[i].rgbBlue = 255;
  }
  
  FreeImage_Save(FIF_PNG, bitmap, image_path);
  FreeImage_Unload(bitmap);
}

void save_image_8bit(const char *image_path, const clcv::image2d<int> & img)
{
  FIBITMAP *bitmap = FreeImage_Allocate(img.ncols(), img.nrows(), 8);
  for(unsigned r = 0; r < img.nrows(); ++r)
  {
    BYTE *bits = (BYTE *)FreeImage_GetScanLine(bitmap, r); 
    for(unsigned c = 0; c < img.ncols(); ++c)
      bits[c] = img(img.nrows() - r - 1, c);
  }
  // Build a binary palette 
  RGBQUAD *pal = FreeImage_GetPalette(bitmap);
  for (int i = 0; i < 256; ++i)
  { 
     pal[i].rgbRed = i; 
     pal[i].rgbGreen = i; 
     pal[i].rgbBlue = i;
  } 
  
  FreeImage_Save(FIF_PNG, bitmap, image_path);
  FreeImage_Unload(bitmap);
}
