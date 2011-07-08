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

// TODO: Experiment with floats and different GPUs
// On some devices (mostly old?) float mul are faster than int mul
// Some experiments should therefore been made with using float instead of ints.
// e.g. using mad on float should be consistently efficient, whereas with ints,
// you have to either use mad24 or regular mad to get the best performance
// (depending on the chipset).


// Copy a sub image into a local memory buffer
void naive_memcpy2d2local(local int * local_out, global const int * in,
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
void naive_localimg_setup(global const int * in, global const int * se,
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
  naive_memcpy2d2local(local_a, in, nrows, ncols,
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
kernel void naive_morph(global const int * in, global int * out,
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

  naive_localimg_setup(in, se, local_img, local_se,
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

// Bitmapped operators

// This binarize version compacts the binarized image to a 1bit per pixel format
// Because of coalescing, and to lessen the constraints on the image size
// rounding, we interpret the image as a 1D data input.
// We don't use reduction here : one thread is responsible for 32 consecutive
// pixels. Therefore (nrows*ncols) needs to be a multiple of 32*64=2048 for
// a reasonable occupancy and to prevent extra checks.
// Be careful though, that's already 8KB of shared memory usage...
// TODO: do real benchmarking about occupancy vs. memory usage tradeoff.
kernel void bitmapped_binarize(global const int * in, global int * out,
                               const int threshold, const int min_val, const int max_val,
                               local int * local_img)
{
  const int thread_idx = get_local_id(0);
  const int nb_threads = get_local_size(0);

  // memcpy2local but unrolled. 2x faster on Nvidia 9400M
  { // unroll
    const int gidx = (get_group_id(0) << 5) * nb_threads + thread_idx;
    local_img[thread_idx + 0*nb_threads] = in[gidx + 0*nb_threads];
    local_img[thread_idx + 1*nb_threads] = in[gidx + 1*nb_threads];
    local_img[thread_idx + 2*nb_threads] = in[gidx + 2*nb_threads];
    local_img[thread_idx + 3*nb_threads] = in[gidx + 3*nb_threads];
    local_img[thread_idx + 4*nb_threads] = in[gidx + 4*nb_threads];
    local_img[thread_idx + 5*nb_threads] = in[gidx + 5*nb_threads];
    local_img[thread_idx + 6*nb_threads] = in[gidx + 6*nb_threads];
    local_img[thread_idx + 7*nb_threads] = in[gidx + 7*nb_threads];
    local_img[thread_idx + 8*nb_threads] = in[gidx + 8*nb_threads];
    local_img[thread_idx + 9*nb_threads] = in[gidx + 9*nb_threads];
    local_img[thread_idx + 10*nb_threads] = in[gidx + 10*nb_threads];
    local_img[thread_idx + 11*nb_threads] = in[gidx + 11*nb_threads];
    local_img[thread_idx + 12*nb_threads] = in[gidx + 12*nb_threads];
    local_img[thread_idx + 13*nb_threads] = in[gidx + 13*nb_threads];
    local_img[thread_idx + 14*nb_threads] = in[gidx + 14*nb_threads];
    local_img[thread_idx + 15*nb_threads] = in[gidx + 15*nb_threads];
    local_img[thread_idx + 16*nb_threads] = in[gidx + 16*nb_threads];
    local_img[thread_idx + 17*nb_threads] = in[gidx + 17*nb_threads];
    local_img[thread_idx + 18*nb_threads] = in[gidx + 18*nb_threads];
    local_img[thread_idx + 19*nb_threads] = in[gidx + 19*nb_threads];
    local_img[thread_idx + 20*nb_threads] = in[gidx + 20*nb_threads];
    local_img[thread_idx + 21*nb_threads] = in[gidx + 21*nb_threads];
    local_img[thread_idx + 22*nb_threads] = in[gidx + 22*nb_threads];
    local_img[thread_idx + 23*nb_threads] = in[gidx + 23*nb_threads];
    local_img[thread_idx + 24*nb_threads] = in[gidx + 24*nb_threads];
    local_img[thread_idx + 25*nb_threads] = in[gidx + 25*nb_threads];
    local_img[thread_idx + 26*nb_threads] = in[gidx + 26*nb_threads];
    local_img[thread_idx + 27*nb_threads] = in[gidx + 27*nb_threads];
    local_img[thread_idx + 28*nb_threads] = in[gidx + 28*nb_threads];
    local_img[thread_idx + 29*nb_threads] = in[gidx + 29*nb_threads];
    local_img[thread_idx + 30*nb_threads] = in[gidx + 30*nb_threads];
    local_img[thread_idx + 31*nb_threads] = in[gidx + 31*nb_threads];
  }

  // Compute the result
  { // Unroll
    const int idx = get_global_id(0);
    const int lidx = thread_idx << 5;
    out[idx] =
      select(min_val, max_val, local_img[lidx + 0] >= threshold) << 31
      | select(min_val, max_val, local_img[lidx + 1] >= threshold) << 30
      | select(min_val, max_val, local_img[lidx + 2] >= threshold) << 29
      | select(min_val, max_val, local_img[lidx + 3] >= threshold) << 28
      | select(min_val, max_val, local_img[lidx + 4] >= threshold) << 27
      | select(min_val, max_val, local_img[lidx + 5] >= threshold) << 26
      | select(min_val, max_val, local_img[lidx + 6] >= threshold) << 25
      | select(min_val, max_val, local_img[lidx + 7] >= threshold) << 24
      | select(min_val, max_val, local_img[lidx + 8] >= threshold) << 23
      | select(min_val, max_val, local_img[lidx + 9] >= threshold) << 22
      | select(min_val, max_val, local_img[lidx + 10] >= threshold) << 21
      | select(min_val, max_val, local_img[lidx + 11] >= threshold) << 20
      | select(min_val, max_val, local_img[lidx + 12] >= threshold) << 19
      | select(min_val, max_val, local_img[lidx + 13] >= threshold) << 18
      | select(min_val, max_val, local_img[lidx + 14] >= threshold) << 17
      | select(min_val, max_val, local_img[lidx + 15] >= threshold) << 16
      | select(min_val, max_val, local_img[lidx + 16] >= threshold) << 15
      | select(min_val, max_val, local_img[lidx + 17] >= threshold) << 14
      | select(min_val, max_val, local_img[lidx + 18] >= threshold) << 13
      | select(min_val, max_val, local_img[lidx + 19] >= threshold) << 12
      | select(min_val, max_val, local_img[lidx + 20] >= threshold) << 11
      | select(min_val, max_val, local_img[lidx + 21] >= threshold) << 10
      | select(min_val, max_val, local_img[lidx + 22] >= threshold) << 9
      | select(min_val, max_val, local_img[lidx + 23] >= threshold) << 8
      | select(min_val, max_val, local_img[lidx + 24] >= threshold) << 7
      | select(min_val, max_val, local_img[lidx + 25] >= threshold) << 6
      | select(min_val, max_val, local_img[lidx + 26] >= threshold) << 5
      | select(min_val, max_val, local_img[lidx + 27] >= threshold) << 4
      | select(min_val, max_val, local_img[lidx + 28] >= threshold) << 3
      | select(min_val, max_val, local_img[lidx + 29] >= threshold) << 2
      | select(min_val, max_val, local_img[lidx + 30] >= threshold) << 1
      | select(min_val, max_val, local_img[lidx + 31] >= threshold) << 0;
  }
}

// Convert a bitmap image to its unbitmap version (one pixel/32bit)
kernel void unbitmap(global const int * in, global int * out, local int * local_img)
{
  const int thread_idx = get_local_id(0);
  const int nb_threads = get_local_size(0);

  // unbitmap to local memory because of coalescing when writing
  // the result back to the output memory.
  { // Unroll
    const int idx = get_global_id(0);
    const int oidx = thread_idx << 5;
    local_img[oidx + 0] = (in[idx] >> 31) & 1;
    local_img[oidx + 1] = (in[idx] >> 30) & 1;
    local_img[oidx + 2] = (in[idx] >> 29) & 1;
    local_img[oidx + 3] = (in[idx] >> 28) & 1;
    local_img[oidx + 4] = (in[idx] >> 27) & 1;
    local_img[oidx + 5] = (in[idx] >> 26) & 1;
    local_img[oidx + 6] = (in[idx] >> 25) & 1;
    local_img[oidx + 7] = (in[idx] >> 24) & 1;
    local_img[oidx + 8] = (in[idx] >> 23) & 1;
    local_img[oidx + 9] = (in[idx] >> 22) & 1;
    local_img[oidx + 10] = (in[idx] >> 21) & 1;
    local_img[oidx + 11] = (in[idx] >> 20) & 1;
    local_img[oidx + 12] = (in[idx] >> 19) & 1;
    local_img[oidx + 13] = (in[idx] >> 18) & 1;
    local_img[oidx + 14] = (in[idx] >> 17) & 1;
    local_img[oidx + 15] = (in[idx] >> 16) & 1;
    local_img[oidx + 16] = (in[idx] >> 15) & 1;
    local_img[oidx + 17] = (in[idx] >> 14) & 1;
    local_img[oidx + 18] = (in[idx] >> 13) & 1;
    local_img[oidx + 19] = (in[idx] >> 12) & 1;
    local_img[oidx + 20] = (in[idx] >> 11) & 1;
    local_img[oidx + 21] = (in[idx] >> 10) & 1;
    local_img[oidx + 22] = (in[idx] >> 9) & 1;
    local_img[oidx + 23] = (in[idx] >> 8) & 1;
    local_img[oidx + 24] = (in[idx] >> 7) & 1;
    local_img[oidx + 25] = (in[idx] >> 6) & 1;
    local_img[oidx + 26] = (in[idx] >> 5) & 1;
    local_img[oidx + 27] = (in[idx] >> 4) & 1;
    local_img[oidx + 28] = (in[idx] >> 3) & 1;
    local_img[oidx + 29] = (in[idx] >> 2) & 1;
    local_img[oidx + 30] = (in[idx] >> 1) & 1;
    local_img[oidx + 31] = (in[idx] >> 0) & 1;
  }
  
  // Write the result back
  { // unroll
    const int gidx = (get_group_id(0) << 5) * nb_threads + thread_idx;
    out[gidx + 0*nb_threads] = local_img[thread_idx + 0*nb_threads];
    out[gidx + 1*nb_threads] = local_img[thread_idx + 1*nb_threads];
    out[gidx + 2*nb_threads] = local_img[thread_idx + 2*nb_threads];
    out[gidx + 3*nb_threads] = local_img[thread_idx + 3*nb_threads];
    out[gidx + 4*nb_threads] = local_img[thread_idx + 4*nb_threads];
    out[gidx + 5*nb_threads] = local_img[thread_idx + 5*nb_threads];
    out[gidx + 6*nb_threads] = local_img[thread_idx + 6*nb_threads];
    out[gidx + 7*nb_threads] = local_img[thread_idx + 7*nb_threads];
    out[gidx + 8*nb_threads] = local_img[thread_idx + 8*nb_threads];
    out[gidx + 9*nb_threads] = local_img[thread_idx + 9*nb_threads];
    out[gidx + 10*nb_threads] = local_img[thread_idx + 10*nb_threads];
    out[gidx + 11*nb_threads] = local_img[thread_idx + 11*nb_threads];
    out[gidx + 12*nb_threads] = local_img[thread_idx + 12*nb_threads];
    out[gidx + 13*nb_threads] = local_img[thread_idx + 13*nb_threads];
    out[gidx + 14*nb_threads] = local_img[thread_idx + 14*nb_threads];
    out[gidx + 15*nb_threads] = local_img[thread_idx + 15*nb_threads];
    out[gidx + 16*nb_threads] = local_img[thread_idx + 16*nb_threads];
    out[gidx + 17*nb_threads] = local_img[thread_idx + 17*nb_threads];
    out[gidx + 18*nb_threads] = local_img[thread_idx + 18*nb_threads];
    out[gidx + 19*nb_threads] = local_img[thread_idx + 19*nb_threads];
    out[gidx + 20*nb_threads] = local_img[thread_idx + 20*nb_threads];
    out[gidx + 21*nb_threads] = local_img[thread_idx + 21*nb_threads];
    out[gidx + 22*nb_threads] = local_img[thread_idx + 22*nb_threads];
    out[gidx + 23*nb_threads] = local_img[thread_idx + 23*nb_threads];
    out[gidx + 24*nb_threads] = local_img[thread_idx + 24*nb_threads];
    out[gidx + 25*nb_threads] = local_img[thread_idx + 25*nb_threads];
    out[gidx + 26*nb_threads] = local_img[thread_idx + 26*nb_threads];
    out[gidx + 27*nb_threads] = local_img[thread_idx + 27*nb_threads];
    out[gidx + 28*nb_threads] = local_img[thread_idx + 28*nb_threads];
    out[gidx + 29*nb_threads] = local_img[thread_idx + 29*nb_threads];
    out[gidx + 30*nb_threads] = local_img[thread_idx + 30*nb_threads];
    out[gidx + 31*nb_threads] = local_img[thread_idx + 31*nb_threads];
  }

}
// MM bimapped operators optimized for radius <= 32
// Don't call it with radius > 32 along the x axis!

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
      local_out[i] = 0;
    else
    {
      const int pidx = mad24(rect_row, in_ncols, rect_col);
      local_out[i] = in[pidx];
    }
  }
}

kernel void bitmapped_dilation_h(global const int * in, global int * out,
                                 const int nrows, const int ncols,
                                 const int se_colrad,
                                 local uint * local_img)
{
  const int x = get_global_id(0);
  const int y = get_global_id(1);
  const int lx = get_local_id(0);
  const int ly = get_local_id(1);
  const int lxsize = get_local_size(0);
  const int lysize = get_local_size(1);
  const int b_ncols = ncols >> 5;

  const int corner_x = get_group_id(0) * lxsize;
  const int corner_y = get_group_id(1) * lysize;

  const int thread_idx = mad24(ly, lxsize, lx);
  const int nb_threads = lxsize * lysize;

  const int padded_nrows = lysize;
  const int padded_ncols = lxsize + 2;

  const int lidx = padded_ncols * ly + lx + 1;
  const int idx = mad24(y, b_ncols, x);
  
  const int size = padded_nrows * padded_ncols;
  
  
  // Copy subimage to local memory
  for (int i = thread_idx; i < size; i += nb_threads)
  {
    const int div = i / padded_ncols;
    const int rect_row = div + corner_y;
    const int rect_col = i - div * padded_ncols + corner_x - 1;

    if (rect_row < 0 || rect_row >= nrows || rect_col < 0 || rect_col >= b_ncols)
    {
      local_img[i] = 0;
    }
    else
    {
      const int pidx = mad24(rect_row, b_ncols, rect_col);
      local_img[i] = in[pidx];
    }
  }
  barrier(CLK_LOCAL_MEM_FENCE);
  
  // Bail out if we are outside the image
  // This is useful if we don't have an image with proper dimensions and we
  // don't want to round them for performance reasons
  if (x >= b_ncols || y >= nrows)
    return;

  // Compute the dilation
  {
    int acc = local_img[lidx];
    for (int i = 1; i <= se_colrad; ++i)
    {
      acc |= (local_img[lidx] >> i) | (local_img[lidx - 1] << (32-i))
          |  (local_img[lidx] << i) | (local_img[lidx + 1] >> (32-i));
    }
    out[idx] = acc;
  }
}

kernel void bitmapped_dilation_v(global const int * in, global int * out,
                                 const int nrows, const int ncols,
                                 const int se_rowrad,
                                 local uint * local_img)
{
  const int x = get_global_id(0);
  const int y = get_global_id(1);
  const int lx = get_local_id(0);
  const int ly = get_local_id(1);
  const int lxsize = get_local_size(0);
  const int lysize = get_local_size(1);
  const int b_ncols = ncols >> 5;

  const int corner_x = get_group_id(0) * lxsize;
  const int corner_y = get_group_id(1) * lysize;

  const int thread_idx = mad24(ly, lxsize, lx);
  const int nb_threads = lxsize * lysize;

  const int padded_nrows = lysize + se_rowrad * 2;
  const int padded_ncols = lxsize;

  const int lidx = padded_ncols * (ly + se_rowrad) + lx;
  const int idx = mad24(y, b_ncols, x);
  
  const int size = padded_nrows * padded_ncols;
  // Copy subimage to local memory
  for (int i = thread_idx; i < size; i += nb_threads)
  {
    const int div = i / padded_ncols;
    const int rect_row = div + corner_y - se_rowrad;
    const int rect_col = i - div * padded_ncols + corner_x;

    if (rect_row < 0 || rect_row >= nrows || rect_col < 0 || rect_col >= b_ncols)
    {
      local_img[i] = 0;
    }
    else
    {
      const int pidx = mad24(rect_row, b_ncols, rect_col);
      local_img[i] = in[pidx];
    }
  }
  barrier(CLK_LOCAL_MEM_FENCE);
  
  // Bail out if we are outside the image
  // This is useful if we don't have an image with proper dimensions and we
  // don't want to round them for performance reasons
  if (x >= b_ncols || y >= nrows)
    return;

  // Compute the dilation
  {
    int slidx = mad24(padded_ncols, -se_rowrad, lidx);
    int acc = local_img[lidx];
    for (int r = -se_rowrad; r <= se_rowrad; ++r)
    {
      acc |= local_img[slidx];
      slidx += padded_ncols;
    }
    // Save the results
    out[idx] = acc;    
  }
}

kernel void bitmapped_erosion_h(global const int * in, global int * out,
                                const int nrows, const int ncols,
                                const int se_colrad,
                                local uint * local_img)
{
  const int x = get_global_id(0);
  const int y = get_global_id(1);
  const int lx = get_local_id(0);
  const int ly = get_local_id(1);
  const int lxsize = get_local_size(0);
  const int lysize = get_local_size(1);
  const int b_ncols = ncols >> 5;

  const int corner_x = get_group_id(0) * lxsize;
  const int corner_y = get_group_id(1) * lysize;

  const int thread_idx = mad24(ly, lxsize, lx);
  const int nb_threads = lxsize * lysize;

  const int padded_nrows = lysize;
  const int padded_ncols = lxsize + 2;

  const int lidx = padded_ncols * ly + lx + 1;
  const int idx = mad24(y, b_ncols, x);
  
  const int size = padded_nrows * padded_ncols;
  
  
  // Copy subimage to local memory
  for (int i = thread_idx; i < size; i += nb_threads)
  {
    const int div = i / padded_ncols;
    const int rect_row = div + corner_y;
    const int rect_col = i - div * padded_ncols + corner_x - 1;

    if (rect_row < 0 || rect_row >= nrows || rect_col < 0 || rect_col >= b_ncols)
    {
      local_img[i] = 0;
    }
    else
    {
      const int pidx = mad24(rect_row, b_ncols, rect_col);
      local_img[i] = in[pidx];
    }
  }
  barrier(CLK_LOCAL_MEM_FENCE);
  
  // Bail out if we are outside the image
  // This is useful if we don't have an image with proper dimensions and we
  // don't want to round them for performance reasons
  if (x >= b_ncols || y >= nrows)
    return;

  // Compute the erosion
  {
    int acc = local_img[lidx];
    for (int i = 1; i <= se_colrad; ++i)
    {
      acc &= ((local_img[lidx] >> i) | (local_img[lidx - 1] << (32-i)))
          &  ((local_img[lidx] << i) | (local_img[lidx + 1] >> (32-i)));
    }
    out[idx] = acc;
  }
}

kernel void bitmapped_erosion_v(global const int * in, global int * out,
                                const int nrows, const int ncols,
                                const int se_rowrad,
                                local uint * local_img)
{
  const int x = get_global_id(0);
  const int y = get_global_id(1);
  const int lx = get_local_id(0);
  const int ly = get_local_id(1);
  const int lxsize = get_local_size(0);
  const int lysize = get_local_size(1);
  const int b_ncols = ncols >> 5;

  const int corner_x = get_group_id(0) * lxsize;
  const int corner_y = get_group_id(1) * lysize;

  const int thread_idx = mad24(ly, lxsize, lx);
  const int nb_threads = lxsize * lysize;

  const int padded_nrows = lysize + se_rowrad * 2;
  const int padded_ncols = lxsize;

  const int lidx = padded_ncols * (ly + se_rowrad) + lx;
  const int idx = mad24(y, b_ncols, x);
  
  const int size = padded_nrows * padded_ncols;
  // Copy subimage to local memory
  for (int i = thread_idx; i < size; i += nb_threads)
  {
    const int div = i / padded_ncols;
    const int rect_row = div + corner_y - se_rowrad;
    const int rect_col = i - div * padded_ncols + corner_x;

    if (rect_row < 0 || rect_row >= nrows || rect_col < 0 || rect_col >= b_ncols)
    {
      local_img[i] = 0;
    }
    else
    {
      const int pidx = mad24(rect_row, b_ncols, rect_col);
      local_img[i] = in[pidx];
    }
  }
  barrier(CLK_LOCAL_MEM_FENCE);
  
  // Bail out if we are outside the image
  // This is useful if we don't have an image with proper dimensions and we
  // don't want to round them for performance reasons
  if (x >= b_ncols || y >= nrows)
    return;

  // Compute the erosion
  {
    int slidx = mad24(padded_ncols, -se_rowrad, lidx);
    int acc = local_img[lidx];
    for (int r = -se_rowrad; r <= se_rowrad; ++r)
    {
      acc &= local_img[slidx];
      slidx += padded_ncols;
    }
    // Save the results
    out[idx] = acc;    
  }
}
