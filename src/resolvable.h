#include <array>
#include <functional>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

namespace resolvable
{

template <int K> class SpeedSet
{
public:
  static constexpr int k_   = K;
  static constexpr int mod_ = K + 1;
  using value_t             = std::array<int, K>;

  explicit SpeedSet(const value_t& components) : v_(components)
  {
    for (int x : v_)
      if (x < 0 || x >= mod_) throw std::invalid_argument("All components must be in Z_{k+1}.");
  }

  SpeedSet(std::initializer_list<int> il)
  {
    if ((int)il.size() != K) throw std::invalid_argument("Initializer list length must equal K.");
    std::copy(il.begin(), il.end(), v_.begin());
    for (int x : v_)
      if (x < 0 || x >= mod_) throw std::invalid_argument("All components must be in Z_{k+1}.");
  }

  static constexpr int k() { return k_; }
  static constexpr int mod() { return mod_; }

  const value_t& vec() const { return v_; }
  int operator[](int i) const { return v_[i]; }

  auto begin() const { return v_.begin(); }
  auto end() const { return v_.end(); }

  int count_zeros() const
  {
    int cnt = 0;
    for (int x : v_)
      if (x == 0) ++cnt;
    return cnt;
  }

  bool operator==(const SpeedSet<K>& other) const { return v_ == other.v_; }

  int count_nonzeros() const { return K - count_zeros(); }

  bool is_scatter() const
  {
    for (int x : v_)
      if (x == 0) return false;
    return true;
  }

  bool is_divisible() const
  {
    constexpr int threshold = (mod_ + 1) / 2; // ceil((k+1)/2) — constexpr!
    return count_zeros() >= threshold;
  }

  bool is_lonely(const std::unordered_set<SpeedSet<K>>& all_r) const
  {
    for (int s = 0; s < mod_; ++s)
    {
      for (const SpeedSet<K>& r : all_r)
      {
        bool all_middle = true;
        for (int i = 0; i < K; ++i)
        {
          int val = (s * v_[i] + r[i]) % mod_;
          if (val == 0 || val == K)
          {
            all_middle = false;
            break;
          }
        }
        if (all_middle) return true;
      }
    }
    return false;
  }

  bool is_resolvable(const std::unordered_set<SpeedSet<K>>& all_r) const
  {
    return is_scatter() || is_divisible() || is_lonely(all_r);
  }

private:
  value_t v_;
};

} // namespace resolvable

template <int K> std::ostream& operator<<(std::ostream& os, const resolvable::SpeedSet<K>& ss)
{
  os << '(';
  for (int i = 0; i < K; ++i)
  {
    if (i) os << ", ";
    os << ss[i];
  }
  return os << ')';
}

template <int K> struct std::hash<resolvable::SpeedSet<K>>
{
  std::size_t operator()(const resolvable::SpeedSet<K>& ss) const noexcept
  {
    std::size_t seed = K;
    for (int x : ss) seed ^= std::hash<int>{}(x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
  }
};

namespace resolvable
{

template <int K> SpeedSet<K> compute_r(long long r, long long p)
{
  constexpr int n = K + 1; // n = k+1

  typename SpeedSet<K>::value_t components;
  for (long long j = 1; j <= K; ++j)
  {
    long long rem     = (r * j) % p;
    components[j - 1] = static_cast<int>((n * rem) / p);
  }
  return SpeedSet<K>(components);
}

template <int K> std::unordered_set<SpeedSet<K>> all_r_components(long long p)
{
  std::unordered_set<SpeedSet<K>> seen;
  for (long long r = 1; r < p; ++r) seen.insert(compute_r<K>(r, p));
  return seen;
}

} // namespace resolvable
