// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//***************************************************************************
//
// File:
//    $HeadURL$
//
// Purpose:
//    [The purpose of this file]
//
// Description:
//    [The set of functions, macros, etc. defined in the file]
//
//***************************************************************************

//************************* System Include Files ****************************

#include <iostream>
using std::cerr;
using std::endl;
using std::hex;
using std::dec;

#include <typeinfo>

#include <string>
using std::string;

#include <cstring>

//*************************** User Include Files ****************************

#include "VMAInterval.hpp"

#include "../common/diagnostics.h"
#include "../common/StrUtil.hpp"

//*************************** Forward Declarations **************************

#define DBG 0

//***************************************************************************

//***************************************************************************
// VMAInterval
//***************************************************************************

string
VMAInterval::toString() const
{
  string self = "["
    + StrUtil::toStr(m_beg, 16) + "-"
    + StrUtil::toStr(m_end, 16) + ")";
  return self;
}


void
VMAInterval::fromString(const char* formattedstr)
{
  const char* s = formattedstr;
  const char* p = formattedstr;
  if (!p) { return; }

  // -----------------------------------------------------
  // 1. ignore leading whitespace
  // -----------------------------------------------------
  while (*p != '\0' && isspace(*p)) { p++; }
  if (*p == '\0') { return; }

  // -----------------------------------------------------
  // 2. parse
  // -----------------------------------------------------
  // skip '['
  DIAG_Assert(*p == '[', DIAG_UnexpectedInput << "'" << s << "'");
  p++;

  // read 'm_beg'
  unsigned endidx;
  m_beg = StrUtil::toUInt64(p, &endidx);
  p += endidx;

  // skip '-'
  DIAG_Assert(*p == '-', DIAG_UnexpectedInput << "'" << s << "'");
  p++;

  // read 'm_end'
  m_end = StrUtil::toUInt64(p, &endidx);
  p += endidx;

  // skip ')'
  DIAG_Assert(*p == ')', DIAG_UnexpectedInput << "'" << s << "'");
}


std::ostream&
VMAInterval::dump(std::ostream& os) const
{
  os << std::showbase;
  os << std::hex << "[" << m_beg << "-" << m_end << ")" << std::dec;
  return os;
}


void
VMAInterval::ddump() const
{
  dump(std::cout);
  std::cout.flush();
}


//***************************************************************************
// VMAIntervalSet
//***************************************************************************

// Given an interval x to insert or delete from the interval set, the
// following cases are possible where x is represented by <> and
// existing interval set by {}.
//
// We first find lb and ub such that: (lb <= x < ub) and lb != x.
//
//   0) lb interval already contains x
//        {lb <> }
//        {lb <> } ...
//        ... {lb <> }
//        ... {lb <> } ...
//   1) x is a mutually exclusive interval:
//      a. <>
//      b. <> {ub} ...
//      c. ... {lb} <>
//      d. ... {lb} <> {ub} ...
//   2) x overlaps only the lb interval
//      a.  ... {lb < } ... {} >
//      b.  ... {lb < } ... {} > {ub} ...
//   3) x overlaps only the ub interval:
//      a.  < {} ... { > ub} ...
//      b.  ... {lb} < {} ... { > ub} ...
//   4) x overlaps both lb and ub interval
//        ... {lb < } ... { > ub} ...
//   5) x subsumes all intervals
//        < {} ... {} >

typedef std::pair <VMAIntervalSet::iterator, bool> iteratorPair;

// insert: See above for cases
iteratorPair
VMAIntervalSet::insert(const VMAIntervalSet::value_type& x)
{
  DIAG_DevMsgIf(DBG, "VMAIntervalSet::insert [begin]\n"
                << "  this: " << toString() << endl
                << "  add : " << x.toString());

  // empty interval
  if (x.empty()) {
    return iteratorPair (end(), false);
  }

  // empty set
  if (empty()) {
    return My_t::insert(x);
  }

  VMA low = x.beg();
  VMA high = x.end();

  // find it = last interval with it.beg <= low if there is one, or
  // else first interval with it.beg > low, but it is never end().
  // this is the only interval that could contain low.
  auto it = upper_bound(VMAInterval(low, low));

  if (it != begin() && (it == end() || it->beg() > low)) {
    it--;
  }

  // if x is a subset of it, then return it.  this is the only case
  // that returns false (not created).
  if (it->beg() <= low && high <= it->end()) {
    return iteratorPair (it, false);
  }

  // if x intersects interval to the left, then extend low.  this can
  // only happen once at it.
  if (it->beg() <= low) {
    auto next_it = it;  next_it++;

    if (it->end() >= low) {
      low = std::min(low, it->beg());
      My_t::erase(it);
    }
    it = next_it;
  }

  // if x intersects intervals to the right, then extend high.  this
  // can happen multiple times.
  while (it != end() && it->beg() <= high) {
    auto next_it = it;  next_it++;
    high = std::max(high, it->end());
    My_t::erase(it);
    it = next_it;
  }

  // finally, re-insert [low, high) which covers all of the erased
  // intervals plus the original x.
  return My_t::insert(VMAInterval(low, high));
}


// erase: See above for cases
void
VMAIntervalSet::erase(const VMAIntervalSet::key_type& x)
{
  if (x.empty()) {
    return;
  }

  // -------------------------------------------------------
  // find [lb, ub) where lb is the first element !< x and ub is the
  // first element > x.  IOW, if lb != end() then (x <= lb < ub).
  // -------------------------------------------------------
  std::pair<VMAIntervalSet::iterator, VMAIntervalSet::iterator> lu =
    equal_range(x);
  VMAIntervalSet::iterator lb = lu.first;
  VMAIntervalSet::iterator ub = lu.second;

  // find (lb <= x < ub) such that lb != x
  if (lb == end()) {
    if (!empty()) { // all elements are less than x
      lb = --end();
    }
  }
  else {
    if (lb == begin()) { // no element is less than x
      lb = end();
    }
    else {
      --lb;
    }
  }

  // -------------------------------------------------------
  // Detect the appropriate case.  Note that we do not have to
  // explicitly consider Case 1 since it amounts to a NOP.
  // -------------------------------------------------------
  VMA lb_beg = (lb != end()) ? lb->beg() : 0;
  VMA ub_end = (ub != end()) ? ub->end() : 0;

  if (lb != end() && lb->contains(x)) {
    // Case 0: split interval: erase [lb, ub);
    //   insert [lb->beg(), x.beg()), [x.end(), lb->end())
    VMA lb_end = lb->end();
    My_t::erase(lb, ub);
    if (lb_beg < x.beg()) {
      My_t::insert( VMAInterval(lb_beg, x.beg()) );
    }
    if (lb_end > x.end()) {
      My_t::insert( VMAInterval(x.end(), lb_end) );
    }
  }
  else if (lb == end() && ub == end()) { // size >= 1
    if (!empty()) {
      // Case 5: erase everything
      My_t::clear();
    }
  }
  else if (lb == end()) {
    // INVARIANT: ub != end()

    if ( !(x.end() < ub->beg()) ) {
      // Case 3a: erase [begin(), ub + 1); insert [x.end(), ub->end())
      VMAIntervalSet::iterator end = ub;
      My_t::erase(begin(), ++end);
      if (x.end() < ub_end) {
        My_t::insert( VMAInterval(x.end(), ub_end) );
      }
    }
  }
  else if (ub == end()) {
    // INVARIANT: lb != end()

    if ( !(lb->end() < x.beg()) ) {
      // Case 2a: erase [lb, end()); insert [lb->beg(), x.beg())
      My_t::erase(lb, end());
      if (lb_beg < x.beg()) {
        My_t::insert( VMAInterval(lb_beg, x.beg()) );
      }
    }
  }
  else {
    // INVARIANT: lb != end() && ub != end()

    if (x.beg() <= lb->end() && x.end() < ub->beg()) {
      // Case 2b: erase [lb, ub); insert [lb->beg(), x.beg())
      My_t::erase(lb, ub);
      if (lb_beg < x.beg()) {
        My_t::insert( VMAInterval(lb_beg, x.beg()) );
      }
    }
    else if (lb->end() < x.beg() && ub->beg() <= x.end()) {
      // Case 3b: erase [lb + 1, ub + 1): insert [x.end(), ub->end())
      VMAIntervalSet::iterator beg = lb;
      VMAIntervalSet::iterator end = ub;
      My_t::erase(++beg, ++end);
      if (x.end() < ub_end) {
        My_t::insert( VMAInterval(x.end(), ub_end) );
      }
    }
    else if ( !(lb->end() < x.beg() && x.end() < ub->beg()) ) {
      // Case 4: erase [lb, ub + 1);
      //   insert [lb->beg(), x.beg()) and [x.end(), ub->end()).
      VMAIntervalSet::iterator end = ub;
      My_t::erase(lb, ++end);
      My_t::insert( VMAInterval(lb_beg, x.beg()) );
      My_t::insert( VMAInterval(x.end(), ub_end) );
    }
  }
}


void
VMAIntervalSet::merge(const VMAIntervalSet& x)
{
  for (const_iterator it = x.begin(); it != x.end(); ++it) {
    insert(*it); // the overloaded insert
  }
}


string
VMAIntervalSet::toString() const
{
  string self = "{";
  bool needSeparator = false;
  for (const_iterator it = this->begin(); it != this->end(); ++it) {
    if (needSeparator) { self += " "; }
    self += (*it).toString();
    needSeparator = true;
  }
  self += "}";
  return self;
}


void
VMAIntervalSet::fromString(const char* formattedstr)
{
  const char* s = formattedstr;
  const char* p = formattedstr;

  // ignore leading whitespace
  if (!p || p[0] == '\0') { return; }

  // skip '{'
  DIAG_Assert(*p == '{', DIAG_UnexpectedInput << "'" << s << "'");
  p++;
  // INVARIANT: q points to either next interval or close of set

  // find intervals: p points to '[' and q points to ')'
  const char* q = p;
  while ( (p = strchr(q, '[')) ) {
    VMAInterval vmaint(p);
    insert(vmaint); // the overloaded insert

    // post-INVARIANT: q points to either next interval or close of set
    q = strchr(p, ')');
    q++;
  }

  // skip '}'
  DIAG_Assert(*q == '}', DIAG_UnexpectedInput << "'" << s << "'");
  p++;
}


std::ostream&
VMAIntervalSet::dump(std::ostream& os) const
{
  bool needSeparator = false;
  os << "{";
  for (const_iterator it = this->begin(); it != this->end(); ++it) {
    if (needSeparator) { os << " "; }
    (*it).dump(os);
    needSeparator = true;
  }
  os << "}";
  return os;
}


void
VMAIntervalSet::ddump() const
{
  dump(std::cout);
  std::cout.flush();
}


//***************************************************************************
// VMAIntervalMap
//***************************************************************************

/*
Cf. LoadModScope::dumpmaps

template<typename T>
void
VMAIntervalMap_ddump(VMAIntervalMap<T>* x)
{
  x->ddump();
}
*/
