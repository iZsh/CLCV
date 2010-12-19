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

#ifndef CLCV_IMAGE2D_H__
#define CLCV_IMAGE2D_H__

#include <assert.h>

namespace clcv
{
  template<typename T>
  class image2d
  {
  public:
    image2d(const unsigned nrows, const unsigned ncols);
    image2d(const image2d<T> & r);
    const image2d<T> & operator=(const image2d<T> & r);
    ~image2d();
    
    image2d<T> clone() const;
    
    unsigned nrows() const;
    unsigned ncols() const;

    T & operator()(unsigned row, unsigned col);
    T operator()(unsigned row, unsigned col) const;

  private:    
    class data_wrapper
    {
    public:
      data_wrapper(unsigned size)
      : m_data(NULL), m_count(1)
      {
        m_data = new T[size];
      }

      void ref() { ++m_count; }
      bool unref()
      {
        assert(m_count > 0);
        if (--m_count == 0)
          delete[] m_data;
        m_data = 0;
        return m_count == 0;
      }
      
      T & operator[](unsigned i) { return m_data[i]; }
      T operator[](unsigned i) const { return m_data[i]; }

    private:
      T * m_data;
      unsigned m_count;
    };

    unsigned m_nrows;
    unsigned m_ncols;
    data_wrapper * m_data;
  };  
}

#include <clcv/image2d.hxx>

#endif