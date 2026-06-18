#define BOOST_TEST_MODULE tunnel_tests

#include <boost/test/included/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

#include "io.h"
#include "utils.h"

BOOST_AUTO_TEST_CASE(parse_port_accepts_valid_values) {
  BOOST_TEST(parse_port("1") == 1);
  BOOST_TEST(parse_port("5555") == 5555);
  BOOST_TEST(parse_port("65535") == 65535);
}

BOOST_AUTO_TEST_CASE(parse_port_rejects_invalid_values) {
  BOOST_CHECK_THROW(parse_port("0"), ParseException);
  BOOST_CHECK_THROW(parse_port("65536"), ParseException);
  BOOST_CHECK_THROW(parse_port("abc"), ParseException);
}

BOOST_AUTO_TEST_CASE(packet_size_round_trip_works) {
  BOOST_TEST(header_from_array(header_to_array(0)) == 0);
  BOOST_TEST(header_from_array(header_to_array(2048)) == 2048);
  BOOST_TEST(header_from_array(header_to_array(4294967295u)) == 4294967295u);
}

BOOST_AUTO_TEST_CASE(client_params_reads_port_and_server) {
  char a0[] = "client";
  char a1[] = "--server";
  char a2[] = "192.168.1.10";
  char a3[] = "--port";
  char a4[] = "7777";
  char* argv[] = {a0, a1, a2, a3, a4};

  ClientParams params(5, argv);

  BOOST_TEST(params.server_ip == "192.168.1.10");
  BOOST_TEST(params.port == 7777);
  BOOST_TEST(params.tun_device == "/dev/net/tun");
}

BOOST_AUTO_TEST_CASE(client_params_reads_tun_device) {
  char a0[] = "client";
  char a1[] = "--tun-device";
  char a2[] = "/dev/custom-tun";
  char* argv[] = {a0, a1, a2};

  ClientParams params(3, argv);

  BOOST_TEST(params.tun_device == "/dev/custom-tun");
}

BOOST_AUTO_TEST_CASE(client_params_rejects_missing_tun_device_value) {
  char a0[] = "client";
  char a1[] = "--tun-device";
  char* argv[] = {a0, a1};

  BOOST_CHECK_THROW(ClientParams(2, argv), ParseException);
}

BOOST_AUTO_TEST_CASE(client_params_rejects_unknown_argument) {
  char a0[] = "client";
  char a1[] = "--wrong";
  char* argv[] = {a0, a1};

  BOOST_CHECK_THROW(ClientParams(2, argv), ParseException);
}

BOOST_AUTO_TEST_CASE(client_params_rejects_missing_server_value) {
  char a0[] = "client";
  char a1[] = "--server";
  char* argv[] = {a0, a1};

  BOOST_CHECK_THROW(ClientParams(2, argv), ParseException);
}

BOOST_AUTO_TEST_CASE(client_params_rejects_invalid_port) {
  char a0[] = "client";
  char a1[] = "--port";
  char a2[] = "abc";
  char* argv[] = {a0, a1, a2};

  BOOST_CHECK_THROW(ClientParams(3, argv), ParseException);
}
