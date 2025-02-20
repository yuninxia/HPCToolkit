// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef HPCTOOLKIT_PROFILE_METRIC_H
#define HPCTOOLKIT_PROFILE_METRIC_H

#include "accumulators.hpp"
#include "attributes.hpp"
#include "expression.hpp"

#include "util/locked_unordered.hpp"
#include "util/uniqable.hpp"
#include "util/ragged_vector.hpp"

#include <atomic>
#include <bitset>
#include <iosfwd>
#include <functional>
#include <optional>
#include <variant>
#include <vector>

namespace hpctoolkit {

class Context;

class Metric;
class StatisticPartial;

/// A Statistic represents a combination of Metric data across all Threads,
/// on a per-Context basis. These are generated via three formulas:
///  - "accumulate": converts the thread-local value into an accumulation,
///  - "combine": combines two accumulations into a valid accumulation,
///  - "finalize": converts an accumulation into a presentable final value.
/// In total, this allows for condensed information regarding the entire
/// profiled execution, while still permitting inspection of individual Threads.
class Statistic final {
public:
  Statistic(const Statistic&) = delete;
  Statistic(Statistic&&) = default;
  Statistic& operator=(const Statistic&) = delete;
  Statistic& operator=(Statistic&&) = default;

  // Only a few combination formulas are permitted. This is the set.
  enum class combination_t { sum, min, max };

  /// Statistics are created by the associated Metric.
  Statistic() = delete;

  /// Get the additional suffix associated with this Statistic.
  /// E.g. "Sum" or "Avg".
  // MT: Safe (const)
  const std::string& suffix() const noexcept { return m_suffix; }

  /// Get whether the percentage should be shown for this Statistic.
  /// TODO: Figure out what property this indicates mathematically
  // MT: Safe (const)
  bool showPercent() const noexcept { return m_showPerc; }

  /// Get whether this Statistic should be presented by default.
  // MT: Safe (const)
  bool visibleByDefault() const noexcept { return m_visibleByDefault; }

  /// Get the Expression used generate the final value for this Statistic.
  /// The variables in this Expression are indices of the Partials for the
  /// associated Metric.
  // MT: Safe (const)
  const Expression& finalizeFormula() const noexcept { return m_formula; }

private:
  std::string m_suffix;
  bool m_showPerc;
  Expression m_formula;
  bool m_visibleByDefault;

  friend class Metric;
  Statistic(std::string, bool, Expression, bool);
};

/// Standard stringification for Statistic::combination_t values
std::ostream& operator<<(std::ostream&, Statistic::combination_t);

/// A StatisticPartial is the "accumulate" and "combine" parts of a Statistic.
/// There may be multiple Partials used for a Statistic, and multiple Statistics
/// can share the same Partial.
class StatisticPartial final {
public:
  StatisticPartial(const StatisticPartial&) = delete;
  StatisticPartial(StatisticPartial&&) = default;
  StatisticPartial& operator=(const StatisticPartial&) = delete;
  StatisticPartial& operator=(StatisticPartial&&) = delete;

  /// Get the Expression representing the "accumulate" function for this
  /// Partial. Single-variable (which is 0).
  // MT: Safe (const)
  const Expression& accumulate() const noexcept { return m_accum; }

  /// Get the combination function used for this Partial
  // MT: Safe (const)
  Statistic::combination_t combinator() const noexcept { return m_combin; }

private:
  const Expression m_accum;
  const Statistic::combination_t m_combin;
  const std::size_t m_idx;

  friend class Metric;
  friend class PerThreadTemporary;
  friend class StatisticAccumulator;
  StatisticPartial(Expression a, Statistic::combination_t c, std::size_t idx)
    : m_accum(std::move(a)), m_combin(std::move(c)), m_idx(idx) {};
};

/// Metrics represent something that is measured at execution.
class Metric final {
public:
  using ud_t = util::ragged_vector<const Metric&>;

  /// Identifier used for a Metric. Provides enough "space" for unique ids for
  /// every StatisticPartial and MetricScope.
  class Identifier final {
  public:
    explicit Identifier(const Metric& metric) : metric(metric), value(-1) {};
    Identifier(const Metric& metric, unsigned int v) : metric(metric), value(v) {};
    ~Identifier() = default;

    Identifier(Identifier&&) = default;
    Identifier(const Identifier&) = default;
    Identifier& operator=(Identifier&&) = default;
    Identifier& operator=(const Identifier&) = default;

    /// Get the base Metric this Identifier is designed for.
    // MT: Safe (const)
    const Metric& getMetric() const noexcept { return metric; }

    /// Alter the base index of an Identifier. `value` must be chosen such
    /// that no other Metric's Identifier has a base index within the interval
    ///     [value, value + max(metric.partials().size(), 1) * metric.scopes().size() )
    // MT: Externally Synchronized
    Identifier& operator=(unsigned int v) noexcept { value = v; return *this; }

    /// Get the base index for this Identifier.
    // MT: Safe (const)
    unsigned int base() const noexcept { return value; }

    /// Get an identifier unique for the Metric. May overlap with identifiers
    /// for Partials or Scopes.
    // MT: Safe (const)
    unsigned int getFor() const noexcept { return value; }
    operator unsigned int() const noexcept { return value; }

    /// Get an identifier unique for a StatisticPartial. Will not overlap with
    /// identifiers for other Metrics. May overlap with identifiers for
    /// specific MetricScopes or whole Metrics.
    // MT: Safe (const)
    unsigned int getFor(const StatisticPartial& part) const noexcept {
      return value + part.m_idx * metric.get().scopes().size();
    }

    /// Get an identifier unique for a StatisticPartial/MetricScope pair. Will
    /// not overlap with identifiers for other Metrics. May overlap with the
    /// identifiers for the whole StatisticPartial or Metric.
    // MT: Safe (const)
    unsigned int getFor(const StatisticPartial& part, MetricScope ms) const noexcept {
      assert(metric.get().scopes().has(ms));
      return value + part.m_idx * metric.get().scopes().size() + static_cast<int>(ms);
    }

    /// Get an identifier unique to the MetricScope. Will not overlap with
    /// identifiers for other Metrics. May overlap with identifiers for
    /// StatisticPartials or the whole Metric.
    // MT: Safe (const)
    unsigned int getFor(MetricScope ms) const noexcept {
      assert(metric.get().scopes().has(ms));
      return value + static_cast<int>(ms);
    }

  private:
    std::reference_wrapper<const Metric> metric;
    unsigned int value;
  };

  /// Structure to be used for creating new Metrics. Encapsulates a number of
  /// smaller settings into a convenient structure.
  struct Settings {
    Settings()
      : scopes(MetricScopeSet::all), visibility(visibility_t::shownByDefault)
      {};
    Settings(std::string n, std::string d) : Settings() {
      name = std::move(n);
      description = std::move(d);
    }

    Settings(Settings&&) = default;
    Settings(const Settings&) = default;
    Settings& operator=(Settings&&) = default;
    Settings& operator=(const Settings&) = default;

    std::string name;
    std::string description;

    // The usual cost-based Metrics have all possible Scopes, but not all
    // Metrics do. This specifies which ones are available.
    MetricScopeSet scopes;

    // Metrics can have multiple visibilities depending on their needs.
    enum class visibility_t {
      shownByDefault, hiddenByDefault, invisible
    } visibility;

    // Metrics may have an ordering id assigned to them by the Sources. This
    // number has no semantic meaning but may be of use for human presentation.
    std::optional<unsigned int> orderId;

    bool operator==(const Settings& o) const noexcept {
      return name == o.name;
    }
  };

  /// Eventually the set of requested Statistics will be more general, but for
  /// now we just use an explicit bitfield.
  struct Statistics final {
    Statistics()
      : sum(false), mean(false), min(false), max(false), stddev(false),
        cfvar(false) {};

    bool sum : 1;
    bool mean : 1;
    bool min : 1;
    bool max : 1;
    bool stddev : 1;
    bool cfvar : 1;
  };

  const std::string& name() const noexcept { return u_settings().name; }
  const std::string& description() const noexcept { return u_settings().description; }
  MetricScopeSet scopes() const noexcept { return u_settings().scopes; }
  Settings::visibility_t visibility() const noexcept { return u_settings().visibility; }
  std::optional<unsigned int> orderId() const noexcept { return u_settings().orderId; }

  mutable ud_t userdata;

  /// List the StatisticPartials that are included in this Metric.
  // MT: Safe (const)
  const std::vector<StatisticPartial>& partials() const noexcept;

  /// List the Statistics that are included in this Metric.
  // MT: Safe (const)
  const std::vector<Statistic>& statistics() const noexcept;

  /// Obtain a pointer to the Statistic Accumulators for a particular Context.
  /// Returns `nullptr` if no Statistic data exists for the given Context.
  // MT: Safe (const), Unstable (before `metrics` wavefront)
  util::optional_ref<const StatisticAccumulator> getFor(const Context& c) const noexcept;

  /// Obtain a pointer to the Thread-local Accumulator for a particular Context.
  /// Returns `nullptr` if no metric data exists for the given Context.
  // MT: Safe (const), Unstable (before notifyThreadFinal)
  util::optional_ref<const MetricAccumulator> getFor(const PerThreadTemporary&, const Context& c) const noexcept;

  Metric(Metric&& m);

  /// Special reference to a Metric for adding Statistics and whatnot.
  /// Useful for limiting access to a Metric for various purposes.
  class StatsAccess final {
  public:
    StatsAccess(Metric& m) : m(m) {};

    /// Request the given standard Statistics to be added to the referenced Metric.
    // MT: Internally Synchronized
    void requestStatistics(Statistics);

    /// Request a Partial representing the sum of relevant values.
    // MT: Internally Synchronized
    std::size_t requestSumPartial();

  private:
    Metric& m;

    std::unique_lock<std::mutex> synchronize();
  };

  /// Convenience function to generate a StatsAccess handle.
  StatsAccess statsAccess() { return StatsAccess{*this}; }

private:
  util::uniqable_key<Settings> u_settings;
  Statistics m_thawed_stats;
  std::size_t m_thawed_sumPartial;
  std::mutex m_frozen_lock;
  std::atomic<bool> m_frozen;
  std::vector<StatisticPartial> m_partials;
  std::vector<Statistic> m_stats;

  friend class ProfilePipeline;
  Metric(ud_t::struct_t&, Settings);
  // "Freeze" the current Statistics and Partials. All requests through
  // StatsAccess must be processed before this point.
  // Returns true if this call was the one to perform the freeze.
  // MT: Internally Synchronized
  bool freeze();

  friend class util::uniqued<Metric>;
  util::uniqable_key<Settings>& uniqable_key() { return u_settings; }
};

/// On occasion there is a need to add new Statistics that are based on values
/// from multiple Metrics instead of just one, usually to present abstract
/// overview values that can't be processed any other way. These are written
/// as "Extra" Statistics, which are much like Statistics but are not bound
/// to any particular Metric.
class ExtraStatistic final {
public:
  ExtraStatistic(ExtraStatistic&& o) = default;
  ExtraStatistic(const ExtraStatistic&) = delete;

  struct MetricPartialRef final {
    MetricPartialRef(const Metric& m, std::size_t i)
      : metric(m), partialIdx(i) {};
    const Metric& metric;
    std::size_t partialIdx;

    const StatisticPartial& partial() const noexcept {
      return metric.partials()[partialIdx];
    }

    bool operator==(const MetricPartialRef& o) const noexcept {
      return &metric == &o.metric && partialIdx == o.partialIdx;
    }
  };

  /// The Settings for an ExtraStatistic are much like those for standard
  /// Metrics, with a few additions.
  struct Settings : public Metric::Settings {
    Settings()
      : Metric::Settings(), showPercent(true), formula(Expression(0.0)) {};
    Settings(Metric::Settings&& s)
      : Metric::Settings(std::move(s)), showPercent(true),
        formula(Expression(0.0)) {};

    Settings(Settings&& o) = default;
    Settings(const Settings&) = default;
    Settings& operator=(Settings&&) = default;
    Settings& operator=(const Settings&) = default;

    /// Whether the percentage should be shown for this ExtraStatistic.
    /// TODO: Figure out what property this indicates mathematically
    bool showPercent : 1;

    /// Formula used to calculate the values for the ExtraStatistic.
    /// Variables in this formula are pointers to Metrics.
    Expression formula;

    /// Printf-style format string to use when rendering resulting values.
    /// This should only contain a single conversion specification (argument)
    std::string format;

    bool operator==(const Settings& o) const noexcept {
      return this->Metric::Settings::operator==(o);
    }
  };

  /// ExtraStatistics are only ever created by the Pipeline.
  ExtraStatistic() = delete;

  const std::string& name() const noexcept { return u_settings().name; }
  const std::string& description() const noexcept { return u_settings().description; }
  MetricScopeSet scopes() const noexcept { return u_settings().scopes; }
  Settings::visibility_t visibility() const noexcept { return u_settings().visibility; }
  bool showPercent() const noexcept { return u_settings().showPercent; }
  const Expression& formula() const noexcept { return u_settings().formula; }
  const std::string& format() const noexcept { return u_settings().format; }
  std::optional<unsigned int> orderId() const noexcept { return u_settings().orderId; }

  /// Get whether this Statistic should be presented by default.
  // MT: Safe (const)
  bool visibleByDefault() const noexcept { return u_settings().visibility == Settings::visibility_t::shownByDefault; }

private:
  util::uniqable_key<Settings> u_settings;

  friend class ProfilePipeline;
  ExtraStatistic(Settings);

  friend class util::uniqued<ExtraStatistic>;
  util::uniqable_key<Settings>& uniqable_key() { return u_settings; }
};

}

namespace std {
  using namespace hpctoolkit;
  template<> struct hash<Metric::Settings> {
    std::size_t operator()(const Metric::Settings&) const noexcept;
  };
  template<> struct hash<ExtraStatistic::Settings> {
    std::hash<Metric::Settings> h;
    std::size_t operator()(const ExtraStatistic::Settings& s) const noexcept {
      return h(s);
    }
  };
}

#endif  // HPCTOOLKIT_PROFILE_METRIC_H
