#include "casm/BP_C++/BP_Vec.hh"
#include "casm/BP_C++/BP_Parse.hh"
#include "casm/crystallography/Structure.hh"

namespace CASM {

  //John G 011013
  //********************************************************************
  //Copy constructor
  template<typename ClustType>
  GenericOrbitree<ClustType>::GenericOrbitree(const GenericOrbitree<ClustType> &starttree) :
    Array<GenericOrbitBranch<ClustType> >() {
    //set_lattice(starttree.lattice, FRAC);

    //copy members
    lattice = starttree.lattice;
    max_num_sites = starttree.max_num_sites;
    min_num_components = starttree.min_num_components;
    max_length = starttree.max_length;
    min_length = starttree.min_length;
    num_clusts = starttree.num_clusts;
    index_to_row = starttree.index_to_row;
    index_to_column = starttree.index_to_column;
    index = starttree.index;
    Norbits = starttree.Norbits;
    subcluster = starttree.subcluster;

    //copy all orbitbranches over
    for(Index b = 0; b < starttree.size(); b++) {
      push_back(starttree[b]);
    }
  };
  //********************************************************************

  template<typename ClustType>
  const GenericOrbit<ClustType> &GenericOrbitree<ClustType>::orbit(Index np, Index no) const {
    return at(np).at(no);
  }

  //********************************************************************

  template<typename ClustType>
  GenericOrbit<ClustType> &GenericOrbitree<ClustType>::orbit(Index np, Index no) {
    return at(np).at(no);
  }

  //********************************************************************

  template<typename ClustType>
  const ClustType &GenericOrbitree<ClustType>::prototype(Index np, Index no) const {
    return at(np).at(no).prototype;
  }

  //********************************************************************

  template<typename ClustType>
  ClustType &GenericOrbitree<ClustType>::prototype(Index np, Index no) {
    return at(np).at(no).prototype;
  }

  //********************************************************************

  template<typename ClustType>
  const ClustType &GenericOrbitree<ClustType>::equiv(Index np, Index no, Index ne) const {
    return at(np).at(no).at(ne);
  }

  //********************************************************************

  template<typename ClustType>
  ClustType &GenericOrbitree<ClustType>::equiv(Index np, Index no, Index ne) {
    return at(np).at(no).at(ne);
  }

  //********************************************************************

  template<typename ClustType>
  Index GenericOrbitree<ClustType>::size(Index np) const {
    return at(np).size();
  }

  //********************************************************************

  template<typename ClustType>
  Index GenericOrbitree<ClustType>::size(Index np, Index no) const {
    return orbit(np, no).size();
  }

  //********************************************************************

  // Count number of basis functions at each orbit and sum result
  template<typename ClustType>
  Index GenericOrbitree<ClustType>::basis_set_size() const {
    Index result(0);
    for(Index np = 0; np < size(); np++) {
      for(Index no = 0; no < at(np).size(); no++) {
        result += prototype(np, no).clust_basis.size();
      }
    }
    return result;
  }

  //*******************************************************************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::resize(Index np) {
    Array<GenericOrbitBranch<ClustType> >::resize(np, GenericOrbitBranch<ClustType>(lattice));
  }

  //********************************************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::push_back(const GenericOrbitBranch<ClustType> &new_branch) {
    Array<GenericOrbitBranch<ClustType> >::push_back(new_branch);
    back().set_lattice(lattice, CART);
    return;
  }


  //********************************************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::push_back(const GenericOrbit<ClustType> &new_orbit) {
    //Add the orbit where it needs to go.
    for(Index np = 0; np < size(); np++) {
      if(at(np).num_sites() == new_orbit.prototype.size()) {
        at(np).push_back(new_orbit);
        return;
      }
    }

    push_back(GenericOrbitBranch<ClustType>(lattice));
    back().push_back(new_orbit);
  }

  //********************************************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::set_lattice(const Lattice &new_lat, COORD_TYPE mode) {
    for(Index nb = 0; nb < size(); nb++)
      at(nb).set_lattice(new_lat, mode);

    lattice = new_lat;

    for(Index nb = 0; nb < size(); nb++)
      at(nb).set_lattice(lattice, mode);

    //phenom_clust.set_lattice(lattice, mode);

  }
  //********************************************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::sort() {
    //Loops over outer array which is the outer array of the point, pair, etc clusters (np stands for cluster size)
    for(Index np = 0; np < size(); np++) {
      sort(np);
    }
    return;
  }

  //********************************************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::sort(Index np) { //(Line 4810 in clusters11.0.h)
    at(np).sort();
  }

  //********************************************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::generate_config_clust_bases() {
    for(Index i = 0; i < size(); i++) {
      for(Index j = 0; j < size(i); j++) {
        prototype(i, j).generate_config_clust_basis();
        for(Index k = 0; k < size(i, j); k++) {
          equiv(i, j, k).clust_basis = prototype(i, j).clust_basis;

          // next: critical step -- make sure that dof IDs are up to date in equivalent basis functions
          //std::cout << "Updating clust_basis of equiv:\n";

          // if symmetry need to consider the effect of equivalence_map symmetry on basis sets a later date
          // we may also need to permute the indices when updating dof IDs (but probably not)
          equiv(i, j, k).clust_basis.update_dof_IDs(prototype(i, j).nlist_inds(), equiv(i, j, k).nlist_inds());
        }
      }
    }
  }

  //********************************************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::generate_clust_bases(Index max_poly_order) {
    generate_clust_bases(Array<BasisSet const *>(), max_poly_order);
  }

  //********************************************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::generate_clust_bases(const Array<BasisSet const *> &global_args, Index max_poly_order) {
    for(Index i = 0; i < size(); i++) {
      for(Index j = 0; j < size(i); j++) {
        // Should this step be a method of Orbit?
        prototype(i, j).generate_clust_basis(global_args);
        for(Index k = 0; k < size(i, j); k++) {
          equiv(i, j, k).clust_basis = prototype(i, j).clust_basis;
          equiv(i, j, k).clust_basis.apply_sym(orbit(i, j).equivalence_map[k][0]);

          // next: critical step -- make sure that dof IDs are up to date in equivalent basis functions
          //std::cout << "Updating clust_basis of equiv:\n";

          // we may also need to permute the indices when updating dof IDs (but probably not)
          equiv(i, j, k).clust_basis.update_dof_IDs(prototype(i, j).nlist_inds(), equiv(i, j, k).nlist_inds());
        }
      }
    }
  }

  //********************************************************************
  template<typename ClustType>
  void GenericOrbitree<ClustType>::fill_discrete_bases_tensors() {
    for(Index i = 0; i < size(); i++) {
      for(Index j = 0; j < size(i); j++) {
        for(Index k = 0; k < size(i, j); k++) {
          at(i)[j][k].fill_discrete_basis_tensors();
        }
      }
    }
  }

  //********************************************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::collect_basis_info(const Structure &struc, const Coordinate &shift) {
    for(Index np = 0; np < size(); np++)
      for(Index no = 0; no < size(np); no++)
        orbit(np, no).collect_basis_info(struc, shift);
  }

  //********************************************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::collect_basis_info(const Structure &struc) {
    for(Index np = 0; np < size(); np++)
      for(Index no = 0; no < size(np); no++)
        orbit(np, no).collect_basis_info(struc);
  }


  //***********************************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::get_s2s_vec() {

    for(Index nb = 0; nb < size(); nb++) {
      for(Index no = 0; no < at(nb).size(); no++) {
        prototype(nb, no).get_s2s_vec();
        for(Index ne = 0; ne < orbit(nb, no).size(); ne++) {
          equiv(nb, no, ne).get_s2s_vec();
        }
      }
    }

  };

  //********************************************************************
  template<typename ClustType>
  Index GenericOrbitree<ClustType>::find(const ClustType &test_clust) const {
    Index i, j, ind;

    ind = 0;
    //Find the OrbitBranch of our current Orbitree - loop through number of OrbitBranches
    for(i = 0; i < size(); i++) {

      //If we're in an OrbitBranch with size of clusters equal to those in test_clust, try to find test_clust
      if(at(i).num_sites() == test_clust.size()) {

        //Loop through all j Orbits in i Orbitbranch:
        for(j = 0; j < at(i).size(); j++) {
          if(orbit(i, j).contains(test_clust))
            return ind + j; //Return the linear orbit index test_clust is found in
        }
      }
      ind += size(i);
    }
    return ind;
  }

  //*******************************

  // Find a cluster within a specified Orbitbranch.  Return the index of the Orbit
  // in which test_clust lives, relative to that specific OrbitBranch (i.e. the
  // first Orbit in this branch, regardless of its position in Orbitree, has an index of 0).
  template<typename ClustType>
  Index GenericOrbitree<ClustType>::find(const ClustType &test_clust, Index nb) const {
    Index i;

    //Loop through all i Orbits in Orbitbranch with index nb:
    for(i = 0; i < at(nb).size(); i++) {
      if(orbit(nb, i).contains(test_clust))
        return i; //Return the Orbit test_clust is found in
    }

    return i; //Return the total number of Orbits in Orbitbrach nb if test_clust isn't found
  }

  //********************************************************************

  template<typename ClustType>
  Index GenericOrbitree<ClustType>::find(const GenericOrbit<ClustType> &test_orbit) const {
    Index i, j, ind;

    ind = 0;

    //Find the OrbitBranch of our current Orbitree - loop through number of OrbitBranches
    for(i = 0; i < size(); i++) {

      //If we're in an OrbitBranch with np equal to the size of clusters in the prototype cluster of test_orbit, try to find test_orbit
      if(at(i).num_sites() == test_orbit.prototype.size()) {

        //Loop through all j Orbits in i Orbitbranch:
        for(j = 0; j < at(i).size(); j++) {
          if(at(i).orbit(j) == test_orbit) //at(i) finds OrbitBranch, and then use OrbitBranch's orbit(no) fxn
            return ind + j;
        }
        ind += size(i);
      }
    }
    return ind; //Return the total number of Orbits if test_clust isn't found
  }

  //*******************************

  template<typename ClustType>
  bool GenericOrbitree<ClustType>::contains(const ClustType &test_clust) {
    for(Index i = 0; i < size(); i++) {
      if(find(test_clust, i) < at(i).size())
        return true;
    }
    return false;

  }
  /*
    template<typename ClustType>
    bool GenericOrbitree<ClustType>::tmp_contains(const ClustType &test_clust){
    if (at(test_clust.size()).size()==0) return false;

    for(Index i = 0; i < size(); i++)
    {
    for (Index j = 0; j < orbit(test_clust.size(),i).size(); j++){
    if (orbit(test_clust.size(),i)[j].is_equivalent(test_clust))
    {
    return true;
    }

    }
    }
    return false;
    }
  */

  //*******************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::get_index() const { //(Line 4826 in clusters11.0.h)
    Index count = 0;
    index.clear();
    index_to_row.clear();
    index_to_column.clear();
    index.resize(size());

    /*Loop over the clusters with the same number of points (np),
     * then inside that loop, loop over all those clusters (nc)
     */

    //Note: Should we make sure to sort the GenericOrbitree<ClustType> first so we
    //don't have to switch any index values around?


    for(Index np = 0; np < size(); np++) { //Start with point clusters (np=1)
      for(Index no = 0; no < size(np); no++) { //no = index of inner array (over orbits)
        orbit(np, no).set_index(count);
        index[np].push_back(count);
        index_to_row.push_back(np);
        index_to_column.push_back(no);
        count++;


        /** Used to be
              count ++;
              index[np].push_back(count);
              index_to_row.push_back(np);
              index_to_column.push_back(no);
        **/
      }
    }

    Norbits = count; // Brian (index.back().back() doesn't work if the last orbitBranch has no orbits
    return;
  }

  //************************************************************
  /**
   * Constructs an orbitree given a primitive Structure.
   *
   * First finds the asymmetric unit, from which it constructs
   * all the pair clusters within a radii specified in max_length.
   * From the pair clusters, it constructs triplet clusters,
   * then from the triplet clusters it builds the quadruplet
   * clusters and so on so forth.
   */
  //************************************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::generate_orbitree(const Structure &prim, bool verbose) {

    if(prim.factor_group().size() == 0) {
      std::cerr << "WARNING: In Orbitree::generate_orbitree, prim's factor_group is empty. It  must at least have one element (identity).\n";
      assert(0);
    }

    Index i, j, np, no;
    Vector3<int> dim; //size of gridstruc
    double dist, min_dist;
    Array<typename ClustType::WhichCoordType> basis, gridstruc;
    std::string clean(80, ' ');

    lattice = prim.lattice();
    Coordinate lat_point(lattice);


    //first get the basis sites

    if(verbose) std::cout << "* Finding Basis:\n";
    // make the basis from which the sites for the local clusters are to be picked
    for(i = 0; i < prim.basis.size(); i++) {
      if(prim.basis[i].site_occupant().size() >= min_num_components) {
        basis.push_back(prim.basis[i]);
        basis.back().set_lattice(lattice);
      }
    }

    double max_radius = max_length.max();
    dim = lattice.enclose_sphere(max_radius);
    Counter<Vector3<int> > grid_count(-dim, dim, Vector3<int>(1));
    if(verbose) std::cout << "dim is " << dim << '\n';
    if(verbose) std::cout << "\n Finding Grid_struc:\n";
    do {
      lat_point(FRAC) = grid_count();

      for(i = 0; i < basis.size(); i++) {
        typename ClustType::WhichCoordType tatom(basis[i] + lat_point);
        //get distance to closest basis site in the unit cell at the origin

        min_dist = 1e20;
        for(j = 0; j < basis.size(); j++) {
          dist = tatom.dist(basis[j]);
          if(dist < min_dist)
            min_dist = dist;
        }
        if(min_dist < max_radius) {
          gridstruc.push_back(tatom);
        }
      }
    }
    while(++grid_count);

    if(verbose) std::cout << "Finished finding grid_struc\n";
    // Get orbitree ready to hold clusters.
    if(size())
      std::cerr << "WARNING:  Orbitree is about to be overwritten! Execution will continue normally, but side effects may occur.\n";

    //Size outer arry to have sufficient space to create orbitree
    resize(max_num_sites + 1);


    // Add orbit corresponding to empty cluster
    at(0).push_back(GenericOrbit<ClustType>(ClustType(lattice)));
    at(0).back().get_equivalent(prim.factor_group());
    at(0).back().get_cluster_symmetry();


    //for each cluster of the previous size, add points from gridstruc
    //   - see if the new cluster satisfies the size requirements
    //   - see if it is new
    //   - generate all its equivalents

    if(verbose) std::cout << "About to begin construction of non-empty clusters\n";
    else std::cout << clean << '\r' << "About to begin construction of non-empty clusters\r" << std::flush;
    for(np = 1; np <= max_num_sites; np++) {
      if(verbose) std::cout << "Doing np = " << np << '\n';
      else std::cout << clean << '\r' << "Doing np = " << np << '\r' << std::flush;

      if(size(np - 1) == 0) {
        std::cerr << "CRITICAL ERROR: Orbitree::generate_orbitree is unable to enumerate clusters of size " << np << '\n';
        get_index();
        print(std::cout);
        exit(1);
      }

      for(no = 0; no < size(np - 1); no++) {
        if(verbose) std::cout << "Adding sites to orbit " << no << " of " << size(np - 1) << "\n";
        else std::cout << clean << '\r' << "Adding sites to orbit " << no << " of " << size(np - 1) << " in branch " << np - 1 << '\r' << std::flush;

        ClustType tclust(lattice);
        for(i = 0; i < orbit(np - 1, no).prototype.size(); i++)
          tclust.push_back(orbit(np - 1, no).prototype[i]);

        for(i = 0; i < gridstruc.size(); i++) {

          //if(orbit(np-1,no).prototype.contains(gridstruc[i])) continue;

          //tclust = orbit(np - 1, no).prototype;
          tclust.push_back(gridstruc[i]);

          tclust.within();
          tclust.calc_properties();

          if(np == 1 && !contains(tclust)) {
            at(np).push_back(GenericOrbit<ClustType>(tclust));
            at(np).back().get_equivalent(prim.factor_group());
            at(np).back().get_cluster_symmetry();
            //at(np).back().get_tensor_basis(np); //temporarily works as rank = np
          }
          else if(tclust.max_length() < max_length[np] && tclust.min_length() > min_length && !contains(tclust)) {
            at(np).push_back(GenericOrbit<ClustType>(tclust));

            at(np).back().get_equivalent(prim.factor_group());
            at(np).back().get_cluster_symmetry();
            //at(np).back().get_tensor_basis(np); //temporarily works as rank = np
          }
          tclust.pop_back();
        }
      }
    }

    if(!verbose) std::cout << clean << '\r' << std::flush;

    sort();
    get_index();
    return;
  }

  //****************************************************************************************************************
  /**
   * Constructs an orbitree given a primitive Structure and the max number of clusters that the user wants
   *
   * First generates a ballpark estimate of the minimum supercell size. Assuming the absence of symmetry,
   * given an nxnxn supercell, the number of clusters that may be generated is given by (the number of ways we can
   * choose pair clusters from the primitive structure)+
   * (the number of ways pair clusters may be chosen from 2 cells)(the number of ways we can choose 2 primitive
   * cells in the superlattice). This formula is inverted to give the number of supercells needed to generate a
   * given number of clusters.
   * The grid is built with that ballpark estimate, pair clusters are generated within that grid. If enough pair
   * clusters havent been generated, a bigger grid is made. This is continued till the maxClust criterion is
   * satisfied.
   */
  //*****************************************************************************************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::generate_orbitree(const Structure &prim, const int maxClust) {
    Index i, j, np, no;
    int numClust, ctr;
    Vector3<int> dim; //size of gridstruc
    double maxClustLength;
    Array<typename ClustType::WhichCoordType> basis;
    Array<typename ClustType::WhichCoordType> gridstruc;
    lattice = prim.lattice();
    Coordinate lat_point(lattice);

    //first get the basis sites
    std::cout << "** Finding Basis:\n";
    // make the basis from which the sites for the local clusters are to be picked

    for(i = 0; i < prim.basis.size(); i++) {
      if(prim.basis[i].site_occupant().size() >= min_num_components) {
        basis.push_back(prim.basis[i]);
        basis.back().set_lattice(lattice);
      }
    }

    //Get a ballpark estimate of the number of cells required
    int cellSize = ceil(pow(((maxClust - (basis.size() * (basis.size() - 1)) / 2.0) / (basis.size() * (2 * basis.size() - 1.0)) + 1.0), 1.0 / 3.0));

    //Build initial grid
    double max_radius = (cellSize) * prim.lattice().lengths.min();
    ctr = 0;
    dim = lattice.enclose_sphere(max_radius);
    gridstruc = lattice.gridstruc_build(max_radius, double(0), basis, lat_point);
    double min_radius = max_radius;
    std::cout << dim << " " << cellSize << "\n";


    // Get orbitree ready to hold clusters.

    if(size())
      std::cerr << "WARNING:  Orbitree is about to be overwritten! Execution will continue normally, but side effects may occur.\n";

    //Size outer arry to have sufficient space to create orbitree
    resize(max_num_sites + 1);


    // Add orbit corresponding to empty cluster
    at(0).push_back(GenericOrbit<ClustType>(ClustType(lattice)));
    at(0).back().get_equivalent(prim.factor_group());
    at(0).back().get_cluster_symmetry();


    //for each cluster of the previous size, add points from gridstruc
    //   - see if the new cluster satisfies the size requirements
    //   - see if it is new
    //   - generate all its equivalents

    std::cout << "About to begin construction of non-empty clusters\n";

    for(np = 1; np <= max_num_sites; np++) {
      std::cout << "Doing np = " << np << '\n';

      if(size(np - 1) == 0) {
        std::cerr << "CRITICAL ERROR: Orbitree::generate_orbitree is unable to enumerate clusters of size " << np << '\n';
        get_index();
        print(std::cout);
        exit(1);
      }
      // If the pair clusters are being built, we need to keep track of the number of clusters that have been found so far
      if(np == 2) {
        maxClustLength = 0;
        i = 0;
        numClust = 0;
        do {
          for(no = 0; no < size(np - 1); no++) {
            std::cout << "Adding sites to orbit " << no << " of " << size(np - 1) << "\n";

            ClustType tclust(lattice);
            for(j = 0; j < orbit(np - 1, no).prototype.size(); j++)
              tclust.push_back(orbit(np - 1, no).prototype[j]);

            for(; i < gridstruc.size(); i++) {

              //tclust = orbit(np - 1, no).prototype;
              tclust.push_back(gridstruc[i]);

              tclust.within();
              tclust.calc_properties();

              if(tclust.min_length() > min_length && !contains(tclust)) {
                std::cout << "Found a new cluster.... adding to Orbitree!\n";
                std::cout << "The minimum length is " << min_length << "\n";
                at(np).push_back(GenericOrbit<ClustType>(tclust));
                at(np).back().get_equivalent(prim.factor_group());
                at(np).back().get_cluster_symmetry();
                numClust = numClust + 1;
                if(maxClustLength < tclust.max_length()) {
                  maxClustLength = tclust.max_length();
                }
              }
              tclust.pop_back();
            }
          }
          if(numClust < maxClust) {
            //Building a bigger grid!
            dim = dim + 1;
            ctr = ctr + 1;
            std::cout << "Max Radius" << max_radius << "\n";
            max_radius = (cellSize + ctr) * prim.lattice().lengths.min();
            std::cout << "Max Radius" << max_radius << "\n";
            gridstruc.append(lattice.gridstruc_build(max_radius, min_radius, basis, lat_point));
            min_radius = max_radius;
            std::cout << "Built a bigger grid!\n" << gridstruc.size() << "\n";
            //End of building a bigger grid
          }
          std::cout << "Couldnt find enough points " << numClust << " " << i << " " << maxClust << "\n";
        }
        while(numClust < maxClust);
        sort(np);

        //Once more clusters than required are found, only the first 'maxClust' number of clusters are kept, which one to keep and which one to remove is decided based on the length of the cluster
        double cutOff = orbit(np, maxClust - 1).max_length();
        std::cout << cutOff << " Cutoff!\n";
        for(i = maxClust; i < size(np); i++) {
          if(orbit(np, i).max_length() > cutOff) {
            std::cout << orbit(np, i).max_length() << " " << orbit(np, i).min_length() << "Deleted this\n";
            at(np).remove(i);
            i = i - 1;
          }
        }

        continue;
      }

      max_length[np] = maxClustLength;
      for(no = 0; no < size(np - 1); no++) {
        std::cout << "Adding sites to orbit " << no << " of " << size(np - 1) << "\n";

        ClustType tclust(lattice);
        for(i = 0; i < orbit(np - 1, no).prototype.size(); i++)
          tclust.push_back(orbit(np - 1, no).prototype[i]);

        for(i = 0; i < gridstruc.size(); i++) {

          //if(orbit(np-1,no).prototype.contains(gridstruc[i])) continue;

          //tclust = orbit(np - 1, no).prototype;
          tclust.push_back(gridstruc[i]);

#ifdef DEBUG
          std::cout << "tclust is \n"
                    << tclust << "\n";
#endif //DEBUG

          tclust.within();
          tclust.calc_properties();

#ifdef DEBUG
          std::cout << "tclust is \n"
                    << tclust << "\n";
#endif //DEBUG

          if(np == 1 && !contains(tclust)) {
            at(np).push_back(GenericOrbit<ClustType>(tclust));
            at(np).back().get_equivalent(prim.factor_group());
            at(np).back().get_cluster_symmetry();

          }
          else if(tclust.max_length() < max_length[np] && tclust.min_length() > min_length && !contains(tclust)) {
            std::cout << "Found a new cluster.... adding to Orbitree!\n";
            std::cout << "The minimum length is " << min_length << "\n";
            at(np).push_back(GenericOrbit<ClustType>(tclust));

#ifdef DEBUG
            std::cout << "The tclust we pushed back is \n"
                      << tclust << "\n";
#endif //DEBUG
            at(np).back().get_equivalent(prim.factor_group());
            at(np).back().get_cluster_symmetry();
          }
          tclust.pop_back();
        }
      }
    }
    sort();
    get_index();
    return;
  }

  //****************************************************************************************************************
  /** Generates all orbitrees upto the nth nearest neighbour as specified in the input array maxNeighbour.
   */
  //*****************************************************************************************************************
  template<typename ClustType>
  void GenericOrbitree<ClustType>::generate_orbitree_neighbour(const Structure &prim, const Array<int> maxNeighbour) {
    Index i, j, np, no;
    int numClust, ctr;
    Vector3<int> dim; //size of gridstruc
    double maxClustLength;
    Array<typename ClustType::WhichCoordType> basis;
    Array<typename ClustType::WhichCoordType> gridstruc;
    Array<double> neighbour_lengths;
    lattice = prim.lattice();
    Coordinate lat_point(lattice);

    //first get the basis sites
    std::cout << "*** Finding Basis:\n";
    // make the basis from which the sites for the local clusters are to be picked

    for(i = 0; i < prim.basis.size(); i++) {
      if(prim.basis[i].site_occupant().size() >= min_num_components) {
        basis.push_back(prim.basis[i]);
        basis.back().set_lattice(lattice);
      }
    }
    //Get a ballpark estimate of the number of cells required
    int cellSize = 1;

    //Build initial grid
    double max_radius = (cellSize) * prim.lattice().lengths.min();
    ctr = 0;
    dim = lattice.enclose_sphere(max_radius);
    gridstruc = lattice.gridstruc_build(max_radius, double(0), basis, lat_point);
    double min_radius = max_radius;
    std::cout << dim << " " << cellSize << "\n";

    // Get orbitree ready to hold clusters.

    if(size())
      std::cerr << "WARNING:  Orbitree is about to be overwritten! Execution will continue normally, but side effects may occur.\n";

    //Size outer arry to have sufficient space to create orbitree
    resize(max_num_sites + 1);

    // Add orbit corresponding to empty cluster
    at(0).push_back(GenericOrbit<ClustType>(ClustType(lattice)));
    at(0).back().get_equivalent(prim.factor_group());
    at(0).back().get_cluster_symmetry();

    //for each cluster of the previous size, add points from gridstruc
    //   - see if the new cluster satisfies the size requirements
    //   - see if it is new
    //   - generate all its equivalents

    std::cout << "About to begin construction of non-empty clusters\n";

    for(np = 1; np <= max_num_sites; np++) {
      std::cout << "Doing np = " << np << '\n';

      if(size(np - 1) == 0) {
        std::cerr << "CRITICAL ERROR: Orbitree::generate_orbitree is unable to enumerate clusters of size " << np << '\n';
        get_index();
        print(std::cout);
        exit(1);
      }
      // If the pair clusters are being built, we need to keep track of the number of clusters that have been found so far
      if(np == 2) {
        maxClustLength = 0;
        i = 0;
        numClust = 0;
        do {
          for(no = 0; no < size(np - 1); no++) {
            std::cout << "Adding sites to orbit " << no << " of " << size(np - 1) << "\n";

            ClustType tclust(lattice);
            for(j = 0; j < orbit(np - 1, no).prototype.size(); j++)
              tclust.push_back(orbit(np - 1, no).prototype[j]);


            for(; i < gridstruc.size(); i++) {

              //tclust = orbit(np - 1, no).prototype;
              tclust.push_back(gridstruc[i]);

              tclust.within();
              tclust.calc_properties();

              if(tclust.min_length() > min_length && !contains(tclust)) {
                at(np).push_back(GenericOrbit<ClustType>(tclust));
                at(np).back().get_equivalent(prim.factor_group());
                at(np).back().get_cluster_symmetry();
                if(maxClustLength < tclust.max_length()) {
                  maxClustLength = tclust.max_length();
                }
              }

              tclust.pop_back();
            }
          }

          sort(np);
          numClust = 1;
          for(Index j = 1; j < size(np); j++) {
            if(orbit(np, j).max_length() == orbit(np, (j - 1)).max_length()) {
              continue;
            }
            else {
              numClust = numClust + 1;
            }
          }

          if(numClust < maxNeighbour[0]) {
            //Building a bigger grid!
            dim = dim + 1;
            ctr = ctr + 1;
            max_radius = (cellSize + ctr) * prim.lattice().lengths.min();
            gridstruc.append(lattice.gridstruc_build(max_radius, min_radius, basis, lat_point));
            min_radius = max_radius;
            //End of building a bigger grid
          }
        }
        while(numClust < maxNeighbour[0]);
        sort(np);
        //Once more clusters than required are found, only the first 'maxNeighbour' number of clusters are kept, which one to keep and which one to remove is decided based on the length of the cluster
        int cutOff = 1;
        neighbour_lengths.push_back(orbit(np, 0).max_length());
        for(i = 1; i < size(np); i++) {
          if(orbit(np, i).max_length() == orbit(np, (i - 1)).max_length()) {
            continue;
          }
          else {
            neighbour_lengths.push_back(orbit(np, i).max_length());
            cutOff = cutOff + 1;
            if(cutOff <= maxNeighbour[0]) {
              continue;
            }
            else {
              at(np).remove(i);
              i = i - 1;
            }
          }
        }
        continue;
      }

      std::cout << neighbour_lengths.size() << "\n";
      if(np != 1) {
        max_length[np] = neighbour_lengths[(maxNeighbour[np - 2] - 1)];
        std::cout << "Looking at np=" << np << "\n";
      }
      for(no = 0; no < size(np - 1); no++) {
        if(np != 1 && (orbit(np - 1, no).max_length() > max_length[np] && !almost_zero((orbit(np - 1, no).max_length() - max_length[np])))) {
          continue;
        }
        std::cout << "Adding sites to orbit " << no << " of " << size(np - 1) << "\n";

        ClustType tclust(lattice);
        for(i = 0; i < orbit(np - 1, no).prototype.size(); i++)
          tclust.push_back(orbit(np - 1, no).prototype[i]);


        for(i = 0; i < gridstruc.size(); i++) {

          //if(orbit(np-1,no).prototype.contains(gridstruc[i])) continue;

          //tclust = orbit(np - 1, no).prototype;
          tclust.push_back(gridstruc[i]);

          tclust.within();
          tclust.calc_properties();

          if(np == 1 && !contains(tclust)) {
            at(np).push_back(GenericOrbit<ClustType>(tclust));
            at(np).back().get_equivalent(prim.factor_group());
            at(np).back().get_cluster_symmetry();

          }
          else if((tclust.max_length() < max_length[np] || almost_zero(tclust.max_length() - max_length[np]))  && tclust.min_length() > min_length && !contains(tclust)) {
            at(np).push_back(GenericOrbit<ClustType>(tclust));
            at(np).back().get_equivalent(prim.factor_group());
            at(np).back().get_cluster_symmetry();
          }
          tclust.pop_back();
        }
      }
    }
    sort();
    get_index();
    return;
  }

  //****************************************************************************************************************
  /**
   * Constructs an orbitree of decorated clusters, using the prototypes of an already constructed undecorated Orbitree
   *     Uses the 'symgroup' and periodicity type 'ptype' to generate equivalent decorated clusters
  *
  *     Generates decorations with at least one site in cluster different from the background of the prim
  */
  //*****************************************************************************************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::generate_decorated_orbitree(const GenericOrbitree<ClustType> &in_tree, const SymGroup &symgroup, PERIODICITY_TYPE ptype, bool full_decor) {
    //std::cout << "begin generate_decorated_orbitree" << std::endl;

    PERIODICITY_MODE P(ptype);

    // copy in_tree to *this
    *this = in_tree;

    resize(max_num_sites + 1);

    // add empty cluster
    at(0).push_back(GenericOrbit<ClustType>(ClustType(lattice)));
    at(0).back().get_equivalent(symgroup);
    at(0).back().get_cluster_symmetry();

    Index np, no, i;
    //Array<Array<int> > full_decor_map;
    Array<Array<int> > decor_map;

    // loop over OrbitBranches
    for(np = 1; np < in_tree.size(); np++) {

      // loop over Orbits in Orbitbranch
      for(no = 0; no < in_tree.size(np); no++) {

        //full_decor_map = in_tree.prototype(np, no).get_full_decor_map();
        if(full_decor)
          decor_map = in_tree.prototype(np, no).get_full_decor_map();
        else
          decor_map = in_tree.prototype(np, no).get_decor_map();

        // add a new Orbit for each unique decoration
        // // exclude full_decor_map[0], since this is all background
        // for(i = 1; i < full_decor_map.size(); i++)
        for(i = 0; i < decor_map.size(); i++) {

          // decorate prototype
          ClustType tclust(in_tree.prototype(np, no));
          //tclust.decorate(full_decor_map[i]);
          tclust.decorate(decor_map[i]);

          // add orbit
          at(np).push_back(GenericOrbit<ClustType>(tclust));
          at(np).back().get_equivalent(symgroup);
          at(np).back().get_cluster_symmetry();

        } // unique decorations

      } // orbits

    } // orbitbranches

    sort();
    get_index();
    set_lattice(lattice, COORD_MODE::CHECK());
    //std::cout << "finish generate_decorated_orbitree" << std::endl;

  };

  //****************************************************************************************************************
  /**
   * Constructs an orbitree of HopClusters, using the prototypes of an already constructed undecorated Orbitree
   *
  *		ClustType must be HopCluster :: Make a derived HopOrbitree???
  *
  *	Formula:
  *		Find unique decorations of each cluster in in_tree.
  *			Check all permutations of the decorations, find prototype HopClusters
  *			Generate HopCluster.clust_group (HopGroup), necessary for the HopCluster local orbitree
  *		For each prototype HopCluster,
  *			use the prim.factor_group() to generate equivalents on translated clusters
  *
  */
  //*****************************************************************************************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::generate_hop_orbitree(const GenericOrbitree<SiteCluster> &in_tree, const Structure &prim) {
    //std::cout << "begin generate_hop_orbitree" << std::endl;
    PERIODICITY_MODE P(PERIODIC);

    /*
        {
            std::ofstream outfile;
            std::stringstream ss;
            ss << "FACTORGROUP";
            outfile.open(ss.str().c_str());

            outfile << "FACTORGROUP:" << std::endl;
            prim.factor_group().print(outfile, COORD_DEFAULT);


            outfile.close();
        }
    */


    // copy in_tree to *this
    this->clear();
    this->max_num_sites = in_tree.max_num_sites;

    resize(max_num_sites + 1);

    Index np, no, nd, ii;
    Array<Array<int> > full_decor_map;
    std::string clean(80, ' ');

    // loop over OrbitBranches
    //   for now, assume no single site hops
    //   (examples of single site hop would be orientation flip, charge state change)
    for(np = 2; np < in_tree.size(); np++) {

      //std::cout << "OrbitBranch " << np << std::endl;
      // loop over Orbits in Orbitbranch
      for(no = 0; no < in_tree.size(np); no++) {
        std::cout << clean << '\r' << "Generate HopOrbitree branch: " << np << " orbit: " << no << "\r" << std::flush;
        full_decor_map = in_tree.prototype(np, no).get_full_decor_map();

        // consider each unique decoration
        // include full_decor_map[0], since all background might be an allowed hop
        for(nd = 0; nd < full_decor_map.size(); nd++) {
          //std::cout << "    Decor " << nd << ": " <<  full_decor_map[nd] << std::endl;

          // create decorated SiteCluster (start point)
          SiteCluster sclust(in_tree.prototype(np, no));
          sclust.decorate(full_decor_map[nd]);

          // might filter decorations here.
          //   Right now, filter to check that there is only 1 Va
          int Va_count = 0;
          for(ii = 0; ii < sclust.size(); ii++) {
            if(sclust[ii].is_vacant())
              Va_count++;
          }

          if(Va_count != 1)
            continue;

          // write undecorated site clust_group
          /*
                    for(ne = 0; ne < in_tree.orbit(np, no).size(); ne++)
                    {
                        std::ofstream outfile;
                        std::stringstream ss;
                        ss << "SITECLUST." << np << "." << no << "." << ne;
                        outfile.open(ss.str().c_str());

                        outfile << "SITECLUSTER Branch:" << np << "  Orbit: " << no << "  Equiv: " << ne << std::endl;
                        in_tree.orbit(np, no).at(ne).print_sites(outfile, 2, '\n');
                        outfile << '\n';
                        outfile << "CLUSTGROUP:" << std::endl;
                        in_tree.orbit(np, no).at(ne).clust_group.print(outfile, COORD_DEFAULT);


                        outfile.close();
                    }
          */

          // initialize forward_permuation
          Array<Index> perm;
          for(ii = 0; ii < np; ii++) {
            perm.push_back(ii);
          }

          // loop over all forward_permutations
          do {
            //std::cout << "      perm: " << perm << std::endl;

            // check that all atoms move (avoid subcluster hops)
            bool perm_ok = true;
            for(ii = 0; ii < np; ii++)
              if(perm[ii] == ii) {
                perm_ok = false;
                break;
              }

            if(!perm_ok)
              continue;

            // check that the permutation is allowed based on the site domains
            if(!ClustType::allowed(sclust, perm))
              continue;


            // if all atoms move, create a test HopCluster
            ClustType tclust(sclust, perm);
            //tclust.print_decorated_sites(std::cout, 2, '\n');

            // check if 'tclust' is symmetrically equivalent to an existing prototype HopCluster
            if(!contains(tclust)) {
              //std::cout << "        Add orbit!" << std::endl;
              // if 'tclust' has not already been found
              at(np).push_back(GenericOrbit<ClustType>(tclust));
              //std::cout << "        get_equivalent()" << std::endl;
              at(np).back().get_equivalent(prim.factor_group());
              //std::cout << "        get_cluster_symmetry()" << std::endl;
              at(np).back().get_cluster_symmetry();

              // // write HopClusters & HopGroup to file
              // for(ne = 0; ne < at(np).back().size(); ne++)
              // {
              //    std::ofstream outfile;
              //    std::stringstream ss;
              //    ss << "HOP." << np << "." << at(np).size() - 1 << "." << ne;
              //    outfile.open(ss.str().c_str());
              //
              //    outfile << "HOP Branch:" << np << "  Orbit: " << at(np).size() - 1 << "  Equiv: " << ne << std::endl;
              //    at(np).back().at(ne).print_decorated_sites(outfile, 2, '\n');
              //    outfile << '\n';
              //    outfile << "HOPGROUP:" << std::endl;
              //    at(np).back().at(ne).clust_group.print(outfile, COORD_DEFAULT);
              //
              //    outfile.close();
              // }

              //std::cout << "        get_tensor_basis()" << std::endl;
              //at(np).back().get_tensor_basis(np); //temporarily works as rank = np
              //std::cout << "        Finish adding orbit" << std::endl;

            }

          }
          while(perm.next_permute());

        } // unique decorations

      } // orbits

    } // orbitbranches

    std::cout << clean << '\r' << std::flush;

    sort();
    get_index();
    set_lattice(lattice, COORD_MODE::CHECK());
    return;
  };


  //****************************************************************************************************************
  /**
   * Constructs an orbitree of HopClusters, using the prototypes read from a file
   *
  *		ClustType must be HopCluster
  *
  *	Formula:
  *		Read a prototype HopCluster from 'filename'
  *		For each prototype HopCluster,
  *			use the prim.factor_group() to generate equivalents on translated clusters
  *			(need to use the SiteCluster.clust_group to generate equivalents on that cluster first?)
  *
  */
  //*****************************************************************************************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::generate_orbitree_from_proto_file(std::string filename, const SymGroup &sym_group, PERIODICITY_TYPE ptype) {
    //std::cout << "begin generate_orbitree_from_proto_file" << std::endl;
    PERIODICITY_MODE P(ptype);
    this->clear();

    BP::BP_Vec<Index> n_equiv;
    BP::BP_Vec<std::string> s_list;
    Array<ClustType> prototype_list;
    ClustType tclust(lattice);
    Index i, j;

    /// Not sure how to treat this
    bool SD_is_on = false;


    // Read the prototype file
    //   - as going check for max number of sites in a cluster:
    BP::BP_Parse file(filename);
    //std::cout << "begin reading file: " << filename << std::endl;

    COORD_MODE C(CART);

    this->max_num_sites = 0;
    //BP_Vec needs unsigned long int
    ulint index;
    while(!file.eof()) {
      s_list = file.getline_string();
      if(s_list.size() == 0)
        continue;

      if(s_list[0] == "COORD_MODE") {

        // look for line "COORD_MODE = Direct"
        if(s_list[2][0] == 'D' || s_list[2][0] == 'd') {
          //std::cout << "  set COORD_MODE = Direct" << std::endl;
          C.set(FRAC);
        }
        // look for line "COORD_MODE = Cartesian"
        else if(s_list[2][0] == 'C' || s_list[2][0] == 'c') {
          //std::cout << "  set COORD_MODE = Cartesian" << std::endl;
          C.set(CART);
        }
        else {
          std::cerr << "Error in GenericOrbitree<ClustType>::generate_orbitree_from_proto_file()." << std::endl
                    << "  COORD_MODE not understood: " << s_list << std::endl;
          exit(1);
        }
      }
      else if(s_list.find_first("Points:", index)) {
        //std::cout << "  read Points" << std::endl;
        Index pts = BP::stoi(s_list[index + 1]);
        if(pts > this->max_num_sites)
          this->max_num_sites = pts;

        //std::cout << "  pts: " << pts << std::endl;
        //std::cout << "  read line" << std::endl;

        // Read line "Prototype of X Equivalent Clusters in Orbit Y"
        s_list = file.getline_string();

        // Store the number of equivalent clusters expected
        //std::cout << "  add n_equiv: " << BP::stoi(s_list[2]) << std::endl;
        n_equiv.add(BP::stoi(s_list[2]));

        // Read the cluster
        //std::cout << "  read cluster" << std::endl;
        tclust.read(file.get_istream(), pts, C.check(), SD_is_on);

        // Add to the list of prototypes
        tclust.calc_properties();
        prototype_list.push_back(tclust);
      }
    }

    //std::cout << " max_num_sites = " << this->max_num_sites << std::endl;
    resize(max_num_sites + 1);


    for(i = 0; i < prototype_list.size(); i++) {
      //std::cout << "        Add orbit!" << std::endl;
      at(prototype_list[i].size()).push_back(GenericOrbit<ClustType>(prototype_list[i]));
      //std::cout << "        get_equivalent()" << std::endl;
      at(prototype_list[i].size()).back().get_equivalent(sym_group);
      //std::cout << "        get_cluster_symmetry()" << std::endl;
      at(prototype_list[i].size()).back().get_cluster_symmetry();

      if(n_equiv[i] != at(prototype_list[i].size()).back().size()) {
        std::cerr << "Error in Orbitree::generate_orbitree_from_proto_file()." << std::endl
                  << "  Expected " << n_equiv[i] << " equivalents, but only generated " << at(prototype_list[i].size()).back().size() << " equivalents." << std::endl
                  << "  Prototype: " << std::endl;
        prototype_list[i].print_sites(std::cerr, 6, '\n');
        std::cerr << "\n" << std::endl;

        for(j = 0; j < at(prototype_list[i].size()).back().size(); j++) {
          std::cerr << "  Equivalent " << j << std::endl;
          at(prototype_list[i].size()).back().at(j).print_sites(std::cerr, 6, '\n');
        }
        exit(1);
      }
    }

    set_lattice(lattice, COORD_MODE::CHECK());
    sort();
    get_index();
    return;
  };


  //********************************************************************
  /**
   * Generates orbitree of all unique clusters within a supercell
   * If two clusters of the same point-size overlap, it keeps the one with shorter length
   */
  //********************************************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::generate_in_cell(const Structure &prim, const Lattice &cell, int num_images) {


    Index i, j, np, no;

    Array<typename ClustType::WhichCoordType> gridstruc;
    Lattice reduced_cell(cell.get_reduced_cell());

    lattice = prim.lattice();

    PrimGrid prim_grid(lattice, reduced_cell);



    // make the grid from which the sites for the clusters are to be picked

    Counter<Vector3<int> > shift_count(Vector3<int> (0, 0, 0), Vector3<int> (1, 1, 1), Vector3<int> (1, 1, 1));
    Coordinate shift(reduced_cell);

    do {
      for(i = 0; i < 3; i++)
        shift.at(i, FRAC) = shift_count[i];

      for(i = 0; i < prim.basis.size(); i++) {
        if(prim.basis[i].site_occupant().size() < min_num_components) continue;

        for(j = 0; j < prim_grid.size(); j++) {
          gridstruc.push_back(prim.basis[i] + prim_grid.coord(j, PRIM));
          //          gridstruc.back().set_lattice(cell);
          gridstruc.back().set_lattice(reduced_cell);
          gridstruc.back().within();
          gridstruc.back() -= shift;
          gridstruc.back().set_lattice(lattice);
        }
      }

    }
    while(++shift_count);


    // Get orbitree ready to hold clusters.
    if(size())
      std::cerr << "WARNING:  Orbitree is about to be overwritten! Execution will continue normally, but side effects may occur.\n";

    //Size outer array to have sufficient space to create orbitree
    resize(max_num_sites + 1);

    // Add orbit corresponding to empty cluster
    at(0).push_back(GenericOrbit<ClustType>(ClustType(lattice)));
    at(0).back().get_equivalent(prim.factor_group());
    at(0).back().get_cluster_symmetry();


    //for each cluster of the previous size, add points from gridstruc
    //   - see if the new cluster satisfies the size requirements
    //   - see if it is new
    //   - generate all its equivalents


    std::cout << "About to begin construction of non-empty clusters\n";
    for(np = 1; np <= max_num_sites; np++) {
      std::cout << "Doing np = " << np << '\n';

      if(size(np - 1) == 0) {
        std::cerr << "WARNING: Orbitree::generate_orbitree is unable to enumerate clusters of size " << np - 1 << " or larger.\n" ;
        get_index();
        sort();
        return;
      }

      for(no = 0; no < size(np - 1); no++) {
        std::cout << "Adding sites to orbit " << no << " of " << size(np - 1) << "\n";

        ClustType tclust(lattice);
        for(i = 0; i < orbit(np - 1, no).prototype.size(); i++)
          tclust.push_back(orbit(np - 1, no).prototype[i]);


        for(i = 0; i < gridstruc.size(); i++) {

          //if(orbit(np - 1, no).prototype.contains(gridstruc[i])) continue;

          //tclust = orbit(np - 1, no).prototype;
          tclust.push_back(gridstruc[i]);

          if(tclust.image_check(reduced_cell, num_images)) continue;
          tclust.within();
          tclust.calc_properties();
          if(!contains(tclust) && tclust.min_length() > min_length) {
            at(np).push_back(GenericOrbit<ClustType>(tclust));
            at(np).back().get_equivalent(prim.factor_group());
            at(np).back().get_cluster_symmetry();
            //at(np).back().get_tensor_basis(np); //temporarily works as rank = np
          }

          tclust.pop_back();
        }
      }
    }
    sort();
    get_index();
    return;

  }

  //********************************************************************
  /**
   * Gets the hierarchy of the clusters.
   *
   */
  //********************************************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::get_hierarchy() const {
    subcluster.clear();

    get_index();

    Array<int> tsubcluster;

    //make subcluster table for the empty cluster
    subcluster.push_back(tsubcluster);

    //take Cluster of size n
    //for each cluster of size < n in Cluster
    //look through Multiplet and see if there is an orbit that matches
    for(Index np = 1; np < size(); np++) {
      for(Index no = 0; no < size(np); no++) {

        ClustType tclust(lattice);

        Array<int> min(orbit(np, no).prototype.size(), 0), //All zeros
              max(orbit(np, no).prototype.size(), 1),        //All ones
              inc(orbit(np, no).prototype.size(), 1);        //All ones

        //enumerate all subclusters of cluster of orbit(np,no), represented in the Counter as 1's and 0's
        Counter<Array<int> > site_counter(min, max, inc);
        subcluster.push_back(tsubcluster);
        do {
          if(site_counter() == min || site_counter() == max) continue;   //make sure you don't add the empty or full subcluster

          for(Index i = 0; i < site_counter().size(); i++) {
            if(site_counter()[i]) {
              tclust.push_back(orbit(np, no).prototype[i]);
            }
          }

          //check if tclust is in list of clusters of size tclust.size()
          subcluster.back().push_back(find(tclust));

        }
        while(++site_counter);

      }
    }
    return;
  }


  //***********************************************************
  /**
   *
   *
   */
  //***********************************************************
  template<typename ClustType>
  void GenericOrbitree<ClustType>::read_prototype_tensor_basis(std::istream &stream, COORD_TYPE mode, const SymGroup &sym_group) {

    int num_orbit = 0, num_branches = 0;
    char ch;
    if(size())
      std::cerr << "WARNING:  Orbitree is about to be overwritten! Execution will continue normally, but side effects may occur.\n";

    ch = stream.peek();
    while((ch != 'B') && (ch != 'b') && !stream.eof()) {
      stream.ignore(256, '\n');
      ch = stream.peek();
      if(stream.eof()) {
        std::cerr << "Did not specify total number of Branches! \n";
        exit(1);
      }
    }

    stream.ignore(256, ' ');
    stream >> num_branches;
#ifdef DEBUG
    std::cout << "Read in number of branches is " << num_branches << "\n";
#endif //DEBUG

    //Size outer arry to have sufficient space to create orbitree
    //resize(max_num_sites+1, GenericOrbitBranch<CoordType>(lattice));
    resize(num_branches);

    for(int b = 0; b < num_branches; b++) {
#ifdef DEBUG
      std::cout << "Branch b = " << b << "\n";
#endif //DEBUG

      stream.ignore(256, '\n');
      stream.ignore(256, '\n');

      ClustType tclust(lattice);
      GenericOrbit<ClustType> torbit(tclust);

      // Reading in total number of orbits
      ch = stream.peek();
# ifdef DEBUG
      std::cout << "ch1 is " << ch << "\n";
# endif //DEBUG

      while((ch != 'O') && (ch != 'o') && !stream.eof()) {
        stream.ignore(1000, '\n');
        ch = stream.peek();

# ifdef DEBUG
        std::cout << "ch2 is " << ch << "\n";
# endif //DEBUG

        if(stream.eof()) {
          std::cerr << "Did not specify total number of Orbits!\n";
          exit(1);
        }
      }

      stream.ignore(256, ' ');
      stream >> num_orbit;
      stream.ignore(1000, '\n');

# ifdef DEBUG
      std::cout << "Number of orbits is " << num_orbit << "\n";
# endif //DEBUG

      if(num_orbit == 0) {
        std::cerr << "ERROR: Did not specify total number of orbits! \n";
        exit(1);
      }

      for(int i = 0; i < num_orbit; i++) {
# ifdef DEBUG
        std::cout << "Orbit " << i << "\n";
# endif //DEBUG

        torbit.clear();
        torbit.read(stream, mode, sym_group, true);
        at(b).push_back(torbit);
      }
    }
    sort();
    get_index();
    return;

  };


  //***********************************************************
  /**
   * Reads in CSPECS
   *
   * The format of CSPECS is as follows:
   * Description of structure/system (ignored by code)
   * Radius or Number
   * cluster size         within radius size or number of clusters
   * 2                    6.3
   *
   */
  //***********************************************************
  template<typename ClustType>
  void GenericOrbitree<ClustType>::read_CSPECS(std::istream &stream) {

    int cluster_size;
    int curr_cluster_size = 0;
    double specs;
    char ch, abc[256];

    min_num_components = 0; //added 03/23/13

    stream.ignore(1000, '\n');
    ch = stream.peek();

    if(ch == 'R' || ch == 'r') {
      max_length.clear();
      // For empty and point clusters;


      stream.ignore(1000, '\n');
      stream.getline(abc, 200);

      while(stream >> cluster_size) {
        if(curr_cluster_size == 0) {
          max_length.push_back(0.0);
          if(cluster_size == 1) {
            // LCSPECS file
            // do nothing
            max_num_sites = 0;

          }
          else if(cluster_size == 2) {
            // CSPECS file
            // push_back a max_length of
            max_length.push_back(0.0);
            max_num_sites = 1;

          }
          else {
            std::cerr << "error in GenericOrbitree<ClustType>::read_CSPECS()" << std::endl
                      << "  Your CSPECS file is wrong.  The first cluster size is: " << cluster_size << std::endl
                      << "  It should be 1 (for local) or 2 (for global)" << std::endl;
            std::exit(1);
          }
        }
        curr_cluster_size = cluster_size;

        stream >> specs;
        max_length.push_back(specs);
        max_num_sites++; //added 10/25/12
        stream.getline(abc, 200);

      }
    }
    else if(ch == 'N' || ch == 'n') {
      num_clusts.clear();

      //TODO: necessary to push back for empty and point clusters?

      stream.ignore(1000, '\n');

      while(stream >> cluster_size) {
        stream >> specs;
        num_clusts.push_back(specs);
        stream.ignore(1000, '\n');
      }
    }
    else {
      std::cerr << "ERROR in 2nd line of CSPECS.  2nd line should indicate either Radius or Number.\n";
      exit(1);
    }

  };

  //*********************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::print(std::ostream &stream) const {

    print_proto_clust(stream);

    /*
      for(Index i = 0; i < size(); i++) { //Loops over outer array (point/pair/triplet arrays)

        for(Index j = 0; j < size(i); j++) { //Loops over each Orbit, j, with Clusters of size i
          stream.width(5);
          stream << index[i][j];
          stream.width(5);

          stream << orbit(i, j).prototype.size();
          stream.width(5); //Size of Cluster

          stream <<  orbit(i, j).size() << std::endl; //Multiplicity

          orbit(i, j).prototype.print(stream); //Prints coordinates from Cluster::print() for the prototype cluster (which prints from Site::print() )
          stream << std::endl;
        };
      };
    */
  };

  //John G 011013
  //***********************************************
  /**
   * Assignment operator
   */
  //***********************************************
  template<typename ClustType>
  GenericOrbitree<ClustType> &GenericOrbitree<ClustType>::operator=(const GenericOrbitree<ClustType> &RHS) {
    //clear();
    //set_lattice(RHS.lattice, FRAC);

    //copy members
    lattice = RHS.lattice;
    max_num_sites = RHS.max_num_sites;
    min_num_components = RHS.min_num_components;
    max_length = RHS.max_length;
    min_length = RHS.min_length;
    num_clusts = RHS.num_clusts;
    index_to_row = RHS.index_to_row;
    index_to_column = RHS.index_to_column;
    index = RHS.index;
    Norbits = RHS.Norbits;
    subcluster = RHS.subcluster;

    //copy all orbitbranches over
    for(Index b = 0; b < RHS.size(); b++) {
      push_back(RHS[b]);
    }
    return *this;
  }



  //***********************************************
  /**
   * Allows for printing using the << operator.
   */
  //***********************************************
  template<typename ClustType>
  std::ostream &operator<< (std::ostream &stream, const GenericOrbitree<ClustType> &orbitree) {
    orbitree.print(stream);
    return stream;
  };


  //***********************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::write_full_clust(std::string file) const {
    //Prints out all equivalent clusters including the prototype cluster (FPCLUST file)
    // Calls ClustType.print_sites() to print each cluster

    std::ofstream out;
    out.open(file.c_str());
    if(!out) {
      std::cerr << "Can't open" << file << ".\n";
      return;
    }

    print_full_clust(out);
  };

  //***********************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::write_proto_clust(std::string file) const {
    //Prints out all prototype clusters (CLUST file)
    // Calls ClustType.print_sites() to print each prototype cluster

    std::ofstream out;
    out.open(file.c_str());
    if(!out) {
      std::cerr << "Can't open" << file << ".\n";
      return;
    }

    print_proto_clust(out);
  };

  //***********************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::write_full_decorated_clust(std::string file) const {
    //Prints out all equivalent clusters including the prototype cluster (FPCLUST file)
    // Calls ClustType.print_sites() to print each cluster


    std::ofstream out;
    out.open(file.c_str());
    if(!out) {
      std::cerr << "Can't open" << file << ".\n";
      return;
    }

    print_full_decorated_clust(out);
  };

  //***********************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::write_proto_decorated_clust(std::string file) const {
    //Prints out all prototype clusters (CLUST file)

    std::ofstream out;
    out.open(file.c_str());
    if(!out) {
      std::cerr << "Can't open" << file << ".\n";
      return;
    }

    print_proto_decorated_clust(out);
  };

  //***********************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::print_full_clust(std::ostream &out) const {
    //Prints out all equivalent clusters including the prototype cluster (FPCLUST file)
    // Calls ClustType.print_sites() to print each cluster

    if(index.size() != size()) get_index();

    out << "COORD_MODE = " << COORD_MODE::NAME() << std::endl << std::endl;

    out.flags(std::ios::showpoint | std::ios::fixed | std::ios::left);
    out.precision(5);

    for(Index i = 0; i < size(); i++) {
      if(size(i) != 0) out << "** Branch " << i << " ** \n" << std::flush;
      for(Index j = 0; j < size(i); j++) { //Loops over all i sized Orbits
        out << "      ** " << index[i][j] << " of " << Norbits << " Orbits **"
            << "  Orbit: " << i << " " << j
            << "  Points: " << orbit(i, j).prototype.size()
            << "  Mult: " << orbit(i, j).size()
            << "  MinLength: " << orbit(i, j).prototype.min_length()
            << "  MaxLength: " << orbit(i, j).prototype.max_length()
            << '\n' << std::flush;

        for(Index k = 0; k < orbit(i, j).size(); k++) { // Loops over each equivalent cluster in j
          out << "            " << k << " of " << orbit(i, j).size() << " Equivalent Clusters in Orbit " << index[i][j] << '\n' << std::flush;
          orbit(i, j).at(k).print_sites(out, 18, '\n');
        }
        out << '\n' << std::flush;
      }
      if(size(i) != 0) out << '\n' << std::flush;
    }
  };

  //***********************************************


  template<typename ClustType>
  void GenericOrbitree<ClustType>::print_full_basis_info(std::ostream &out) const {
    //Prints out all equivalent clusters including the prototype cluster (FPCLUST file)
    // Calls ClustType.print_sites() to print each cluster

    if(index.size() != size()) get_index();

    out << "COORD_MODE = " << COORD_MODE::NAME() << std::endl << std::endl;

    out.flags(std::ios::showpoint | std::ios::fixed | std::ios::left);
    out.precision(5);

    for(Index i = 0; i < size(); i++) {
      if(size(i) != 0) out << "** Branch " << i << " ** \n" << std::flush;
      for(Index j = 0; j < size(i); j++) { //Loops over all i sized Orbits
        out << "      ** " << index[i][j] << " of " << Norbits << " Orbits **"
            << "  Orbit: " << i << " " << j
            << "  Points: " << orbit(i, j).prototype.size()
            << "  Mult: " << orbit(i, j).size()
            << "  MinLength: " << orbit(i, j).prototype.min_length()
            << "  MaxLength: " << orbit(i, j).prototype.max_length()
            << '\n' << std::flush;

        for(Index k = 0; k < orbit(i, j).size(); k++) { // Loops over each equivalent cluster in j
          out << "            " << k << " of " << orbit(i, j).size() << " Equivalent Clusters in Orbit " << index[i][j] << '\n' << std::flush;
          orbit(i, j).at(k).print_basis_info(out, 18, '\n');
        }
        out << '\n' << std::flush;
      }
      if(size(i) != 0) out << '\n' << std::flush;
    }
  };

  //***********************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::print_proto_clust(std::ostream &out) const {
    //Prints out all prototype clusters (CLUST file)
    // Calls ClustType.print_sites() to print each prototype cluster

    if(index.size() != size()) get_index();

    out << "COORD_MODE = " << COORD_MODE::NAME() << std::endl << std::endl;

    out.flags(std::ios::showpoint | std::ios::fixed | std::ios::left);
    out.precision(5);

    for(Index i = 0; i < size(); i++) {
      if(size(i) != 0) out << "** Branch " << i << " ** \n" << std::flush;
      for(Index j = 0; j < size(i); j++) { //Loops over all i sized Orbits
        out << "      ** " << index[i][j] << " of " << Norbits << " Orbits **"
            << "  Orbit: " << i << " " << j
            << "  Points: " << orbit(i, j).prototype.size()
            << "  Mult: " << orbit(i, j).size()
            << "  MinLength: " << orbit(i, j).prototype.min_length()
            << "  MaxLength: " << orbit(i, j).prototype.max_length()
            << '\n' << std::flush;

        if(orbit(i, j).size() > 0) {
          for(int k = 0; k < 1; k++) { // Loops over each equivalent cluster in j
            out << "            " << "Prototype" << " of " << orbit(i, j).size() << " Equivalent Clusters in Orbit " << index[i][j] << '\n' << std::flush;
            orbit(i, j).at(k).print_sites(out, 18, '\n');

          }
        }
        out << '\n' << std::flush;
      }
      if(size(i) != 0) out << '\n' << std::flush;
    }
  };

  //***********************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::print_full_decorated_clust(std::ostream &out) const {
    //Prints out all equivalent clusters including the prototype cluster (FPCLUST file)
    // Calls ClustType.print_sites() to print each cluster

    if(index.size() != size()) get_index();

    out << "COORD_MODE = " << COORD_MODE::NAME() << std::endl << std::endl;

    out.flags(std::ios::showpoint | std::ios::fixed | std::ios::left);
    out.precision(5);

    for(Index i = 0; i < size(); i++) {
      if(size(i) != 0) out << "** Branch " << i << " ** \n" << std::flush;
      for(Index j = 0; j < size(i); j++) { //Loops over all i sized Orbits
        out << "      ** " << index[i][j] << " of " << Norbits << " Orbits **"
            << "  Orbit: " << i << " " << j
            << "  Points: " << orbit(i, j).prototype.size()
            << "  Mult: " << orbit(i, j).size()
            << "  MinLength: " << orbit(i, j).prototype.min_length()
            << "  MaxLength: " << orbit(i, j).prototype.max_length()
            << '\n' << std::flush;
        for(Index k = 0; k < orbit(i, j).size(); k++) { // Loops over each equivalent cluster in j
          out << "            " << k << " of " << orbit(i, j).size() << " Equivalent Clusters in Orbit " << index[i][j] << '\n' << std::flush;
          orbit(i, j).at(k).print_decorated_sites(out, 18, '\n');
        }
        out << '\n' << std::flush;
      }
      if(size(i) != 0) out << '\n' << std::flush;
    }
  };

  //***********************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::print_proto_decorated_clust(std::ostream &out) const {
    //Prints out all decorated prototype clusters (like BCLUST file)

    if(index.size() != size()) get_index();

    out << "COORD_MODE = " << COORD_MODE::NAME() << std::endl << std::endl;

    out.flags(std::ios::showpoint | std::ios::fixed | std::ios::left);
    out.precision(5);

    for(Index i = 0; i < size(); i++) {
      if(size(i) != 0) out << "** Branch " << i << " ** \n" << std::flush;
      for(Index j = 0; j < size(i); j++) { //Loops over all i sized Orbits
        out << "      ** " << index[i][j] << " of " << Norbits << " Orbits **"
            << "  Orbit: " << i << " " << j
            << "  Points: " << orbit(i, j).prototype.size()
            << "  Mult: " << orbit(i, j).size()
            << "  MinLength: " << orbit(i, j).prototype.min_length()
            << "  MaxLength: " << orbit(i, j).prototype.max_length()
            << '\n' << std::flush;

        if(orbit(i, j).size() > 0) {
          for(int k = 0; k < 1; k++) { // Loops over each equivalent cluster in j
            out << "            " << "Prototype" << " of " << orbit(i, j).size() << " Equivalent Clusters in Orbit " << index[i][j] << '\n' << std::flush;
            orbit(i, j).at(k).print_decorated_sites(out, 18, '\n');

          }
        }

        out << '\n' << std::flush;
      }
      if(size(i) != 0) out << '\n' << std::flush;
    }
  };


  //*********************************************************
  /**
   *
   *
   */
  //*********************************************************
  template<typename ClustType>
  void GenericOrbitree<ClustType>::write_full_tensor_basis(std::string file, Index np) const {

    bool read_eci;

    std::ofstream out;
    out.open(file.c_str());
    if(index.size() != size()) {
      get_index();
    }

    if(!out) {
      std::cerr << "Can't open " << file << ".\n";
      return;
    }

    out.flags(std::ios::showpoint | std::ios::fixed | std::ios::left);
    out.precision(5);

    int no = 0, io = 0;
    for(Index i = 0; i < size(); i++) {
      for(Index j = 0; j < at(i).size(); j++) { //Loops over all i sized Orbits
        if(prototype(i, j).size() >= np)
          no++;
      }
    }

    out << "Branches " << size() << "\n";
    //out << "Orbits " << no << "\n";

    // std::cout << "INSIDE ORBITREE::WRITE_TENSOR_BASIS.  NUMBER OF BRANCHES IS "
    // 	      << size() << "\n";


    //Loop over branches
    for(Index i = 0; i < size(); i++) {
      out << "Branch " << (i + 1) << " of " << size() << "\n";
      out << "Orbits " << at(i).size() << "\n";
      //Loop over orbits
      for(Index j = 0; j < at(i).size(); j++) { //Loops over all i sized Orbits

        if(prototype(i, j).size() >= np) {
          io++;
          //out << "Orbit " << io << " of " << no << "\n";
          out << "Orbit " << (j + 1) << " of " << at(i).size() << "\n";
          out << "Clusters in Orbit: "
              << orbit(i, j).size() << "\n";

          for(Index k = 0; k < orbit(i, j).size(); k++) {
            equiv(i, j, k).print(out, '\n');
            out << "\n";
            out << "Tensor Basis \n";

            if(equiv(i, j, k).tensor_basis.size()) {

              out << equiv(i, j, k).tensor_basis.size() << "  "
                  << equiv(i, j, k).tensor_basis[0].rank() << "  "
                  << equiv(i, j, k).tensor_basis[0].dim() << "\n";

              for(Index t = 0; t < equiv(i, j, k).tensor_basis.size(); t++) {

                if(std::isnan(equiv(i, j, k).tensor_basis.eci(t))) {
                  out << "<ECI> * \n";
                  out << equiv(i, j, k).tensor_basis[t] << "\n\n";
                  read_eci = false;
                }
                else {
                  out << equiv(i, j, k).tensor_basis.eci(t) << " * \n";
                  out << equiv(i, j, k).tensor_basis[t] << "\n\n";
                  read_eci = true;
                }
              }
              if(read_eci == false) {
                out << "Force Constant Tensor of this Cluster is \n"
                    << "N/A\n\n";
              }
              else {
                out << "Force Constant Tensor of this Cluster is \n"
                    << equiv(i, j, k).eci << "\n\n";
              }

            }
            out << "************************************************ \n";
          } //End loop over clusters
        }
      }
    }


  };

  //*********************************************************
  /**
   *
   * @param file Name of the file being written to
   * @param np Number of points in the prototype cluster
   */
  //*********************************************************
  template<typename ClustType>
  void GenericOrbitree<ClustType>::write_prototype_tensor_basis(std::string file, Index np, std::string path) const {

    path.append(file);
    std::ofstream out;
    bool read_eci;
    //out.open(file.c_str());
    out.open(path.c_str());
    if(index.size() != size()) {
      get_index();
    }

    if(!out) {
      std::cerr << "Can't open " << file << ".\n";
      return;
    }

    out.flags(std::ios::showpoint | std::ios::fixed | std::ios::left);
    out.precision(5);

    int no = 0, io = 0;
    for(Index i = 0; i < size(); i++) {
      for(Index j = 0; j < at(i).size(); j++) { //Loops over all i sized Orbits

        if(prototype(i, j).size() >= np)
          no++;
      }
    }

    out << "Branches " << size() << "\n";
    //out << "Orbits " << no << "\n";

    for(Index i = 0; i < size(); i++) {
      out << "Branch " << (i + 1) << " of " << size() << "\n";
      out << "Orbits " << at(i).size() << "\n";

      for(Index j = 0; j < at(i).size(); j++) { //Loops over all i sized Orbits

        if(prototype(i, j).size() >= np) {
          io++;
          //out << "Orbit " << io << " of " << no << "\n";
          out << "Orbit " << (j + 1) << " of " << at(i).size() << "\n";
          out << "Clusters in Orbit: 1 \n";
          orbit(i, j).prototype.print(out, '\n'); //Changed 04/04/13 to accomodate new cluster print
          out << "\n"; //Added 04/04/13 to accomodate new cluster print
          out << "Tensor Basis \n";

          if(orbit(i, j).prototype.tensor_basis.size()) {

            out << orbit(i, j).prototype.tensor_basis.size() << "  "
                << orbit(i, j).prototype.tensor_basis[0].rank() << "  "
                << orbit(i, j).prototype.tensor_basis[0].dim() << "\n";

            for(Index t = 0; t < orbit(i, j).prototype.tensor_basis.size(); t++) {

              if(std::isnan(orbit(i, j).prototype.tensor_basis.eci(t))) {
                out << "<ECI> * \n";
                out << orbit(i, j).prototype.tensor_basis[t] << "\n\n";
                read_eci = false;
              }
              else {
                out << orbit(i, j).prototype.tensor_basis.eci(t) << " * \n";
                out << orbit(i, j).prototype.tensor_basis[t] << "\n\n";
                read_eci = true;
              }
            }
            if(read_eci == false) {
              out << "Force Constant Tensor of this Cluster is \n"
                  << "N/A\n\n";
            }
            else {
              out << "Force Constant Tensor of this Cluster is \n"
                  << orbit(i, j).prototype.eci << "\n\n";
            }
          }
          else {
            out << 0 << " " << 0 << " " << 0 << "\n";
          }
          out << "************************************************ \n";
        }
      }

    } //End loop over Branches


  };

  //***********************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::get_dynamical_matrix(MatrixXcd &dmat, const Coordinate &k, Index bands_per_site) {
    Index np, no, ne, i, j;
    std::complex<double> tphase;

    //loop over all the orbits of orbitree
    for(np = 0; np < size(); np++) {
      for(no = 0; no < size(np); no++) {

        //Loop over all equivalent clusters
        for(ne = 0; ne < size(np, no); ne++) {
          /*
          //check rank for extra safety, but seems like a waste to do it for every k-point
          if(equiv(np,no,ne).eci.rank()<2){
          std::cerr << "FATAL ERROR: Attempting to construct dynamical matrix, "
          << "but an eci has been initialized improperly! Exiting...\n";
          exit(0);
          }
          */
          tphase = equiv(np, no, ne).get_phase(k);

          //Loop over all elements of the eci tensor.  These correspond to the number of bands per site.
          for(i = 0; i < bands_per_site; i++) {
            for(j = 0; j < bands_per_site; j++) {
              // Multiply each element of the eci tensor by the phase factor and
              // add it to the block of the matrix corresponding to that equivalent cluster.

              dmat(equiv(np, no, ne)[0].basis_ind() * bands_per_site + i,
                   equiv(np, no, ne)[1].basis_ind() * bands_per_site + j)
              += tphase * equiv(np, no, ne).eci(i, j);
            }
          }
        }
      }
    }
    return;
  }

  //********************************************************************
  /**
   * It calculates the ECI tensors for the
   * clusters (petals).  The ECI tensors for the self-interaction pairs
   * (the pivot point with itself) is the negative of the sum of
   * the force constant tensors of the petals of its flower.
   */
  //********************************************************************
  template<typename CoordType>
  void GenericOrbitree<CoordType>::calc_tensors() {
    std::cout << "********* In Orbitree::calc_tensors! **********\n";
    // Calculate ECI's
    // Loop over branches of tree
    std::cout << "The size of the branch is " << size() << "\n";
    for(Index i = 0; i < size(); i++) {

      std::cout << "Number of orbits: " << at(i).size() << "\n";
      // Loop over orbits of orbitbranch
      for(Index j = 0; j < at(i).size(); j++) {
        orbit(i, j).calc_eci(2);
      }
    }

    std::cout << "Done with first two for loops \n";
    //Generate self-interaction pair(s') ECI tensors
    //Loop over branches
    //std::cout << "size of flower tree is " << size() << "\n";
    //std::cout << "size of orbit(0,0) is " << orbit(0,0).size() << "\n";
    for(Index b = 0; b < size(); b++) {

      std::cout << "b = " << b << "\n\n";
      // std::cout << "The self interaction term is "
      // 		<< equiv(b,0,0) << "\nand\n"
      // 		<< prototype(b,0) << "\n";
      equiv(b, 0, 0).eci.redefine(Array<Index>(2, 3));
      equiv(b, 0, 0).eci = 0.0;
      prototype(b, 0).eci.redefine(Array<Index>(2, 3));
      prototype(b, 0).eci = 0.0;

      //TODO: Change this so if it reads in a self-interaction,
      //won't overwrite?

      //loop over orbits (flower)
      for(Index d = 1; d < at(b).size(); d++) {
        //loop over clusters (petals)
        //summing over all the eci's in the flower (ECI of
        //self-interaction pair has to be invariant under symmetry)
        for(Index e = 0; e < orbit(b, d).size(); e++) {
          equiv(b, 0, 0).eci -= equiv(b, d, e).eci;
          prototype(b, 0).eci -= equiv(b, d, e).eci;
        }
      }
      std::cout << "Self-interaction force constant for cluster \n";
      prototype(b, 0).print(std::cout, '\n');
      std::cout << "\n is: \n" << prototype(b, 0).eci << "\n";
    } // End loop over branches to generate self-interaction pair
  };

  //John G 050513
  //************************************************************
  /**
   * \brief   Constructs a local orbitree about a Phenom Cluster, given a Structure.
   *
   *   - First finds the point clusters, then pair clusters, then triplets, etc.
   *   - If 'include_phenom_clust_sites' is true, then clusters include sites in the phenom_clust, else they are not.
   *   - Sets the Orbitrees 'phenom_clust' to 'tmp_phenom_clust'
   */
  //************************************************************

  template<typename ClustType> template<class PhenomType>
  void GenericOrbitree<ClustType>::generate_local_orbitree(const Structure &prim, const PhenomType &tmp_phenom_clust,
                                                           bool include_phenom_clust_sites) {
    //std::cout << "\n\nBegin generate_local_orbitree() --------------\n\n" << std::flush;

    /// make an orbitree of local clusters around the cluster 'clust' in the structure 'prim'
    ///		must set GenericOrbitree<ClustType>::max_num_sites and GenericOrbitree<ClustType>::max_length
    ///		prior to calling GenericOrbitree<ClustType>::generate_local_orbitree

    //std::cout << "phenom_clust: " << std::endl;
    //phenom_clust.print_v2(std::cout);

    PhenomType phenom_clust(tmp_phenom_clust);

    //std::cout << "phenom_clust: " << std::endl;
    //phenom_clust.print_v2(std::cout);


    Index i, j, np, no;
    Vector3<int> dim;			//size of gridstruc
    double dist, max_dist;
    Array<typename ClustType::WhichCoordType> basis, gridstruc;

    lattice = prim.lattice();
    Coordinate lat_point(lattice);


    // Make 'basis', a list of the basis sites in prim that we want to make clusters from
    //std::cout << "Finding Basis:\n" << std::flush;
    for(i = 0; i < prim.basis.size(); i++) {
      if(prim.basis[i].site_occupant().size() >= min_num_components) {
        basis.push_back(prim.basis[i]);
        basis.back().set_lattice(lattice);
      }
    }


    // Make 'gridstruc', a list of all the sites we might make clusters from

    // 'max_radius' is the maximum distance from any of the sites in 'phenom_clust' for sites in the local clusters
    double max_radius = max_length.max();

    // 'dim' is size of grid so that all sites within max_radius are included
    dim = lattice.enclose_sphere(max_radius);
    Counter<Vector3<int> > grid_count(-dim, dim, Vector3<int>(1));
    //std::cout << "dim is " << dim << '\n' << std::flush;
    //std::cout << "\n Finding Grid_struc:\n" << std::flush;

    // check all sites to see if they are close enough to 'phenom_clust' that they might be included in the local clusters
    do {
      lat_point(FRAC) = grid_count();

      for(i = 0; i < basis.size(); i++) {
        // check site at basis[i] + lat_point
        typename ClustType::WhichCoordType tatom(basis[i] + lat_point);

        // don't add sites that are a part of the cluster
        // site occupant makes this comparison work incorrectly, so check with min_length instead
        //if(!include_phenom_clust_sites) if(phenom_clust.contains(tatom)) continue;
        if(!include_phenom_clust_sites) {
          bool point_is_in_phenom = false;

          for(Index j = 0; j < phenom_clust.size(); j++) {
            if(phenom_clust[j].Coordinate::operator==(tatom)) {
              point_is_in_phenom = true;
              break;
            }
          }

          if(point_is_in_phenom)
            continue;
        }


        // get distance to farthest site in the cluster
        max_dist = 0;
        for(j = 0; j < phenom_clust.size(); j++) {
          dist = tatom.dist(phenom_clust[j]);
          if(dist > max_dist)
            max_dist = dist;
        }

        // if that distance is less than max_radius, include it in the gridstruc
        if(max_dist < max_radius) {
          gridstruc.push_back(tatom);
          //std::cout << "i: " << gridstruc.size() << "  " << tatom; // << std::endl;
        }
      }

    }
    while(++grid_count);
    //std::cout << "Finished finding grid_struc\n" << std::flush;

    // Get orbitree ready to hold clusters.
    if(size())
      std::cerr << "WARNING:  Orbitree is about to be overwritten! Execution will continue normally, but side effects may occur.\n" << std::flush;

    //Size outer array (# of sites in cluster) to have sufficient space to create orbitree
    resize(max_num_sites + 1);


    //for each cluster of the previous size, add points from gridstruc
    //   - see if the new cluster satisfies the size requirements
    //   - see if it is new
    //   - generate all its equivalents

    //std::cout << "About to begin construction of non-empty clusters\n" << std::flush;


    // we want to find equivalent clusters by using the cluster group of the 'phenom_clust'
    //   maybe don't need this, phenom_clust likely already has cluster group calculated...

    {
      PERIODICITY_MODE periodicity_mode1(PERIODIC);
      phenom_clust.get_clust_group(prim.factor_group());
    }

    // write phenom_clust to check clust group
    //{
    //	std::ofstream outfile;
    //	std::stringstream ss;
    //	ss << "PHENOM";
    //	outfile.open(ss.str().c_str());
    //
    //	outfile << "PHENOM" << std::endl;
    //	phenom_clust.print_decorated_sites(outfile, 2, '\n');
    //	outfile << '\n';
    //	outfile << "CLUSTGROUP:" << std::endl;
    //	phenom_clust.clust_group.print(outfile, COORD_DEFAULT);
    //
    //
    //	outfile.close();
    //	int pause;
    //	std::cout << "Pause: " ;
    //	std::cin >> pause;
    //	std::cout << std::endl;
    //}


    // for local clusters, we want the PERIODICITY_MODE to be LOCAL
    PERIODICITY_MODE periodicity_mode2(LOCAL);
    //std::cout << "  PMODE: " << PERIODICITY_MODE::CHECK() << endl;

    // Add orbit corresponding to empty cluster
    at(0).push_back(GenericOrbit<ClustType>(ClustType(lattice)));
    at(0).back().get_equivalent(phenom_clust.clust_group);
    at(0).back().get_cluster_symmetry();


    // loop through OrbitBranches of Orbits of clusters of np sites
    for(np = 1; np <= max_num_sites; np++) {
      //std::cout << "OrbitBranch: " << np << "\n" << std::flush;

      if(size(np - 1) == 0) {
        std::cerr << "WARNING: Orbitree::generate_local_orbitree is unable to enumerate clusters of size " << np << ".\n";
        std::cerr << "                found no clusters of size " << np - 1 << ".\n" << std::flush;
        sort();
        get_index();
        //print(std::cout);
        return;
        //exit(1);
      }

      // loop through all orbits of the previous size
      for(no = 0; no < size(np - 1); no++) {
        //std::cout << "  Adding sites to orbit " << no << " of " << size(np - 1) << "\n" << std::flush;

        // base test clust on prototype cluster of smaller size
        ClustType tclust(lattice);
        for(i = 0; i < orbit(np - 1, no).prototype.size(); i++)
          tclust.push_back(orbit(np - 1, no).prototype[i]);

        // check all sites in gridstruc
        for(i = 0; i < gridstruc.size(); i++) {

          //tclust = orbit(np - 1, no).prototype;

          // add a site to the test clust
          tclust.push_back(gridstruc[i]);

#ifdef DEBUG
          std::cout << "tclust is \n" << tclust << "\n" << std::flush;
#endif //DEBUG

          //tclust.within(); // don't 'within' for local cluster

          // calculate lengths between sites in tclust and phenom_clust
          //   set max_length to maximum distance between cluster sites or phenom_clust sites
          //   set min_length to minimum distance between clusters sites (does not include phenom sites)
          tclust.calc_properties(phenom_clust);

          //std::cout << "tclust: " << std::endl;
          //tclust.print_v2(std::cout);
          //std::cout<< "  tclust.max_length(): " << tclust.max_length() << "  limit: " << max_length[np] << std::endl;
          //std::cout<< "  tclust.min_length(): " << tclust.min_length() << "  limit: " << min_length << std::endl;
          //std::cout<< "  !contains(tclust): " << !contains(tclust) << std::endl;


#ifdef DEBUG
          std::cout << "tclust is \n" << tclust << "\n" << std::flush;
#endif //DEBUG

          if(np == 1 && !contains(tclust)) {
            //std::cout << "-- add point cluster" << std::endl << std::endl;
            at(np).push_back(GenericOrbit<ClustType>(tclust));
            at(np).back().get_equivalent(phenom_clust.clust_group);
            at(np).back().get_cluster_symmetry();

          }
          else if(tclust.max_length() < max_length[np] && tclust.min_length() > min_length && !contains(tclust)) {
            //std::cout << "-- Found a new cluster.... adding to Orbitree!\n" << std::flush;
            //std::cout << "   The minimum length is " << min_length << "\n" << std::flush;
            at(np).push_back(GenericOrbit<ClustType>(tclust));

#ifdef DEBUG
            std::cout << "The tclust we pushed back is \n" << tclust << "\n" << std::flush;
#endif //DEBUG

            at(np).back().get_equivalent(phenom_clust.clust_group);
            at(np).back().get_cluster_symmetry();
          }

          tclust.pop_back();
        }
      }
    }
    sort();
    get_index();
    return;
  }


  //************************************************************
  /**
   *		Apply symmetry to every orbit in orbitree
   */
  template<typename ClustType>
  void GenericOrbitree<ClustType>::apply_sym(const SymOp &op) {
    for(Index i = 0; i < size(); i++) {
      for(Index j = 0; j < size(i); j++) {
        orbit(i, j).apply_sym(op);
      }
    }
  }

  //***********************************************

  template<typename ClustType>
  void GenericOrbitree<ClustType>::write_eci_in(std::string filename) const {
    BP::BP_Write file(filename);
    file.newfile();

    print_eci_in(file.get_ostream());

  }

  template<typename ClustType>
  void GenericOrbitree<ClustType>::print_eci_in(std::ostream &out) const {
    if(index.size() != size()) get_index();
    if(subcluster.size() != size()) get_hierarchy();

    out << std::left
        << std::setw(8) << "label"
        << std::setw(8) << "weight"
        << std::setw(8) << "mult"
        << std::setw(8) << "size"
        << std::setw(12) << "length"
        << std::setw(8) << "hierarchy" << std::endl;

    for(Index i = 0; i < size(); i++) {
      for(Index j = 0; j < size(i); j++) {
        out << std::left
            << std::setw(8) << index[i][j]
            << std::setw(8) << 0
            << std::setw(8) << orbit(i, j).size()
            << std::setw(8) << orbit(i, j).prototype.size()
            << std::setw(12) << orbit(i, j).prototype.max_length();

        // print hierarchy
        out << std::left << std::setw(8) << 0;
        for(Index k = 0; k < subcluster[ index[i][j]].size(); k++) {
          out << std::left
              << std::setw(8) << subcluster[ index[i][j] ][k];
        }
        out << '\n' << std::flush;

      }
    }

    //std::cout << "finish print_eci_in" << std::endl;

  }

  //********************************************************************
  /*
  template<typename ClustType>
  jsonParser &GenericOrbitree<ClustType>::to_json(jsonParser &json) const {
    json.put_obj();

    // template<typename ClustType>
    // class GenericOrbitree : public Array< GenericOrbitBranch<ClustType> >
    json["branches"].put_array(size());
    for(Index i = 0; i < size(); i++) {
      json["branches"][i] = at(i);
    }

    // Lattice lattice;
    json["lattice"] = lattice;

    // int max_num_sites, min_num_components;
    json["max_num_sites"] = max_num_sites;
    json["min_num_components"] = min_num_components;

    // Array<double> max_length;
    json["max_length"] = max_length;

    // double min_length;
    json["min_length"] = min_length;

    // Array<int> num_clusts;
    json["num_clusts"] = num_clusts;

    // mutable Array<int> index_to_row, index_to_column;
    json["index_to_row"] = index_to_row;
    json["index_to_column"] = index_to_column;

    // mutable Array< Array<int> > index;
    json["index"] = index;

    // mutable int Norbits;
    json["Norbits"] = Norbits;

    // mutable Array< Array<int> > subcluster;
    json["subcluster"] = subcluster;
    return json;

  }
  */
  //*********************************************************
  /*
    WARNING: Ensure that you have initialized the basis set
    in ref_struc.basis[i]
   */
  //*********************************************************
  template<typename ClustType>
  void GenericOrbitree<ClustType>::read_orbitree_from_json(const std::string &json_file_name, const SymGroup &sym_group, const Structure &ref_struc) {
    jsonParser json(json_file_name);
    (*this).from_json(json);
    //Check if the basis set has been initialized in ref_struc
    bool basis_set_init = true;
    for(int i = 0; i < ref_struc.basis.size(); i++) {
      if(ref_struc.basis[i].occupant_basis().size() <= 0) {
        std::cerr << "WARNING in GenericOrbitree<ClustType>::read_orbitree_from_json. The Basis Set in the structure you passed in has"
                  << " not been initialized. You may want to re-try this method after you initialize the Basis Set if you want to "
                  << "calculate correlations. " << std::endl;
        basis_set_init = false;
        break;
      }
    }
    int np, no;
    std::cout << "In read_orbitree_from_json. Initializing the occupant basis" << std::endl;
    if(basis_set_init) {
      //std::cout<<"basis_set_init is true"<<std::endl;
      //Set the basis sets for all the prototype clusters
      for(np = 0; np < (*this).size(); np++) {
        for(no = 0; no < at(np).size(); no++) {
          //std::cout<<"Working on np:"<<np<<"  no:"<<no<<std::endl;
          at(np).at(no).prototype.update_data_members(ref_struc);
        }
      }
    }
    //Gets the equivalent clusters and the cluster symmetry
    for(np = 0; np < (*this).size(); np++) {
      for(no = 0; no < at(np).size(); no++) {
        at(np).at(no).get_equivalent(sym_group);
        at(np).at(no).get_cluster_symmetry();
        // std::cout<<"Sym Group of np: "<<np<<"  no:"<<no<<std::endl;
        // at(np).at(no).prototype.clust_group.print(std::cout,FRAC);
        // std::cout<<"Permute group of the same"<<std::endl;
        // at(np).at(no).prototype.permute_group.print_permutation(std::cout);
      }
    }
  }


  /// \brief Add more orbits to Orbitree based on JSON input
  ///
  /// Expected JSON format:
  /// \code
  /// {
  ///   “orbits” : [
  ///    { "coordinate_mode" : “Direct”,   // could be “Fractional”, “Direct”, or “Cartesian”, future “UnitCellCoordinate”
  ///      "prototype" : [
  ///        [ 0.000000000000, 0.000000000000, 0.000000000000 ],
  ///        [ 1.000000000000, 0.000000000000, 0.000000000000 ],
  ///        [ 2.000000000000, 0.000000000000, 0.000000000000 ],
  ///        [ 3.000000000000, 0.000000000000, 0.000000000000 ]
  ///      ],
  ///      “include_subclusters” : true  (Default true if not specified)
  ///    },
  ///    ... more orbit specs ...
  /// }
  /// \endcode
  ///
  template<typename ClustType>
  bool GenericOrbitree<ClustType>::read_custom_clusters_from_json(const jsonParser &json, const Structure &struc, const SymGroup &sym_group, bool verbose) {

    // Initializing data
    Array<ClustType> proto_clust;
    ClustType temp_clust(lattice);
    COORD_TYPE json_coord_mode;
    Vector3<double> json_coord;
    Array< Vector3<double> > json_coord_list;
    int custom_max_num_sites = 0;
    int custom_min_num_components = 100;

    const jsonParser &orbit_specs = json;

    if(verbose) std::cout << "Number of clusters found: " << orbit_specs.size() << std::endl;
    //proto_clust.resize(json["clusters"].size(), temp_clust);
    for(int i = 0; i < orbit_specs.size(); i++) {
      try {
        std::string in_mode = orbit_specs[i]["coordinate_mode"].get<std::string>();
        if(in_mode == "Cartesian") {
          json_coord_mode = CART;
        }
        else if(in_mode == "Direct" || in_mode == "Fractional") {
          json_coord_mode = FRAC;
        }
        else {
          std::cerr << "ERROR in GenericOrbitree<ClustType>::read_custom_clusters_from_json. "
                    << "The specified coord_mode for orbit " << i << " is invalid. Please try to re-read"
                    << " the file after correcting the error." << std::endl;
          throw 2000;
        }

      }
      catch(int exception_code) {
        if(exception_code != 2000)
          std::cerr << "ERROR in GenericOrbitree<ClustType>::read_custom_clusters_from_json. You have not specified 'coordinate_mode'"
                    << " for orbit " << i << " in your json. Please correct it and try to re-read the file. Returning after"
                    << " throwing an error" << std::endl;
        throw;
      }
      try {
        json_coord_list.clear();
        if(verbose) std::cout << "Cluster " << i << " contains " << orbit_specs[i]["prototype"].size() << "sites" << std::endl;
        //json_coord_list.resize(json["clusters"][i]["coordinate"].size());
        for(int j = 0; j < orbit_specs[i]["prototype"].size(); j++) {
          json_coord_list.push_back(orbit_specs[i]["prototype"][j].get< Vector3<double> >());
        }
        if(verbose) std::cout << "Added the clusters into the temporary array. The array is:" << std::endl << json_coord_list << std::endl;
      }
      catch(...) {
        std::cerr << "ERROR in GenericOrbitree<ClustType>::read_custom_clusters_from_json. Ran into some trouble reading"
                  << " the coordinates of orbit " << i << " in your json. Please correct it and try to re-read the file. "
                  << "Returning after throwing an error" << std::endl;
        throw;
      }
      if(verbose) std::cout << "Loaded all the sites in the cluster into memory, converting into CASM data structures" << std::endl;
      //Looking for the correct sites in the PRIM and loading in the correct information
      temp_clust.clear();
      for(int j = 0; j < json_coord_list.size(); j++) {
        //Converting to a Coordinate
        Coordinate tcoord(json_coord_list[j], struc.lattice(), json_coord_mode);
        int site_loc = struc.find(tcoord);
        if(site_loc != struc.basis.size()) {
          temp_clust.push_back(struc.basis[site_loc]);
          temp_clust.back()(CART) = tcoord(CART);
        }
      }
      temp_clust.calc_properties();
      proto_clust.push_back(temp_clust);
      if(verbose) std::cout << "Finished initializing cluster " << i << std::endl;
      //Checking to see if the cluster is already in the Orbitree
    }

    //Add the empty cluster if the Orbitree is empty
    if(this->size() == 0) {
      (*this).push_back(GenericOrbitBranch<ClustType>(lattice));
      max_num_sites = 0;
      min_num_components = 1 ; //IS THIS THE BEST WAY TO DO IT?
      std::cerr << "WARNING in GenericOrbitree<ClustType>::read_custom_cluster_from_json your Orbitree "
                << "is not initialized completely, (ie) max_num_sites was not set and the Orbitree did "
                << "not contain the empty cluster. This has been fixed for you, but you may want to "
                << "initialize it properly in the future before reading in custom clusters" << std::endl;
    }

    if(verbose) std::cout << "Finished loading all the cluster data. Trying to import it into Orbitree" << std::endl;
    if(verbose) std::cout << "The number of sites in this Orbitree is:" << max_num_sites << std::endl;
    for(int i = 0; i < proto_clust.size(); i++) {
      if(proto_clust[i].size() > custom_max_num_sites)
        custom_max_num_sites = proto_clust[i].size();
      for(int j = 0; j < proto_clust[i].size(); j++) {
        if(proto_clust[i][j].allowed_occupants().size() < custom_min_num_components)
          custom_min_num_components = proto_clust[i][j].allowed_occupants().size();
      }
    }
    if(custom_min_num_components < min_num_components)
      min_num_components = custom_min_num_components;
    if(verbose) std::cout << "The min_num_components is: " << custom_min_num_components << std::endl;
    if(verbose) std::cout << "The max num sites in the custom clusters is:" << custom_max_num_sites << std::endl;
    while(size() <= custom_max_num_sites) {
      push_back(GenericOrbitBranch<ClustType>(lattice));
      max_num_sites = custom_max_num_sites;
    }

    for(int i = 0; i < proto_clust.size(); i++) {
      std::cout << "Working on cluster: " << i << std::endl;
      if(contains(proto_clust[i])) {
        std::cout << "Proto_clust: " << std::endl;
        proto_clust[i].print(std::cout);
        std::cout << "This cluster is already in the Orbitree. Not adding it to the list" << std::endl;
        continue;
      }
      //std::cout << "        Add orbit!" << std::endl;
      at(proto_clust[i].size()).push_back(GenericOrbit<ClustType>(proto_clust[i]));
      //std::cout << "        get_equivalent()" << std::endl;
      at(proto_clust[i].size()).back().get_equivalent(sym_group);
      //std::cout << "        get_cluster_symmetry()" << std::endl;
      at(proto_clust[i].size()).back().get_cluster_symmetry();

      try {
        bool include_subclusters;
        // check if should include_subclusters.  default is true
        orbit_specs[i].get_else(include_subclusters, "include_subclusters", true);
        if(include_subclusters) {
          add_subclusters(at(proto_clust[i].size()).back().prototype, struc, verbose);
        }
      }
      catch(...) {
        std::cerr << "ERROR in GenericOrbitree<ClustType>::read_custom_clusters_from_json reading " <<
                  "\"include_subclusters\"." << std::endl;
        throw;
      }
    }
    sort();
    get_index();
    get_hierarchy();
    return true;
  }

  //********************************************************************
  /// Adding in subclusters of a specific cluster into *this Orbitree
  template<typename ClustType>
  void GenericOrbitree<ClustType>::add_subclusters(const ClustType &big_clust, const Structure &prim, bool verbose) {
    if(verbose) std::cout << "In Orbitree::add_subclusters. Working on cluster: " << big_clust << std::endl;
    if(prim.factor_group().size() == 0) {
      std::cerr << "WARNING: In Orbitree::add_subclusters, prim's factor_group is empty. It  must at least have one element (identity).\n";
      assert(0);
    }

    //Ensure that the lattices are the same:
    if(!(lattice == prim.lattice())) {
      std::cerr << "WARNING in Orbitree::add_subclusters, the lattice in prim and the lattice"
                << " that was used to construct this cluster are not the same" << std::endl;
      assert(0);
    }

    if(verbose) std::cout << "Size of this is : " << size() << std::endl;
    if(verbose) std::cout << "Size of cluster is : " << big_clust.size() << std::endl;
    if((size() - 1) < big_clust.size()) {
      std::cout << "Adding more Branches to this orbitree" << std::endl;
      for(Index temp = 0; temp <= (big_clust.size() - size()); temp++)
        (*this).push_back(GenericOrbitBranch<ClustType>(lattice));
      max_num_sites = big_clust.size();
    }

    Index i, j;
    std::string clean(80, ' ');
    Array<int> master_choose(big_clust.size(), 0);
    if(verbose) std::cout << "Master_Choose : " << master_choose << std::endl;

    for(i = 1; i <= big_clust.size(); i++) {
      if(verbose) std::cout << "Working on a subcluster of size: " << i << std::endl;
      Array<int> choose = master_choose;
      for(j = 0; j < i; j++)
        choose[choose.size() - j - 1] = 1;
      ClustType test_clust(prim.lattice());
      do {
        if(verbose) std::cout << "Choose is: " << choose << std::endl;
        test_clust.clear();
        for(j = 0; j < choose.size(); j++) {
          if(choose[j] == 1)
            test_clust.push_back(big_clust.at(j));
        }
        test_clust.within();
        test_clust.calc_properties();

        if(!contains(test_clust)) {
          if(verbose) std::cout << "Adding this cluster: " << test_clust << std::endl;
          at(i).push_back(GenericOrbit< ClustType >(test_clust));
          at(i).back().get_equivalent(prim.factor_group());
          at(i).back().get_cluster_symmetry();
        }
      }
      while(choose.next_permute());
    }
    sort();
    get_index();
    get_hierarchy();

  }

  //********************************************************************

  /// Assumes the pivot lattice is already set
  template<typename ClustType>
  void GenericOrbitree<ClustType>::from_json(const jsonParser &json) {
    try {

      // Read the lattice first

      // Lattice lattice;
      CASM::from_json(lattice, json["lattice"]);


      //std::cout<<"Set the lattice"<<std::endl;
      // template<typename ClustType>
      // class GenericOrbitree : public Array< GenericOrbitBranch<ClustType> >
      //std::cout<<"Number of branches:"<<json["branches"].size()<<std::endl;
      resize(json["branches"].size());
      for(int i = 0; i < json["branches"].size(); i++) {
        //std::cout<<"Working on branch:"<<i<<std::endl;
        CASM::from_json(at(i), json["branches"][i]);
      }

      // int max_num_sites, min_num_components;
      CASM::from_json(max_num_sites, json["max_num_sites"]);
      CASM::from_json(min_num_components, json["min_num_components"]);

      // Array<double> max_length;
      CASM::from_json(max_length, json["max_length"]);

      // double min_length;
      CASM::from_json(min_length, json["min_length"]);

      // Array<int> num_clusts;
      CASM::from_json(num_clusts, json["num_clusts"]);

      // mutable Array<int> index_to_row, index_to_column;
      CASM::from_json(index_to_row, json["index_to_row"]);
      CASM::from_json(index_to_column, json["index_to_column"]);

      // mutable Array< Array<int> > index;
      CASM::from_json(index, json["index"]);

      // mutable int Norbits;
      CASM::from_json(Norbits, json["Norbits"]);

      // mutable Array< Array<int> > subcluster;
      CASM::from_json(subcluster, json["subcluster"]);

      std::cerr << "WARNING in GenericOrbitree<ClustType>::from_json "
                << "I HOPE YOU ARE NOT USING THIS AS A STAND ALONE RO"
                << "UTINE. Use it only as part of "
                << "GenericOrbitree<ClustType>::read_orbitree_from_json";
    }
    catch(...) {
      /// re-throw exceptions
      throw;
    }
  }

};
