/*
 *  version.cpp
 *
 */

#include "casm/version/version.hh"

using namespace CASM;

const std::string &CASM::version() {
  static const std::string &ver = "v0.1.1_query_operators";
  return ver;
};