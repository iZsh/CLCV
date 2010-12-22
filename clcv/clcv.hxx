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

#ifndef CLCV_CLCV_HXX__
#define CLCV_CLCV_HXX__

#include <vector>
#include <cstdlib>
#include <clcv/clinit.h>
#include <clcv/clcv.h>

using namespace std;

namespace clcv
{
  template<typename T>
  inline
  CLCV<T>::CLCV(cl_device_type device_type)
  : m_device_id(0), m_context(), m_queue(), m_program(),
  m_global_work_size(cl::NullRange), m_local_work_size(cl::NullRange),
  m_bufferpairs(),
  m_images(), m_next_image_id(1), m_current_image_id(0), m_ses(), m_next_se_id(0)
  {
    m_device_id = get_device_fallback(device_type);
    m_context = clcv::get_context(m_device_id);
    m_queue = get_command_queue(m_context);
    m_program = load_program(m_context, "clcv/clcv.cl");
  }

  template<typename T>
  inline
  CLCV<T>::~CLCV()
  {
    for (typename clcv_image_map::iterator it = m_images.begin(); it != m_images.end(); ++it)
      delete[] it->second.data;
    m_images.clear();
    m_ses.clear();
  }

  template<typename T>
  inline
  cl_device_id CLCV<T>::get_device_id() const
  {
    return m_device_id;
  }
  
  template<typename T>
  inline
  cl_device_type CLCV<T>::get_device_type() const
  {
    vector<cl::Device> devices = m_context.getInfo<CL_CONTEXT_DEVICES>();
    assert(devices.size() == 1);
    return devices[0].getInfo<CL_DEVICE_TYPE>();
  }

  
  template<typename T>
  inline
  cl::Context & CLCV<T>::get_context()
  {
    return m_context;
  }
  
  template<typename T>
  inline
  cl::CommandQueue & CLCV<T>::get_queue()
  {
    return m_queue;
  }
  
  template<typename T>
  inline
  void CLCV<T>::flush()
  {
    m_queue.flush();
  }

  template<typename T>
  inline
  void CLCV<T>::finish()
  {
    m_queue.finish();
  }
  
  template<typename T>
  inline
  void CLCV<T>::set_global_work_size(const cl::NDRange & global_work_size)
  {
    m_global_work_size = global_work_size;
  }
  
  template<typename T>
  inline
  void CLCV<T>::set_local_work_size(const cl::NDRange & local_work_size)
  {
    const cl::NDRange unit_range(1,1);
    m_local_work_size = get_device_type() == CL_DEVICE_TYPE_CPU ? unit_range : local_work_size;
  }
  
  template<typename T>
  inline
  const cl::NDRange & CLCV<T>::get_global_work_size() const
  {
    return m_global_work_size;
  }
  
  template<typename T>
  inline
  const cl::NDRange & CLCV<T>::get_local_work_size() const
  {
    return m_local_work_size;
  }
  
  template<typename T>
  inline
  typename CLCV<T>::clcv_bufferpair CLCV<T>::get_bufferpair(unsigned size)
  {
    clcv_bufferpairmap::iterator it = m_bufferpairs.find(size);
    if (it != m_bufferpairs.end())
    {
      return it->second;
    }
    cl::Buffer a_mem(get_context(), CL_MEM_READ_WRITE, size);
    cl::Buffer b_mem(get_context(), CL_MEM_READ_WRITE, size);
    clcv_bufferpair bufferpair(a_mem, b_mem);
    m_bufferpairs[size] = bufferpair;
    return bufferpair;
  }
  
  template<typename T>
  inline
  void CLCV<T>::clear_bufferpairs()
  {
    m_bufferpairs.clear();
  }
  
  template<typename T>
  inline
  clcv_image_id CLCV<T>::open(const image2d<T> & img)
  {
    unsigned size = img.nrows() * img.ncols() * sizeof (T);
    T * data = new T[size];
    { // Copy the image
      int i = 0;
      for (coord r = 0; r < img.nrows(); ++r)
        for (coord c = 0; c < img.ncols(); ++c)
          data[i++] = img(r, c);
    }
    clcv_image image =
      {
        data, img.nrows(), img.ncols(),
        get_bufferpair(size),
        cl::NDRange(img.ncols(), img.nrows())
      };
    unsigned id = m_next_image_id++;
    m_images[id] = image;
    m_current_image_id = id;
    // Enqueue
    write_mem(m_queue, get_in_buffer(), data, size);
    return id;
  }

  template<typename T>
  inline
  cl::Event CLCV<T>::fetch(const size_t size)
  {
    clcv_image & img = get_image();
    return read_mem(m_queue, img.data, get_in_buffer(), size);    
  }

  template<typename T>
  inline
  cl::Event CLCV<T>::fetch()
  {
    clcv_image & img = get_image();
    return read_mem(m_queue, img.data, get_in_buffer(),
                    img.nrows * img.ncols * sizeof (T));    
  }
  
  template<typename T>
  inline
  image2d<T> CLCV<T>::save(clcv_image_id image_id)
  {
    clcv_image & imgsrc = get_image(image_id);
    image2d<T> img(imgsrc.nrows, imgsrc.ncols);
    assert(imgsrc.nrows == img.nrows() && imgsrc.ncols == img.ncols());
    for (unsigned r = 0; r < imgsrc.nrows; ++r)
      for (unsigned c = 0; c < imgsrc.ncols; ++c)
        img(r, c) = imgsrc.data[r * imgsrc.ncols + c];
    return img;
  }
  
  template<typename T>
  inline
  void CLCV<T>::close(clcv_image_id image_id)
  {
    typename clcv_image_map::iterator it = m_images.find(image_id);
    if (it != m_images.end())
    {
      delete[] it->second.data;
      m_images.erase(it);
    }
    m_current_image_id = 0;
  }

  template<typename T>
  inline
  unsigned CLCV<T>::get_nrows()
  {
    return get_image().nrows;
  }

  template<typename T>
  inline
  unsigned CLCV<T>::get_ncols()
  {
    return get_image().ncols;
  }
  
  template<typename T>
  inline
  cl::Buffer & CLCV<T>::get_in_buffer()
  {
    return get_in_buffer(m_current_image_id);
  }

  template<typename T>
  inline
  cl::Buffer & CLCV<T>::get_out_buffer()
  {
    return get_out_buffer(m_current_image_id);
  }
  
  template<typename T>
  inline
  clcv_se_id CLCV<T>::load_se(const win2d<T> & win)
  {
    size_t size = win.count() * sizeof (T) * 3;
    T * buffer = new T[size];
    
    // convert the window to an array of coord
    unsigned i = 0;
    for (typename win2d<T>::const_iter it = win.begin(); it != win.end(); ++it)
    {
      buffer[i++] = it->first.row();
      buffer[i++] = it->first.col();
      buffer[i++] = it->second;
    }
    // And finally allocate the cl_mem
    cl::Buffer se_mem(get_context(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, size, buffer);
    // Save it
    clcv_se se = { se_mem, win.nrows(), win.ncols(), win.maxrow(), win.maxcol(), size, win.count() };
    // Clean it
    delete[] buffer;
    // and return it
    clcv_se_id id = m_next_se_id++;
    m_ses[id] = se;
    return id;
  }
  
  template<typename T>
  inline
  void CLCV<T>::unload_se(clcv_se_id se_id)
  {
    typename clcv_se_map::iterator it = m_ses.find(se_id);
    if (it != m_ses.end())
      m_ses.erase(se_id);
  }

  template<typename T>
  inline
  typename CLCV<T>::clcv_image & CLCV<T>::get_image(clcv_image_id image_id)
  {
    typename clcv_image_map::iterator it = m_images.find(image_id);
    assert(it != m_images.end());
    return it->second;
  }
  
  template<typename T>
  inline
  typename CLCV<T>::clcv_image & CLCV<T>::get_image()
  {
    return get_image(m_current_image_id);
  }
  
  template<typename T>
  inline
  typename CLCV<T>::clcv_se & CLCV<T>::get_se(clcv_se_id se_id)
  {
    typename clcv_se_map::iterator it = m_ses.find(se_id);
    assert(it != m_ses.end());
    return it->second;
  }

  template<typename T>
  inline
  cl::Buffer & CLCV<T>::get_in_buffer(clcv_image_id image_id)
  {
    return get_image(image_id).buffers.first;
  }

  template<typename T>
  inline
  cl::Buffer & CLCV<T>::get_out_buffer(clcv_image_id image_id)
  {
    return get_image(image_id).buffers.second;
  }

  template<typename T>
  inline
  void CLCV<T>::swap_buffers(clcv_image_id image_id)
  {
    cl::Buffer in = get_in_buffer(image_id);
    get_in_buffer(image_id) = get_out_buffer(image_id);
    get_out_buffer(image_id) = in;
  }
  
  template<typename T>
  inline
  void CLCV<T>::swap_buffers()
  {
    swap_buffers(m_current_image_id);
  }

  template<typename T>
  inline
  cl::Kernel CLCV<T>::create_unbitmap(const cl::Buffer & image_in, const cl::Buffer & image_out,
                                      const cl_int nrows, const cl_int ncols)
  {
    cl::Kernel kernel(m_program, "unbitmap");
    kernel.setArg(0, image_in);
    kernel.setArg(1, image_out);
    return kernel;    
  }
  
  template<typename T>
  inline
  cl::Event CLCV<T>::push_unbitmap()
  {
    const int size = get_image().nrows * get_image().ncols;
    if (get_device_type() == CL_DEVICE_TYPE_CPU)
      assert(size % 32 == 0);
    else
      assert(size % (32*64) == 0);

    const cl::NDRange unit_range(1);
    const cl::NDRange parallel_range(64);
    const cl::NDRange g_size(size / 32);
    
    const cl::NDRange & l_size =
    get_device_type() == CL_DEVICE_TYPE_CPU ? unit_range : parallel_range;
    
    cl::Kernel kernel = create_unbitmap(get_in_buffer(), get_out_buffer(),
                                        get_nrows(), get_ncols());
    swap_buffers();
    cl::Event event;
    m_queue.enqueueNDRangeKernel(kernel, cl::NullRange, g_size, l_size, NULL, &event);
    return event;
  }
  
  template<typename T>
  inline
  cl::Kernel CLCV<T>::create_binarize(const cl::Buffer & image_in, const cl::Buffer & image_out,
                                      const cl_int nrows, const cl_int ncols,
                                      const cl_int threshold, const cl_int min, const cl_int max)
  {
    cl::Kernel kernel(m_program, "binarize");
    kernel.setArg(0, image_in);
    kernel.setArg(1, image_out);
    kernel.setArg(2, nrows);
    kernel.setArg(3, ncols);
    kernel.setArg(4, threshold);
    kernel.setArg(5, min);
    kernel.setArg(6, max);
    return kernel;    
  }
  
  template<typename T>
  inline
  cl::Event CLCV<T>::push_binarize(const cl_int threshold, const cl_int min, const cl_int max)
  {
    const cl::NDRange & l_size = get_local_work_size();
    const cl::NDRange & g_size = get_global_work_size().dimensions() == 0 ?
      get_image().global_work_size
    : get_global_work_size();

    cl::Kernel kernel = create_binarize(get_in_buffer(), get_out_buffer(),
                                        get_nrows(), get_ncols(),
                                        threshold, min, max);
    swap_buffers();
    cl::Event event;
    m_queue.enqueueNDRangeKernel(kernel, cl::NullRange, g_size, l_size, NULL, &event);
    return event;
  }

  template<typename T>
  inline
  cl::Kernel CLCV<T>::create_bitmappedbinarize(const cl::Buffer & image_in, const cl::Buffer & image_out,
                                               const cl_int nrows, const cl_int ncols,
                                               const cl_int threshold, const bool inverted)
  {
    const int nb_threads = get_device_type() == CL_DEVICE_TYPE_CPU ? 1 : 64;
    assert((nrows*ncols) % (32*nb_threads) == 0);

    cl::Kernel kernel(m_program, "bitmappedbinarize");
    kernel.setArg(0, image_in);
    kernel.setArg(1, image_out);
    kernel.setArg(2, threshold);
    kernel.setArg(3, inverted ? 1 : 0);
    kernel.setArg(4, inverted ? 0 : 1);
    kernel.setArg(5, 32 * nb_threads * sizeof (cl_int), NULL);
    return kernel;
  }

  template<typename T>
  inline
  cl::Event CLCV<T>::push_bitmappedbinarize(const cl_int threshold, const bool inverted)
  {
    const int size = get_image().nrows * get_image().ncols;
    if (get_device_type() == CL_DEVICE_TYPE_CPU)
      assert(size % 32 == 0);
    else
      assert(size % (32*64) == 0);

    const cl::NDRange unit_range(1);
    const cl::NDRange parallel_range(64);
    const cl::NDRange g_size(size / 32);

    const cl::NDRange & l_size =
    get_device_type() == CL_DEVICE_TYPE_CPU ? unit_range : parallel_range;
    
    cl::Kernel kernel = create_bitmappedbinarize(get_in_buffer(), get_out_buffer(),
                                                 get_nrows(), get_ncols(),
                                                 threshold, inverted);
    swap_buffers();
    cl::Event event;
    m_queue.enqueueNDRangeKernel(kernel, cl::NullRange, g_size, l_size, NULL, &event);
    return event;
  }
  
  template<typename T>
  inline
  cl::Kernel CLCV<T>::create_naivemorph(const cl::Buffer & image_in,
                                        const cl::Buffer & image_out,
                                        const cl_int nrows, const cl_int ncols,
                                        const clcv_se_id se_id, const cl_int se_targetsum,
                                        const cl::NDRange & local_work_size)
  {
    clcv_se & se = get_se(se_id);
    
    cl::Kernel kernel(m_program, "naivemorph");
    kernel.setArg(0, image_in);
    kernel.setArg(1, image_out);
    kernel.setArg(2, nrows);
    kernel.setArg(3, ncols);
    kernel.setArg(4, se.clmem);
    kernel.setArg(5, se.rowrad);
    kernel.setArg(6, se.colrad);
    kernel.setArg(7, se.se_nonzero);
    kernel.setArg(8, se_targetsum);
    kernel.setArg(9, se.buffer_size, NULL);
    cl_int local_size = (((const size_t*)local_work_size)[0] + se.colrad*2)
      * (((const size_t*)local_work_size)[1] + se.rowrad*2) * sizeof (T);
    kernel.setArg(10, local_size, NULL);
    
    return kernel;
  }

  template<typename T>
  inline
  cl::Event CLCV<T>::push_naivemorph(const clcv_se_id se_id, const cl_int se_targetsum)
  {
    const cl::NDRange & l_size = get_local_work_size();
    const cl::NDRange & g_size = get_global_work_size().dimensions() == 0 ?
    get_image().global_work_size
    : get_global_work_size();

    cl::Kernel kernel = create_naivemorph(get_in_buffer(), get_out_buffer(),
                                          get_nrows(), get_ncols(),
                                          se_id, se_targetsum,
                                          l_size);
    swap_buffers();
    cl::Event event;
    m_queue.enqueueNDRangeKernel(kernel, cl::NullRange, g_size, l_size, NULL, &event);
    return event;    
  }

  template<typename T>
  inline
  cl::Kernel CLCV<T>::create_naivedilation(const cl::Buffer & image_in,
                                           const cl::Buffer & image_out,
                                           const cl_int nrows, const cl_int ncols,
                                           const clcv_se_id se_id,
                                           const cl::NDRange & local_work_size)
  {
    clcv_se & se = get_se(se_id);
    cl_int se_nonzero = se.se_nonzero;

    return create_naivemorph(image_in, image_out, nrows, ncols,
                             se_id, -se_nonzero+1, local_work_size);
  }

  template<typename T>
  inline
  cl::Event CLCV<T>::push_naivedilation(const clcv_se_id se_id)
  {
    clcv_se & se = get_se(se_id);
    cl_int se_nonzero = se.se_nonzero;

    return push_naivemorph(se_id, -se_nonzero+1);
  }

  template<typename T>
  inline
  cl::Kernel CLCV<T>::create_naiveerosion(const cl::Buffer & image_in,
                                          const cl::Buffer & image_out,
                                          const cl_int nrows, const cl_int ncols,
                                          const clcv_se_id se_id,
                                          const cl::NDRange & local_work_size)
  {
    clcv_se & se = get_se(se_id);
    cl_int se_nonzero = se.se_nonzero;
    
    return create_naivemorph(image_in, image_out, nrows, ncols,
                             se_id, se_nonzero, local_work_size);
  }
  
  template<typename T>
  inline
  cl::Event CLCV<T>::push_naiveerosion(const clcv_se_id se_id)
  {
    clcv_se & se = get_se(se_id);
    cl_int se_nonzero = se.se_nonzero;
    
    return push_naivemorph(se_id, se_nonzero);
  }

  template<typename T>
  inline
  cl::Event CLCV<T>::push_naiveopening(const clcv_se_id se_id)
  {
    push_naiveerosion(se_id);
    return push_naivedilation(se_id);
  }
  
  template<typename T>
  inline
  cl::Event CLCV<T>::push_naiveclosing(const clcv_se_id se_id)
  {
    push_naivedilation(se_id);
    return push_naiveerosion(se_id);
  }

}

#endif
