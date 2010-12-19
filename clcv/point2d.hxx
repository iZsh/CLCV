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

#ifndef CLCV_POINT2D_HXX__
#define CLCV_POINT2D_HXX__

namespace clcv
{
  inline
  point2d::point2d()
  {
    m_value[0] = 0;
    m_value[1] = 0;
  }
  
  inline
  point2d::point2d(coord row, coord col)
  {
    m_value[0] = row;
    m_value[1] = col;    
  }
  
  inline
  coord & point2d::operator[](unsigned dim)
  {
    return m_value[dim];
  }

  inline
  coord point2d::operator[](unsigned dim) const
  {
    return m_value[dim];
  }
  
  inline
  coord & point2d::row ()
  {
    return m_value[0];
  }
  
  inline
  coord point2d::row() const
  {
    return m_value[0];
  }
  
  inline
  coord & point2d::col ()
  {
    return m_value[1];
  }
  
  inline
  coord point2d::col() const
  {
    return m_value[1];
  }

  inline
  std::ostream & operator<<(std::ostream & o, const clcv::point2d & p)
  {
    return o << "(" << p.row() << ", " << p.col() << ")";
  }
  
}

#endif
