#define BOOST_TEST_MODULE StructureTest
#include <boost/test/included/unit_test.hpp>
#include "../client/trusted/structures/aad_tuple.h"

using namespace boost::unit_test;

BOOST_AUTO_TEST_SUITE(client_test_suite)

BOOST_AUTO_TEST_CASE(aad_serialization_test) {
  c1::client::AadTuple a1{c1::PeerInformation{12, c1::Uri(127, 0, 0, 1, 9999)},
                          c1::PeerInformation{72, c1::Uri(127, 0, 0, 1, 11111)},
                          12,
                          std::vector<c1::client::OverlayStructureSchemeMessage>()
  };
  std::vector<uint8_t> vec;
  a1.serialize(vec);
  size_t cur = 0;
  auto a2 = c1::client::AadTuple::deserialize(vec, cur);
  BOOST_ASSERT(a1 == a2);
}

BOOST_AUTO_TEST_SUITE_END();