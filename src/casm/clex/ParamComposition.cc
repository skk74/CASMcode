#include "casm/clex/ParamComposition.hh"

#include <cmath>
#include <unistd.h>

#include "casm/misc/algorithm.hh"
#include "casm/misc/CASM_Eigen_math.hh"
#include "casm/crystallography/Structure.hh"
#include "casm/clex/PrimClex.hh"

namespace CASM {


  //*************************************************************
  //GENERATE Routines

  //*************************************************************
  /* GENERATE COMPONENTS

     This routine generates the set ofx unique alloying components
     that are listed in the prim structure. The unique alloying
     components are stored as an std::vector< std::string >
  */
  //*************************************************************

  void ParamComposition::generate_components() {

    if(m_components.size() != 0) {
      std::cerr << "WARNING in ParamComposition::generate_components(), the components data member in the class";
      std::cerr << " is not empty. Clearing it anyways.\n";
      m_components.clear();
    }

    auto struc_molecule = m_prim_struc->struc_molecule();
    for(auto it = struc_molecule.begin(); it != struc_molecule.end(); ++it) {
      m_components.push_back(it->name());
    }

    return;
  }

  //---------------------------------------------------------------------------

  //*************************************************************
  /* GENERATE SUBLATTICE MAP

     This routine generates a matrix that has sublattice sites on
     which a specific component is allowed to alloy. Consider the
     example:
     species [1]   [2]  -> sublattice index
     [Ga]     1     0
     [As]     1     1
     [In]     0     1

     The 1's indicate that that component is allowed on that
     specific sublattice.

  */
  //*************************************************************
  void ParamComposition::generate_sublattice_map() {
    if(m_components.size() == 0)
      generate_components();
    //figuring out the number of sublattices on which alloying is happening
    std::vector< std::vector< std::string> > tocc;
    std::vector< std::string > tlist;
    for(Index i = 0; i < m_prim_struc->basis().size(); i++) {
      tlist = m_prim_struc->basis()[i].allowed_occupants();
      //keep only those sublattices where alloying is allowed
      // if(tlist.size() == 1) {
      //   continue;
      // }
      tocc.push_back(tlist);
    }
    //resize the sublattice_map array to <number_components, number_sublattices>
    m_sublattice_map.setZero(m_components.size(), tocc.size());
    for(Index i = 0; i < tocc.size(); i++) {
      for(Index j = 0; j < tocc[i].size(); j++) {
        //If the component is not found in the components array, something is wrong!
        if(!contains(m_components, tocc[i][j])) {
          std::cerr << "ERROR:Your component matrix has been initialized badly. Quitting\n";
          exit(666);
        }
        //Find the position of the component in the components array and increment sublattice_map
        Index pos = find_index(m_components, tocc[i][j]);
        m_sublattice_map(pos, i)++;
      }
    }
  }

  //---------------------------------------------------------------------------

  //*************************************************************
  /*   GENERATE_END_MEMBERS

       End members are generated by assigning priority values to each
       component. Based on the priority value, the number of atoms for
       every component is maximized. The routine then iterates thorough
       all possible permutations of the priority values to generate all
       possible end members
  */
  //*************************************************************

  void ParamComposition::generate_prim_end_members() {
    if(m_sublattice_map.rows() == 0 || m_sublattice_map.cols() == 0)
      generate_sublattice_map();

    //the number of atoms of components[priority_index[0]] is
    //maximized first following this the number of atoms of
    //components[priority_index[1]] is maxed out and so on
    std::vector<int> priority_index;

    //Holds a list of possible end members, this list is appended to
    //as and when we find an end_member.
    std::vector< Eigen::MatrixXi > tend_members;
    Eigen::MatrixXi tend;

    //set the size of priority index to the number of components in
    //the system, following this it is seeded with a starting priority
    priority_index.resize(m_components.size());
    for(Index i = 0; i < priority_index.size(); i++) {
      priority_index[i] = i;//Initialize the priority_index to [0,1,2,...,(N-1)]
    }
    do {
      //tsublat_comp is meant to keep track of which compositions have
      //already been maxed out
      Eigen::MatrixXi tsublat_comp = m_sublattice_map;

      //resize and set tend to 0's,we will add to this
      //array as we loop over the priority indices
      tend.setZero(1, m_sublattice_map.rows());

      //calculate the end_member that corresponds to this
      //priority_index
      for(Index i = 0; i < priority_index.size(); i++) {
        tend(0, priority_index[i]) = tsublat_comp.row(priority_index[i]).sum();
        //set the value to 0 in those sublattices that have been maxed out
        max_out(priority_index[i], tsublat_comp);
      }

      //figure out if it is already in our list of end members
      bool is_unique = true;
      for(Index i = 0; i < tend_members.size(); i++) {
        if(tend == tend_members[i]) {
          is_unique = false;
          break;
        }
      }
      if(is_unique) {
        tend_members.push_back(tend); //add to the list if it is unique
      }
    }
    while(next_permutation(priority_index.begin(), priority_index.end())); //repeat the above for all permutations of priority_index

    //Store tend_members as an Eigen::MatrixXi
    //makes it easier to find the rank of the space
    m_prim_end_members.resize(tend_members.size(), m_sublattice_map.rows());
    for(Index i = 0; i < tend_members.size(); i++) {
      for(EigenIndex j = 0; j < m_sublattice_map.rows(); j++) {
        m_prim_end_members(i, j) = double(tend_members[i](0, j));
      }
    }
  }

  //---------------------------------------------------------------------------

  //*********************************************************************
  /* GENERATE_COMPOSITION_SPACE

     generates all possible composition axes that result in positive
     values of the parametric composition.
     ALGORITHM:
       - start by finding the rank of the space that user has defined
         in the PRIM
       - pick one of the end_members as the origin. To enumerate all
         possible axes, we loop through all possible end_members
       - (rank-1) number of end_members are picked as spanning end
         members from the remaining list of end_members that we get
         from the PRIM
       - A composition object is calculated that is then used to
         calculated the parametric Composition given the current
         choice of end members and origin. If it results in positive
         numbers for all the end members that are listed for the PRIM,
         this set of (origin,spanning end members) is pushed back onto
         the allowed list of composition axes
       - The process is repeated for all such unique combinations of
         (origin, spanning end members)
   */
  //*********************************************************************


  void ParamComposition::generate_composition_space(bool verbose) {
    //Eigen object to do the QR decomposition of the list of prim_end_members
    Eigen::FullPivHouseholderQR<Eigen::MatrixXd> qr(m_prim_end_members);
    Eigen::VectorXd torigin; //temp origin
    Eigen::VectorXd test_comp;
    std::vector< Eigen::VectorXd > tspanning; //set of spanning end members
    bool is_positive; //flag to test for positive composition axes
    std::vector<int> priority_index;

    // If there is already a set of enumerated spaces for this
    // Composition object
    if(m_allowed_list.size() != 0) {
      std::cerr << "WARNING in ParamComposition::generate_composition_space, your allowed_list is non-empty. If you are not careful,you may end up with repeats of allowed composition axes" << std::endl;
    }

    // calculate the rank of the space.
    // NOTE to the wise: # of spanning members = rank-1
    m_rank_of_space = qr.rank();
    if(verbose)
      std::cout << "Rank of space : " << m_rank_of_space << std::endl;

    //This array is used to figure out which of the end members to
    //select as spanning end members
    std::vector<int> binary_choose(m_prim_end_members.rows() - 1, 0);

    //the priority index is used to pick a set of origin and end
    //members to span the space
    for(EigenIndex i = 0; i < m_prim_end_members.rows(); i++) {
      priority_index.push_back(i);
    }

    for(int i = 0; i + 1 < (m_rank_of_space); i++) {
      binary_choose[binary_choose.size() - 1 - i] = 1;
    }
    if(verbose)
      std::cout << "Binary choose: " << binary_choose << std::endl;

    if(verbose)
      std::cout << "Computing the possible composition axes ... " << std::endl;

    //loop through all the end members and start them off as the origin
    for(EigenIndex i = 0; i < m_prim_end_members.rows(); i++) {
      torigin = m_prim_end_members.row(i).transpose();

      if(verbose)
        std::cout << "The origin is: " << torigin << std::endl;

      //NOTE: priority seems to be a misnomer here, its really the
      //list of end members that are in contention to be considered as
      //the set of spanning end members. Consider changing the name
      std::vector<int> tpriority = priority_index;
      tpriority.erase(tpriority.begin() + i); //remove the 'origin' from contention
      std::vector<int> tbinary_choose(binary_choose.begin(), binary_choose.end());

      //loop over all permutations of priority_index and pick rank-1
      //end members to span the space. Only keep those combinations
      //that result in positive compositions
      do {
        tspanning.clear();
        std::vector< Eigen::VectorXi > tvec; //hold the list of end members that are being considered

        if(verbose)
          std::cout << "The end members being considered: " << std::endl;

        for(Index j = 0; j < tbinary_choose.size(); j++) {
          if(tbinary_choose[j] == 1) {
            tspanning.push_back((m_prim_end_members.row(tpriority[j]).transpose() - torigin));
            tvec.push_back(m_prim_end_members.row(tpriority[j]).transpose().cast<int>());
          }
          if(verbose)
            std::cout << m_prim_end_members.row(tpriority[j]) << std::endl;
        }

        if(verbose)
          std::cout << "---" << std::endl;

        //initialize a ParamComposition object with these spanning vectors and origin
        ParamComposition tcomp = calc_composition_object(torigin, tspanning);
        is_positive = true;

        //loop through end members and see what the values of the compositions works out to
        if(verbose)
          std::cout << "Calculated compositions:" << std::endl;

        for(EigenIndex j = 0; j < m_prim_end_members.rows(); j++) {
          // Calculates the composition value given the previously
          // initialized Composition object
          test_comp = tcomp.calc(m_prim_end_members.row(j), NUMBER_ATOMS);
          if(verbose)
            std::cout << m_prim_end_members.row(j) << "  :  " << test_comp << std::endl;

          for(EigenIndex k = 0; k < test_comp.size(); k++) {
            //if the calculated parametric composition value is either
            //less than 0 or is nan(this will occur if the current set
            //of origin and spanning end members form a subspace in
            //the composition space for this PRIM)
            if((test_comp(k) < 0 && !almost_zero(test_comp(k))) || std::isnan(test_comp(k))) {
              is_positive = false;
              break;
            }
          }
          if(!is_positive) {
            break;
          }
        }
        if(is_positive) {
          //push back this composition object, its good!
          m_allowed_list.push_back(tcomp);
        }

      }
      while(next_permutation(tbinary_choose.begin(), tbinary_choose.end()));
      fflush(stdout);
    }
    std::cout << "                                                                                                                          \r";
    fflush(stdout);
  }

  //---------------------------------------------------------------------------

  //*************************************************************
  //PRINT Routines
  void ParamComposition::print_composition_formula(std::ostream &stream, const int &stream_width) const {
    int composition_var = (int)'a';
    std::stringstream tstr;
    for(Index i = 0; i < m_components.size(); i++) {
      bool first_char = true;
      tstr << m_components[i] << "(";
      if(!almost_zero(m_origin(i))) {
        first_char = false;
        tstr << m_origin(i);
      }
      for(int j = 0; j + 1 < (m_rank_of_space); j++) {
        if(almost_zero(m_comp[PARAM_COMP](i, j))) {
          continue;
        }
        if(almost_zero(m_comp[PARAM_COMP](i, j) - 1)) {
          if(first_char) {
            tstr << (char)(composition_var + j);
            first_char = false;
          }
          else {
            tstr << '+' << (char)(composition_var + j);
          }
        }
        else if(almost_zero(m_comp[PARAM_COMP](i, j) + 1)) {
          tstr << '-' << (char)(composition_var + j);
          first_char = false;
        }
        else {
          stream << m_comp[PARAM_COMP](i, j) << (char)(composition_var + j);
          first_char = false;
        }
      }
      tstr << ")";
    }

    stream << tstr.str().c_str();

    return;
  }

  //---------------------------------------------------------------------------

  void ParamComposition::print_member_formula(const Eigen::VectorXd &member, std::ostream &stream, const int &stream_width) const {
    std::stringstream tstr;
    for(EigenIndex i = 0; i < member.size(); i++) {
      if(almost_zero(member(i))) {
        continue;
      }
      if(almost_zero(member(i) - 1)) {
        tstr << m_components[i];
      }
      else {
        tstr << m_components[i] << int(member(i));
      }
    }
    stream << std::setw(stream_width) << tstr.str().c_str();
  }

  //---------------------------------------------------------------------------

  void ParamComposition::print_origin_formula(std::ostream &stream, const int &stream_width) const {
    print_member_formula(m_origin, stream, stream_width);
  }

  //---------------------------------------------------------------------------

  void ParamComposition::print_composition_axes(std::ostream &stream) const {
    stream << "Number of choices of composition axes: " << m_allowed_list.size() << std::endl;
    //Print Header: ORIGIN, <COMPOUNDS AT THE ENDS OF DIFFERENT AXES> , GENERAL FORMULA

    stream << std::setw(10) << "INDEX";
    stream << std::setw(10) << "ORIGIN";
    for(int i = 0; i + 1 < m_rank_of_space; i++) {
      stream << std::setw(10) << (char)((int)'a' + i);
    }
    stream << "    ";
    stream << "GENERAL FORMULA";
    stream << std::endl;

    stream << std::setw(10) << "  ---";
    stream << std::setw(10) << "  ---";
    for(int i = 0; i < (m_rank_of_space - 1); i++) {
      stream << std::setw(10) << "  ---";
    }
    stream << "    ";
    stream << "---" << std::endl;

    for(Index i = 0; i < m_allowed_list.size(); i++) {
      stream << std::setw(10) << i;
      m_allowed_list[i].print_origin_formula(stream, 10);
      std::vector< Eigen::VectorXd > allowed_spanning_end_members;
      allowed_spanning_end_members = m_allowed_list[i].spanning_end_members();
      for(Index j = 0; j < allowed_spanning_end_members.size(); j++) {
        print_member_formula(allowed_spanning_end_members[j], stream, 10);
      }
      stream << "    ";
      m_allowed_list[i].print_composition_formula(stream, 20);
      stream << std::endl;
    }
    //        print_end_member_formula(1,std::cout,10);
    // for(Index i=0;i<allowed_list.size();i++){
    //     allowed_list[i].print_composition_formula(std::cout);
    //     std::cout<<std::endl;
    // }
  }

  //---------------------------------------------------------------------------

  void ParamComposition::print_curr_composition_axes(std::ostream &stream) const {
    //Print Header: ORIGIN, <COMPOUNDS AT THE ENDS OF DIFFERENT AXES> , GENERAL FORMULA

    stream << std::setw(20) << "ORIGIN";
    for(int i = 0; i < (m_rank_of_space - 1); i++) {
      stream << std::setw(10) << (char)((int)'a' + i);
    }
    stream << "    ";
    stream << "GENERAL FORMULA";
    stream << std::endl;

    stream << std::setw(20) << "  ---";
    for(int i = 0; i < (m_rank_of_space - 1); i++) {
      stream << std::setw(10) << "  ---";
    }
    stream << "    ";
    stream << "---" << std::endl;

    print_origin_formula(stream, 20);
    std::vector< Eigen::VectorXd > allowed_spanning_end_members;
    allowed_spanning_end_members = spanning_end_members();
    for(int j = 0; j < allowed_spanning_end_members.size(); j++) {
      print_member_formula(allowed_spanning_end_members[j], stream, 10);
    }
    stream << "    ";
    print_composition_formula(stream, 20);
    stream << std::endl;

  }


  //*************************************************************
  //CALC ROUTINES

  //*************************************************************
  /*
     CALC

     To calculate the composition AFTER having set the origin and
     spanning end members for the class, you need to pass it the
     "given" values i.e. either the parametric composition or the
     number of atoms per primClex unit.
     If you want the PARAM_COMP given NUMBER_ATOMS set the mode to
     PARAM_COMP.
     If you eant the NUMBER_ATOMS given PARAM_COMP set the mode to
     NUMBER_ATOMS.
     i.e. set the mode to whatever is the quantity that you are giving
     the class
   */
  //*************************************************************

  Eigen::VectorXd ParamComposition::calc(const Eigen::VectorXd &tcomp, const int &MODE) {
    if(MODE == PARAM_COMP) {
      return m_origin + m_comp[PARAM_COMP] * tcomp;
    }
    else {
      return (m_comp[NUMBER_ATOMS] * (tcomp - m_origin)).head(m_rank_of_space - 1);
    }
  }

  //*************************************************************

  Eigen::VectorXd ParamComposition::calc_param_composition(const Eigen::VectorXd &num_atoms_per_prim) const {
    return (m_comp[NUMBER_ATOMS] * (num_atoms_per_prim - m_origin)).head(m_rank_of_space - 1);
  }

  //*************************************************************

  Eigen::VectorXd ParamComposition::calc_num_atoms(const Eigen::VectorXd &param_composition) const {
    return m_origin + m_comp[PARAM_COMP] * param_composition;
  }

  //*************************************************************

  std::vector<std::pair<std::string, Index> > ParamComposition::fixed_components() {
    std::vector<std::pair<std::string, Index> >  tcompon;
    if(m_prim_end_members.cols() == 0)
      generate_prim_end_members();
    Eigen::MatrixXd end_members = m_prim_end_members.transpose();
    Eigen::VectorXd sum_vec(Eigen::VectorXd::Zero(end_members.rows()));
    for(Index i = 1; i < end_members.cols(); i++) {
      sum_vec += ((end_members.col(i) - end_members.col(0)).array().abs()).matrix();
    }
    for(Index i = 0; i < sum_vec.size(); i++) {
      if(almost_zero(sum_vec[i]))
        tcompon.push_back(std::pair<std::string, Index> (m_components[i], round(end_members(i, 0))));
    }
    return tcompon;
  }

  //*************************************************************

  //Given an origin and spanning vectors, returns a ParamComposition object that points to the same Prim as (*this)
  ParamComposition ParamComposition::calc_composition_object(const Eigen::VectorXd &torigin, const std::vector< Eigen::VectorXd> tspanning) {
    //holds the temporary transformation matrix that is going to be
    //used to initialize the new composition object
    Eigen::MatrixXd tmat;
    if(tspanning[0].size() != m_components.size()) {
      std::cerr << "ERROR in ParamComposition::calc_composition_object the spanning vectors are not as long as the number of ";
      std::cerr << "components in this system. I'm confused and recommend you quit and try again. However, not going to force quit\n";
    }
    tmat.resize(m_components.size(), m_components.size());
    //copy the spanning vectors into tmat
    for(Index i = 0; i < tspanning.size(); i++) {
      tmat.col(i) = tspanning[i];
    }
    //generate an orthogonal set if there aren't as many spanning
    //vectors as the number of components in the system
    if(tspanning.size() < (m_components.size())) {
      Eigen::FullPivHouseholderQR<Eigen::MatrixXd> qr(tmat.leftCols(tspanning.size()));
      Eigen::MatrixXd torthogonal = qr.matrixQ();
      tmat.rightCols((m_components.size() - tspanning.size())) = torthogonal.rightCols((m_components.size() - tspanning.size()));
      //copy the orthogonalized vectors into tmat. We now have a
      //complete spanning set in this component space
    }
    return ParamComposition(m_components, tmat, torigin, m_rank_of_space, *m_prim_struc, PARAM_COMP);
  }

  //assuming that you have filled in the prim_end_members and the
  //origin. This fills up the transformation matrices
  void ParamComposition::calc_transformation_matrices() {
    Eigen::MatrixXd tmat;
    tmat.resize(m_components.size(), m_components.size());
    for(Index i = 0; i < m_spanning_end_members.size(); i++) {
      tmat.col(i) = m_spanning_end_members[i] - m_origin;
    }
    if(m_spanning_end_members.size() < m_components.size()) {
      Eigen::FullPivHouseholderQR<Eigen::MatrixXd> qr(tmat.leftCols(m_spanning_end_members.size()));
      Eigen::MatrixXd torthogonal = qr.matrixQ();
      tmat.rightCols((m_components.size() - m_spanning_end_members.size())) = torthogonal.rightCols((m_components.size() - m_spanning_end_members.size()));
      //copy the orthogonalized vectors into tmat. We now have a
      //complete spanning set in this component space
    }
    if(m_comp.size() != 2) {
      m_comp.resize(2);
    }
    m_comp[PARAM_COMP] = tmat;
    m_comp[NUMBER_ATOMS] = m_comp[PARAM_COMP].inverse();
  }

  /*
    //use this to write out the composition object into a JSON format
    ptree ParamComposition::calc_composition_ptree() const {
      ptree comp_ptree;
      //add the allowed coomposition axes into the ptree
      if(m_allowed_list.size() > 0) {
        std::string root("");
        //      ptree axes_list;
        //add ptrees that contain information from the allowed composition axes
        for(Index i = 0; i < m_allowed_list.size(); i++) {
          std::stringstream strs;
          strs << root << "allowed_axes.";
          strs << i;
          comp_ptree.put_child(strs.str().c_str(), m_allowed_list[i].calc_composition_ptree());
        }
      }

      //print the composition members
      //components
      if(m_components.size() > 0) {
        std::string root("");
        root.append("components");
        std::string components_string;
        for(Index i = 0; i < m_components.size(); i++) {
          components_string.append(m_components[i]);
          components_string.append("   ");
        }
        boost::algorithm::trim(components_string);
        comp_ptree.put(root, components_string.c_str());
      }

      //origin
      if(m_origin.size() > 0) {
        std::string root("");
        root.append("origin");
        std::stringstream origin_strs;
        origin_strs << m_origin;
        std::string origin_str;
        origin_str = origin_strs.str();
        boost::replace_all(origin_str, "\n", "   ");
        comp_ptree.put(root, origin_str.c_str());
      }

      //spanning end members
      if(m_rank_of_space > 0 && m_comp[PARAM_COMP].rows() > 0) {
        std::string root("");
        root.append("end_members");
        //std::vector< Eigen::VectorXd > tspanning_end_members = spanning_end_members();
        //      ptree end_members_ptree;
        for(Index i = 0; i < m_spanning_end_members.size(); i++) {
          std::stringstream tspan_strs;
          tspan_strs << m_spanning_end_members[i];
          std::string tspan_str = tspan_strs.str();
          boost::replace_all(tspan_str, "\n", "   ");
          //        end_members_ptree.put();
          std::stringstream tname;
          tname << root << ".";
          tname << char('a' + i);
          comp_ptree.put(tname.str().c_str(), tspan_str.c_str());
        }
      }

      //rank_of_space
      if(m_rank_of_space > 0) {
        std::string root("");
        root.append("rank_of_space");
        std::stringstream rank_strs;
        rank_strs << m_rank_of_space;
        comp_ptree.put(root.c_str(), rank_strs.str().c_str());
      }

      return comp_ptree;

    }
  */
  //**************************************************************
  /*SPANNING END MEMBERS

    Returns an std::vector< Eigen::VectorXi > that contain the spanning end
    members listed in the same order as they occur in the
    transformation matrix.

  */
  void ParamComposition::calc_spanning_end_members() {
    std::vector< Eigen::VectorXd > tspan_end;
    if(m_rank_of_space <= 0) {
      std::cerr << "WARNING something is wrong in ParamComposition::spanning_end_members. The rank_of_space in the ParamComposition object is <=0. I do not know how to calculate the end_members in such a space" << std::endl;
      m_spanning_end_members = tspan_end;
      return;
    }
    for(int i = 0; i + 1 < m_rank_of_space; i++) {
      tspan_end.push_back((m_comp[PARAM_COMP].col(i) + m_origin));
    }
    m_spanning_end_members = tspan_end;
  }

  //*************************************************************
  //READ
  /*
    void ParamComposition::read(const std::string &comp_filename) {
      std::ifstream in_comp;
      in_comp.open(comp_filename.c_str());
      if(!in_comp) {
        std::cerr << "ERROR in ParamComposition::read. Could not read the file: " << comp_filename.c_str() << std::endl;
        std::cerr << "Continuing anyways. However, things could go horribly wrong. I recommend you quit." << std::endl;
        return;
      }
      read(in_comp);

    }

    void ParamComposition::read(std::istream &stream) {
      ptree comp_ptree;
      read_json(stream, comp_ptree);
      read(comp_ptree);
    }

    void ParamComposition::read(ptree comp_ptree) {

      //std::cout<<"In ParamComposition::read"<<std::endl;
      //try to read in the allowed_axes
      try {
        const ptree &allowed_axes_ptree = comp_ptree.child("allowed_axes");
        //std::cout<<"Reading the allowed_axes field"<<std::endl;
        int allowed_axes_num = 0;
        do {
          try {
            std::stringstream tstr;
            tstr << allowed_axes_num;
            ParamComposition tc(allowed_axes_ptree.child(tstr.str().c_str()) , *m_prim_struc);
            m_allowed_list.push_back(tc);
          }
          catch(std::exception const &e) {
            break;
          }
          allowed_axes_num++;
        }
        while(true);
      }
      catch(std::exception const &e) {
        //continue
      }

      //try to read in the components
      try {
        if(m_components.size() != 0) {
          std::cerr << "WARNING in ParamComposition::read. your components matrix is not empty. Clearing that array and any other non-empty data members" << std::endl;
          m_components.clear();
          m_comp.clear();
          m_allowed_list.clear();
        }
        std::string tcomp = comp_ptree.get<std::string>("components");
        boost::char_separator< char > sep(" ");
        boost::tokenizer< boost::char_separator< char > > tokens(tcomp, sep);
        BOOST_FOREACH(const std::string & t, tokens) {
          m_components.push_back(t);
        }

        //resize the other matrices to match this one
        m_comp.resize(2);
        m_comp[0].resize(m_components.size(), m_components.size());
        m_comp[1].resize(m_components.size(), m_components.size());
      }
      catch(std::exception const &e) {
        //      continue;
      }

      //try to read in the origin
      try {
        std::string tcomp = comp_ptree.get<std::string>("origin");
        if(m_components.size() == 0) {
          std::cerr << "ERROR in ParamComposition::read. You are trying to set an origin without specifying the components. This is dangerous, and not allowed. QUITTING." << std::endl;
          exit(666);
        }
        m_origin = eigen_vector_from_string(tcomp, m_components.size());
      }
      catch(std::exception const &e) {
        //      continue;
      }

      //read in the end members and then set up the transformation matrices
      try {
        const ptree &end_members_ptree = comp_ptree.child("end_members");
        std::string comp_axis(1, 'a');
        do {
          try {
            std::string span_string = end_members_ptree.get< std::string > (comp_axis.c_str());
            m_spanning_end_members.push_back(eigen_vector_from_string(span_string, m_components.size()));
          }
          catch(std::exception const &e) {
            break;
          }
          comp_axis[0]++;
        }
        while(true);
        //update the transformation matrices in comp.
        calc_transformation_matrices();
      }
      catch(std::exception const &e) {
        //      continue;
      }

      //reading in the rank_of_space
      try {
        std::string trank_str = comp_ptree.get<std::string>("rank_of_space");
        m_rank_of_space = atoi(trank_str.c_str());
      }
      catch(std::exception const &e) {
        m_rank_of_space = -1;
        //continue
      }

    }
  */
  //*************************************************************
  //MISCELLANEOUS

  //*************************************************************
  /* MAX OUT
     Given a sublat_comp, say:
     [1]  [2]
     [Ga]  1    0
     [As]  1    1
     [In]  0    1
     say that we have our priority_index set up to maximize [Ga]
     we need tochange the 1 in [As] 1st column to 0, since Ga now
     occupies that sublattice. Max Out does the appropriate subt-
     ractions and returns the modified matrix
  */
  //*************************************************************
  void ParamComposition::max_out(const int &component_index, Eigen::MatrixXi &sublat_comp) const {
    for(EigenIndex i = 0; i < sublat_comp.cols(); i++) {
      if(sublat_comp(component_index, i) > 0) {
        for(EigenIndex j = 0; j < sublat_comp.rows(); j++) {
          sublat_comp(j, i) = 0;
        }
      }
    }
    return;
  }

  void ParamComposition::select_composition_axes(const Index &choice) {

    //printing the data from the 'choice'
    // std::cout<<"This is the data from choice: "<<std::endl;
    // allowed_list[choice].print_composition_matrices(std::cout);
    if(m_allowed_list.size() < choice + 1) {
      std::cerr << "ERROR in ParamComposition::select_composition_axes. Your value of choice is outside the range of allowed_list" << std::endl;
      exit(666);
    }
    m_comp = m_allowed_list[choice].comp();
    //    components = allowed_list[choice].components();
    m_origin = m_allowed_list[choice].origin();
    m_rank_of_space = m_allowed_list[choice].rank_of_space();
    m_spanning_end_members = m_allowed_list[choice].spanning_end_members();
    //    print_composition_matrices(std::cout);
  }


  //*************************************************************
  //ACCESSORS

  std::string ParamComposition::composition_formula() const {
    std::stringstream ss;
    print_composition_formula(ss, 20);
    return ss.str();
  }


}

