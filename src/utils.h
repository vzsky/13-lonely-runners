#pragma once

#include <algorithm>
#include <cassert>
#include <chrono>
#include <climits>
#include <cstdint>
#include <format>
#include <functional>
#include <iostream>
#include <numeric>
#include <thread>
#include <utility>
#include <vector>

/************************************
 * Logging
 ************************************/

#include <iostream>
#include <string>
#include <vector>

namespace logging
{

inline thread_local std::vector<std::string> env_stack;

struct EnvGuard
{
  EnvGuard(std::string s) { env_stack.push_back(std::move(s)); }
  ~EnvGuard() { env_stack.pop_back(); }
};

inline void print_env()
{
  for (auto& e : env_stack) std::cout << "[" << e << "] ";
}

template <typename... Ts> void log(Ts&&... xs)
{
  print_env();
  ((std::cout << std::forward<Ts>(xs) << " "), ...);
  std::cout << std::endl;
}

#define PushLogScope(x)                                                                                      \
  logging::EnvGuard _log_env_guard_##__LINE__ { x }

#define Log(...) logging::log(__VA_ARGS__)

} // namespace logging

/************************************
 * Misc
 ************************************/

inline size_t parallelize_core()
{
  unsigned int hw = std::thread::hardware_concurrency();
  return hw ? std::max(20u, hw) : 4u;
}

// Fallback GCD for subset-gcd checking (used in sieve condition)
template <typename T> inline T gcd_fallback(T a, T b)
{
  a = std::abs(a);
  b = std::abs(b);
  while (b)
  {
    long long t = b;
    b           = a % b;
    a           = t;
  }
  return a;
}

template <typename T, typename... Ts> inline T gcd_fallback(T a, T b, Ts... rest)
{
  return gcd_fallback(gcd_fallback(a, b), rest...);
}

inline int mod_pow(int a, int e, int p)
{
  int res = 1 % p;
  a %= p;

  while (e > 0)
  {
    if (e & 1) res = (int)((long long)res * a % p);
    a = (int)((long long)a * a % p);
    e >>= 1;
  }

  return res;
}

inline int mod_inverse(int a, int p) { return mod_pow(a, p - 2, p); }

inline std::string print_time()
{
  auto now = std::chrono::system_clock::now();
  return std::format("{:%H:%M:%S}", now);
}

template <typename Func> void timeit(const std::string& s, Func&& f)
{
  using namespace std::chrono;
  const auto start = high_resolution_clock::now();

  f();

  const auto end = high_resolution_clock::now();
  const auto us = duration_cast<microseconds>(end - start).count();
  const auto sec    = us / 1'000'000;
  const auto rem_us = us % 1'000'000;
  Log(s, std::format("-- Time elapsed: {}.{:06d} s", sec, rem_us));
}

template <typename Func> void timeit(Func&& f) { timeit("", f); }

template <int I, int N, typename F> constexpr void For(F&& f)
{
  if constexpr (I < N)
  {
    f(std::integral_constant<int, I>{});
    For<int(I + 1), N>(f);
  }
}
