#include <tuple>

#include "src/driver.h"
#include "src/lift_strategy.h"

template <int K> struct LrcVerifier
{
};

template <> struct LrcVerifier<6>
{
  using Primes =
      PrimeList<47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149,
                151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241>;
  using Config = std::tuple<TightLargePrime, Force<7>>;
};

template <> struct LrcVerifier<7>
{
  using Primes =
      PrimeList<47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149,
                151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241>;
  using Config = std::tuple<Force<2>, Force<2>, Force<2>>;
};

template <> struct LrcVerifier<8>
{
  using Primes =
      PrimeList<47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149,
                151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241>;
  using Config = std::tuple<Squeeze<2>, Force<3>, Force<3>>;
};

template <> struct LrcVerifier<9>
{
  using Primes =
      PrimeList<19, 53, 59, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151,
                157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257,
                263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317>;
  using Config = std::tuple<Squeeze<2>, Force<2>, Force<5>>;
};

template <> struct LrcVerifier<10>
{
  using Primes = PrimeList<127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199,
                           211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293,
                           307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397,
                           401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467>;
  using Config = std::tuple<TightLargePrime, Squeeze<2>>;
};

template <> struct LrcVerifier<11>
{
  using Primes = PrimeList<23, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211,
                           223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307,
                           311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401,
                           409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499,
                           503, 509, 521, 523, 541, 547, 557, 563, 569, 571, 577>;
  using Config = std::tuple<Force<2>, Force<2>, Squeeze<2>, Force<2>, Force<2>, Force<3>, Squeeze<3>>;
};

template <> struct LrcVerifier<12>
{
  using Primes =
      PrimeList<139, 149, 151, 167, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 251, 257, 263,
                269, 271, 277, 281, 283, 307, 311, 313, 347, 379, 433, 241, 293, 317, 331, 337, 349, 353, 359,
                367, 373, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 503, 509, 521, 523, 571, 577,
                613, 617, 619, 541, 547, 557, 599, 601, 607, 631, 641, 659, 661, 677, 683>;
  using Config = std::tuple<TightLargePrime, Squeeze<2>, Squeeze<3>, Print<>>;
};

template <> struct LrcVerifier<13>
{
  using Primes = PrimeList<199, 211, 223, 227, 229, 233, 239, 251, 257, 263, 269, 271, 277, 281, 283, 307,
                           311, 313, 347, 379, 433, 439, 443, 449, 457, 461, 241, 293, 293, 317, 331, 337,
                           349, 353, 359, 367, 373, 383, 389, 397, 401, 409, 419, 421, 431>;
  using Config = std::tuple<Squeeze<2>, Squeeze<3>, Print<>>;
};

template <> struct LrcVerifier<14>
{
  using Primes = PrimeList<239, 251, 257, 263, 269, 271, 277, 281, 283, 307, 311, 313, 347, 379, 433, 439,
                           443, 449, 457, 461, 241, 293, 293, 317, 331, 337, 349, 353, 359, 367, 373, 383,
                           389, 397, 401, 409, 419, 421, 431>;
  using Config = std::tuple<Squeeze<2>, Print<10>, Squeeze<3>, Print<10>>;
};

int main()
{
  constexpr int K = 9;
  using Primes    = LrcVerifier<K>::Primes;
  using Config    = LrcVerifier<K>::Config;
  roll_works<Config, K>(Primes{});
}
