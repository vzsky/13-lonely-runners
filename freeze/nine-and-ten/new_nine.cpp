#include "../../src/driver.h"
#include "../../src/lift_strategy.h"

int main()
{
  using Primes =
      PrimeList<47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149,
                151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241>;
  using Config = std::tuple<Force<3>, Force<3>>;
  roll_works<Config, 8>(Primes{});
}
