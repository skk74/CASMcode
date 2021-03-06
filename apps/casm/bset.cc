#include "bset.hh"

#include <cstring>

#include "casm_functions.hh"
#include "casm/CASM_classes.hh"

namespace CASM {


  // ///////////////////////////////////////
  // 'clusters' function for casm
  //    (add an 'if-else' statement in casm.cpp to call this)

  int bset_command(int argc, char *argv[]) {

    po::variables_map vm;

    /// Set command line options using boost program_options
    po::options_description desc("'casm bset' usage");
    desc.add_options()
    ("help,h", "Write help documentation")
    ("update,u", "Update basis set")
    ("orbits", "Pretty-print orbit prototypes")
    ("clusters", "Pretty-print all clusters")
    ("force,f", "Force overwrite");

    try {
      po::store(po::parse_command_line(argc, argv, desc), vm); // can throw
      bool call_help = false;

      /** --help option
       */
      if(vm.count("help") || call_help) {
        std::cout << "\n";
        std::cout << desc << std::endl;

        std::cout << "DESCRIPTION" << std::endl;
        std::cout << "    Generate and inspect cluster basis functions. A bspecs.json file should be available at\n"
                  << "        $ROOT/basis_set/$current_bset/bspecs.json\n"
                  << "    Run 'casm format --bspecs' for an example file.\n\n" ;

        return 0;
      }

      po::notify(vm); // throws on error, so do after help in case
      // there are any problems
    }
    catch(po::error &e) {
      std::cerr << desc << std::endl;
      std::cerr << "\nERROR: " << e.what() << std::endl << std::endl;
      return 1;
    }
    catch(std::exception &e) {
      std::cerr << desc << std::endl;
      std::cerr << "\nERROR: "  << e.what() << std::endl;
      return 1;

    }


    fs::path root = find_casmroot(fs::current_path());
    if(root.empty()) {
      std::cout << "Error in 'casm bset': No casm project found." << std::endl;
      return 1;
    }
    fs::current_path(root);

    if(vm.count("update")) {

      // initialize project info
      DirectoryStructure dir(root);
      ProjectSettings set(root);
      Structure prim(read_prim(dir.prim()));

      std::cout << "\n***************************\n" << std::endl;


      if(!fs::is_regular_file(dir.bspecs(set.bset()))) {
        std::cout << "Error in 'casm bset': No basis set specifications file found at: " << dir.bspecs(set.bset()) << std::endl;
        return 1;
      }

      bool any_existing_files = false;

      auto lambda = [&](const fs::path & p) {
        if(fs::exists(p)) {
          if(!any_existing_files) {
            std::cout << "Existing files:\n";
            any_existing_files = true;
          }
          std::cout << "  " << p << "\n";
        }
      };

      lambda(dir.clust(set.bset()));
      lambda(dir.eci_in(set.bset()));
      lambda(dir.clexulator_src(set.name(), set.bset()));
      lambda(dir.clexulator_o(set.name(), set.bset()));
      lambda(dir.clexulator_so(set.name(), set.bset()));
      lambda(dir.prim_nlist(set.bset()));

      std::cout << "\n";

      if(any_existing_files) {
        if(vm.count("force")) {
          std::cout << "Using --force. Will overwrite existing files.\n\n";
          fs::remove(dir.clexulator_src(set.name(), set.bset()));
          fs::remove(dir.clexulator_o(set.name(), set.bset()));
          fs::remove(dir.clexulator_so(set.name(), set.bset()));
          
          std::cout << "\n***************************\n" << std::endl;

        }
        else {
          std::cout << "Exiting due to existing files.  Use --force to force overwrite.\n\n";
          return 1;
        }
      }

      SiteOrbitree tree(prim.lattice());

      try {
        jsonParser bspecs_json;
        bspecs_json.read(dir.bspecs(set.bset()));
        std::string basis_functions = bspecs_json["basis_functions"]["site_basis_functions"].get<std::string>();

        std::cout << "Using " << basis_functions << " site basis functions." << std::endl << std::endl;
        prim.fill_occupant_bases(basis_functions[0]);

        std::cout << "Generating orbitree: \n";
        tree = make_orbitree(prim, bspecs_json);
        std::cout << "  DONE.\n\n";

        tree.collect_basis_info(prim);
        tree.generate_clust_bases();
      }
      catch(std::exception &e) {
        std::cerr << "\n\nError reading: " << dir.bspecs(set.bset()) << std::endl
                  << "               " << e.what() << std::endl;
        throw;
      }

      // -- write eci.in ----------------
      tree.write_eci_in(dir.eci_in(set.bset()).string());

      std::cout << "Wrote: " << dir.eci_in(set.bset()) << "\n\n";


      // -- write clust.json ----------------
      jsonParser clust_json;
      to_json(jsonHelper(tree, prim), clust_json).write(dir.clust(set.bset()));

      std::cout << "Wrote: " << dir.clust(set.bset()) << "\n\n";


      // -- generate and write prim_nlist.json ----------------
      Array<UnitCellCoord> nlist;
      expand_nlist(prim, tree, nlist);

      write_prim_nlist(nlist, dir.prim_nlist(set.bset()));
      std::cout << "Wrote: " << dir.prim_nlist(set.bset()) << "\n\n";


      // -- write global Clexulator
      fs::ofstream outfile;
      outfile.open(dir.clexulator_src(set.name(), set.bset()));
      print_clexulator(prim, tree, nlist, set.global_clexulator(), outfile);
      outfile.close();

      std::cout << "Wrote: " << dir.clexulator_src(set.name(), set.bset()) << "\n\n";

      // -- clear correlations for all configurations

    }
    else if(vm.count("orbits") || vm.count("clusters")) {
      
      DirectoryStructure dir(root);
      ProjectSettings set(root);
      
      if(!fs::exists(dir.clust(set.bset()))) {
        std::cerr << "ERROR: No 'clust.json' file found. Make sure to update your basis set with 'casm bset -u'.\n";
        return 1;
      }
      
      std::cout << "Initialize primclex: " << root << std::endl << std::endl;
      PrimClex primclex(root, std::cout);
      std::cout << "  DONE." << std::endl << std::endl;
      
      primclex.read_global_orbitree(dir.clust(set.bset()));
      
      if( vm.count("orbits")) {
        std::cout << "\n***************************\n" << std::endl;
        primclex.get_global_orbitree().print_proto_clust(std::cout);
        std::cout << "\n***************************\n" << std::endl;
      }
      if( vm.count("clusters")) {
        std::cout << "\n***************************\n" << std::endl;
        primclex.get_global_orbitree().print_full_clust(std::cout);
        std::cout << "\n***************************\n" << std::endl;
      }
    }
    else {
      std::cerr << "\n" << desc << "\n";
    }

    return 0;
  };

}

