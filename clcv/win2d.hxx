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

#ifndef CLCV_WIN2D_HXX__
#define CLCV_WIN2D_HXX__

#include <limits>
#include <algorithm>

namespace clcv
{
  template<typename T>
  inline
  win2d<T>::win2d()
  : m_points()
  {
  }
  
  template<typename T>
  inline
  win2d<T>::win2d(const unsigned n, const coord c[])
  : m_points()
  {
    for (unsigned i = 0; i < 3*n; i += 3)
      add(c[i], c[i+1], c[i+2]);
  }

  template<typename T>
  inline
  win2d<T> & win2d<T>::add(const point2d & p, const T weight)
  {
    m_points.insert(make_pair(p, weight));
    return *this;
  }
  
  template<typename T>
  inline
  win2d<T> & win2d<T>::add(const coord row, const coord col, const T weight)
  {
    m_points.insert(make_pair(point2d(row, col), weight));
    return *this;
  }

  template<typename T>
  inline
  unsigned win2d<T>::count() const
  {
    return m_points.size();
  }

  template<typename T>
  inline
  typename win2d<T>::const_iter win2d<T>::begin() const
  {
    const_iter it = m_points.begin();
    return it;
  }
  
  template<typename T>
  inline
  typename win2d<T>::const_iter win2d<T>::end() const
  {
    const_iter it = m_points.end();
    return it;
  }

  template<typename T>
  inline
  coord win2d<T>::minrow() const
  {
    return mindim(0);
  }

  template<typename T>
  inline
  coord win2d<T>::maxrow() const
  {
    return maxdim(0);
  }
  
  template<typename T>
  inline
  unsigned win2d<T>::nrows() const
  {
    return maxrow() - minrow() + 1;
  }

  template<typename T>
  inline
  coord win2d<T>::mincol() const
  {
    return mindim(1);
  }

  template<typename T>
  inline
  coord win2d<T>::maxcol() const
  {
    return maxdim(1);
  }

  template<typename T>
  inline
  unsigned win2d<T>::ncols() const
  {
    return maxcol() - mincol() + 1;
  }
  
  template<typename T>
  inline
  coord win2d<T>::mindim(unsigned d) const
  {
    assert(count() > 0);
    assert(d < 2);
    coord mind = std::numeric_limits<coord>::max();
    for (const_iter it = begin(); it != end(); ++it)
      mind = std::min(it->first[d], mind);
    return mind;    
  }
  
  template<typename T>
  inline
  coord win2d<T>::maxdim(unsigned d) const
  {
    assert(count() > 0);
    assert(d < 2);
    coord maxd = std::numeric_limits<coord>::min();
    for (const_iter it = begin(); it != end(); ++it)
      maxd = std::max(it->first[d], maxd);
    return maxd;    
  }

  template<typename T>
  inline
  std::ostream & operator<<(std::ostream & o, const clcv::win2d<T> & win)
  {
    o << "[ ";
    typename win2d<T>::const_iter it = win.begin();
    o << "{";
    o << it->first;
    o << ", ";
    o << it->second;
    o << "}";
    while (++it != win.end())
    {
      o << ", ";
      o << "{";
      o << it->first;
      o << ", ";
      o << it->second;
      o << "}";
    }
    o << " ]";
    return o;
  }

}

#endif
