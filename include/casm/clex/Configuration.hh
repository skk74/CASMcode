#ifndef CONFIGURATION_HH
#define CONFIGURATION_HH

#include <map>

#define BOOST_NO_SCOPED_ENUMS
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>

#include "casm/container/Array.hh"
#include "casm/container/LinearAlgebra.hh"
#include "casm/symmetry/PermuteIterator.hh"
#include "casm/clex/Properties.hh"
#include "casm/clex/Correlation.hh"
#include "casm/clusterography/Orbitree.hh"
#include "casm/clex/ConfigDoF.hh"

namespace CASM {

  class PrimClex;
  class Supercell;
  class UnitCellCoord;
  class Clexulator;

  class Configuration {
  private:

    /// Configuration data is saved in several files:
    ///
    /// config.json:             casmroot/supercells/SCEL_NAME/CONFIG_ID/config.json
    /// POS:                     casmroot/supercells/SCEL_NAME/CONFIG_ID/POS
    /// corr.json:               casmroot/supercells/SCEL_NAME/CONFIG_ID/CURR_CLEX/corr.json
    /// properties.calc.json:    casmroot/supercells/SCEL_NAME/CONFIG_ID/CURR_CALCTYPE/properties.calc.json
    /// param_composition.json:  casmroot/supercells/SCEL_NAME/CONFIG_ID/CURR_CALCTYPE/CURR_REF/param_composition.json
    /// properties.ref.json:     casmroot/supercells/SCEL_NAME/CONFIG_ID/CURR_CALCTYPE/CURR_REF/properties.ref.json
    /// properties.calc.json:    casmroot/supercells/SCEL_NAME/CONFIG_ID/CURR_CALCTYPE/CURR_REF/properties.calc.json     // contains param_comp
    /// properties.delta.json:   casmroot/supercells/SCEL_NAME/CONFIG_ID/CURR_CALCTYPE/CURR_REF/properties.delta.json


    /// Identification

    // Configuration id is the index into Supercell::config_list
    std::string id;

    /// const pointer to the (non-const) Supercell for this Configuration
    Supercell *supercell;

    /// a jsonParser object indicating where this Configuration came from
    jsonParser m_source;
    bool source_updated;


    // symmetric multiplicity (i.e., size of configuration's factor_group)
    int multiplicity;


    /// Degrees of Freedom

    // 'occupation' is a list of the indices describing the occupants in each crystal site.
    //   get_prim().basis[ get_b(i) ].site_occupant[ occupation[i]] -> Molecule on site i
    //   This means that for the background structure, 'occupation' is all 0

    // Configuration sites are arranged by basis, and then prim:
    //   occupation: [basis0                |basis1               |basis2          |...] up to prim.basis.size()
    //       basis0: [prim0|prim1|prim2|...] up to supercell.volume()
    //
    bool dof_updated;
    ConfigDoF m_configdof;


    /// Properties
    ///Keeps track of whether the Configuration properties change since reading. Be sure to set to true in your routine if it did!
    /// PROPERTIES (AS OF 07/27/15)
    /*  reference:
     *
     *
     *  calculated:
     *    calculated["energy"]
     *    calculated["relaxed_energy"]
     *
     *  delta:
     *    delta["energy"]
     *    delta["relaxed_energy"]
     *
     *  generated:
     *    generated["is_groundstate"]
     *	  generated["dist_from_hull"]
     *    generated["sublat_struct_fact"]
     *    generated["struct_fact"]
     */
    bool prop_updated;
    Properties reference;   //Stuff you use as reference to get delta properties
    Properties calculated;  //Stuff you got directly from your DFT calculations
    DeltaProperties delta;  //calculated-reference
    Properties generated;   //Everything else you came up with through casm


    /// Composition -- calculated on-the-fly, see functions below


    /// Correlations


    // Let's just deal with one orbitree at a time for now...

    bool corr_updated;

    //  correlations
    // correlations can be used for multiple CLEX, if basis functions are the same
    Correlation correlations;

    bool m_selected;

  public:
    typedef ConfigDoF::displacement_matrix_t displacement_matrix_t;
    typedef ConfigDoF::displacement_t displacement_t;
    typedef ConfigDoF::const_displacement_t const_displacement_t;


    //********* CONSTRUCTORS *********

    /// Construct a default Configuration
    Configuration(Supercell &_supercell, const jsonParser &source = jsonParser(), const ConfigDoF &_dof = ConfigDoF());

    /// Construct by reading from main data file (json)
    Configuration(const jsonParser &json, Supercell &_supercell, Index _id);


    /// Construct a Configuration with occupation specified by string 'con_name'
    //Configuration(Supercell &_supercell, std::string con_name, bool select, const jsonParser &source = jsonParser());


    //********** DESTRUCTORS *********

    //********** MUTATORS  ***********

    void set_multiplicity(int m) {
      multiplicity = m;
    }

    void set_id(Index _id);

    void set_source(const jsonParser &source);

    void push_back_source(const jsonParser &source);

    // ** Degrees of Freedom **
    //
    // ** Note: Properties and correlations are not automatically updated when dof are changed, **
    // **       nor are the written records automatically updated                               **

    void set_occupation(const Array<int> &newoccupation);

    void set_occ(Index site_l, int val);

    void set_displacement(const displacement_matrix_t &_disp);

    void set_deformation(const Eigen::Matrix3d &_deformation);

    Configuration canonical_form(PermuteIterator it_begin, PermuteIterator it_end, PermuteIterator &it_canon, double tol = TOL) const;

    bool is_canonical(PermuteIterator it_begin, PermuteIterator it_end, double tol = TOL) const {
      return m_configdof.is_canonical(it_begin, it_end, tol);
    }

    bool is_primitive(PermuteIterator it_begin, double tol = TOL) const {
      return m_configdof.is_primitive(it_begin, tol);
    }

    // ** Properties **
    //
    // ** Note: DeltaProperties are automatically updated, but not written upon changes **

    /// Read calculation results into the configuration
    //void read_calculated();

    void set_calc_properties(const jsonParser &json);

    bool read_calc_properties(jsonParser &parsed_props) const;
    /// Generate reference Properties from param_composition and reference states
    ///   For now only linear interpolation
    void generate_reference();

    void set_selected(bool _selected) {
      m_selected = _selected;
    }

    void set_reference(const Properties &ref);


    void set_correlations(Clexulator &clexulator);
    void set_correlations_orbitree(const SiteOrbitree &site_orbitree);
    //void set_correlations_old(const SiteOrbitree &site_orbitree);

    ///Add or modify variables relating to hull
    void set_hull_data(bool is_groundstate, double dist_from_hull);

    void clear_hull_data();

    //********** ACCESSORS ***********

    std::string get_id() const;


    int get_multiplicity()const {
      return multiplicity;
    }

    std::string name() const;

    const jsonParser &source() const;

    fs::path get_path() const;

    ///Returns number of sites, NOT the number of primitives that fit in here
    Index size() const;

    const Structure &get_prim() const;

    bool selected() const {
      return m_selected;
    }

    //PrimClex &get_primclex();
    const PrimClex &get_primclex() const;

    Supercell &get_supercell();
    const Supercell &get_supercell() const;

    UnitCellCoord get_uccoord(Index site_l) const;

    int get_b(Index site_l) const;

    const ConfigDoF &configdof() const {
      return m_configdof;
    }

    bool has_occupation() const {
      return configdof().has_occupation();
    }

    const Array<int> &occupation() const {
      return configdof().occupation();
    }

    const int &occ(Index site_l) const {
      return configdof().occ(site_l);
    }

    const Molecule &get_mol(Index site_l) const;


    bool has_displacement() const {
      return configdof().has_displacement();
    }

    const displacement_matrix_t &displacement() const {
      return configdof().displacement();
    }

    const_displacement_t disp(Index site_l) const {
      return configdof().disp(site_l);
    }

    const Eigen::Matrix3d &deformation() const {
      return configdof().deformation();
    }

    bool is_strained() const {
      return configdof().is_strained();
    }

    fs::path get_reference_state_dir() const;

    const Properties &ref_properties() const;

    const Properties &calc_properties() const;

    const DeltaProperties &delta_properties() const;

    const Properties &generated_properties() const;


    //const Correlation &get_correlations() const;


    // Returns composition on each sublattice: sublat_comp[ prim basis site / sublattice][ molecule_type]
    //   molucule_type is ordered as in the Prim structure's site_occupant list for that basis site (includes vacancies)
    ReturnArray< Array < double > > get_sublattice_composition() const;

    // Returns number of each molecule by sublattice:
    //   sublat_num_each_molecule[ prim basis site / sublattice ][ molecule_type]
    //   molucule_type is ordered as in the Prim structure's site_occupant list for that basis site
    ReturnArray< Array<int> > get_sublat_num_each_molecule() const;

    // Returns composition, not counting vacancies
    //    composition[ molecule_type ]: molecule_type ordered as prim structure's get_struc_molecule(), with [Va]=0.0
    ReturnArray<double> get_composition() const;

    // Returns composition, including vacancies
    //    composition[ molecule_type ]: molecule_type ordered as prim structure's get_struc_molecule()
    ReturnArray<double> get_true_composition() const;

    /// Returns num_each_molecule[ molecule_type], where 'molecule_type' is ordered as Structure::get_struc_molecule()
    ReturnArray<int> get_num_each_molecule() const;

    /// Returns parametric composition, as calculated using PrimClex::param_comp
    Eigen::VectorXd get_param_composition() const;

    /// Returns num_each_component[ component_type] per prim cell,
    ///   where 'component_type' is ordered as ParamComposition::get_components
    Eigen::VectorXd get_num_each_component() const;

    //-----------------------------------
    //Structure Factor
    Eigen::VectorXd get_struct_fact_intensities() const;
    Eigen::VectorXd get_struct_fact_intensities(const Eigen::VectorXd &component_intensities) const;

    void calc_sublat_struct_fact();
    void calc_struct_fact();
    void calc_sublat_struct_fact(const Eigen::VectorXd &intensities);
    void calc_struct_fact(const Eigen::VectorXd &intensities);

    Eigen::MatrixXcd sublat_struct_fact();
    Eigen::MatrixXd struct_fact();

    //********* IO ************

    /// Writes the Configuration to the correct casm directory
    ///   Uses PrimClex's current settings to write the appropriate
    ///   Properties, DeltaProperties and Correlations files
    jsonParser &write(jsonParser &json) const;

    /// Write the POS file to get_pos_path
    void write_pos() const;

    void print_occupation(std::ostream &stream) const;

    void print_config_list(std::ostream &stream, int composition_flag) const;

    void print_composition(std::ostream &stream) const;

    void print_true_composition(std::ostream &stream) const;

    void print_sublattice_composition(std::ostream &stream) const;

    ///Old CASM style corr.in output for one configuration

    void print_correlations_simple(std::ostream &corrstream) const;

    fs::path calc_properties_path() const;
    /// Path to various files
    fs::path get_pos_path() const;


  private:
    /// Convenience accessors:
    int &_occ(Index site_l) {
      return m_configdof.occ(site_l);
    }

    displacement_t _disp(Index site_l) {
      return m_configdof.disp(site_l);
    }



    /// Reads the Configuration from the expected casm directory
    ///   Uses PrimClex's current settings to read in the appropriate
    ///   Properties, DeltaProperties and Correlations files if they exist
    ///
    /// This is private, because it is only called from the constructor:
    ///   Configuration(const Supercell &_supercell, Index _id)
    ///   It's called from the constructor because of the Supercell pointer
    ///
    void read(const jsonParser &json);

    /// Functions used to perform read()
    void read_dof(const jsonParser &json);
    void read_corr(const jsonParser &json);
    void read_properties(const jsonParser &json);

    /// Functions used to perform write to config_list.json:
    jsonParser &write_dof(jsonParser &json) const;
    jsonParser &write_source(jsonParser &json) const;
    jsonParser &write_pos(jsonParser &json) const;
    jsonParser &write_corr(jsonParser &json) const;
    jsonParser &write_param_composition(jsonParser &json) const;
    jsonParser &write_properties(jsonParser &json) const;

    bool reference_states_exist() const;
    void read_reference_states(Array<Properties> &ref_state_prop, Array<Eigen::VectorXd> &ref_state_comp) const;
    void generate_reference_scalar(std::string propname, const Array<Properties> &ref_state_prop, const Array<Eigen::VectorXd> &ref_state_comp);

  };

  /// \brief Returns correlations using 'clexulator'.
  Correlation correlations(const Configuration &config, Clexulator &clexulator);

}

#endif
