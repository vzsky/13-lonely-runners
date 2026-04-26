#include "../src/driver.h"
#include "../src/lift_strategy.h"

int main()
{
  using Primes =
      PrimeList<19, 53, 59, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151,
                157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257,
                263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317>;
  using Config = std::tuple<Squeeze<2>, Force<2>, Force<5>>;
  roll_works<Config, 9>(Primes{});
}
