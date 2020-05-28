#ifndef CASM_ClusterSymCompare_impl
#define CASM_ClusterSymCompare_impl

#include "casm/clusterography/ClusterSymCompare.hh"
#include "casm/crystallography/Structure.hh"
#include "casm/symmetry/SymPermutation.hh"

namespace CASM {

  /// \brief Make orbit invariants from one element in the orbit
  template<typename Base>
  typename ClusterSymCompare<Base>::InvariantsType
  ClusterSymCompare<Base>::make_invariants_impl(const Element &obj) const {
    return traits<MostDerived>::make_invariants(obj, this->derived());
  }


  /// \brief Orders 'prepared' elements in the same orbit
  ///
  /// - Returns 'true' to indicate A < B
  /// - Equivalence is indicated by \code !compare(A,B) && !compare(B,A) \endcode
  /// - Assumes elements are 'prepared' before being compared
  /// Implementation:
  /// - First compares by number of sites in cluster
  /// - Then compare all displacements, from longest to shortest
  template<typename Base>
  bool ClusterSymCompare<Base>::
  invariants_compare_impl(const InvariantsType &A, const InvariantsType &B) const {
    return CASM::compare(A, B, this->derived().tol());
  }

  /// \brief Compares 'prepared' clusters
  ///
  /// - Returns 'true' to indicate A < B
  /// - Equivalence is indicated by \code !compare(A,B) && !compare(B,A) \endcode
  /// - Assumes elements are 'prepared' before being compared
  template<typename Base>
  bool ClusterSymCompare<Base>::
  compare_impl(const ClusterType &A, const ClusterType &B) const {
    return A < B;
  }

  /// \brief Applies SymOp to cluster
  template<typename Base>
  typename ClusterSymCompare<Base>::ClusterType ClusterSymCompare<Base>::
  copy_apply_impl(SymOp const &op, ClusterType obj) const {
    return traits<MostDerived>::copy_apply(op, obj, this->derived());
  }

  /// \brief Returns transformation that takes 'obj' to its prepared (canonical) form
  ///
  /// - For now returns pointer to SymPermutation object that encodes permutation due to sorting elements
  template<typename Base>
  std::unique_ptr<SymOpRepresentation> ClusterSymCompare<Base>::canonical_transform_impl(Element const &obj)const {
    return std::unique_ptr<SymOpRepresentation>(new SymPermutation(obj.sort_permutation()));
  }


  // -- AperiodicSymCompare<IntegralCluster> -------------------------------------

  /// \brief Constructor
  ///
  /// \param tol Tolerance for invariants_compare of site-to-site distances
  ///
  template<typename Element>
  AperiodicSymCompare<Element>::
  AperiodicSymCompare(PrimType_ptr prim_ptr, double tol): m_prim(prim_ptr), m_tol(tol) {}

  /// \brief Prepare an element for comparison
  ///
  /// - Returns sorted
  template<typename Element>
  Element AperiodicSymCompare<Element>::
  spatial_prepare_impl(Element obj) const {
    return obj;
  }

  /// \brief Access spatial transform that was used during most recent spatial preparation of an element
  /// - Always identity
  template<typename Element>
  SymOp const &AperiodicSymCompare<Element>::
  spatial_transform_impl() const {
    return m_spatial_transform;
  }

  /// \brief Prepare an element for comparison
  ///
  /// - Returns sorted
  template<typename Element>
  Element AperiodicSymCompare<Element>::
  representation_prepare_impl(Element obj) const {
    return obj.sort();
  }

  // -- PrimPeriodicSymCompare<IntegralCluster> -------------------------------------

  /// \brief Constructor
  ///
  /// \param tol Tolerance for invariants_compare of site-to-site distances
  ///
  template<typename Element>
  PrimPeriodicSymCompare<Element>::
  PrimPeriodicSymCompare(PrimType_ptr prim_ptr, double tol): m_prim(prim_ptr), m_tol(tol) {}

  /// \brief Prepare an element for comparison
  ///
  /// - translates so that obj[0] is in the origin unit cell
  template<typename Element>
  Element PrimPeriodicSymCompare<Element>::
  spatial_prepare_impl(Element obj) const {
    if(!obj.size()) {
      return obj;
    }
    const auto pos = traits<PrimPeriodicSymCompare<Element>>::position(obj, this->derived());
    this->m_spatial_transform = SymOp::translation(-prim().lattice().lat_column_mat() * pos.unitcell().template cast<double>());
    return obj - pos.unitcell();
  }

  /// \brief Access spatial transform that was used during most recent spatial preparation of an element
  template<typename Element>
  SymOp const &PrimPeriodicSymCompare<Element>::
  spatial_transform_impl() const {
    return m_spatial_transform;
  }

  /// \brief Prepare an element for comparison
  ///
  /// - Sorts so that, after translation obj[0] is in the origin unit cell
  template<typename Element>
  Element PrimPeriodicSymCompare<Element>::
  representation_prepare_impl(Element obj) const {
    if(!obj.size()) {
      return obj;
    }
    obj.sort();

    return obj;
  }


  // -- ScelPeriodicSymCompare<IntegralCluster> -------------------------------------

  /// \brief Constructor
  ///
  /// \param tol Tolerance for invariants_compare of site-to-site distances
  ///
  template<typename Element>
  ScelPeriodicSymCompare<Element>::
  ScelPeriodicSymCompare(
    PrimType_ptr prim_ptr,
    transf_mat_type transf_mat,
    double tol):
    m_prim(prim_ptr),
    m_bring_within_f(transf_mat),
    m_tol(tol) {}

  /// \brief Prepare an element for comparison
  ///
  /// - translates so that obj[0] is within the supercell
  template<typename Element>
  Element ScelPeriodicSymCompare<Element>::
  spatial_prepare_impl(Element obj) const {
    if(!obj.size()) {
      return obj;
    }
    const auto pos = traits<ScelPeriodicSymCompare<Element>>::position(obj, *this);
    this->m_spatial_transform = SymOp::translation(this->m_prim->lattice().lat_column_mat() * (m_bring_within_f(pos).unitcell() - pos.unitcell()).template cast<double>());
    return obj + (m_bring_within_f(pos).unitcell() - pos.unitcell());
  }

  /// \brief Access spatial transform that was used during most recent spatial preparation of an element
  template<typename Element>
  SymOp const &ScelPeriodicSymCompare<Element>::
  spatial_transform_impl() const {
    return m_spatial_transform;
  }

  /// \brief Prepare an element for comparison
  ///
  /// - Sorts UnitCellCoord
  template<typename Element>
  Element ScelPeriodicSymCompare<Element>::
  representation_prepare_impl(Element obj) const {
    if(!obj.size()) {
      return obj;
    }
    obj.sort();

    return obj;
  }


  // -- WithinScelSymCompare<IntegralCluster> -------------------------------------

  /// \brief Constructor
  ///
  /// \param tol Tolerance for invariants_compare of site-to-site distances
  ///
  template<typename Element>
  WithinScelSymCompare<Element>::
  WithinScelSymCompare(
    PrimType_ptr prim_ptr,
    transf_mat_type transf_mat,
    double tol):
    m_prim(prim_ptr),
    m_bring_within_f(transf_mat),
    m_tol(tol) {}

  /// \brief Returns transformation that takes 'obj' to its prepared (canonical) form
  ///
  /// - For now returns pointer to SymPermutation object that encodes permutation due to sorting elements
  template<typename Element>
  std::unique_ptr<SymOpRepresentation> WithinScelSymCompare<Element>::
  canonical_transform_impl(Element const &obj)const {
    Element tobj = traits<WithinScelSymCompare<Element>>::bring_within(obj, this->derived());
    return std::unique_ptr<SymOpRepresentation>(new SymPermutation(tobj.sort_permutation()));
  }

  /// \brief Prepare an element for comparison
  ///
  /// - Does nothing, since fully prepared version is just sorted and 'within'-ed version of the cluster
  template<typename Element>
  Element WithinScelSymCompare<Element>::
  spatial_prepare_impl(Element obj) const {
    return obj;
  }

  /// \brief Access spatial transform that was used during most recent spatial preparation of an element
  /// - Always identity
  template<typename Element>
  SymOp const &WithinScelSymCompare<Element>::
  spatial_transform_impl() const {
    return m_spatial_transform;
  }

  /// \brief Prepare an element for comparison
  ///
  /// - Puts all sites within the supercell, then sorts
  template<typename Element>
  Element WithinScelSymCompare<Element>::
  representation_prepare_impl(Element obj) const {
    if(!obj.size()) {
      return obj;
    }
    obj = traits<WithinScelSymCompare<Element>>::bring_within(obj, this->derived());
    obj.sort();
    return obj;
  }
}

#endif
