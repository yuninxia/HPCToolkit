// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#include "mpi/all.hpp"

#include "util/log.hpp"

#include <limits>
#include <mpi.h>

#include <cassert>
#include <cstring>
#include <mutex>
#include <thread>

using namespace hpctoolkit::mpi;
using namespace detail;

struct detail::Datatype {
  Datatype(MPI_Datatype d, std::size_t s) : value(d), sz(s) {};
  MPI_Datatype value;
  std::size_t sz;
};

static Datatype d_char(MPI_CHAR, sizeof(char));
template<> const Datatype& detail::asDatatype<char>() { return d_char; }
static Datatype d_int8(MPI_INT8_T, sizeof(int8_t));
template<> const Datatype& detail::asDatatype<int8_t>() { return d_int8; }
static Datatype d_int16(MPI_INT16_T, sizeof(int16_t));
template<> const Datatype& detail::asDatatype<int16_t>() { return d_int16; }
static Datatype d_int32(MPI_INT32_T, sizeof(int32_t));
template<> const Datatype& detail::asDatatype<int32_t>() { return d_int32; }
static Datatype d_ll(MPI_LONG_LONG, sizeof(long long));
template<> const Datatype& detail::asDatatype<long long>() { return d_ll; }
static Datatype d_int64(MPI_INT64_T, sizeof(int64_t));
template<> const Datatype& detail::asDatatype<int64_t>() { return d_int64; }
static Datatype d_uint8(MPI_UINT8_T, sizeof(uint8_t));
template<> const Datatype& detail::asDatatype<uint8_t>() { return d_uint8; }
static Datatype d_uint16(MPI_UINT16_T, sizeof(uint16_t));
template<> const Datatype& detail::asDatatype<uint16_t>() { return d_uint16; }
static Datatype d_uint32(MPI_UINT32_T, sizeof(uint32_t));
template<> const Datatype& detail::asDatatype<uint32_t>() { return d_uint32; }
static Datatype d_ull(MPI_UNSIGNED_LONG_LONG, sizeof(unsigned long long));
template<> const Datatype& detail::asDatatype<unsigned long long>() { return d_ull; }
static Datatype d_uint64(MPI_UINT64_T, sizeof(uint64_t));
template<> const Datatype& detail::asDatatype<uint64_t>() { return d_uint64; }
static Datatype d_float(MPI_FLOAT, sizeof(float));
template<> const Datatype& detail::asDatatype<float>() { return d_float; }
static Datatype d_double(MPI_DOUBLE, sizeof(double));
template<> const Datatype& detail::asDatatype<double>() { return d_double; }

struct BaseOp : hpctoolkit::mpi::Op {
  BaseOp(MPI_Op o) : op(o) {};
  MPI_Op op;
};

static BaseOp o_max = MPI_MAX;
const Op& Op::max() noexcept { return o_max; }
static BaseOp o_min = MPI_MIN;
const Op& Op::min() noexcept { return o_min; }
static BaseOp o_sum = MPI_SUM;
const Op& Op::sum() noexcept { return o_sum; }


std::size_t World::m_rank = 0;
std::size_t World::m_size = 0;

static bool done = false;
static void escape() {
  // We use std::exit when things go south, but MPI doesn't react very quickly
  // to that. So we issue an Abort ourselves.
  if(!done) MPI_Abort(MPI_COMM_WORLD, 0);
}

// We support serialized MPI if necessary
static std::mutex lock;
static bool needsLock = true;
static std::unique_lock<std::mutex> mpiLock() {
  return needsLock ? std::unique_lock<std::mutex>(lock)
                   : std::unique_lock<std::mutex>();
}

void World::initialize() noexcept {
  int available;
  MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &available);
  std::atexit(escape);
  if(available < MPI_THREAD_SERIALIZED)
    util::log::fatal{} << "MPI does not have sufficient thread support!";
  needsLock = available < MPI_THREAD_MULTIPLE;

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  m_rank = rank;

  int size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  m_size = size;
}
void World::finalize() noexcept {
  MPI_Finalize();
  done = true;
}

namespace {
struct SmallMem final {
  SmallMem(std::size_t sz)
    : mem(sz > sizeof buf ? std::make_unique<char[]>(sz) : nullptr) {};

  operator void*() noexcept { return mem ? mem.get() : buf; }

  SmallMem(SmallMem&&) = delete;
  SmallMem(const SmallMem&) = delete;
  SmallMem& operator=(SmallMem&&) = delete;
  SmallMem& operator=(const SmallMem&) = delete;

  std::unique_ptr<char[]> mem;
  char buf[1024];
};
}


// Many MPI functions use `int` as the size of the data to send, however we use bytes and `size_t`
// instead. This template helps break these calls down into smaller pieces that MPI can handle.
template<class F>
static void segment(void* data, std::size_t cnt, const Datatype& ty, F&& f) {
  for(std::size_t off = 0; off < cnt; off += std::numeric_limits<int>::max()) {
    int sz = std::min<std::size_t>(cnt - off, std::numeric_limits<int>::max());
    f((void*)((char*)data + off * ty.sz), sz);
  }
}
template<class F>
static void segment(const void* data, std::size_t cnt, const Datatype& ty, F&& f) {
  for(std::size_t off = 0; off < cnt; off += std::numeric_limits<int>::max()) {
    int sz = std::min<std::size_t>(cnt - off, std::numeric_limits<int>::max());
    f((const void*)((const char*)data + off * ty.sz), sz);
  }
}
template<class F>
static void segment(void* data1, void* data2, std::size_t cnt, const Datatype& ty, F&& f) {
  for(std::size_t off = 0; off < cnt; off += std::numeric_limits<int>::max()) {
    int sz = std::min<std::size_t>(cnt - off, std::numeric_limits<int>::max());
    f((void*)((char*)data1 + off * ty.sz), (void*)((char*)data2 + off * ty.sz), sz);
  }
}
template<class F>
static void segment(const void* data1, const void* data2, std::size_t cnt, const Datatype& ty, F&& f) {
  for(std::size_t off = 0; off < cnt; off += std::numeric_limits<int>::max()) {
    int sz = std::min<std::size_t>(cnt - off, std::numeric_limits<int>::max());
    f((const void*)((const char*)data1 + off * ty.sz), (const void*)((const char*)data2 + off * ty.sz), sz);
  }
}


void hpctoolkit::mpi::barrier() {
  auto l = mpiLock();
  if(MPI_Barrier(MPI_COMM_WORLD) != MPI_SUCCESS)
    util::log::fatal{} << "Error while performing an MPI barrier!";
}

void detail::bcast(void* data, std::size_t cnt, const Datatype& ty,
                   std::size_t root) {
  auto l = mpiLock();
  segment(data, cnt, ty, [&](void* data, int cnt) {
    if(MPI_Bcast(data, cnt, ty.value, root, MPI_COMM_WORLD) != MPI_SUCCESS)
      util::log::fatal{} << "Error while performing an MPI broadcast!";
  });
}

void detail::reduce(void* data, std::size_t cnt, const Datatype& ty,
                    std::size_t root, const Op& op) {
  SmallMem send(cnt * ty.sz);
  std::memcpy(send, data, cnt * ty.sz);
  auto l = mpiLock();
  segment(send, data, cnt, ty, [&](void* send, void* data, int cnt) {
    if(MPI_Reduce(send, data, cnt, ty.value,
                  static_cast<const BaseOp&>(op).op, root, MPI_COMM_WORLD)
       != MPI_SUCCESS)
      util::log::fatal{} << "Error while performing an MPI reduction!";
  });
}

void detail::allreduce(void* data, std::size_t cnt, const Datatype& ty,
                       const Op& op) {
  SmallMem send(cnt * ty.sz);
  std::memcpy(send, data, cnt * ty.sz);
  auto l = mpiLock();
  segment(send, data, cnt, ty, [&](void* send, void* data, int cnt) {
    if(MPI_Allreduce(send, data, cnt, ty.value,
                     static_cast<const BaseOp&>(op).op, MPI_COMM_WORLD)
       != MPI_SUCCESS)
      util::log::fatal{} << "Error while performing an MPI all-reduction!";
  });
}

void detail::scan(void* data, std::size_t cnt, const Datatype& ty,
                  const Op& op) {
  SmallMem send(cnt * ty.sz);
  std::memcpy(send, data, cnt * ty.sz);
  auto l = mpiLock();
  segment(send, data, cnt, ty, [&](void* send, void* data, int cnt) {
    if(MPI_Scan(send, data, cnt, ty.value,
                static_cast<const BaseOp&>(op).op, MPI_COMM_WORLD) != MPI_SUCCESS)
      util::log::fatal{} << "Error while performing an MPI inclusive scan!";
  });
}

void detail::exscan(void* data, std::size_t cnt, const Datatype& ty,
                    const Op& op) {
  SmallMem send(cnt * ty.sz);
  std::memcpy(send, data, cnt * ty.sz);
  auto l = mpiLock();
  segment(send, data, cnt, ty, [&](void* send, void* data, int cnt) {
    if(MPI_Exscan(send, data, cnt, ty.value,
       static_cast<const BaseOp&>(op).op, MPI_COMM_WORLD) != MPI_SUCCESS)
      util::log::fatal{} << "Error while performing an MPI exclusive scan!";
  });
}

void detail::gather_root(void* recv, std::size_t cnt, const Datatype& ty,
                    std::size_t rootRank) {
  assert(World::rank() == rootRank && "mpi::detail::gather_root is only valid at the root!");
  assert(cnt <= std::numeric_limits<int>::max() && "attempt to call mpi::detail::gather_root with a large cnt");
  SmallMem send(cnt * ty.sz);
  std::memcpy(send, (char*)recv + cnt * ty.sz * rootRank, cnt * ty.sz);
  auto l = mpiLock();
  if(MPI_Gather(send, cnt, ty.value, recv, cnt, ty.value, rootRank,
                MPI_COMM_WORLD) != MPI_SUCCESS)
    util::log::fatal{} << "Error while performing an MPI gather!";
}

void detail::gather(void* send, std::size_t cnt, const Datatype& ty,
                    std::size_t rootRank) {
  assert(World::rank() != rootRank && "mpi::detail::gather is not valid at the root!");
  assert(cnt <= std::numeric_limits<int>::max() && "attempt to call mpi::detail::gather with a large cnt");
  auto l = mpiLock();
  if(MPI_Gather(send, cnt, ty.value, nullptr, cnt, ty.value, rootRank,
                MPI_COMM_WORLD) != MPI_SUCCESS)
    util::log::fatal{} << "Error while performing an MPI gather!";
}

void detail::gatherv_root(void* recv, const std::size_t* cnts,
                          const Datatype& ty, std::size_t rootRank) {
  assert(World::rank() == rootRank && "mpi::detail::gatherv_root is only valid at the root!");
  std::vector<int> icnts(World::size());
  std::vector<int> ioffsets(World::size());
  for(std::size_t i = 0, idx = 0; i < World::size(); i++) {
    assert(idx <= std::numeric_limits<int>::max() && "attempt to call mpi::detail::gatherv_root with a large cnt");
    icnts[i] = cnts[i];
    ioffsets[i] = idx;
    idx += cnts[i];
  }
  SmallMem send(cnts[rootRank] * ty.sz);
  std::memcpy(send, (char*)recv + ty.sz * ioffsets[rootRank], cnts[rootRank] * ty.sz);
  auto l = mpiLock();
  if(MPI_Gatherv(send, cnts[rootRank], ty.value, recv, icnts.data(), ioffsets.data(),
                 ty.value, rootRank, MPI_COMM_WORLD) != MPI_SUCCESS)
    util::log::fatal{} << "Error while performing an MPI vectorized gather (root)!";
}

void detail::gatherv(void* send, std::size_t cnt, const Datatype& ty,
                     std::size_t rootRank) {
  assert(World::rank() != rootRank && "mpi::detail::gatherv is not valid at the root!");
  assert(cnt <= std::numeric_limits<int>::max() && "attempt to call mpi::detail::gatherv with a large cnt");
  auto l = mpiLock();
  if(MPI_Gatherv(send, cnt, ty.value, nullptr, nullptr, nullptr, ty.value,
                 rootRank, MPI_COMM_WORLD) != MPI_SUCCESS)
    util::log::fatal{} << "Error while performing an MPI vectorized gather (non-root)!";
}

void detail::scatter_root(void* send, std::size_t cnt, const Datatype& ty,
                          std::size_t rootRank) {
  assert(World::rank() == rootRank && "mpi::detail::scatter_root is only valid at the root!");
  assert(cnt <= std::numeric_limits<int>::max() && "attempt to call mpi::detail::scatter_root with a large cnt");
  SmallMem recv(cnt * ty.sz);
  auto l = mpiLock();
  if(MPI_Scatter(send, cnt, ty.value, recv, cnt, ty.value, rootRank,
                MPI_COMM_WORLD) != MPI_SUCCESS)
    util::log::fatal{} << "Error while performing an MPI scatter!";
}

void detail::scatter(void* data, std::size_t cnt, const Datatype& ty,
                    std::size_t rootRank) {
  assert(World::rank() != rootRank && "mpi::detail::scatter is not valid at the root!");
  assert(cnt <= std::numeric_limits<int>::max() && "attempt to call mpi::detail::scatter with a large cnt");
  auto l = mpiLock();
  if(MPI_Scatter(nullptr, cnt, ty.value, data, cnt, ty.value, rootRank,
                 MPI_COMM_WORLD) != MPI_SUCCESS)
    util::log::fatal{} << "Error while performing an MPI scatter!";
}

void detail::scatterv_root(void* send, const std::size_t* cnts, const Datatype& ty,
                      std::size_t rootRank) {
  assert(World::rank() == rootRank && "mpi::detail::scatterv_root is only valid at the root!");
  std::vector<int> icnts(World::size());
  std::vector<int> ioffsets(World::size());
  for(std::size_t i = 0, idx = 0; i < World::size(); i++) {
    assert(idx <= std::numeric_limits<int>::max() && "attempt to call mpi::detail::scatterv_root with a large cnt");
    icnts[i] = cnts[i];
    ioffsets[i] = idx;
    idx += cnts[i];
  }
  SmallMem recv(cnts[rootRank] * ty.sz);
  auto l = mpiLock();
  if(MPI_Scatterv(send, icnts.data(), ioffsets.data(), ty.value,
                  recv, icnts[rootRank], ty.value, rootRank, MPI_COMM_WORLD) != MPI_SUCCESS)
    util::log::fatal{} << "Error while performing an MPI vectorized scatter (root)!";
}

void detail::scatterv(void* data, std::size_t cnt, const Datatype& ty,
                      std::size_t rootRank) {
  assert(World::rank() != rootRank && "mpi::detail::scatterv is not valid at the root!");
  assert(cnt <= std::numeric_limits<int>::max() && "attempt to call mpi::detail::scatterv with a large cnt");
  auto l = mpiLock();
  if(MPI_Scatterv(nullptr, nullptr, nullptr, ty.value,
                  data, cnt, ty.value, rootRank, MPI_COMM_WORLD) != MPI_SUCCESS)
    util::log::fatal{} << "Error while performing an MPI vectorized scatter (non-root)!";
}

void detail::send(const void* data, std::size_t cnt, const Datatype& ty,
                  Tag tag, std::size_t dst) {
  auto l = mpiLock();
  segment(data, cnt, ty, [&](const void* data, int cnt) {
    if(MPI_Send(data, cnt, ty.value, dst, static_cast<int>(tag), MPI_COMM_WORLD) != MPI_SUCCESS)
      util::log::fatal{} << "Error while performing an MPI send!";
  });
}

void detail::recv(void* data, std::size_t cnt, const Datatype& ty,
                  Tag tag, std::size_t src) {
  auto l = mpiLock();
  segment(data, cnt, ty, [&](void* data, int cnt) {
    if(MPI_Recv(data, cnt, ty.value, src, static_cast<int>(tag), MPI_COMM_WORLD, MPI_STATUS_IGNORE)
       != MPI_SUCCESS)
      util::log::fatal{} << "Error while performing an MPI receive!";
  });
}


std::optional<std::size_t> detail::recv_server(void* data, std::size_t cnt,
    const Datatype& ty, Tag tag) {
  assert(cnt <= std::numeric_limits<int>::max() && "attempt to call mpi::detail::recv_server with a large cnt");

  auto l = mpiLock();
  MPI_Status stat;
  if(l) {
    // Slow version, try to release the lock as often as possible.
    MPI_Request req;
    if(MPI_Irecv(data, cnt, ty.value, MPI_ANY_SOURCE, static_cast<int>(tag), MPI_COMM_WORLD, &req) != MPI_SUCCESS)
      util::log::fatal{} << "Error while performing an MPI non-blocking receive!";
    int done = 0;
    while(1) {
      if(MPI_Test(&req, &done, &stat) != MPI_SUCCESS)
        util::log::fatal{} << "Error while testing an MPI non-blocking receive!";
      if(done) break;
      l.unlock();
      std::this_thread::yield();
      l.lock();
    }
  } else {
    if(MPI_Recv(data, cnt, ty.value, MPI_ANY_SOURCE, static_cast<int>(tag), MPI_COMM_WORLD, &stat) != MPI_SUCCESS)
      util::log::fatal{} << "Error while performing an MPI server receive!";
  }
  int cntrecvd;
  if(MPI_Get_count(&stat, ty.value, &cntrecvd) != MPI_SUCCESS)
    util::log::fatal{} << "Error decoding a server message status!";
  if(cntrecvd == 0)
    return std::nullopt;  // Cancellation message is a 0-length message
  return stat.MPI_SOURCE;
}

void detail::cancel_server(const Datatype& ty, Tag tag) {
  auto l = mpiLock();
  if(MPI_Send(nullptr, 0, ty.value, World::rank(), static_cast<int>(tag), MPI_COMM_WORLD) != MPI_SUCCESS)
    util::log::fatal{} << "Error while self-sending a cancellation message!";
}
