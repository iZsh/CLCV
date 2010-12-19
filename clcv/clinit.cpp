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

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <assert.h>
#include <clcv/clinit.h>

using namespace std;

namespace clcv
{
  
  cl_device_id get_device(cl_device_type device_type)
  {
    cl_device_id device = 0;
    clGetDeviceIDs(NULL, device_type, 1, &device, NULL);
    return device;  
  }
  
  cl_device_id get_cpu_device()
  {
    cl_device_id device;
    cl_int err = 0;
    err = clGetDeviceIDs(NULL, CL_DEVICE_TYPE_CPU, 1, &device, NULL);
    assert(err == CL_SUCCESS);
    return device;
  }
  
  cl_device_id get_gpu_device()
  {
    return get_device(CL_DEVICE_TYPE_GPU);
  }
  
  cl_device_id get_device_fallback(cl_device_type device_type)
  {
    cl_device_id device = get_device(device_type);
    return device ? device : get_cpu_device(); 
  }
  
  cl::Context get_context(cl_device_id device)
  {
    vector<cl::Device> devices;
    devices.push_back(device);
    
    return cl::Context(devices);
  }
  
  cl::CommandQueue get_command_queue(cl::Context & context)
  {
    vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
    return cl::CommandQueue(context, devices[0], CL_QUEUE_PROFILING_ENABLE);
  }
  
  cl::Program load_program(cl::Context & context, const char * path, const char * options)
  {  
    ifstream file(path);
    if (!file.is_open())
    {
      string msg("Failed to open kernel file '");
      msg += path;
      msg += "'";
      throw runtime_error(msg);
    }
    
    string prog(istreambuf_iterator<char>(file), (istreambuf_iterator<char>()));
    
    cl::Program::Sources source(1, make_pair(prog.c_str(), prog.length()));
    cl::Program program = cl::Program(context, source);
    vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
    try {
      program.build(devices, options);
    } catch(cl::Error & err) {
      // Get the build log
      cerr << "Build failed! " << endl;
      cerr << "Build log:" << endl;
      cerr << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << endl;
      throw err;
    }
    
    return program;
  }
  
  cl::Buffer create_buffer(cl::Context & context, size_t size, cl_mem_flags flags)
  {
    return cl::Buffer(context, flags, size);
  }
  
  cl::Event write_mem(cl::CommandQueue & cmd_queue,
                      cl::Buffer & dest, const void * src, size_t size,
                      cl_bool blocking)
  {
    cl::Event event;
    cmd_queue.enqueueWriteBuffer(dest, blocking, 0, size, src, NULL, &event);
    return event;
  }
  
  cl::Event read_mem(cl::CommandQueue & cmd_queue,
                     void * dest, const cl::Buffer & src, size_t size,
                     cl_bool blocking)
  {
    cl::Event event;
    cmd_queue.enqueueReadBuffer(src, blocking, 0, size, dest, NULL, &event);
    return event;
  }

}
