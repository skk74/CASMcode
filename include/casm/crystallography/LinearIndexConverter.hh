#ifndef LINEARINDEXCONVERTER_HH
#define LINEARINDEXCONVERTER_HH

#include "casm/crystallography/LatticePointWithin.hh"
#include "casm/crystallography/UnitCellCoord.hh"
#include <unordered_map>
#include <vector>

namespace CASM {
  namespace xtal {

    /**
     * Converts back and forth between UnitCellCoord and its linear index,
     * where the linear index is guaranteed to preserve order based on the
     * sublattice index of the UnitCellCoord, and the Smith Normal Form
     * of the UnitCell.
     */

    class LinearIndexConverter {
    public:
      typedef OrderedLatticePointGenerator::matrix_type matrix_type;

      /// Initialize with the transformation that defines how to convert from the tiling unit (prim)
      /// to the superlattice, and the number of basis sites in the primitive cell
      LinearIndexConverter(const matrix_type &transformation_matrix, int basis_sites_in_prim)
        : m_linear_index_to_bijk(_make_all_ordered_bijk_values(OrderedLatticePointGenerator(transformation_matrix), basis_sites_in_prim)),
          m_basis_sites_in_prim(basis_sites_in_prim),
          m_automatically_bring_bijk_within(true),
          m_bring_within_f(transformation_matrix) {
        _throw_if_bad_basis_sites_in_prim(m_basis_sites_in_prim);

        for(Index ix = 0; ix < m_linear_index_to_bijk.size(); ++ix) {
          const auto &bijk = m_linear_index_to_bijk[ix];
          m_bijk_to_linear_index[bijk] = ix;
        }
      }

      /// Initialize with the primitie tiling unit, the superlattice, and the number of basis sites
      /// in the primitive unit
      LinearIndexConverter(const Lattice &tiling_unit, const Lattice &superlattice, int basis_sites_in_prim)
        : LinearIndexConverter(make_transformation_matrix(tiling_unit, superlattice, TOL), basis_sites_in_prim) {
      }

      /// Prevent the index converter from bringing UnitCellCoord within the supercell when querying for the index
      void dont_bring_within();

      /// Automatically bring UnitCellCoord values within the supercell when querying for the index (on by default)
      void do_bring_within();

      /// Given the linear index, retreive the corresponding UnitCellCoord
      UnitCellCoord operator[](Index ix) const;

      /// Given the UnitCellCoord, retreive its corresponding linear index.
      /// If applicable, brings the UnitCellCoord within the superlattice
      Index operator[](const UnitCellCoord &bijk) const;

    private:
      /// Convert from linear index to UnitCellCoord
      std::vector<UnitCellCoord> m_linear_index_to_bijk;

      /// Convert from UnitCellCoord to linear index
      std::unordered_map<UnitCellCoord, Index> m_bijk_to_linear_index;

      /// Stores a cache of UnitCellCoord values that landed outside of the superlattice
      mutable std::unordered_map<UnitCellCoord, Index> m_bijk_to_linear_index_outside_of_superlattice;

      /// How many blocks of "b", i.e. number of atoms in the primitive cell, as specified at construction
      int m_basis_sites_in_prim;

      /// If set to true, UnitCellCoord values will be brought within the supercell before querying for the index
      bool m_automatically_bring_bijk_within;

      /// Functor to bring UnitCellCoord values back into the superlattice
      LatticePointWithin m_bring_within_f;

      /// Returns the total number of sites within the superlattice
      Index _total_sites() const {
        return m_linear_index_to_bijk.size();
      }

      /// Throws exception if the specified index is out of the allowed range
      void _throw_if_incompatible_index(Index ix) const;

      /// Throws exception if the specified UnitCellCoord has a sublattice index that isn't compatible.
      /// If the state is set to not automatically bring the UnitCellCoord within the superlattice, then
      /// any UnitCellCoord outside the boundary will also trigger an exception
      void _throw_if_incompatible_bijk(const UnitCellCoord &bijk) const;

      /// Enumerates every possible UnitCellCoord and returns them in the expected order (blocks by
      /// basis site, with the Smith Normal Form order within each block
      static std::vector<UnitCellCoord> _make_all_ordered_bijk_values(const OrderedLatticePointGenerator &make_point,
                                                                      int basis_sites_in_prim);

      /// Throws exception if the number of sites specified in the tiling unit is less than 1
      static void _throw_if_bad_basis_sites_in_prim(int basis_sites_in_prim);

    };
  } // namespace xtal
} // namespace CASM

#endif
