// The eight queens puzzle
// - function: negated number of checking queen pairs
// - domain: [0, n - 1]^n, n = 8
// - representation: integer
// - variation type: random reset, one-point crossover recombination
// - parents/surivor selection: stochastic universal sampling (SUS)
// - termination condition: fitness function treshold

#include <cassert>
#include <cstddef>
#include <fstream>
#include <string>
#include <quile/quile.h>

using namespace quile;

const std::size_t n = 8;

using type = int;

// Function
fitness f(const auto& chessboard) {
  assert(n == chessboard.size());
  int counts = 0;
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = i + 1; j < n; ++j) {
      const int d = j - i;
      if (chessboard[i] == chessboard[j]
          || chessboard[i] + d == chessboard[j]
          || chessboard[i] == chessboard[j] + d) {
        ++counts;
      }
    }
  }
  return -counts;
}

// Domain
constexpr domain<type, n> d = uniform_domain<type, n>(range<type>{0, n - 1});

using G = genotype<g_integer<type, n, &d>>;

std::string Forsyth_Edwards_Notation(const G& g) {
  std::string res{};
  for (std::size_t i = 0; auto x : g) {
    if (x != 0) {
      res += std::to_string(x);
    }
    res += 'Q';
    if (x != 7) {
      res += std::to_string(n - 1 - x);
    }
    if (++i != n) {
      res += '/';
    }
  }
  res += " w - - 0 0";
  return res;
}

int main() {
  const fitness_function<G> ff = [](const G& g) { return f(g.data()); };
  const fitness_db<G> fd{ff, constraints_satisfied<G>};
  const fitness_proportional_selection<G> fps{fd};

  // First generation creator
  const auto p0 =
    random_population<constraints_satisfied<G>, G>{};
  // Parents selection 
  const auto p1 = stochastic_universal_sampling<G>{fps};
  // Survivor selection
  const auto p2 = adapter<G>(stochastic_universal_sampling<G>{fps});

  const std::size_t generation_sz{1000};
  const std::size_t parents_sz{42};
  const auto tc = fitness_treshold_termination<G>(fd, 0., 0.01);
  
  const variation<G> v{random_reset<G>(1. / n), one_point_xover<G>};
  
  std::ofstream file{"evolution.dat"};
  for (std::size_t i = 0;
       const auto& x : evolution<G>(v,
                                    p0, p1, p2,
                                    tc,
                                    generation_sz, parents_sz,
                                    1)) {
    for (const auto& xx : x) {
      file << i;
      for (auto p : xx) {
        file << ' ' << p;
      }
      file << ' ' << Forsyth_Edwards_Notation(xx)
           << ' ' << fd(xx)
           << '\n';
    }
    ++i;
  }
}