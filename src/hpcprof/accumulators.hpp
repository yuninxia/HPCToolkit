// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef HPCTOOLKIT_PROFILE_ACCUMULATORS_H
#define HPCTOOLKIT_PROFILE_ACCUMULATORS_H

#include "scope.hpp"

#include "util/locked_unordered.hpp"
#include "util/streaming_sort.hpp"

#include <atomic>
#include <bitset>
#include <chrono>
#include <iosfwd>
#include <optional>
#include <vector>

namespace hpctoolkit {

class Metric;
class StatisticPartial;
class Thread;
class Context;
class ContextReconstruction;
class ContextFlowGraph;

/// Every Metric can have values at multiple Scopes pertaining to the subtree
/// rooted at a particular Context with Metric data.
enum class MetricScope : size_t {
  /// Encapsulates the current Context, and no other nodes. This references
  /// exactly where the data arose, and is the smallest MetricScope.
  point,

  /// Encapsulates the current Context and any descendants not connected by a
  /// call-type Relation (call or inlined_call). This represents the cost of a
  /// function call ignoring any child function calls.
  function,

  /// Identical to function, but on a Type::*_loop Scope does not include any
  /// descendents below a child Type::*_loop Scope.
  /// Called "exclusive" in the Viewer.
  lex_aware,

  /// Encapsulates the current Context and all descendants. This represents
  /// the entire execution spawned by a single source code construct, and is
  /// the largest MetricScope.
  /// Called "inclusive" in the Viewer.
  execution,
};

/// Standardized stringification for MetricScope constants.
std::ostream& operator<<(std::ostream&, MetricScope);
const std::string& stringify(MetricScope);

/// Bitset-like object used as a set of Scope values.
class MetricScopeSet final : private std::bitset<4> {
private:
  using base = std::bitset<4>;
  MetricScopeSet(const base& b) : base(b) {};
public:
  using int_type = uint8_t;
  constexpr MetricScopeSet() = default;
  constexpr MetricScopeSet(MetricScope s) : base(1<<static_cast<size_t>(s)) {};
  MetricScopeSet(std::initializer_list<MetricScope> l) : base(0) {
    for(const auto s: l) base::set(static_cast<size_t>(s));
  }
  explicit constexpr MetricScopeSet(int_type val) : base(val) {};

  static inline constexpr struct all_t {} all = {};
  constexpr MetricScopeSet(all_t)
    : base(std::numeric_limits<unsigned long long>::max()) {}

  bool has(MetricScope s) const noexcept { return base::operator[](static_cast<size_t>(s)); }
  auto operator[](MetricScope s) noexcept { return base::operator[](static_cast<size_t>(s)); }

  using base::size;

  MetricScopeSet operator|(const MetricScopeSet& o) { return (base)*this | (base)o; }
  MetricScopeSet operator+(const MetricScopeSet& o) { return (base)*this | (base)o; }
  MetricScopeSet operator&(const MetricScopeSet& o) { return (base)*this & (base)o; }
  MetricScopeSet& operator|=(const MetricScopeSet& o) { base::operator|=(o); return *this; }
  MetricScopeSet& operator+=(const MetricScopeSet& o) { base::operator|=(o); return *this; }
  MetricScopeSet& operator&=(const MetricScopeSet& o) { base::operator&=(o); return *this; }
  bool operator==(const MetricScopeSet& o) { return base::operator==(o); }
  bool operator!=(const MetricScopeSet& o) { return base::operator!=(o); }

  int_type toInt() const noexcept { return base::to_ullong(); }

  using base::count;

  class const_iterator final {
  public:
    const_iterator() {};

    const_iterator(const const_iterator&) = default;
    const_iterator& operator=(const const_iterator&) = default;

    bool operator==(const const_iterator& o) const {
      return set == o.set && (set == nullptr || scope == o.scope);
    }
    bool operator!=(const const_iterator& o) const { return !operator==(o); }

    MetricScope operator*() const { return scope; }
    const MetricScope* operator->() const { return &scope; }

    const_iterator& operator++() {
      assert(set != nullptr);
      size_t ms = static_cast<size_t>(scope);
      for(++ms; ms < set->size() && !set->has(static_cast<MetricScope>(ms));
          ++ms);
      if(ms >= set->size()) set = nullptr;
      else scope = static_cast<MetricScope>(ms);
      return *this;
    }
    const_iterator operator++(int) {
      const_iterator old = *this;
      operator++();
      return old;
    }

  private:
    friend class MetricScopeSet;
    const MetricScopeSet* set = nullptr;
    MetricScope scope;
  };

  const_iterator begin() const {
    const_iterator it;
    it.set = this;
    it.scope = static_cast<MetricScope>(0);
    if(!base::operator[](0)) ++it;
    return it;
  }
  const_iterator end() const { return {}; }
};

/// Accumulator structure for the data implicitly bound to a Thread and Context.
class MetricAccumulator final {
public:
  MetricAccumulator() = default;

  MetricAccumulator(const MetricAccumulator&) = delete;
  MetricAccumulator& operator=(const MetricAccumulator&) = delete;
  MetricAccumulator(MetricAccumulator&&) = delete;
  MetricAccumulator& operator=(MetricAccumulator&&) = delete;

  /// Add some value to this Accumulator. Only point-Scope is allowed.
  // MT: Internally Synchronized
  void add(double) noexcept;

  /// Get the Thread-local sum of Metric value, for a particular Metric Scope.
  // MT: Safe (const), Unstable (before ThreadFinal wavefront)
  std::optional<double> get(MetricScope) const noexcept;

private:
  MetricScopeSet getNonZero() const noexcept;

  bool isLoop;

  friend class PerThreadTemporary;
  std::atomic<double> point = 0;
  double function = 0;
  double function_noloops = 0;
  double execution = 0;
};

/// Accumulators and other related fields local to a Thread.
class PerThreadTemporary final {
public:
  // Access to the backing thread.
  operator Thread&() noexcept { return m_thread; }
  operator const Thread&() const noexcept { return m_thread; }
  Thread& thread() noexcept { return m_thread; }
  const Thread& thread() const noexcept { return m_thread; }

  // Movable, not copiable
  PerThreadTemporary(const PerThreadTemporary&) = delete;
  PerThreadTemporary(PerThreadTemporary&&) = default;
  PerThreadTemporary& operator=(const PerThreadTemporary&) = delete;
  PerThreadTemporary& operator=(PerThreadTemporary&&) = delete;

  /// Reference to the Metric data for a particular Context in this Thread.
  /// Returns `std::nullopt` if none is present.
  // MT: Safe (const), Unstable (before notifyThreadFinal)
  auto accumulatorsFor(const Context& c) const noexcept {
    return c_data.find(c);
  }

  /// Reference to all of the Metric data on Thread.
  // MT: Safe (const), Unstable (before notifyThreadFinal)
  const auto& accumulators() const noexcept { return c_data; }

private:
  Thread& m_thread;

  friend class ProfilePipeline;
  PerThreadTemporary(Thread& t) : m_thread(t) {};

  // Finalize the MetricAccumulators for a Thread.
  // MT: Internally Synchronized
  void finalize() noexcept;

  // Bits needed for handling timepoints
  std::chrono::nanoseconds minTime = std::chrono::nanoseconds::max();
  std::chrono::nanoseconds maxTime = std::chrono::nanoseconds::min();
  template<class Tp>
  struct TimepointsData {
    bool unboundedDisorder = false;
    util::bounded_streaming_sort_buffer<Tp, util::compare_only_first<Tp>> sortBuf;
    std::vector<Tp> staging;
  };
  TimepointsData<std::pair<std::chrono::nanoseconds,
    std::reference_wrapper<const Context>>> ctxTpData;
  util::locked_unordered_map<util::reference_index<const Metric>,
    TimepointsData<std::pair<std::chrono::nanoseconds, double>>> metricTpData;

  friend class Metric;
  util::locked_unordered_map<util::reference_index<const Context>,
    util::locked_unordered_map<util::reference_index<const Metric>,
      MetricAccumulator>> c_data;
  util::locked_unordered_map<util::reference_index<const ContextReconstruction>,
    util::locked_unordered_map<util::reference_index<const Metric>,
      MetricAccumulator>> r_data;

  struct RGroup {
    util::locked_unordered_map<util::reference_index<const Context>,
      util::locked_unordered_map<util::reference_index<const Metric>,
        MetricAccumulator>> c_data;
    util::locked_unordered_map<util::reference_index<const ContextFlowGraph>,
      util::locked_unordered_map<util::reference_index<const Metric>,
        MetricAccumulator>> fg_data;

    std::mutex lock;
    std::unordered_map<Scope,
      std::unordered_set<util::reference_index<Context>>> c_entries;
    std::unordered_map<util::reference_index<ContextFlowGraph>,
      std::unordered_set<util::reference_index<const ContextReconstruction>>>
        fg_reconsts;
  };
  util::locked_unordered_map<uint64_t, RGroup> r_groups;
};

/// Accumulator structure for the Statistics implicitly bound to a Context.
class StatisticAccumulator final {
public:
  using raw_t = std::array<double, 5>;

  /// Get a tuple of raw accumulator values set to 0. This function should be
  /// used instead of manually filling raw_t for internal state.
  static raw_t rawZero() noexcept {
    return raw_t{0, 0, 0, 0, 0};
  }

private:
  /// Each Accumulator is a vector of Partial accumulators, each of which is
  /// bound implicitly to a particular Statistic::Partial.
  class Partial final {
  public:
    Partial() = default;
    ~Partial() = default;

    Partial(const Partial&) = delete;
    Partial& operator=(const Partial&) = delete;
    Partial(Partial&&) = delete;
    Partial& operator=(Partial&&) = delete;

  private:
    std::optional<double> get(MetricScope) const noexcept;
    raw_t getRaw() const noexcept;

    friend class StatisticAccumulator;
    friend class PerThreadTemporary;
    std::atomic<bool> isLoop = false;
    std::atomic<double> point = 0;
    std::atomic<double> function = 0;
    std::atomic<double> function_noloops = 0;
    std::atomic<double> execution = 0;
  };

public:
  class PartialRef final {
  public:
    PartialRef() = delete;
    PartialRef(const PartialRef&) = delete;
    PartialRef& operator=(const PartialRef&) = delete;
    PartialRef(PartialRef&&) = default;
    PartialRef& operator=(PartialRef&&) = delete;

    ~PartialRef() {
      assert(added && "Created a PartialRef but did not add any value!");
    }

    /// Get this Partial's accumulation, for a particular MetricScope.
    // MT: Safe (const), Unstable (before `metrics` wavefront)
    std::optional<double> get(MetricScope ms) const noexcept {
      return partial.get(ms);
    }

    /// Get the tuple of raw accumulator values for this Partial. Can be passed
    /// to addRaw() to include this Partial's accumulation within another.
    // MT: Safe (const), Unstable (before `metrics` wavefront)
    raw_t getRaw() const noexcept {
      return partial.getRaw();
    }

    /// Add a sequence of raw accumulator values to this Partial.
    // MT: Internally Synchronized
    void addRaw(const raw_t&) noexcept;

  private:
    friend class StatisticAccumulator;
    PartialRef(Partial& p, const StatisticPartial& sp)
      : partial(p), statpart(sp) {};

#ifndef NDEBUG
    bool added = false;
#endif
    Partial& partial;
    const StatisticPartial& statpart;
  };

  class PartialCRef final {
  public:
    PartialCRef() = delete;
    PartialCRef(const PartialCRef&) = delete;
    PartialCRef& operator=(const PartialCRef&) = delete;
    PartialCRef(PartialCRef&&) = default;
    PartialCRef& operator=(PartialCRef&&) = delete;

    /// Get this Partial's accumulation, for a particular MetricScope.
    // MT: Safe (const), Unstable (before `metrics` wavefront)
    std::optional<double> get(MetricScope ms) const noexcept {
      return partial.get(ms);
    }

    /// Get the tuple of raw accumulator values for this Partial. Can be passed
    /// to addRaw() to include this Partial's accumulation within another.
    // MT: Safe (const), Unstable (before `metrics` wavefront)
    raw_t getRaw() const noexcept {
      return partial.getRaw();
    }

  private:
    friend class StatisticAccumulator;
    PartialCRef(const Partial& p)
      : partial(p) {};

    const Partial& partial;
  };

  explicit StatisticAccumulator(const Metric&);
  ~StatisticAccumulator() = default;

  StatisticAccumulator(const StatisticAccumulator&) = delete;
  StatisticAccumulator& operator=(const StatisticAccumulator&) = delete;
  StatisticAccumulator(StatisticAccumulator&&) = default;
  StatisticAccumulator& operator=(StatisticAccumulator&&) = delete;

  /// Get the Partial accumulator for a particular Partial Statistic.
  // MT: Safe (const)
  PartialCRef get(const StatisticPartial&) const noexcept;
  PartialRef get(const StatisticPartial&) noexcept;

private:
  friend class PerThreadTemporary;
  std::vector<Partial> partials;
};

/// Accumulators and related fields local to a Context. In particular, holds
/// the statistics.
class PerContextAccumulators final {
public:
  PerContextAccumulators() = default;
  ~PerContextAccumulators() = default;

  PerContextAccumulators(const PerContextAccumulators&) = delete;
  PerContextAccumulators(PerContextAccumulators&&) = default;
  PerContextAccumulators& operator=(const PerContextAccumulators&) = delete;
  PerContextAccumulators& operator=(PerContextAccumulators&&) = delete;

  /// Access the Statistics data attributed to this Context
  // MT: Safe (const), Unstable (before `metrics` wavefront)
  const auto& statistics() const noexcept { return m_statistics; }

  /// Access the Statistics data for a particular Metric, attributed to this Context
  // MT: Internally Synchronized
  StatisticAccumulator& statisticsFor(const Metric& m) noexcept;

  /// Helper wrapper to allow a MetricScopeSet to be atomically modified.
  /// Used for the metricUsage() data.
  class AtomicMetricScopeSet final {
  public:
    AtomicMetricScopeSet() : val(MetricScopeSet().toInt()) {}
    ~AtomicMetricScopeSet() = default;

    AtomicMetricScopeSet(const AtomicMetricScopeSet&) = delete;
    AtomicMetricScopeSet(AtomicMetricScopeSet&&) = delete;
    AtomicMetricScopeSet& operator=(const AtomicMetricScopeSet&) = delete;
    AtomicMetricScopeSet& operator=(AtomicMetricScopeSet&&) = delete;

    /// Union the given MetricScopeSet into this atomic set
    // MT: Internally Synchronized
    AtomicMetricScopeSet& operator|=(MetricScopeSet ms) noexcept {
      val.fetch_or(ms.toInt(), std::memory_order_relaxed);
      return *this;
    }

    /// Load the current value of the MetricScopeSet
    // MT: Safe (const), Unstable (before `metrics` wavefront)
    MetricScopeSet get() const noexcept {
      return MetricScopeSet(val.load(std::memory_order_relaxed));
    }
    operator MetricScopeSet() const noexcept { return get(); }

  private:
    std::atomic<MetricScopeSet::int_type> val;
  };

  /// Access the use of this Context in Metric values
  // MT: Safe (const), Unstable (before `metrics` wavefront)
  const auto& metricUsage() const noexcept { return m_metricUsage; }

  /// Get the use of this Context for a particular Metric.
  // MT: Safe (const), Unstable (before `metrics` wavefront)
  MetricScopeSet metricUsageFor(const Metric& m) const noexcept {
    if(auto use = m_metricUsage.find(m)) return use->get();
    return {};
  }

  /// Mark this Context as having been used for the given Metric values
  // MT: Internally Synchronized
  void markUsed(const Metric& m, MetricScopeSet ms) noexcept;

private:
  friend class PerThreadTemporary;
  friend class Metric;
  friend class ProfilePipeline;
  util::locked_unordered_map<util::reference_index<const Metric>,
    StatisticAccumulator> m_statistics;
  util::locked_unordered_map<util::reference_index<const Metric>,
    AtomicMetricScopeSet> m_metricUsage;
};

}  // namespace hpctoolkit

#endif  // HPCTOOLKIT_PROFILE_ACCUMULATORS_H
