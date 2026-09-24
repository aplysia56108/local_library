#include <iostream>
#include <string>
#include <vector>

std::string escape_json_string(const std::string& input) {
  std::string output;
  output.reserve(input.size() * 2);

  for (char c : input) {
    switch (c) {
      case '\\':
        output += "\\\\";
        break;
      case '"':
        output += "\\\"";
        break;
      case '\t':
        output += "\\t";
        break;
      default:
        output += c;
        break;
    }
  }
  return output;
}

int main() {
  std::ios_base::sync_with_stdio(false);
  std::cin.tie(nullptr);

  std::string line;
  std::vector<std::string> lines;

  while (std::getline(std::cin, line)) {
    lines.push_back(escape_json_string(line));
  }

  std::cout << "\"body\": [\n";
  for (size_t i = 0; i < lines.size(); ++i) {
    std::cout << "  \"" << lines[i] << "\"";
    if (i + 1 < lines.size()) {
      std::cout << ",";
    }
    std::cout << "\n";
  }
  std::cout << "]\n";

  return 0;
}