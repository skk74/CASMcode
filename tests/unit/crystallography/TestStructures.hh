#ifndef CASMtest_TestStructures
#define CASMtest_TestStructures

#include "casm/crystallography/BasicStructure.hh"
#include "casm/crystallography/Site.hh"
#include "casm/crystallography/Molecule.hh"

using namespace CASM;

namespace test {

  inline BasicStructure<Site> ZrO_prim() {

    // lattice vectors as rows
    Eigen::Matrix3d lat;
    lat << 3.233986860000, 0.000000000000, 0.000000000000,
        -1.616993430000, 2.800714770000, 0.000000000000,
        0.000000000000, 0.000000000000, 5.168678340000;

    BasicStructure<Site> struc(Lattice(lat.transpose()));
    struc.set_title("ZrO");

    Molecule O = Molecule::make_atom("O");
    Molecule Zr = Molecule::make_atom("Zr");
    Molecule Va = Molecule::make_vacancy();

    struc.push_back(Site(Coordinate(Eigen::Vector3d::Zero(), struc.lattice(), FRAC), {Zr}));
    struc.push_back(Site(Coordinate(Eigen::Vector3d(2. / 3., 1. / 3., 1. / 2.), struc.lattice(), FRAC), {Zr}));
    struc.push_back(Site(Coordinate(Eigen::Vector3d(1. / 3., 2. / 3., 1. / 4.), struc.lattice(), FRAC), {Va, O}));
    struc.push_back(Site(Coordinate(Eigen::Vector3d(1. / 3., 2. / 3., 3. / 4.), struc.lattice(), FRAC), {Va, O}));

    return struc;
  }

  inline BasicStructure<Site> FCC_ternary_prim() {

    // lattice vectors as cols
    Eigen::Matrix3d lat;
    lat << 0.0, 2.0, 2.0,
        2.0, 0.0, 2.0,
        2.0, 2.0, 0.0;

    BasicStructure<Site> struc {Lattice{lat}};
    struc.set_title("FCC_ternary");

    Molecule A = Molecule::make_atom("A");
    Molecule B = Molecule::make_atom("B");
    Molecule C = Molecule::make_atom("C");

    struc.push_back(Site(Coordinate(Eigen::Vector3d::Zero(), struc.lattice(), CART), {A, B, C}));

    return struc;

  }

}

#endif
