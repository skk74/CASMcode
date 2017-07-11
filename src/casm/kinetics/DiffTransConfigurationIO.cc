#include "casm/kinetics/DiffTransConfigurationIO.hh"
#include "casm/app/ClexDescription.hh"
#include "casm/app/ProjectSettings.hh"

namespace CASM {

  template class BaseDatumFormatter<Kinetics::DiffTransConfiguration>;
  template class DataFormatterOperator<bool, std::string, Kinetics::DiffTransConfiguration>;
  template class DataFormatterOperator<bool, bool, Kinetics::DiffTransConfiguration>;
  template class DataFormatterOperator<bool, double, Kinetics::DiffTransConfiguration>;
  template class DataFormatterOperator<double, double, Kinetics::DiffTransConfiguration>;
  template class DataFormatterOperator<Index, double, Kinetics::DiffTransConfiguration>;
  template class DataFormatter<Kinetics::DiffTransConfiguration>;
  template bool DataFormatter<Kinetics::DiffTransConfiguration>::evaluate_as_scalar<bool>(Kinetics::DiffTransConfiguration const &) const;
  template double DataFormatter<Kinetics::DiffTransConfiguration>::evaluate_as_scalar<double>(Kinetics::DiffTransConfiguration const &) const;
  template class DataFormatterDictionary<Kinetics::DiffTransConfiguration>;

  namespace Kinetics {
    namespace DiffTransConfigIO {

      // --- LocalCorr implementations -----------

      const std::string LocalCorr::Name = "local_corr";

      const std::string LocalCorr::Desc =
        "Local Correlation values (evaluated basis functions). "
        "If no arguments, prints all local correlations, using the basis set for the default "
        "cluster expansion for this diff_trans_config as listed by 'casm settings -l'. "
        "If one argument, accepts either: "
        "1) a cluster expansion name, for example 'local_corr(kra_barrier)', and "
        "evaluates all basis functions, or "
        "2) an integer index or range of indices of basis functions to evaluate, "
        "for example 'local_corr(6)', or 'local_corr(0:6)'. "
        "If two arguments, accepts cluster expansion name and an integer index or "
        "range of basis functions to evaluate, for example 'local_corr(kra_barrier,6)' "
        "or 'local_corr(kra_barrier,0:6)'.";

      /// \brief Returns the correlations
      Eigen::VectorXd LocalCorr::evaluate(const DiffTransConfiguration &dtconfig) const {
        return correlations(dtconfig, m_clexulator);
      }

      /// \brief If not yet initialized, use the default clexulator from the PrimClex
      void LocalCorr::init(const DiffTransConfiguration &_tmplt) const {
        if(!m_clexulator.initialized()) {
          const PrimClex &primclex = _tmplt.primclex();
          //Need to get default clex for a given hop based on the orbit name of the diff_trans_config
          //unless each hop has its own basis set folder similar to default
          //The fact that each hop has its own basis set folder might cause problems for selecting defaults
          //for selections in which diff_trans_configs contain different orbit names.
          //The evaluation should return a characteristic value (negative value) for mismatch hops to corr request
          ClexDescription desc = m_clex_name.empty() ?
                                 primclex.settings().default_clex() : primclex.settings().clex(m_clex_name);
          m_clexulator = primclex.clexulator(desc);
        }

        VectorXdAttribute<DiffTransConfiguration>::init(_tmplt);

      }

      /// \brief Expects 'local_corr', 'local_corr(clex_name)', 'local_corr(index_expression)', or
      /// 'local_corr(clex_name,index_expression)'
      bool LocalCorr::parse_args(const std::string &args) {
        std::vector<std::string> splt_vec;
        boost::split(splt_vec, args, boost::is_any_of(","), boost::token_compress_on);

        if(!splt_vec.size()) {
          return true;
        }
        else if(splt_vec.size() == 1) {
          if((splt_vec[0].find_first_not_of("0123456789") == std::string::npos) ||
             (splt_vec[0].find(':') != std::string::npos)) {
            _parse_index_expression(splt_vec[0]);
          }
          else {
            m_clex_name = splt_vec[0];
          }
        }
        else if(splt_vec.size() == 2) {
          m_clex_name = splt_vec[0];
          _parse_index_expression(splt_vec[1]);
        }
        else {
          std::stringstream ss;
          ss << "Too many arguments for 'local_corr'.  Received: " << args << "\n";
          throw std::runtime_error(ss.str());
        }
        return true;
      }

      // --- LocalClex implementations -----------

      const std::string LocalClex::Name = "local_clex";

      const std::string LocalClex::Desc =
        "Predicted local property value."
        " Accepts arguments ($clex_name,$norm)."
        " ($clex_name is a cluster expansion name as listed by 'casm settings -l', default=the default clex)"
        " ($norm is the normalization, either 'per_species', or 'per_unitcell' <--default)";

      LocalClex::LocalClex() :
        ScalarAttribute<DiffTransConfiguration>(Name, Desc) {
        parse_args("");
      }

      LocalClex::LocalClex(const Clexulator &clexulator, const ECIContainer &eci) :
        ScalarAttribute<DiffTransConfiguration>(Name, Desc),
        m_clexulator(clexulator),
        m_eci(eci) {
      }

      /// \brief Returns the atom fraction
      double LocalClex::evaluate(const DiffTransConfiguration &dtconfig) const {
        return m_eci * correlations(dtconfig, m_clexulator);
      }

      /// \brief Clone using copy constructor
      std::unique_ptr<LocalClex> LocalClex::clone() const {
        return std::unique_ptr<LocalClex>(this->_clone());
      }

      /// \brief Checks to see if clexulator and eci match orbit name of dtconfig
      bool LocalClex::validate(const DiffTransConfiguration &dtconfig) const {
        //actually check here
        return false;
      }

      /// \brief If not yet initialized, use the default cluster expansion from the PrimClex
      void LocalClex::init(const DiffTransConfiguration &_tmplt) const {
        if(!m_clexulator.initialized()) {
          const PrimClex &primclex = _tmplt.primclex();
          ClexDescription desc = m_clex_name.empty() ?
                                 primclex.settings().default_clex() : primclex.settings().clex(m_clex_name);
          m_clexulator = primclex.clexulator(desc);
          m_eci = primclex.eci(desc);
          if(m_eci.index().back() >= m_clexulator.corr_size()) {
            Log &err_log = default_err_log();
            err_log.error<Log::standard>("bset and eci mismatch");
            err_log << "basis set size: " << m_clexulator.corr_size() << std::endl;
            err_log << "max eci index: " << m_eci.index().back() << std::endl;
            throw std::runtime_error("Error: bset and eci mismatch");
          }
        }
      }

      /// \brief Expects 'clex', 'clex(formation_energy)'
      bool LocalClex::parse_args(const std::string &args) {
        std::vector<std::string> splt_vec;
        boost::split(splt_vec, args, boost::is_any_of(","), boost::token_compress_on);

        m_clex_name = "";
        if(splt_vec.size()) {
          m_clex_name = splt_vec[0];
        }

        if(splt_vec.size() > 1) {
          std::stringstream ss;
          ss << "Too many arguments for 'local_clex'.  Received: " << args << "\n";
          throw std::runtime_error(ss.str());
        }

        return true;
      }

      /// \brief Clone using copy constructor
      LocalClex *LocalClex::_clone() const {
        return new LocalClex(*this);
      }
    }
  }

  template<>
  StringAttributeDictionary<Kinetics::DiffTransConfiguration> make_string_dictionary<Kinetics::DiffTransConfiguration>() {

    using namespace Kinetics::DiffTransConfigIO;
    StringAttributeDictionary<Kinetics::DiffTransConfiguration> dict;

    dict.insert(
      name<Kinetics::DiffTransConfiguration>()
    );
    return dict;
  }

  template<>
  BooleanAttributeDictionary<Kinetics::DiffTransConfiguration> make_boolean_dictionary<Kinetics::DiffTransConfiguration>() {

    using namespace Kinetics::DiffTransConfigIO;
    BooleanAttributeDictionary<Kinetics::DiffTransConfiguration> dict;
    return dict;
  }

  template<>
  IntegerAttributeDictionary<Kinetics::DiffTransConfiguration> make_integer_dictionary<Kinetics::DiffTransConfiguration>() {

    using namespace Kinetics::DiffTransConfigIO;
    IntegerAttributeDictionary<Kinetics::DiffTransConfiguration> dict;
    return dict;
  }

  template<>
  ScalarAttributeDictionary<Kinetics::DiffTransConfiguration> make_scalar_dictionary<Kinetics::DiffTransConfiguration>() {

    using namespace Kinetics::DiffTransConfigIO;
    ScalarAttributeDictionary<Kinetics::DiffTransConfiguration> dict;
    return dict;
  }

  template<>
  VectorXiAttributeDictionary<Kinetics::DiffTransConfiguration> make_vectorxi_dictionary<Kinetics::DiffTransConfiguration>() {
    using namespace Kinetics::DiffTransConfigIO;
    VectorXiAttributeDictionary<Kinetics::DiffTransConfiguration> dict;
    return dict;
  }

  template<>
  VectorXdAttributeDictionary<Kinetics::DiffTransConfiguration> make_vectorxd_dictionary<Kinetics::DiffTransConfiguration>() {

    using namespace Kinetics::DiffTransConfigIO;
    VectorXdAttributeDictionary<Kinetics::DiffTransConfiguration> dict;
    return dict;
  }

}
