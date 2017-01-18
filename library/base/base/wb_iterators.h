/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef __BASE_ITERATORS_H__
#define __BASE_ITERATORS_H__

namespace base {

  //==============================================================================
  //
  //==============================================================================
  // template helps to reduce amount of typings when walking STL containers
  // for example:
  //   std::vector<int> v;
  //   for (const_range<std::vector<int> >  it(v); it; ++it)
  //     printf("%i\n", *v);
  //
  // or for the std::map.
  //   typedef std::map<std::string, grt::SomeLengthyTypeNameRef>   AMapType
  //
  //   AMapType map = ...;
  //   for (const_range<AMapType> it(map); it; ++it)
  //   {
  //     process(it->first, it->second);
  //   }
  //
  // And compare this short snippet above with
  //
  //   AMapType map = ...;
  //   const AMapType::const_iterator last = map.end(); // This more efficient than to call map.end() every time
  //   for (AMaoType::const_iterator it = map.begin(); it != last; ++it)
  //   {
  //     process(it->first, it->second);
  //   }
  template <typename Cont>
  class const_range {
  public:
    const_range(const Cont& cont) : it(cont.begin()), last(cont.end()) {
    }

    operator bool() const {
      return it != last;
    }

    typename Cont::const_iterator& operator++() {
      ++it;
      return it;
    }

    typename Cont::const_iterator& operator--() {
      --it;
      return it;
    }

    typename Cont::const_iterator::reference operator*() const {
      return *it;
    }

    typename Cont::const_iterator::pointer operator->() const {
      return &(*it);
    }

  private:
    typename Cont::const_iterator it;
    const typename Cont::const_iterator last;
  };
}

#endif
