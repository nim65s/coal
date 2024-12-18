//
// Copyright (c) 2017-2024 CNRS INRIA
// This file was borrowed from the Pinocchio library:
// https://github.com/stack-of-tasks/pinocchio
//

#ifndef COAL_SERIALIZATION_ARCHIVE_H
#define COAL_SERIALIZATION_ARCHIVE_H

#include "coal/fwd.hh"

#include <boost/serialization/nvp.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <fstream>
#include <string>
#include <sstream>
#include <stdexcept>

#if BOOST_VERSION / 100 % 1000 == 78 && __APPLE__
// See https://github.com/qcscine/utilities/issues/5#issuecomment-1246897049 for
// further details

#ifndef BOOST_ASIO_DISABLE_STD_ALIGNED_ALLOC
#define DEFINE_BOOST_ASIO_DISABLE_STD_ALIGNED_ALLOC
#define BOOST_ASIO_DISABLE_STD_ALIGNED_ALLOC
#endif

#include <boost/asio/streambuf.hpp>

#ifdef DEFINE_BOOST_ASIO_DISABLE_STD_ALIGNED_ALLOC
#undef BOOST_ASIO_DISABLE_STD_ALIGNED_ALLOC
#endif

#else
#include <boost/asio/streambuf.hpp>
#endif

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>

// Handle NAN inside TXT or XML archives
#include <boost/math/special_functions/nonfinite_num_facets.hpp>

namespace coal {
namespace serialization {

///
/// \brief Loads an object from a TXT file.
///
/// \tparam T Type of the object to deserialize.
///
/// \param[out] object Object in which the loaded data are copied.
/// \param[in]  filename Name of the file containing the serialized data.
///
template <typename T>
inline void loadFromText(T& object, const std::string& filename) {
  std::ifstream ifs(filename.c_str());
  if (ifs) {
    std::locale const new_loc(ifs.getloc(),
                              new boost::math::nonfinite_num_get<char>);
    ifs.imbue(new_loc);
    boost::archive::text_iarchive ia(ifs, boost::archive::no_codecvt);
    ia >> object;
  } else {
    const std::string exception_message(filename +
                                        " does not seem to be a valid file.");
    throw std::invalid_argument(exception_message);
  }
}

///
/// \brief Saves an object inside a TXT file.
///
/// \tparam T Type of the object to deserialize.
///
/// \param[in]  object Object in which the loaded data are copied.
/// \param[in]  filename Name of the file containing the serialized data.
///
template <typename T>
inline void saveToText(const T& object, const std::string& filename) {
  std::ofstream ofs(filename.c_str());
  if (ofs) {
    boost::archive::text_oarchive oa(ofs);
    oa & object;
  } else {
    const std::string exception_message(filename +
                                        " does not seem to be a valid file.");
    throw std::invalid_argument(exception_message);
  }
}

///
/// \brief Loads an object from a std::stringstream.
///
/// \tparam T Type of the object to deserialize.
///
/// \param[out] object Object in which the loaded data are copied.
/// \param[in]  is  string stream constaining the serialized content of the
/// object.
///
template <typename T>
inline void loadFromStringStream(T& object, std::istringstream& is) {
  std::locale const new_loc(is.getloc(),
                            new boost::math::nonfinite_num_get<char>);
  is.imbue(new_loc);
  boost::archive::text_iarchive ia(is, boost::archive::no_codecvt);
  ia >> object;
}

///
/// \brief Saves an object inside a std::stringstream.
///
/// \tparam T Type of the object to deserialize.
///
/// \param[in]   object Object in which the loaded data are copied.
/// \param[out]  ss String stream constaining the serialized content of the
/// object.
///
template <typename T>
inline void saveToStringStream(const T& object, std::stringstream& ss) {
  boost::archive::text_oarchive oa(ss);
  oa & object;
}

///
/// \brief Loads an object from a std::string
///
/// \tparam T Type of the object to deserialize.
///
/// \param[out] object Object in which the loaded data are copied.
/// \param[in]  str  string constaining the serialized content of the object.
///
template <typename T>
inline void loadFromString(T& object, const std::string& str) {
  std::istringstream is(str);
  loadFromStringStream(object, is);
}

///
/// \brief Saves an object inside a std::string
///
/// \tparam T Type of the object to deserialize.
///
/// \param[in] object Object in which the loaded data are copied.
///
/// \returns a string  constaining the serialized content of the object.
///
template <typename T>
inline std::string saveToString(const T& object) {
  std::stringstream ss;
  saveToStringStream(object, ss);
  return ss.str();
}

///
/// \brief Loads an object from a XML file.
///
/// \tparam T Type of the object to deserialize.
///
/// \param[out] object Object in which the loaded data are copied.
/// \param[in] filename Name of the file containing the serialized data.
/// \param[in] tag_name XML Tag for the given object.
///
template <typename T>
inline void loadFromXML(T& object, const std::string& filename,
                        const std::string& tag_name) {
  if (filename.empty()) {
    COAL_THROW_PRETTY("Tag name should not be empty.", std::invalid_argument);
  }

  std::ifstream ifs(filename.c_str());
  if (ifs) {
    std::locale const new_loc(ifs.getloc(),
                              new boost::math::nonfinite_num_get<char>);
    ifs.imbue(new_loc);
    boost::archive::xml_iarchive ia(ifs, boost::archive::no_codecvt);
    ia >> boost::serialization::make_nvp(tag_name.c_str(), object);
  } else {
    const std::string exception_message(filename +
                                        " does not seem to be a valid file.");
    throw std::invalid_argument(exception_message);
  }
}

///
/// \brief Saves an object inside a XML file.
///
/// \tparam T Type of the object to deserialize.
///
/// \param[in] object Object in which the loaded data are copied.
/// \param[in] filename Name of the file containing the serialized data.
/// \param[in] tag_name XML Tag for the given object.
///
template <typename T>
inline void saveToXML(const T& object, const std::string& filename,
                      const std::string& tag_name) {
  if (filename.empty()) {
    COAL_THROW_PRETTY("Tag name should not be empty.", std::invalid_argument);
  }

  std::ofstream ofs(filename.c_str());
  if (ofs) {
    boost::archive::xml_oarchive oa(ofs);
    oa& boost::serialization::make_nvp(tag_name.c_str(), object);
  } else {
    const std::string exception_message(filename +
                                        " does not seem to be a valid file.");
    throw std::invalid_argument(exception_message);
  }
}

///
/// \brief Loads an object from a binary file.
///
/// \tparam T Type of the object to deserialize.
///
/// \param[out] object Object in which the loaded data are copied.
/// \param[in] filename Name of the file containing the serialized data.
///
template <typename T>
inline void loadFromBinary(T& object, const std::string& filename) {
  std::ifstream ifs(filename.c_str(), std::ios::binary);
  if (ifs) {
    boost::archive::binary_iarchive ia(ifs);
    ia >> object;
  } else {
    const std::string exception_message(filename +
                                        " does not seem to be a valid file.");
    throw std::invalid_argument(exception_message);
  }
}

///
/// \brief Saves an object inside a binary file.
///
/// \tparam T Type of the object to deserialize.
///
/// \param[in] object Object in which the loaded data are copied.
/// \param[in] filename Name of the file containing the serialized data.
///
template <typename T>
void saveToBinary(const T& object, const std::string& filename) {
  std::ofstream ofs(filename.c_str(), std::ios::binary);
  if (ofs) {
    boost::archive::binary_oarchive oa(ofs);
    oa & object;
  } else {
    const std::string exception_message(filename +
                                        " does not seem to be a valid file.");
    throw std::invalid_argument(exception_message);
  }
}

///
/// \brief Loads an object from a binary buffer.
///
/// \tparam T Type of the object to deserialize.
///
/// \param[out] object Object in which the loaded data are copied.
/// \param[in] buffer Input buffer containing the serialized data.
///
template <typename T>
inline void loadFromBuffer(T& object, boost::asio::streambuf& buffer) {
  boost::archive::binary_iarchive ia(buffer);
  ia >> object;
}

///
/// \brief Saves an object to a binary buffer.
///
/// \tparam T Type of the object to serialize.
///
/// \param[in]  object Object in which the loaded data are copied.
/// \param[out] buffer Output buffer containing the serialized data.
///
template <typename T>
void saveToBuffer(const T& object, boost::asio::streambuf& buffer) {
  boost::archive::binary_oarchive oa(buffer);
  oa & object;
}

}  // namespace serialization
}  // namespace coal

#endif  // ifndef COAL_SERIALIZATION_ARCHIVE_H
