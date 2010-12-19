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

#ifndef CLCV_CLINFO_H__
#define CLCV_CLINFO_H__

#include <string>
#include <OpenCL/OpenCL.h>
#define __CL_ENABLE_EXCEPTIONS
#include <clcv/cl.hpp>

void platform_info(cl_platform_id platform_id);
void get_all_platforms_info();

std::string get_device_name(cl_device_id device);
void device_info(cl_device_id device_id);
void get_all_devices_info();

const char * channel_order_to_s(cl_channel_order channel_order);
const char * channel_data_type_to_s(cl_channel_type channel_type);
void get_device_supported_image_formats(cl_device_id device_id);

cl_int get_nb_of_bank(cl_device_id device);

cl_ulong get_elapsed_time(cl::Event & event);

#endif
