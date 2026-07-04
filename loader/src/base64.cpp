#include "base64.hpp"
#include <cstdint>

std::string base64_encode(const unsigned char *data, size_t len) {
  static const char encoding_table[] = {
      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
      'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
      'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
      'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};
  std::string output;
  output.reserve(((len + 2) / 3) * 4);
  size_t i = 0;
  while (i < len) {
    uint32_t octet_a = i < len ? data[i++] : 0;
    uint32_t octet_b = i < len ? data[i++] : 0;
    uint32_t octet_c = i < len ? data[i++] : 0;

    uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

    output.push_back(encoding_table[(triple >> 3 * 6) & 0x3F]);
    output.push_back(encoding_table[(triple >> 2 * 6) & 0x3F]);
    output.push_back(encoding_table[(triple >> 1 * 6) & 0x3F]);
    output.push_back(encoding_table[(triple >> 0 * 6) & 0x3F]);
  }

  size_t mod = len % 3;
  if (mod == 1) {
    output[output.size() - 1] = '=';
    output[output.size() - 2] = '=';
  } else if (mod == 2) {
    output[output.size() - 1] = '=';
  }

  return output;
}