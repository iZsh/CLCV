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
#include <assert.h>
#include <clcv/clinfo.h>

using namespace std;

void platform_info(cl_platform_id platform_id)
{
  int err;
  size_t returned_size;
  
  cl_char platform_version[1024] = {0};
  cl_char platform_name[1024] = {0};
  cl_char platform_vendor[1024] = {0};
  cl_char platform_extensions[1024] = {0};
  
  err = clGetPlatformInfo(platform_id, CL_PLATFORM_VERSION, sizeof (platform_version), platform_version, &returned_size);
  err|= clGetPlatformInfo(platform_id, CL_PLATFORM_NAME, sizeof (platform_name), platform_name, &returned_size);
  err|= clGetPlatformInfo(platform_id, CL_PLATFORM_VENDOR, sizeof (platform_vendor), platform_vendor, &returned_size);
  err|= clGetPlatformInfo(platform_id, CL_PLATFORM_EXTENSIONS, sizeof (platform_extensions), platform_extensions, &returned_size);

  cout << "===============================================================" << endl;
  cout << "Platform Version: " << platform_version << endl;
  cout << "Platform Name: " << platform_name << endl;
  cout << "Platform Vendor: " << platform_name << endl;
  cout << "Platform Extensions: " << platform_extensions << endl;
  cout << "===============================================================" << endl;  
}

void get_all_platforms_info()
{
  cl_uint nb_platforms = 0;
  cl_int err = 0;
  cl_platform_id platforms[16] = {0};
  
  err = clGetPlatformIDs(16, platforms, &nb_platforms);
  if (err != CL_SUCCESS) {
    cout << "Cannot get the platforms listing" << endl;
    return;
  }
	
  for (cl_uint i = 0; i < nb_platforms; ++i)
    platform_info(platforms[i]);
}

std::string get_device_name(cl_device_id device)
{
  cl_char vendor_name[1024] = {0};
  cl_char device_name[1024] = {0};
  cl_int err = clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(vendor_name),
                               vendor_name, NULL);
  err |= clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name),
                         device_name, NULL);
  assert(err == CL_SUCCESS);
  string name((const char *)vendor_name);
  name += " ";
  name += (const char *)device_name;
  return name;
}

void device_info(cl_device_id device_id)
{
  int err;
	size_t returned_size;
	
  // Report the device vendor and device name
  // 
  cl_char vendor_name[1024] = {0};
  cl_char device_name[1024] = {0};
  cl_char device_profile[1024] = {0};
  cl_uint clock_frequency = 0;
  cl_char device_extensions[1024] = {0};
  size_t profiling_timer_resolution = 0;
  cl_uint vector_types[] =
  {
    CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR,
    CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT,
    CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT,
    CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG,
    CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT,
    CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE
  }; 
  const char *vector_type_names[] = {"char","short","int","long","float","double"};
  cl_uint vector_width = 0;
  
  cl_uint device_address_bits = 0;
  cl_uint mem_base_addr_align = 0;
  cl_uint min_data_type_align_size = 0;
  cl_ulong global_mem_size = 0;
  cl_ulong global_mem_cache_size = 0;
  cl_ulong max_mem_alloc_size = 0;
  cl_device_local_mem_type local_mem_type = 0;
  cl_ulong local_mem_size = 0;
  cl_ulong max_constant_buffer_size = 0;
  
  cl_uint max_compute_units;
  size_t max_work_item_dims = 0;
  size_t max_work_group_size = 0;
  size_t max_work_item_sizes[3] = {0};
  
  cl_bool image_support = false;
  cl_uint max_read_image_args = 0, max_write_image_args = 0;
  size_t image2d_max_width = 0, image2d_max_height = 0;
  cl_uint max_samplers = 0;
  
  // General
  err = clGetDeviceInfo(device_id, CL_DEVICE_VENDOR, sizeof(vendor_name), vendor_name, &returned_size);
  err|= clGetDeviceInfo(device_id, CL_DEVICE_NAME, sizeof(device_name), device_name, &returned_size);
  err|= clGetDeviceInfo(device_id, CL_DEVICE_PROFILE, sizeof(device_profile), device_profile, &returned_size);
  err|= clGetDeviceInfo(device_id, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_frequency), &clock_frequency, &returned_size);
  err|= clGetDeviceInfo(device_id, CL_DEVICE_EXTENSIONS, sizeof(device_extensions), device_extensions, &returned_size);
  err|= clGetDeviceInfo(device_id, CL_DEVICE_PROFILING_TIMER_RESOLUTION, sizeof(profiling_timer_resolution), &profiling_timer_resolution, &returned_size);
  
  // Memory	
  err|= clGetDeviceInfo(device_id, CL_DEVICE_ADDRESS_BITS, sizeof(device_address_bits), &device_address_bits, &returned_size);
  err|= clGetDeviceInfo(device_id, CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(mem_base_addr_align), &mem_base_addr_align, &returned_size);
  err|= clGetDeviceInfo(device_id, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, sizeof(min_data_type_align_size), &min_data_type_align_size, &returned_size);
  
  err|= clGetDeviceInfo(device_id, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(global_mem_size), &global_mem_size, &returned_size);
  err|= clGetDeviceInfo(device_id, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, sizeof(global_mem_cache_size), &global_mem_cache_size, &returned_size);
  err|= clGetDeviceInfo(device_id, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(max_mem_alloc_size), &max_mem_alloc_size, &returned_size);
  
  err|= clGetDeviceInfo(device_id, CL_DEVICE_LOCAL_MEM_TYPE, sizeof(local_mem_type), &local_mem_type, &returned_size);
  err|= clGetDeviceInfo(device_id, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(local_mem_size), &local_mem_size, &returned_size);
  
  err|= clGetDeviceInfo(device_id, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, sizeof(max_constant_buffer_size), &max_constant_buffer_size, &returned_size);
  
  // Work
  err|= clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_work_group_size), &max_work_group_size, &returned_size);
  err|= clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(max_work_item_dims), &max_work_item_dims, &returned_size);
  err|= clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(max_work_item_sizes), max_work_item_sizes, &returned_size);
  err|= clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(max_compute_units), &max_compute_units, &returned_size);
  
  // Image
  err|= clGetDeviceInfo(device_id, CL_DEVICE_IMAGE_SUPPORT, sizeof(image_support), &image_support, &returned_size);
  err|= clGetDeviceInfo(device_id, CL_DEVICE_MAX_READ_IMAGE_ARGS, sizeof(max_read_image_args), &max_read_image_args, &returned_size);
  err|= clGetDeviceInfo(device_id, CL_DEVICE_MAX_WRITE_IMAGE_ARGS, sizeof(max_write_image_args), &max_write_image_args, &returned_size);
  err|= clGetDeviceInfo(device_id, CL_DEVICE_IMAGE2D_MAX_WIDTH, sizeof(image2d_max_width), &image2d_max_width, &returned_size);
  err|= clGetDeviceInfo(device_id, CL_DEVICE_IMAGE2D_MAX_HEIGHT, sizeof(image2d_max_height), &image2d_max_height, &returned_size);
  err|= clGetDeviceInfo(device_id, CL_DEVICE_MAX_SAMPLERS, sizeof(max_samplers), &max_samplers, &returned_size);

  cout << "===============================================================" << endl;
  cout << "Vendor: " << vendor_name << endl;
  cout << "Device Name: " << device_name << endl;
  cout << "===============================================================" << endl;
  
  cout << "General:" << endl;
  cout << "• Profile: " << device_profile << endl;
  cout << "• Clock Frequency: " << clock_frequency << "Mhz" << endl;
  cout << "• Supported Extensions: " << device_extensions << endl;
  cout << "• Timer Resolution: " << profiling_timer_resolution << "ns" << endl;
  
  for(int i=0; i < 6; ++i){
    err|= clGetDeviceInfo(device_id, vector_types[i], sizeof(clock_frequency), &vector_width, &returned_size);
    cout << "• Vector type width for: " << vector_type_names[i] << " = " << vector_width << endl;
  }
  cout << endl;
  
  cout << "Memory:" << endl;
  cout << "• General:" << endl;
  cout << "\tAddress bits: " << device_address_bits << endl;
  cout << "\tMem base Addr: " << mem_base_addr_align << endl;
  cout << "\tMin data type align size: " << min_data_type_align_size << endl;
  cout << "• Global:" << endl;
  cout << "\tGlobal Mem Size: " << global_mem_size / (1024 * 1024) << "MB" << endl;
  cout << "\tGlobal Mem Cache Size (Bytes): " << global_mem_cache_size << endl;
  cout << "\tMax Mem Alloc Size: " << max_mem_alloc_size / (1024 * 1024) << "MB" << endl;
  cout << "• Local:" << endl;
  cout << "\tLocal Mem Type (Local=1, Global=2): " << local_mem_type << endl;
  cout << "\tLocal Mem Size: " << (int)local_mem_size / 1024 << "KB" << endl;
  cout << "• Constant:" << endl;
  cout << "\tMax Constant Buffer Size: " << max_constant_buffer_size / 1024 << "KB" << endl;
  cout << endl;
  
  cout << "Work:" << endl;
  cout << "• Max Compute Units: " << max_compute_units << endl;
  cout << "• Max Work Group Size: " << max_work_group_size << endl;
  cout << "• Max Work Item Dims: " << max_work_item_dims << endl;
  for(size_t i=0;i<max_work_item_dims;i++) 
    cout << "• Max Work Items in Dim " << i + 1 << ": " << max_work_item_sizes[i] << endl;
  cout << endl;
  
  cout << "Image:" << endl;
  cout << "• Image support: " << image_support << endl;
  cout << "• Max number of simulataneous read images: " << max_read_image_args << endl;
  cout << "• Max number of simulataneous write images: " << max_write_image_args << endl;
  cout << "• Max width of 2D image: " << image2d_max_width << endl;
  cout << "• Max height of 2D image: " << image2d_max_height << endl;	
  cout << "• Max Samplers: " << max_samplers << endl;
  cout << "• Supported Formats: " << endl;
  get_device_supported_image_formats(device_id);
  cout << endl;
}

const char * channel_order_to_s(cl_channel_order channel_order)
{
  switch (channel_order) {
    case CL_R: return "CL_R";
    case CL_A: return "CL_A";
    case CL_INTENSITY: return "CL_INTENSITY";
    case CL_LUMINANCE: return "CL_LUMINANCE";
    case CL_RG: return "CL_RG";
    case CL_RA: return "CL_RA";
    case CL_RGB: return "CL_RGB";
    case CL_RGBA: return "CL_RGBA";
    case CL_ARGB: return "CL_ARGB";
    case CL_BGRA: return "CL_BGRA";
    default: return "UNKNOWN";      
  }
}

const char * channel_data_type_to_s(cl_channel_type channel_type)
{
  switch (channel_type) {
    case CL_SNORM_INT8: return "CL_SNORM_INT8";
    case CL_SNORM_INT16: return "CL_SNORM_INT16";
    case CL_UNORM_INT8: return "CL_UNORM_INT8";
    case CL_UNORM_INT16: return "CL_UNORM_INT16";
    case CL_UNORM_SHORT_565: return "CL_UNORM_SHORT_565";
    case CL_UNORM_SHORT_555: return "CL_UNORM_SHORT_555";
    case CL_UNORM_INT_101010: return "CL_UNORM_INT_101010";
    case CL_SIGNED_INT8: return "CL_SIGNED_INT8";
    case CL_SIGNED_INT16: return "CL_SIGNED_INT16";
    case CL_SIGNED_INT32: return "CL_SIGNED_INT32";
    case CL_UNSIGNED_INT8: return "CL_UNSIGNED_INT8";
    case CL_UNSIGNED_INT16: return "CL_UNSIGNED_INT16";
    case CL_UNSIGNED_INT32: return "CL_UNSIGNED_INT32";
    case CL_HALF_FLOAT: return "CL_HALF_FLOAT";
    case CL_FLOAT: return "CL_FLOAT";
    default: return "UNKNOWN";
  }
}

static void get_device_supported_image_formats_(cl_device_id device_id, cl_mem_flags mem_flags)
{
  int err = 0;
  cl_image_format image_formats[128] = {{0,0}};
  cl_uint nb_image_formats = 0;
  
  cl_context context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
  
  err = clGetSupportedImageFormats(context, mem_flags, CL_MEM_OBJECT_IMAGE2D, 128, image_formats, &nb_image_formats);
  if (err != CL_SUCCESS) {
    cout << "Could not get the list of supported image formats" << endl;
    return;
  }
  for (int i = 0; i < nb_image_formats; ++i)
    cout << "\t\t" << channel_order_to_s(image_formats[i].image_channel_order)
    << "[" << channel_data_type_to_s(image_formats[i].image_channel_data_type) << "]"
    << endl;
  
  clReleaseContext(context);  
}

void get_device_supported_image_formats(cl_device_id device_id)
{
  cout << "\t• Read Only:" << endl;
  get_device_supported_image_formats_(device_id, CL_MEM_READ_ONLY);
  cout << "\t• Write Only:" << endl;
  get_device_supported_image_formats_(device_id, CL_MEM_WRITE_ONLY);
}

void get_all_devices_info()
{
  cl_uint nb_devices = 0;
  cl_int err = 0;
  cl_device_id devices[16] = {0};
  
  err = clGetDeviceIDs(NULL, CL_DEVICE_TYPE_ALL, 16, devices, &nb_devices);
  if (err != CL_SUCCESS) {
    cout << "Cannot get the devices listing" << endl;
    return;
  }
	
  for (cl_uint i = 0; i < nb_devices; ++i)
    device_info(devices[i]);
}

cl_ulong get_elapsed_time(cl::Event & event)
{  
  cl_ulong starttime = event.getProfilingInfo<CL_PROFILING_COMMAND_START>();
  cl_ulong endtime = event.getProfilingInfo<CL_PROFILING_COMMAND_END>();
  
  return endtime - starttime;
}

cl_int get_nb_of_bank(cl_device_id device)
{
  cl_ulong local_mem_size;
  cl_int err = clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE,
                              sizeof(local_mem_size), &local_mem_size, NULL);
  assert(err == CL_SUCCESS);
  // Assume 1K bank size
  return local_mem_size / 1024;
}

