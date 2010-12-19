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

// Copy a sub image into a local memory buffer
void memcpy2d2local(local int * local_out, global const int * in,
                    const int in_nrows, const int in_ncols,
                    const int rect_nrows, const int rect_ncols,
                    const int orig_r, const int orig_c,
                    const int thread_idx, const int nb_threads)
{
  const int size = rect_nrows * rect_ncols;

  for (int i = thread_idx; i < size; i += nb_threads)
  {
    const int div = i / rect_ncols;
    const int rect_row = div + orig_r;
    const int rect_col = i - div * rect_ncols + orig_c;

    if (rect_row < 0 || rect_row >= in_nrows || rect_col < 0 || rect_col >= in_ncols)
      local_out[i] = -1;
    else
    {
      const int pidx = mad24(rect_row, in_ncols, rect_col);
      local_out[i] = in[pidx] * 2 - 1;
    }
  }

}

// Copy a linear array into a local memory buffer
void memcpy2local(local int * local_out, global const int * in,
                  const int size,
                  const int thread_idx, const int nb_threads)
{
  for (int i = thread_idx; i < size; i += nb_threads)
    local_out[i] = in[i];
}

// memset a local memory buffer
void memset2local(local int * ptr, const int value, const int size,
                  const int thread_idx, const int nb_threads)
{
  for (int i = thread_idx; i < size; i += nb_threads)
    ptr[i] = value;
}

// Common image setup. Copy the structuring element and the workgroup+apron
// into two local memory buffers
// TODO: Align read operations for coalescing (taking into account the apron)
void localimg_setup(global const int * in, global const int * se,
                    local int * local_a, local int * local_se,
                    const int thread_idx, const int nb_threads,
                    const int nrows, const int ncols,
                    const int padded_nrows, const int padded_ncols,
                    const int r, const int c,
                    const int se_size)
{
  // Copy the SE into the local memory
  memcpy2local(local_se, se, se_size, thread_idx, nb_threads);
  // Copy the sub image into the local memory  
  memcpy2d2local(local_a, in, nrows, ncols,
                 padded_nrows, padded_ncols,
                 r, c,
                 thread_idx, nb_threads);
  // Synch all threads
  barrier(CLK_LOCAL_MEM_FENCE);
}

// Simple binarize/threshold operator. Global to global memory
kernel void binarize(global const int * in, global int * out,
                     const int nrows, const int ncols,
                     const int threshold, const int min_val, const int max_val)
{  
  const int x = get_global_id(0);
  const int y = get_global_id(1);
  const int idx = mad24(y, ncols, x);
  out[idx] = select(min_val, max_val, in[idx] >= threshold);  
}

// Naive mathematical morpholgy operator.
// Just iterate through the structuring element for each pixel
kernel void naivemorph(global const int * in, global int * out,
                       const int nrows, const int ncols,
                       global int * se, const int se_rowrad, const int se_colrad,
                       const int se_count, const int se_targetsum,
                       local int * local_se,
                       local int * local_img)
{
  const int x = get_global_id(0);
  const int y = get_global_id(1);
  const int lx = get_local_id(0);
  const int ly = get_local_id(1);
  const int lxsize = get_local_size(0);
  const int lysize = get_local_size(1);
  const int corner_x = get_group_id(0) * lxsize;
  const int corner_y = get_group_id(1) * lysize;

  const int thread_idx = mad24(ly, lxsize, lx);
  const int nb_threads = lxsize * lysize;

  const int se_size = se_count * 2 + se_count;

  const int padded_nrows = lysize + se_rowrad * 2;
  const int padded_ncols = lxsize + se_colrad * 2;

  const int lidx = mad24(padded_ncols, ly + se_rowrad, lx + se_colrad);
  const int idx = mad24(y, ncols, x);

  localimg_setup(in, se, local_img, local_se,
                 thread_idx, nb_threads,
                 nrows, ncols, padded_nrows, padded_ncols,
                 corner_y - se_rowrad, corner_x - se_colrad,
                 se_size);

  int acc = 0;
  for (int i = 0; i < (se_count*2+se_count); i += 3)
  {
    const int r = local_se[i];
    const int c = local_se[i+1];
    const int w = local_se[i+2];
    const int ridx = mad24(padded_ncols, r, lidx + c);
    const int val = local_img[ridx];
    acc = mad24(val, w, acc);
  }
  out[idx] = select(0, 1, acc >= se_targetsum);
}
