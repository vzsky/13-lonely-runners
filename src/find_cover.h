#pragma once

#include <algorithm>
#include <atomic>
#include <bitset>
#include <cassert>
#include <climits>
#include <cstdint>
#include <functional>
#include <iostream>
#include <numeric>
#include <thread>
#include <utility>
#include <vector>

#include "speedset.h"
#include "utils.h"

namespace find_cover
{

template <int K, int P> struct Context
{
  using CoveredBitset = std::bitset<P / 2>;
  using CovArray      = std::array<CoveredBitset, P / 2>;
  CovArray cov{};

  Context()
  {
    for (int i = 0; i < P / 2; ++i)
      for (int t = 1; t <= P / 2; ++t)
      {
        int pos     = P / 2 - t;
        int rem     = int((1LL * t * (i + 1)) % P);
        cov[i][pos] = (1LL * rem * (K + 1) < P) || (1LL * (P - rem) * (K + 1) < P);
      }
  }
};

template <int K, int P> inline const Context<K, P> context{};

template <int K, int P> struct Dfs
{
  static constexpr int bitlen = P / 2;
  using CoveredBitset         = typename Context<K, P>::CoveredBitset;

  struct State
  {
    struct AvailableChoice;

    int depth;
    CoveredBitset covered;
    SpeedSet<K> elems;
    AvailableChoice choice;

  } state;

  SetOfSpeedSets<K> solutions{};
  const typename Context<K, P>::CovArray& cov = context<K, P>.cov;

  void run()
  {
    if (state.depth == K)
    {
      if (state.covered.count() != bitlen) return;
      solutions.insert(state.elems.get_canonical_representation(P));
      return;
    }

    int nextToCover = state.choice.get_next_to_cover(state.covered);
    if (early_return_bound(nextToCover)) return;

    const auto saved_choice = state.choice;

    for (int i = 0; i < bitlen; ++i)
    {
      if (state.choice.isEliminated(i)) continue;
      if (nextToCover == -1 || cov[i][nextToCover])
      {
        state.elems.insert(i + 1);
        CoveredBitset mem = state.covered;
        state.covered |= cov[i];
        state.depth++;

        run();

        state.elems.remove(i + 1);
        state.choice.eliminate(i);
        state.covered = mem;
        state.depth--;
      }
    }

    state.choice = saved_choice;
  }

private:
  bool early_return_bound(int nextToCover)
  {
    if (nextToCover != -1 && !state.choice.canBeCovered(nextToCover)) return true;
    if (state.depth < K - 4 || nextToCover == -1) return false; // TODO: K - 4 is arbitrary

    int slots = K - state.depth - 1;

    CoveredBitset nextC = ~state.covered;
    nextC[nextToCover]  = 0;

    int totalToCover = bitlen - state.covered.count();

    int bestCovering_next = 0;
    int bestCovering      = 0;
    for (int i = 0; i < bitlen; ++i)
    {
      if (state.choice.isEliminated(i)) continue;
      int c        = (nextC & cov[i]).count();
      bestCovering = std::max(bestCovering, c);
      if (cov[i][nextToCover]) bestCovering_next = std::max(bestCovering_next, c + 1);
    }

    return totalToCover > bestCovering_next + bestCovering * slots;
  }
};

template <int K, int P> static SetOfSpeedSets<K> find_all_covers_parallel()
{
  // Fix first coordinate to 1 and generate all second coordinate per worker thread.
  using CoveredBitset = typename Dfs<K, P>::CoveredBitset;
  const auto& cov     = context<K, P>.cov;

  typename Dfs<K, P>::State::AvailableChoice base_choice;
  CoveredBitset first_covered;
  SpeedSet<K> elems{};
  elems.insert(1);
  first_covered |= cov[0];

  int nextToCover1 = base_choice.get_next_to_cover(first_covered);

  std::vector<int> top_candidates;
  for (int i = 0; i < P / 2; ++i)
    if (nextToCover1 == -1 || cov[i][nextToCover1]) top_candidates.push_back(i);

  const size_t ncands = top_candidates.size();
  std::vector<typename Dfs<K, P>::State::AvailableChoice> choices(ncands + 1);

  choices[0] = base_choice;
  for (size_t idx = 0; idx < ncands; ++idx)
  {
    choices[idx + 1] = choices[idx];
    choices[idx + 1].eliminate(top_candidates[idx]);
  }

  size_t nthreads = parallelize_core();
  if (nthreads > ncands) nthreads = ncands;

  std::atomic<size_t> next_idx{0};
  std::vector<SetOfSpeedSets<K>> thread_results(nthreads);
  std::vector<std::thread> threads;

  Log("Spawning", nthreads, "threads for", ncands, "DfsStates");
  for (size_t t = 0; t < nthreads; ++t)
  {
    threads.emplace_back([&, t]
    {
      while (true)
      {
        size_t idx = next_idx.fetch_add(1, std::memory_order_relaxed);
        if (idx >= ncands) break;

        int i = top_candidates[idx];

        SpeedSet<K> local_elems = elems;
        local_elems.insert(i + 1);

        Dfs<K, P> d(typename Dfs<K, P>::State{2, first_covered | cov[i], local_elems, choices[idx]});
        d.run();
        thread_results[t].merge(d.solutions);
      }
    });
  }

  for (auto& th : threads) th.join();

  SetOfSpeedSets<K> base_solutions;
  for (size_t t = 0; t < nthreads; ++t) base_solutions.merge(thread_results[t]);

  return base_solutions;
}

template <int K, int P> struct Dfs<K, P>::State::AvailableChoice
{
private:
  using ElimArray   = std::array<char, P / 2>;
  using RemainArray = std::array<char, P / 2>;

  ElimArray _eliminated{};  // bool for each choice
  RemainArray _remaining{}; // count of active choice that cover position i
public:
  AvailableChoice()
  {
    for (int i = 0; i < bitlen; ++i)
      for (int pos = 0; pos < bitlen; ++pos)
        if (context<K, P>.cov[i][pos]) _remaining[pos]++;
  }

  bool isEliminated(size_t i) { return _eliminated[i]; }
  bool canBeCovered(size_t i) { return _remaining[i] != 0; }

  int get_next_to_cover(CoveredBitset current_covered)
  {
    int nextToCover = -1, best = INT_MAX;
    for (int pos = 0; pos < bitlen; ++pos)
      if (!current_covered[pos] && _remaining[pos] < best)
      {
        best        = _remaining[pos];
        nextToCover = pos;
      }
    return nextToCover;
  }

  void eliminate(int i)
  {
    _eliminated[i] = 1;
    for (int pos = 0; pos < bitlen; ++pos)
      if (context<K, P>.cov[i][pos]) _remaining[pos]--;
  }
};

} // namespace find_cover
