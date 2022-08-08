#ifndef MITHRIL_SRC_NANOTUBE_H
#define MITHRIL_SRC_NANOTUBE_H

#include <algorithm>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/connected_components.hpp>
#include <cstddef>
#include <iterator>
#include <quile/quile.h>
#include <ranges>
#include <sstream>
#include <tuple>
#include <vector>

namespace mithril {

// Finding neighbors in hexagonal lattice with periodic boundary conditions.
class hex_lattice_pbc
{
public:
  hex_lattice_pbc(std::size_t n_phi, std::size_t n_z);
  std::size_t right(std::size_t i) const;
  std::size_t left(std::size_t i) const;
  std::size_t up_right(std::size_t i) const;
  std::size_t up_left(std::size_t i) const { return left(up_right(i)); }
  std::size_t down_right(std::size_t i) const;
  std::size_t down_left(std::size_t i) const { return left(down_right(i)); }
  std::vector<std::size_t> neighbors(std::size_t i) const;

private:
  std::size_t n_phi_;
  std::size_t n_z_;
};

// Finding neighbors in hexagonal lattice without periodic boundary conditions.
class hex_lattice_ord
{
public:
  hex_lattice_ord(std::size_t n_phi, std::size_t n_z);
  std::size_t right(std::size_t i) const;
  std::size_t left(std::size_t i) const;
  std::size_t up_right(std::size_t i) const;
  std::size_t up_left(std::size_t i) const;
  std::size_t down_right(std::size_t i) const;
  std::size_t down_left(std::size_t i) const;
  std::vector<std::size_t> neighbors(std::size_t i) const;

private:
  std::size_t n_phi_;
  std::size_t n_z_;
};

// Atoms' indices.
template<quile::binary_chromosome G>
std::vector<std::size_t>
atoms(const G& g)
{
  std::vector<std::size_t> res{};
  for (std::size_t i = 0; i < G::size(); ++i) {
    if (g.value(i)) {
      res.push_back(i);
    }
  }
  return res;
}

// Number of atoms encoded by genotype.
template<quile::binary_chromosome G>
std::size_t
number_of_atoms(const G& g)
{
  return std::ranges::count(g, true);
}

// Neighbor atoms of index i (periodic boundary condition).
template<quile::binary_chromosome G, std::size_t n_phi, std::size_t n_z>
std::vector<std::size_t>
neighbor_atoms_pbc(const G& g, std::size_t i)
{
  assert(i < G::size());
  static const hex_lattice_pbc hl{ n_phi, n_z };
  std::vector<std::size_t> res{};
  std::ranges::copy_if(hl.neighbors(i),
                       std::back_inserter(res),
                       [&](std::size_t j) { return g.value(j); });
  return res;
}

// Neighbor atoms of index i (without periodic boundary condition).
template<quile::binary_chromosome G, std::size_t n_phi, std::size_t n_z>
std::vector<std::size_t>
neighbor_atoms_ord(const G& g, std::size_t i)
{
  assert(i < G::size());
  static const hex_lattice_ord hl{ n_phi, n_z };
  std::vector<std::size_t> res{};
  std::ranges::copy_if(hl.neighbors(i),
                       std::back_inserter(res),
                       [&](std::size_t j) { return g.value(j); });
  return res;
}

// Number of neighbor atoms of index i (periodic boundary condition).
template<quile::binary_chromosome G, std::size_t n_phi, std::size_t n_z>
std::size_t
number_of_neighbor_atoms_pbc(const G& g, std::size_t i)
{
  assert(i < G::size());
  static const hex_lattice_pbc hl{ n_phi, n_z };
  return std::ranges::count(
    hl.neighbors(i), true, [&](std::size_t j) { return g.value(j); });
}

// Number of neighbor atoms of index i (without periodic boundary condition).
template<quile::binary_chromosome G, std::size_t n_phi, std::size_t n_z>
std::size_t
number_of_neighbor_atoms_ord(const G& g, std::size_t i)
{
  assert(i < G::size());
  static const hex_lattice_ord hl{ n_phi, n_z };
  return std::ranges::count(
    hl.neighbors(i), true, [&](std::size_t j) { return g.value(j); });
}

// Motif decomposition (n_0, n_1, ..., n_6).
template<quile::binary_chromosome G, std::size_t n_phi, std::size_t n_z>
std::vector<std::size_t>
decomposition(const G& g)
{
  std::vector<std::size_t> res{ 0, 0, 0, 0, 0, 0, 0 };
  for (std::size_t i = 0; i < G::size(); ++i) {
    if (g.value(i)) {
      ++res[number_of_neighbor_atoms_pbc<G, n_phi, n_z>(g, i)];
    }
  }
  return res;
}

// Energy from decomposition model.
template<quile::binary_chromosome G, std::size_t n_phi, std::size_t n_z>
double
energy_from_model(const G& g, const double* decomposition_values)
{
  double res{ 0. };
  for (std::size_t i = 0; auto n : decomposition<G, n_phi, n_z>(g)) {
    res += n * decomposition_values[i++];
  }
  return res / number_of_atoms(g);
}

// Predicate testing whether atoms are connected within unit cell.
template<quile::binary_chromosome G, std::size_t n_phi, std::size_t n_z>
bool
atoms_connected_in_unit_cell(const G& g)
{
  static const hex_lattice_ord hl{ n_phi, n_z };
  boost::adjacency_matrix<boost::undirectedS> am{ G::size() };
  for (auto i : atoms(g)) {
    for (auto j : neighbor_atoms_ord<G, n_phi, n_z>(g, i)) {
      boost::add_edge(i, j, am);
    }
  }
  std::vector<std::size_t> c(boost::num_vertices(am));
  return number_of_atoms(g) + boost::connected_components(am, c.data()) ==
         1 + G::size();
}

// Predicate testing whether at least one atom at unit cell boundary along
// nanotube has at least one neighbor (periodic boundary condition).
template<quile::binary_chromosome G, std::size_t n_phi, std::size_t n_z>
bool
adjacency_at_unit_cell_boundary_along_nanotube(const G& g)
{
  assert(G::size() == 2 * n_phi * n_z);
  static const hex_lattice_pbc hl{ n_phi, n_z };
  for (std::size_t i = 2 * n_z - 1; i < G::size(); i += 2 * n_z) {
    if (g.value(i) && (g.value(hl.up_right(i)) || g.value(hl.down_right(i)))) {
      return true;
    }
  }
  for (std::size_t i = n_z - 1; i < G::size(); i += 2 * n_z) {
    if (g.value(i) && g.value(hl.right(i))) {
      return true;
    }
  }
  return false;
}

// Predicate testing whether at least one atom at unit cell boundary at
// circumference of nanotube has at least one neighbor (periodic boundary
// condition).
template<quile::binary_chromosome G, std::size_t n_phi, std::size_t n_z>
bool
adjacency_at_unit_cell_boundary_at_circumference(const G& g)
{
  assert(G::size() == 2 * n_phi * n_z);
  static const hex_lattice_pbc hl{ n_phi, n_z };
  for (std::size_t i = G::size() - n_z; i < G::size(); ++i) {
    if (g.value(i) && (g.value(hl.up_left(i)) || g.value(hl.up_right(i)))) {
      return true;
    }
  }
  return false;
}

// operator>> reads genotype from the stream.
// TODO: Move this to the Quilë library.
template<quile::chromosome G>
std::istream&
operator>>(std::istream& is, G& g)
{
  for (std::size_t i = 0; i < G::size(); ++i) {
    typename G::gene_t v{};
    is >> v;
    g.value(i, v);
  }
  return is;
}

} // namespace mithril

#endif // MITHRIL_SRC_NANOTUBE_H