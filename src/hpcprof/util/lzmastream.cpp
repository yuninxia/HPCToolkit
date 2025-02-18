// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#include "lzmastream.hpp"

#include "log.hpp"

#include <cassert>
#include <cstring>

using namespace hpctoolkit::util;

#define BUFSIZE 4096

static lzma_ret maybeThrowLZMA_decoder(lzma_ret ret) {
  switch(ret) {
  case LZMA_OK:
  case LZMA_STREAM_END:
  // These aren't problems per-se
  case LZMA_NO_CHECK:
  case LZMA_UNSUPPORTED_CHECK:
  case LZMA_GET_CHECK:
    break;
  case LZMA_MEM_ERROR:
    throw std::runtime_error("LZMA decoder ran out of memory");
  case LZMA_FORMAT_ERROR:
  case LZMA_MEMLIMIT_ERROR:
    throw std::runtime_error("LZMA decoder hit memory limit (the impossible happened?)");
  case LZMA_OPTIONS_ERROR:
    throw std::runtime_error("LZMA decoder with wrong options");
  case LZMA_DATA_ERROR:
    throw std::runtime_error("attempt to decode a corrupted LZMA/XZ stream");
  case LZMA_BUF_ERROR:
    throw std::runtime_error("LZMA decoder failed (multiple times) to make progress (4K is too small a buffer size?)");
  case LZMA_PROG_ERROR:
    throw std::runtime_error("LZMA decoder encountered a really bad error");
  default:
    throw std::runtime_error("LZMA decoder returned an unknown error code");
  }
  return ret;
}


lzmastreambuf::lzmastreambuf(std::streambuf* base)
  : base(base), stream(LZMA_STREAM_INIT), in_buffer(new char[BUFSIZE]),
    in_dz_buffer(new char[BUFSIZE]), tail(false) {
  maybeThrowLZMA_decoder(lzma_auto_decoder(&stream, UINT64_MAX, 0));
  setg(nullptr, nullptr, nullptr);
}

lzmastreambuf::~lzmastreambuf() {
  lzma_end(&stream);
  delete[] in_buffer;
  delete[] in_dz_buffer;
}

lzmastreambuf::int_type lzmastreambuf::underflow() {
  using base_traits_type = std::remove_reference<decltype(*base)>::type::traits_type;
  if(gptr() == egptr()) {
    // The next bytes will land at the beginning of the buffer
    stream.next_out = (uint8_t*)in_dz_buffer;
    stream.avail_out = BUFSIZE;

    if(!tail) {
      // Shift the leftovers down to the start of the buffer
      std::memmove(in_buffer, stream.next_in, stream.avail_in);
      stream.next_in = (const uint8_t*)in_buffer;

      // Pull in some bytes for us to use
      stream.avail_in += base->sgetn(in_buffer + stream.avail_in, BUFSIZE - stream.avail_in);
    }

    // Run the decompression
    if(maybeThrowLZMA_decoder(lzma_code(&stream, LZMA_RUN)) == LZMA_STREAM_END
       && !tail) {
      // Put back all the bytes we don't need anymore
      for(size_t i = 0; i < stream.avail_in; i++) {
        if(base_traits_type::eq_int_type(base_traits_type::eof(), base->sungetc())) {
          // Hrm. This is a problem.
          setg(nullptr, nullptr, nullptr);
          return traits_type::eof();
        }
      }
      tail = true;
    }

    // Update the pointers as needed
    setg(in_dz_buffer, in_dz_buffer, (char*)stream.next_out);
  }
  return gptr() == egptr() ? traits_type::eof() : traits_type::not_eof(*gptr());
}
