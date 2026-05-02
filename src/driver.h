#pragma once

#include <cassert>
#include <format>
#include <tuple>
#include <utility>

#include "find_cover.h"
#include "lift_strategy.h"
#include "utils.h"

template <int... nums>
  requires(isPrime(nums) && ...)
struct PrimeList
{
};

template <typename Config, int P, int K> void check_prime()
{
  Log(std::format("now={}", print_time()));
  Log(std::format("Parameters: p = {}, k = {}", P, K));

  auto st = timeit([&]() -> State<1, P, K>
  {
    PushLogScope("FindCover");
    auto ansatz = find_cover::find_all_covers_parallel<P, K>();
    Log(std::format("Step 1 (l=1): S size = {}", ansatz.size()));
    return {ansatz};
  });

  const auto final_st = [&]
  {
    PushLogScope("Lift");
    return apply_config<Config>(std::move(st));
  }();

  if (!final_st.ansatz.empty())
    Log(std::format("Counter Example Mod {}", P));
  else
    Log(std::format("Subproof Mod {} Done", P));
  Log();
}

template <typename Config, int K, int... Primes> void roll_works(PrimeList<Primes...>)
{
  timeit("All work done", [] { (check_prime<Config, Primes, K>(), ...); });
}
