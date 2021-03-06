#ifndef PRIMGRID_HH
#define PRIMGRID_HH


#include <iostream>
#include <cmath>
#include <cassert>

#include "casm/container/LinearAlgebra.hh"
#include "casm/container/Permutation.hh"


namespace CASM {

  class Lattice;
  class UnitCellCoord;
  class Coordinate;
  class SymOp;
  class SymGroup;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  class PrimGrid {
  private:
    ///m_lat[PRIM] is primitive lattice, lat[SCEL] is super lattice
    Lattice const *m_lat[2];

    /// Number of primgrid lattice points in the supercell
    long int m_N_vol;

    /// Number of basis atoms in the primitive cell
    long int m_NB;

    /// The transformation matrix, 'trans_mat', is:
    ///   lat[SCEL]->lat_column_mat() = (lat[PRIM]->lat_column_mat())*trans_mat;
    ///   plane_mat = trans_mat.determinant()*trans_mat.inverse();
    Matrix3<int> m_plane_mat, m_trans_mat;

    /// The Smith Normal Form decomposition is: trans_mat = U*S*V, with det(U)=det(V)=1; S is diagonal
    Matrix3<int> m_U, m_invU;

    /// Permutations that describe how translation permutes sites of the supercell
    mutable Array<Permutation> m_trans_permutations;

    ///==============================================================================================
    /// Because
    ///        m_lat[SCEL]->lat_column_mat() = (m_lat[PRIM]->lat_column_mat())*trans_mat;
    /// and
    ///        trans_mat=U*S*V
    /// we know that
    ///        (m_lat[SCEL]->lat_column_mat())*V.inverse() = (m_lat[PRIM]->lat_column_mat())*U*S;
    ///
    /// In other words, [(m_lat[PRIM]->lat_column_mat())*U] is a primitive lattice that perfectly tiles
    /// the equivalent super lattice [(m_lat[SCEL]->lat_column_mat())*V.inverse()]   -- (because S is diagonal)
    ///
    /// We thus use (m,n,p) on the grid specified by [(m_lat[PRIM]->lat_column_mat())*U] as a canonical indexing
    ///
    /// We can do this by manipulating fractional coordinates:
    ///         trans_mat*super_frac_coord = prim_frac_coord
    ///
    /// so
    ///         U*S*V*super_frac_coord = prim_frac_coord
    ///
    /// and finally
    ///         S*V*super_frac_coord = U.inverse()*prim_frac_coord
    ///
    /// meaning that multiplication of prim_frac_coord by invU gives the canonical index
    ///
    /// This means that:
    ///
    ///  (m,n,p) = invU * (i,j,k)    and    (i,j,k) = U * (m,n,p)
    ///
    ///  where (i,j,k) are the UnitCellCoords relative to m_lat[PRIM], and (m,n,p) are canonical UnitCellCoords,
    ///  relative to (m_lat[PRIM]->lat_column_mat())*U

    // stride maps canonical 3d index (m,n,p) onto linear index -- l = m + n*stride[0] + p*stride[1]
    // S is diagonals of smith normal form S matrix
    int m_stride[2], m_S[3];


    /// Convert UnitCellCoord (bijk) to canonical UnitCellCoord (bmnp)
    /// mnp = invU * ijk
    UnitCellCoord to_canonical(const UnitCellCoord &bijk) const;

    /// Convert canonical UnitCellCoord (bmnp) to UnitCellCoord (bijk)
    /// U*mnp = ijk
    UnitCellCoord from_canonical(const UnitCellCoord &bmnp) const;


  public:
    PrimGrid(const Lattice &p_lat, const Lattice &s_lat, Index NB = 1);
    PrimGrid(const Lattice &p_lat, const Lattice &s_lat, const Matrix3<int> &U, const Matrix3<int> &Smat, Index NB);

    Index size() const {
      return m_N_vol;
    }

    const Matrix3<int> &matrixU()const {
      return m_U;
    };
    const Matrix3<int> &invU()const {
      return m_invU;
    };
    Matrix3<int> matrixS()const;
    int S(Index i) const {
      return m_S[i];
    };

    // find linear index that is translational equivalent to Coordinate or UnitCellCoord
    Index find(const Coordinate &_coord) const;
    Index find(const UnitCellCoord &_coord) const;

    // map a UnitCellCoord inside the supercell
    UnitCellCoord get_within(const UnitCellCoord &_uccoord)const;

    // get Coordinate or UnitCellCoord from linear index
    Coordinate coord(Index l, CELL_TYPE lat_mode)const;
    Coordinate coord(const UnitCellCoord &bijk, CELL_TYPE lat_mode)const;
    UnitCellCoord uccoord(Index i)const;

    Index make_permutation_representation(const SymGroup &group, Index basis_permute_rep)const;

    // Returns Array of permutations.  Permutation 'l' describes the effect of translating PrimGrid site 'l'
    // to the origin.  NB is the number of primitive-cell basis sites. -- keep public for now
    ReturnArray<Permutation > make_translation_permutations(Index NB)const;

    /// const access to m_trans_permutations. Generates permutations if they don't already exist.
    const Array<Permutation> &translation_permutations() const {
      if(m_trans_permutations.size() != m_NB * m_N_vol)
        m_trans_permutations = make_translation_permutations(m_NB);
      return m_trans_permutations;
    }

    const Permutation &translation_permutation(Index i) const {
      return translation_permutations()[i];
    }

    SymOp sym_op(Index l) const;
  };

}
#endif
