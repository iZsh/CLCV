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

#ifndef CLCV_POINT2D_H__
#define CLCV_POINT2D_H__

#include <iostream>
#include <clcv/coord.h>

namespace clcv
{
  class point2d
  {
  public:
    point2d();
    point2d(coord row, coord col);

    // access coordinates
    // dim = 0 for row
    // dim = 1 for col
    coord & operator[](unsigned dim);
    coord operator[](unsigned dim) const;
    
    coord & row();
    coord row() const;

    coord & col();
    coord col() const;
    
  private:
    coord m_value[2];
  };  

  std::ostream & operator<<(std::ostream & o, const clcv::point2d & p);

}

namespace std {
template<>
struct less<clcv::point2d>
{
  bool operator()(const clcv::point2d & l, const clcv::point2d & r) const
  {
    if (l.row() != r.row())
      return l.row() < r.row();
    return l.col() < r.col();
  }
};
}

#include <clcv/point2d.hxx>

#endif
