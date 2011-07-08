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

#ifndef CLCV_CLCV_H__
#define CLCV_CLCV_H__

#include <map>
#define __CL_ENABLE_EXCEPTIONS
#include <clcv/cl.hpp>
#include <clcv/win2d.h>
#include <clcv/image2d.h>

namespace clcv
{
  typedef unsigned clcv_image_id;
  typedef unsigned clcv_se_id;

  template<typename T>
  class CLCV
  {
  public:
    CLCV(cl_device_type device_type = CL_DEVICE_TYPE_GPU);
    ~CLCV();
    
    // General
    cl_device_id get_device_id() const;
    cl_device_type get_device_type() const;
    cl::Context & get_context();
    cl::CommandQueue & get_queue();
    void flush();
    void finish();
    void set_global_work_size(const cl::NDRange & global_work_size);
    void set_local_work_size(const cl::NDRange & local_work_size);
    const cl::NDRange & get_global_work_size() const;
    const cl::NDRange & get_local_work_size() const;

    // Image related
    clcv_image_id open(const image2d<T> & img);
    cl::Event fetch(const size_t size);
    cl::Event fetch();
    image2d<T> save(clcv_image_id image_id);
    void close(clcv_image_id image_id);
    unsigned get_nrows();
    unsigned get_ncols();
    cl::Buffer & get_in_buffer();
    cl::Buffer & get_out_buffer();
    
    // SE related
    clcv_se_id load_se(const win2d<T> & win);
    void unload_se(clcv_se_id se_id);

    // Kernel related

    cl::Kernel create_unbitmap(const cl::Buffer & image_in, const cl::Buffer & image_out,
                               const cl_int nrows, const cl_int ncols);
    
    cl::Event push_unbitmap();

    // Binarization
    cl::Kernel create_binarize(const cl::Buffer & image_in, const cl::Buffer & image_out,
                               const cl_int nrows, const cl_int ncols,
                               const cl_int threshold, const cl_int min, const cl_int max);
    cl::Event push_binarize(const cl_int threshold, const cl_int min, const cl_int max);
    
    // Bitmapped binarization
    cl::Kernel create_bitmappedbinarize(const cl::Buffer & image_in, const cl::Buffer & image_out,
                               const cl_int nrows, const cl_int ncols,
                               const cl_int threshold, const bool inverted = false);
    cl::Event push_bitmappedbinarize(const cl_int threshold, const bool inverted = false);
    
    // Naive morpho implementations:
    // - One pixel per T
    // - Iterate through the SE for each pixel
    cl::Kernel create_naivemorph(const cl::Buffer & image_in, const cl::Buffer & image_out,
                                 const cl_int nrows, const cl_int ncols,
                                 const clcv_se_id se_id, const cl_int se_targetsum,
                                 const cl::NDRange & local_work_size);
    cl::Event push_naivemorph(const clcv_se_id se_id, const cl_int se_targetsum);
    cl::Kernel create_naivedilation(const cl::Buffer & image_in, const cl::Buffer & image_out,
                                    const cl_int nrows, const cl_int ncols,
                                    const clcv_se_id se_id,
                                    const cl::NDRange & local_work_size);
    cl::Event push_naivedilation(const clcv_se_id se_id);
    cl::Kernel create_naiveerosion(const cl::Buffer & image_in, const cl::Buffer & image_out,
                                   const cl_int nrows, const cl_int ncols,
                                   const clcv_se_id se_id,
                                   const cl::NDRange & local_work_size);
    cl::Event push_naiveerosion(const clcv_se_id se_id);
    cl::Event push_naiveopening(const clcv_se_id se_id);
    cl::Event push_naiveclosing(const clcv_se_id se_id);
    
    // Bitmapped implementation
    cl::Kernel create_bitmappedmorph_dilation_h(const cl::Buffer & image_in,
                                                const cl::Buffer & image_out,
                                                const cl_int nrows, const cl_int ncols,
                                                const cl_int se_colrad,
                                                const cl::NDRange & local_work_size);
    cl::Event push_bitmappedmorph_dilation_h(const cl_int se_colrad);

    cl::Kernel create_bitmappedmorph_dilation_v(const cl::Buffer & image_in,
                                                  const cl::Buffer & image_out,
                                                  const cl_int nrows, const cl_int ncols,
                                                  const cl_int se_rowrad,
                                                  const cl::NDRange & local_work_size);
    cl::Event push_bitmappedmorph_dilation_v(const cl_int se_rowrad);

    cl::Event push_bitmappedmorph_dilation(const cl_int se_rowrad, const cl_int se_colrad);
    
    cl::Kernel create_bitmappedmorph_erosion_h(const cl::Buffer & image_in,
                                               const cl::Buffer & image_out,
                                               const cl_int nrows, const cl_int ncols,
                                               const cl_int se_colrad,
                                               const cl::NDRange & local_work_size);
    cl::Event push_bitmappedmorph_erosion_h(const cl_int se_colrad);
    
    cl::Kernel create_bitmappedmorph_erosion_v(const cl::Buffer & image_in,
                                               const cl::Buffer & image_out,
                                               const cl_int nrows, const cl_int ncols,
                                               const cl_int se_rowrad,
                                               const cl::NDRange & local_work_size);
    cl::Event push_bitmappedmorph_erosion_v(const cl_int se_rowrad);

    cl::Event push_bitmappedmorph_erosion(const cl_int se_rowrad, const cl_int se_colrad);

    cl::Event push_bitmappedmorph_opening(const cl_int se_rowrad, const cl_int se_colrad);
    cl::Event push_bitmappedmorph_closing(const cl_int se_rowrad, const cl_int se_colrad);

  private: struct clcv_image;
  private: struct clcv_se;
  protected:
    // image
    clcv_image & get_image(clcv_image_id image_id);
    clcv_image & get_image();
    // se
    clcv_se & get_se(clcv_se_id se_id);
    // buffers
    typedef std::pair<cl::Buffer, cl::Buffer> clcv_bufferpair;
    clcv_bufferpair get_bufferpair(unsigned size);
    void clear_bufferpairs();
    cl::Buffer & get_in_buffer(clcv_image_id image_id);
    cl::Buffer & get_out_buffer(clcv_image_id image_id);
    void swap_buffers(clcv_image_id image_id);
    void swap_buffers();
    
    // Utilities
    static int round(int v, int r);
    static size_t xdim(const cl::NDRange & ndrange);
    static size_t ydim(const cl::NDRange & ndrange);

  private:
    CLCV(const CLCV &);
    CLCV & operator=(const CLCV &);
    
    cl_device_id m_device_id;
    cl::Context m_context;
    cl::CommandQueue m_queue;
    cl::Program m_program;
    
    cl::NDRange m_global_work_size;
    cl::NDRange m_local_work_size;    

    typedef std::map<unsigned, clcv_bufferpair> clcv_bufferpairmap;
    clcv_bufferpairmap m_bufferpairs;
    
    struct clcv_image {
      T * data;
      unsigned nrows;
      unsigned ncols;
      clcv_bufferpair buffers;
      cl::NDRange global_work_size;
    };
    
    struct clcv_se {
      cl::Buffer clmem;
      unsigned nrows;
      unsigned ncols;
      unsigned rowrad;
      unsigned colrad;
      unsigned buffer_size;
      unsigned se_nonzero;
    };
    
    typedef std::map<clcv_image_id, clcv_image> clcv_image_map;
    clcv_image_map m_images;
    clcv_image_id m_next_image_id;
    clcv_image_id m_current_image_id;
    
    typedef std::map<clcv_se_id, clcv_se> clcv_se_map;
    clcv_se_map m_ses;
    clcv_se_id m_next_se_id;
  };
}

#include <clcv/clcv.hxx>

#endif
