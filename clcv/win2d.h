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

#ifndef CLCV_WIN2D_H__
#define CLCV_WIN2D_H__

#include <assert.h>
#include <iostream>
#include <set>
#include <clcv/point2d.h>

namespace clcv
{
  template<typename T>
  class win2d
  {
  public:
    typedef typename std::set<std::pair<point2d, T> >::const_iterator const_iter;

    win2d();
    // from an array of coordinates
    win2d(const unsigned n, const coord c[]);
    // Add a point
    win2d & add(const point2d & p, const T weight);
    win2d & add(const coord row, const coord col, const T weight);
    // accessors
    const_iter begin() const;
    const_iter end() const;
    // stats
    unsigned count() const;
    coord minrow() const;
    coord maxrow() const;
    unsigned nrows() const;
    coord mincol() const;
    coord maxcol() const;
    unsigned ncols() const;
  
  protected:
    coord mindim(unsigned d) const;
    coord maxdim(unsigned d) const;
    
  private:
    std::set<std::pair<point2d, T> > m_points;
  };

  template<typename T>
  std::ostream & operator<<(std::ostream & o, const clcv::win2d<T> & win);

  // Standard windows
  template<typename T>
  inline const win2d<T> & win_c4p()
  {
    static const coord c[] = { -1,0,1, 0,-1,1, 0,0,1, 0,1,1,  1,0,1 };
    static const win2d<T> win(5, c);
    return win;
  }

  template<typename T>
  inline const win2d<T> & win_c8p()
  {
    static const coord c[] = { -1,-1,1, -1,0,1, -1,1,1,  0,-1,1, 0,0,1, 0,1,1,  1,-1,1,  1,0,1,  1,1,1 };
    static const win2d<T> win(9, c);
    return win;
  }

  template<typename T>
  inline win2d<T> win_rect(unsigned nrows, unsigned ncols)
  {
    assert(nrows >= 1 && (nrows % 2) == 1);
    assert(ncols >= 1 && (ncols % 2) == 1);

    win2d<T> win;
    int half_nrows = nrows / 2;
    int half_ncols = ncols / 2;

    for (coord row = - half_nrows; row <= half_nrows; ++row)
      for (coord col = - half_ncols; col <= half_ncols; ++col)
        win.add(row, col, 1);
    return win;
  }
  
}

namespace std {
  template<typename T>
  struct less<std::pair<clcv::point2d, T> >
  {
    bool operator()(const std::pair<clcv::point2d, T> & l,
                    const std::pair<clcv::point2d, T> & r) const
    {
      if (l.first.row() != r.first.row())
        return l.first.row() < r.first.row();
      if (l.first.col() != r.first.col())
        return l.first.col() < r.first.col();
      return l.second < r.second;
    }
  };
}


#include <clcv/win2d.hxx>

#endif
