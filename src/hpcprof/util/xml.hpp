// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*-

#ifndef HPCTOOLKIT_PROFILE_UTIL_XML_H
#define HPCTOOLKIT_PROFILE_UTIL_XML_H

#include <iosfwd>
#include <string>

namespace hpctoolkit::util {

/// Alternative to std::quoted for XML outputs.
class xmlquoted {
public:
  xmlquoted(std::string_view s, bool q = true);

  friend std::ostream& operator<<(std::ostream&, const xmlquoted&);

private:
  std::string_view str;
  bool addquotes;
};

std::ostream& operator<<(std::ostream&, const xmlquoted&);

} // namespace hpctoolkit::util

#endif // HPCTOOLKIT_PROFILE_UTIL_XML_H
