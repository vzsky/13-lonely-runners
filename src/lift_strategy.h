#include <algorithm>
#include <cassert>
#include <format>
#include <iostream>
#include <tuple>
#include <utility>

#include "speedset.h"
#include "lift.h"

// ============================================================================
// State
// ============================================================================

template <int K, int L> struct State
{
  SetOfSpeedSets<K> S;
};

// ============================================================================
// Strategies
// ============================================================================

template <int Arg> struct Force
{
  friend std::ostream& operator<<(std::ostream& os, const Force&) { return os << "Force " << Arg; }

  template <int P, int K, int L> State<K, L * Arg> operator()(State<K, L> st) const
  {
    auto T = lift::find_lifted_covers_parallel<P, K, L, Arg>(st.S);
    Log(std::format("Forcing Lift c={}: T size = {}", Arg, T.size()));
    return {std::move(T)};
  }
};

struct Project
{
  friend std::ostream& operator<<(std::ostream& os, const Project&) { return os << "Project"; }

  template <int P, int K, int L> State<K, 1> operator()(State<K, L> st) const
  {
    SetOfSpeedSets<K> T;
    for (auto s : st.S) T.insert(s.project(P).get_sorted_set());
    Log("Projected down");
    return {std::move(T)};
  }
};

template <int Limit = 0> struct Print
{
  friend std::ostream& operator<<(std::ostream& os, const Print&) { return os << "Print"; }

  template <int P, int K, int L> State<K, L> operator()(State<K, L> st) const
  {
    if (st.S.size() == 0)
      Log("seeds: empty");
    else if (Limit == 0 || st.S.size() <= Limit)
      Log("seeds: ", st.S);
    return st;
  }
};

struct TightLargePrime
{
  friend std::ostream& operator<<(std::ostream& os, const TightLargePrime&)
  {
    return os << "Remove (1..k) for large prime";
  }

  template <int P, int K, int L> State<K, L> operator()(State<K, L> st) const
  {
    if (P < K * (K + 1)) {
      Log("Prime too small -- (1..k) not removed");
      return st;
    }

    st.S.erase([]
    {
      SpeedSet<K> onetok{};
      for (int i = 1; i <= K; i++) onetok.insert(i);
      return onetok;
    }());

    return st;
  }
};

template <int Arg, int MaxIter = 3> struct Squeeze
{
  friend std::ostream& operator<<(std::ostream& os, const Squeeze&) { return os << "Squeeze"; }

private:
  template <int P, int K, int CurL, int Remaining>
  static SetOfSpeedSets<K> loop(SetOfSpeedSets<K> lifted, SetOfSpeedSets<K> last)
  {
    if (last.size() == 0) return last;
    SetOfSpeedSets<K> U;
    for (auto s : lifted) U.insert(s.project(P).get_sorted_set());

    if (U.size() == last.size()) return last;

    Log(std::format("squeezing (l={}): S size = {}", CurL, lifted.size()));

    if constexpr (Remaining == 0)
      return U;
    else
    {
      SetOfSpeedSets<K> next = lift::find_lifted_covers_parallel<P, K, CurL, Arg>(lifted);
      return loop<P, K, CurL * Arg, Remaining - 1>(std::move(next), std::move(U));
    }
  }

public:
  template <int P, int K, int L> State<K, 1> operator()(State<K, L> st) const
  {
    SetOfSpeedSets<K> lifted = lift::find_lifted_covers_parallel<P, K, L, Arg>(st.S);
    return {loop<P, K, L * Arg, MaxIter - 1>(std::move(lifted), std::move(st.S))};
  }
};

// ============================================================================
// Tuple visitor
// ============================================================================

// tuples of functions represent functions composition
template <int P, int K, std::size_t I = 0, typename Tuple, typename S> auto apply_config(S st, const Tuple& t)
{
  if constexpr (I < std::tuple_size_v<Tuple>)
  {
    decltype(std::get<I>(t).template operator()<P, K>(st)) result;

    if (st.S.size() == 0)
      return decltype(apply_config<P, K, I + 1>(result, t)) {}; // empty return

    Log(std::format("Step 2.{}", I));
    timeit([&]
    {
      PushLogScope(std::format("2.{}", I));
      result = std::get<I>(t).template operator()<P, K>(st);
      Log(std::get<I>(t), " :: Done :: ", "S.size() =", result.S.size());
    });
    return apply_config<P, K, I + 1>(result, t);
  }
  else
    return st;
}
