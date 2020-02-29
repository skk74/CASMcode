#ifndef CASM_XTAL_DOFSET
#define CASM_XTAL_DOFSET

#include "casm/casm_io/json/jsonParser.hh"
#include "casm/crystallography/AnisoValTraits.hh"
#include "casm/external/Eigen/src/Core/Matrix.h"
#include <unordered_set>
#include <vector>

namespace CASM {
  namespace xtal {
    class SymOp;

    /**
     * DoFSet specifies all identifying information for a vector of continuous independent variables (Degrees of Freedom / DoFs)
     * DoFSets are associated with a specific DoF 'type', which has a predefined 'standard' coordinate system
     * ex: displacement -> 3-vector (x,y,z) -> displacement components (relative to fixed laboratory frame)
     *     strain -> 6-vector (e_xx, e_yy, e_zz, sqrt(2)*e_yz, sqrt(2)*e_xz, sqrt(2)*e_xy) -> tensor elements
     * DoFSets have a typename, which specifies the type, and a set of basis vectors, which are denoted relative
     * to the DoF type's standard axes. This allows the DoFSet components to be specified by the user,
     * including the ability to only allow DoF values within a subspace of the standard values.
     * DoFSet records  the DoF typename, the names of the vector components, and the axes of the
     * vector components (relative to a set of standard axes)
     */
    class DoFSet {
    public:
      using BasicTraits = AnisoValTraits;

      DoFSet(const BasicTraits &init_traits, const std::vector<std::string> &init_component_names, const Eigen::MatrixXd &init_basis)
        : m_traits(init_traits), m_component_names(init_component_names), m_basis(init_basis), m_basis_inverse(this->basis().inverse()) {
        assert(m_component_names.size() == this->dimensions());
        assert(m_basis.cols() == this->dimensions());
        assert(m_basis.rows() == this->traits().dim()); // TODO: This makes sense I think?
      }

      DoFSet(const BasicTraits &init_traits)
        : DoFSet(init_traits, init_traits.standard_var_names(), Eigen::MatrixXd::Identity(init_traits.dim(), init_traits.dim())) {
      }

      /// \brief Returns type_name of DoFSet, which should be a standardized DoF type (e.g., "disp", "magspin", "GLstrain")
      const std::string &type_name() const {
        return m_traits.name();
      }

      /// Returns the names of each of the component axes
      const std::vector<std::string> &component_names() const {
        return this->m_component_names;
      }

      /// \brief Returns traits object for the DoF type of this DoFSet
      BasicTraits const &traits() const {
        return m_traits;
      }

      /// Returns the number of dimension of the DoF, corresponding to the number of axes in the vector space
      Index dimensions() const {
        return this->basis().cols();
      }

      /// Matrix that relates DoFSet variables to a conventional coordiante system
      Eigen::MatrixXd const &basis() const {
        return m_basis;
      }

      /// \brief returns true of \param rhs has identical components and basis to this DoFSet
      bool is_identical(DoFSet const &rhs) const;

    private:
      /// AnisoValTraits. Describes the type of DoF, and can convert Cartesian symmetry representations into the appropriate representation
      BasicTraits m_traits;

      /// Names for each axis of the basis, for example "x". "y", "z" for displacement
      std::vector<std::string> m_component_names;

      /// The basis defines the space of the DoF, which should be a linear combination of the AnisoValTraits conventional coordinates.
      /// For example, you may want to define displacements that only happen along a particular direction
      Eigen::MatrixXd m_basis;
      Eigen::MatrixXd m_basis_inverse;
    };

    /**
     * Identical to xtal::DoFSet, but also keeps track of a list of molecule names that the DoFSet does not
     * apply to. For example, don't apply displacements to a vacancy.
     */

    class SiteDoFSet : public DoFSet {
    public:
      SiteDoFSet(const DoFSet &init_dofset, const std::unordered_set<std::string> &init_exclude_occs)
        : DoFSet(init_dofset), m_excluded_occs(init_exclude_occs) {
      }

      SiteDoFSet(const DoFSet &init_dofset) : DoFSet(init_dofset) {}

      SiteDoFSet(const BasicTraits &init_traits,
                 const std::vector<std::string> &init_component_names,
                 const Eigen::MatrixXd &init_basis,
                 const std::unordered_set<std::string> &init_exclude_occs)
        : SiteDoFSet(DoFSet(init_traits, init_component_names, init_basis), init_exclude_occs) {
      }

      SiteDoFSet(const BasicTraits &init_traits, const std::unordered_set<std::string> &init_exclude_occs)
        : SiteDoFSet(DoFSet(init_traits), init_exclude_occs) {
      }

      /// \brief Returns true if DoFSet is inactive (e.g., takes zero values) when specified occupant is present
      bool is_excluded_occ(std::string const &_occ_name) const {
        return m_excluded_occs.count(_occ_name);
      }

      /// Return all occupants that the DoFSet should not be applied to
      const std::unordered_set<std::string> &excluded_occs() const {
        return this->m_excluded_occs;
      }

    private:
      std::unordered_set<std::string> m_excluded_occs;
    };

    /**
     * Comparator class for checking equivalence of two DoFSet values.
     * Evaluate by constructing object with one of the values, and then pass
     * the other DoFSet to the oveloaded operator()/
     */

    struct DoFSetEquals_f {
    public:

      DoFSetEquals_f(const DoFSet &reference_value, double tol):
        m_reference_dofset(reference_value), m_tol(tol)
      {}

      ///Returns true if the passed value matches the stored value that *this was constructed with
      bool operator()(const DoFSet &other_value) const;

    private:

      /// Values passed to operator() will be compared against this
      DoFSet m_reference_dofset;

      /// Tolerance value for making comparisons
      double m_tol;
    };


  } // namespace xtal

  //This is how it's currently organized for xtal::Lattice, but maybe we want something else  (see SymTools.hh)
  namespace sym {
    /// \brief Copy and apply SymOp to a DoFSet
    xtal::DoFSet copy_apply(const xtal::SymOp &op, const xtal::DoFSet &_dof);
  }

  template<> xtal::SiteDoFSet from_json<xtal::SiteDoFSet>(const jsonParser &json);

  jsonParser &to_json(xtal::SiteDoFSet const &_dof, jsonParser &json);
  jsonParser &to_json(xtal::DoFSet const &_dof, jsonParser &json);

  /* template<> xtal::DoFSet from_json<xtal::DoFSet>(const jsonParser &json); */
  /* xtal::SiteDoFSet& from_json( xtal::SiteDoFSet &_dof,const jsonParser &json); */
  /* xtal::DoFSet& from_json( xtal::DoFSet &_dof,const jsonParser &json); */

  /* template <> */
  /* struct jsonConstructor<xtal::SiteDoFSet> { */
  /*   static xtal::SiteDoFSet from_json(const jsonParser &json, xtal::SiteDoFSet::BasicTraits const &_type); */
  /* }; */
  /* template <> */
  /* struct jsonConstructor<xtal::DoFSet> { */
  /*   static xtal::DoFSet from_json(const jsonParser &json, xtal::DoFSet::BasicTraits const &_type); */
  /* }; */
} // namespace CASM

#endif
