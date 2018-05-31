#ifndef NETWORK_SGX_EXAMPLE_SERIALIZATION_H
#define NETWORK_SGX_EXAMPLE_SERIALIZATION_H

#include <vector>
#include <cstdint>

namespace c1 {

/**
 * Virtual class to enable classes to subclass from this to make sure they have a serializable() function.
 */
class Serializable {
 public:
  virtual void serialize(std::vector<uint8_t> &working_vec) const = 0;
  virtual ~Serializable() {}
};

/**
 * Serialize a number into working_vec
 * @tparam T type of the number
 * @param working_vec
 * @param number
 */
template<typename T>
void serialize_number(std::vector<uint8_t> &working_vec, T number) {
  for (int i = 0; i < sizeof(T); ++i) {
    working_vec.push_back((number >> 8 * (sizeof(T) - i - 1)) & 0xFF);
  }
}

/**
 * Deserialize a number from working_vec at current position cur and update cur appropriately.
 * @tparam T type of the number
 * @param working_vec
 * @param cur index of the first byte of the number in working_vec
 * @return the deserialized number
 */
template<typename T>
T deserialize_number(const std::vector<uint8_t> &working_vec, size_t &cur) {
  // result.length_ = ((working_vec[cur++]<<24)|(working_vec[cur++]<<16)|(working_vec[cur++]<<8)|(working_vec[cur++]));
  T result = 0;
  for (int i = 0; i < sizeof(T); ++i) {
    result |= (working_vec[cur + i] << 8 * (sizeof(T) - i - 1));
  }
  cur += sizeof(T);
  return result;
}

/**
 * Serialize a vector of Serializable objects.
 * @tparam T type of the elements in the vector
 * @param working_vec the byte vector into which the serialized data is written
 * @param vec the vector to be serialized
 */
template<typename T, typename std::enable_if<std::is_base_of<Serializable, T>::value>::type * = nullptr>
void serialize_vec(std::vector<uint8_t> &working_vec, const std::vector<T> &vec) {
  //std::vector<uint8_t> result;
  serialize_number(working_vec, vec.size());
  for (const auto &elem : vec) {
    elem.serialize(working_vec);
  }
}

/**
 * Deserialize a vector of Serializable objects that have a deserialize(working_vec, cur) function.
 * @tparam T type of the object in the vector
 * @param working_vec vector holding the serialized data
 * @param cur index of the first byte of the serialized vector in working_vec
 * @return the deserialized vector
 */
template<typename T, typename std::enable_if<std::is_base_of<Serializable, T>::value>::type * = nullptr>
std::vector<T> deserialize_vec(const std::vector<uint8_t> &working_vec, size_t &cur) {
  //std::vector<uint8_t> result;
  std::vector<T> result;
  auto size = deserialize_number<typename std::vector<T>::size_type>(working_vec, cur);
  //ocall_print_string(("Size is: " + std::to_string(size)).c_str());
  result.reserve(size);
  for (int i = 0; i < size; ++i) {
    result.emplace_back(T::deserialize(working_vec, cur));
  }
  return result;
}

template<typename T>
T deserialize_number_from_pointer(const std::vector<uint8_t> *working_vec, size_t &cur) {
  // result.length_ = ((working_vec[cur++]<<24)|(working_vec[cur++]<<16)|(working_vec[cur++]<<8)|(working_vec[cur++]));
  T result = 0;
  for (int i = 0; i < sizeof(T); ++i) {
    result |= (*working_vec)[cur + i] << 8 * (sizeof(T) - i - 1);
  }
  cur += sizeof(T);
  return result;
}

template<typename T, typename std::enable_if<std::is_base_of<Serializable, T>::value>::type * = nullptr>
std::vector<T> deserialize_vec_from_pointer(const std::vector<uint8_t> *working_vec, size_t &cur) {
  //std::vector<uint8_t> result;
  std::vector<T> result;
  auto size = deserialize_number_from_pointer<size_t>(working_vec, cur);
  result.reserve(size);
  for (int i = 0; i < size; ++i) {
    result.emplace_back(T::deserialize(*working_vec, cur));
  }
  return result;
}

}

#endif //NETWORK_SGX_EXAMPLE_SERIALIZATION_H
