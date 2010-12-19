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

#ifndef CLCV_CLINIT_H__
#define CLCV_CLINIT_H__

#include <string>
#include <OpenCL/OpenCL.h>
#define __CL_ENABLE_EXCEPTIONS
#include <clcv/cl.hpp>

namespace clcv
{

  cl_device_id get_device(cl_device_type device_type);
  cl_device_id get_cpu_device();
  cl_device_id get_gpu_device();
  cl_device_id get_device_fallback(cl_device_type device_type = CL_DEVICE_TYPE_GPU);
  
  cl::Context get_context(cl_device_id device);
  cl::CommandQueue get_command_queue(cl::Context & context);
  
  cl::Program load_program(cl::Context & context, const char * path, const char * options = NULL);
  
  cl::Buffer create_buffer(cl::Context & context, size_t size, cl_mem_flags flags);
  cl::Event write_mem(cl::CommandQueue & cmd_queue,
                      cl::Buffer & dest, const void * src, size_t size,
                      cl_bool blocking = CL_FALSE);
  cl::Event read_mem(cl::CommandQueue & cmd_queue,
                     void * dest, const cl::Buffer & src, size_t size,
                     cl_bool blocking = CL_FALSE);
}
  
#endif
