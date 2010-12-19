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

#ifndef CLCV_IMAGE2D_HXX__
#define CLCV_IMAGE2D_HXX__

namespace clcv
{
  template<typename T>
  inline
  image2d<T>::image2d(const unsigned nrows, const unsigned ncols)
  : m_nrows(nrows), m_ncols(ncols), m_data(NULL)
  {
    m_data = new data_wrapper(nrows * ncols);
  }
  
  template<typename T>
  inline
  image2d<T>::image2d(const image2d<T> & r)
  : m_nrows(r.m_nrows), m_ncols(r.m_ncols), m_data(r.m_data)
  {
    m_data->ref();
  }

  template<typename T>
  inline
  const image2d<T> & image2d<T>::operator=(const image2d<T> & r)
  {
    if (this != &r)
    {
      m_nrows = r.m_nrows;
      m_ncols = r.m_ncols;
      m_data = r.m_data;
      m_data->ref();
    }
    return *this;
  }
  
  template<typename T>
  inline
  image2d<T>::~image2d()
  {
    if (m_data->unref())
      delete m_data;
    m_data = NULL;
  }
  
  template<typename T>
  inline
  image2d<T> image2d<T>::clone() const
  {
    image2d<T> img(nrows(), ncols());
    for (unsigned r = 0; r < nrows(); ++r)
      for (unsigned c = 0; c < ncols(); ++c)
        img(r, c) = operator()(r, c);
    return img;
  }
  
  template<typename T>
  inline
  unsigned image2d<T>::nrows() const
  {
    return m_nrows;
  }
  
  template<typename T>
  inline
  unsigned image2d<T>::ncols() const
  {
    return m_ncols;
  }
  
  template<typename T>
  inline
  T & image2d<T>::operator()(unsigned row, unsigned col)
  {
    return (*m_data)[row * m_ncols + col];
  }
  
  template<typename T>
  inline
  T image2d<T>::operator()(unsigned row, unsigned col) const
  {
    return (*m_data)[row * m_ncols + col];
  }
  
}

#endif
