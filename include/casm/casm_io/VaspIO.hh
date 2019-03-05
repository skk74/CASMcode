#ifndef CASM_VaspIO
#define CASM_VaspIO

#include "casm/CASM_global_definitions.hh"
#include "casm/CASM_global_enum.hh"
#include "casm/crystallography/Lattice.hh"

namespace CASM {

  class Site;
  template<typename T> class BasicStructure;
  class Configuration;
  class Coordinate;
  class SimpleStructure;

  /// \brief Functions and classes related to VASP input/output
  namespace VaspIO {

    /** \addtogroup VaspIO

        \brief Functions and classes related to VASP input/output

        \ingroup casmIO

    */

    /// \brief Store SelectiveDynamics options
    ///
    /// \ingroup VaspIO
    ///
    class SelectiveDynamics {

    public:

      /// \brief Default Constructor sets to true for all directions
      SelectiveDynamics() {
        m_option << 1, 1, 1;
      }

      SelectiveDynamics(Eigen::Ref<const Eigen::Vector3i> const &_option) :
        m_option(_option) { }

      SelectiveDynamics(bool x, bool y, bool z) {
        m_option << int(x), int(y), int(z);
      }

      int &operator[](int i) {
        return m_option[i];
      }

      const int &operator[](int i) const {
        return m_option[i];
      }


    private:

      Eigen::Vector3i m_option;

    };

    /// \brief Write SelectiveDynamics options
    ///
    /// \relatesalso SelectiveDynamics
    ///
    inline std::ostream &operator<<(std::ostream &sout, const SelectiveDynamics &sel) {
      for(int i = 0; i < 3; ++i)
        sout << (sel[i] ? " T" : " F");
      return sout;
    };



    /// \brief Print POSCAR with formating options
    ///
    /// Example:
    /// \code
    /// std::ostream file("POSCAR");
    /// Configuration config;
    /// PrintPOSCAR printer(config);
    /// printer.title("My system");
    /// printer.set_cart();
    /// printer.sort();
    /// printer.print(file);
    /// file.close();
    /// \endcode
    ///
    /// \ingroup VaspIO
    ///

    class PrintPOSCAR {

    public:

      typedef std::string AtomName;

      /// \brief Atom name, Coordinate, SelectiveDynamics
      typedef std::tuple<AtomName, Coordinate, SelectiveDynamics> tuple_type;

      typedef std::vector<tuple_type>::iterator iterator;

      typedef std::vector<tuple_type>::const_iterator const_iterator;


      /// \brief Construct PrintPOSCAR object
      PrintPOSCAR(const BasicStructure<Site> &struc);

      /// \brief Construct PrintPOSCAR object
      PrintPOSCAR(const SimpleStructure &_sstruc, std::string const &_title = "");

      /// \brief Construct PrintPOSCAR object
      PrintPOSCAR(const Configuration &config);

      /// \brief Construct PrintPOSCAR object
      //PrintPOSCAR(const Supercell &scel, const ConfigDoF &configdof);

      /// \brief Construct PrintPOSCAR object
      ///
      /// By default:
      /// - title = ""
      /// - scale = 1.0
      /// - coordinate mode = frac (Direct)
      /// - atom names line is printed
      /// - No selective dynamics
      /// - atom names appended to each coordinate line
      /// - {"Va", "va", "VA"} atoms ignored
      ///
      PrintPOSCAR() :
        m_title(""),
        m_scale(1.0),
        m_coord_mode(FRAC),
        m_atom_names(true),
        m_sel_dynamics(false),
        m_append_atom_names(true),
        m_ignore {"VA", "Va", "va"} {}

      /// \brief Set title
      void set_title(std::string title) {
        m_title = title;
      }

      /// \brief Set scaling factor
      void set_scale(double s) {
        m_scale = s;
      }

      /// \brief Set coordinate mode to Direct (fractional)
      void set_direct() {
        m_coord_mode = FRAC;
      }
      /// \brief Set coordinate mode to fractional (Direct)
      void set_frac() {
        m_coord_mode = FRAC;
      }

      /// \brief Set coordinate mode to Cartesian
      void set_cart() {
        m_coord_mode = CART;
      }

      /// \brief Set coordinate mode
      void set_coord_mode(COORD_TYPE mode) {
        m_coord_mode = mode;
      }

      /// \brief Set selective dynamics off
      void set_selective_dynamics_off() {
        m_sel_dynamics = false;
      }

      /// \brief Set selective dynamics on
      void set_selective_dynamics_on() {
        m_sel_dynamics = true;
      }

      /// \brief Do not print atom names line
      void set_atom_names_off() {
        m_atom_names = false;
      }

      /// \brief Print atom names line
      void set_atom_names_on() {
        m_atom_names = true;
      }

      /// \brief Do not append atom name to end of each coordinate line
      void set_append_atom_names_off() {
        m_append_atom_names = false;
      }

      /// \brief Append atom name to end of each coordinate line
      void set_append_atom_names_on() {
        m_append_atom_names = false;
      }

      /// \brief Access set of atom names which should not be printed, such as for vacancies
      std::set<std::string> &ignore() {
        return m_ignore;
      }

      /// \brief const Access set of atom names which should not be printed, such as for vacancies
      const std::set<std::string> &ignore() const {
        return m_ignore;
      }

      /// \brief const Access lattice
      const Lattice &lattice() const {
        return m_lat;
      }

      /// \brief Iterate over tuples of (AtomName, Coordinate, SelectiveDynamics)
      iterator begin() {
        return m_atom_order.begin();
      }

      /// \brief Iterate over tuples of (AtomName, Coordinate, SelectiveDynamics)
      iterator end() {
        return m_atom_order.end();
      }

      /// \brief Iterate over tuples of (AtomName, Coordinate, SelectiveDynamics)
      const_iterator cbegin() const {
        return m_atom_order.cbegin();
      }

      /// \brief Iterate over tuples of (AtomName, Coordinate, SelectiveDynamics)
      const_iterator cend() const {
        return m_atom_order.cend();
      }

      /// \brief Default sort is by atom name
      void sort();

      /// \brief Print POSCAR to stream
      void print(std::ostream &sout) const;

      /// \brief Print POSCAR to log (enables indentation)
      void print(Log &sout) const;

    protected:

      /// \brief Print POSCAR, provide a range of std::tuple<Atom name, Coordinate, SelectiveDynamics>
      template<typename TupleIterator>
      void _print(std::ostream &sout, TupleIterator begin, TupleIterator end) const;

      /// \brief Print POSCAR, provide a range of std::tuple<Atom name, Coordinate, SelectiveDynamics>
      template<typename TupleIterator>
      void _print(Log &sout, TupleIterator begin, TupleIterator end) const;

    private:

      std::string m_title;
      double m_scale;
      COORD_TYPE m_coord_mode;
      bool m_atom_names;
      bool m_sel_dynamics;
      bool m_append_atom_names;
      Lattice m_lat;

      /// \brief List of atom names which should not be printed (primarily for vacancies)
      std::set<std::string> m_ignore;

      /// \brief (AtomName, Coordinate, SelectiveDynamics)
      std::vector<tuple_type> m_atom_order;

    };

  }
}

#endif
