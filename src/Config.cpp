#include "Config.hpp"

#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

using std::string;
using std::vector;

Config::Config(const std::string &filename) : depth_(0) {
  std::fstream config_file;
  config_file.open(filename.c_str());
  if (!config_file.is_open()) {
    throw InvalidConfig("Could not open config file: " + filename);
  }
  std::stringstream stream;
  stream << config_file.rdbuf();
  content_ = remove_spaces(stream.str());
  throw_if_invalid();
}

Config::Config() : Config("{}", true) {}

Config::Config(const Config& other): content_(other.content_) {}

Config Config::operator[](const std::string &key) {
  try {
    get_value(key);
  } catch (InvalidConfig &) {
    throw std::out_of_range("Trying to search for a key: \"" + key +
                            "\" in a string that is not an object");
  }
  if (values_found_.empty()) {
    throw std::out_of_range("Key " + key + " not found");
  }
  return Config(values_found_.back(), true);
}

vector<Config> Config::get_vec(const std::string &key) {
  try {
    get_value(key);
  } catch (InvalidConfig &) {
    throw std::out_of_range("Trying to search for a key: \"" + key +
                            "\" in a string that is not an object");
  }
  std::vector<Config> res;
  if (values_found_.empty()) {
    return res;
    // throw std::out_of_range("Key " + key + " not found");
  }
  for (size_t i = 0; i < values_found_.size(); i++) {
    res.push_back(Config(values_found_[i], true));
  }
  return res;
}

std::string Config::unwrap() {
  try {
    throw_if_invalid();
    throw std::invalid_argument(
        "Config unwrap failed - content is not a string: " + content_);
  } catch (InvalidConfig &) {
    return content_;
  }
}

std::string Config::get_content() { return content_; }

// '  {  "a b"   : "c d"  } ' -> '{"a b":"c d"}'
string Config::remove_spaces(const string &s) {
  string res = "";
  bool in_quote = false;
  for (std::string::const_iterator it = s.begin(); it < s.end(); ++it) {
    if (*it == '"')
      in_quote = !in_quote;
    if (in_quote || !isspace(static_cast<unsigned char>(*it)))
      res += *it;
  }
  return res;
}

Config::Config(const std::string &content, bool /* dummy */)
    : depth_(0), content_(content) {}

void Config::throw_if_invalid() {
  if (eat_obj(content_) != "")
    throw InvalidConfig("invalid content after }");
}

void Config::get_value(const std::string &s) {
  key_found_ = "";
  values_found_.clear();
  key_to_find_ = s;
  depth_ = 0;
  eat_obj(content_);
}

string Config::eat_obj(const string &s) {
  if (s.empty() || s[0] != '{') {
    throw InvalidConfig("Missing value");
  }
  bool in_quote = false;
  int open_curly = 1;
  for (size_t i = 1; i < s.length(); i++) {
    if (s[i] == '"')
      in_quote = !in_quote;
    if (!in_quote && s[i] == '{')
      open_curly++;
    if (!in_quote && s[i] == '}')
      open_curly--;
    if (open_curly == 0) {
      if (depth_ == 1 && key_found_ == key_to_find_) {
        values_found_.push_back(s.substr(0, i + 1));
      }
      search_linklist(s.substr(1, i - 1));
      return s.substr(i + 1);
    }
  }
  throw InvalidConfig("Missing closing curly bracket");
}

void Config::search_linklist(string s) {
  depth_++;
  while (!s.empty()) {
    s = eat_link(s);
    if (s.empty()) {
      depth_--;
      return;
    }
    if (s[0] != ',')
      throw InvalidConfig("Missing comma");
    s = s.substr(1);
  }
  throw InvalidConfig("Missing key-value pair after comma");
}

string Config::eat_link(string s) {

  if (s.empty() || s[0] != '"')
    throw InvalidConfig("Missing opening double quote");
  size_t i = s.find('"', 1);
  if (i == s.npos)
    throw InvalidConfig("Missing closing double quote");
  key_found_ = s.substr(1, i - 1);
  if (key_found_.length() == 0)
    throw InvalidConfig("Empty key");
  s = s.substr(i + 1);
  if (s.empty() || s[0] != ':')
    throw InvalidConfig("Missing value");
  return eat_value(s.substr(1));
}

string Config::eat_value(const string &s) {
  if (s.empty())
    throw InvalidConfig("Missing value");
  if (s[0] == '"') {
    int i = s.find('"', 1);
    if (depth_ == 1 && key_found_ == key_to_find_) {
      values_found_.push_back(s.substr(1, i - 1));
      key_found_ = "";
    }
    return s.substr(i + 1);
  }
  return eat_obj(s);
}

bool Config::key_exists(const std::string &key) {
  return !get_vec(key).empty();
}

std::map<string, string> Config::get_content_dict() {
  std::map<string, string> content;
  bool in_quote = false;
  bool is_key = true;
  int curly_depth = 0;
  std::string key, value;

  for (size_t i = 1; i < content_.size(); ++i) {
    char c = content_[i];
    if (c == '"') {
      in_quote = !in_quote;
      continue;
    }
    if (in_quote) {
      if (is_key) {
        key += c;
      } else {
        value += c;
      }
    } else {
      if (c == ':') {
        is_key = false;
      } else if (c == ',') {
        if (!key.empty() && !value.empty()) {
          content[key] = value;
        }
        key.clear();
        value.clear();
        is_key = true;
      } else if (c == '}') {
        if (!key.empty() && !value.empty()) {
          content[key] = value;
        }
        break;
      } else if (!is_key && !in_quote && c == '{' && value.empty()) {
        curly_depth = 1;
        value += content_[i];
        ++i;
        while (i < content_.size()) {
          std::cout << content_[i];
          if (content_[i] == '{') {
            curly_depth += 1;
          }
          if (content_[i] == '}') {
            curly_depth -= 1;
          }
          value += content_[i];
          if (curly_depth == 0) {
            break;
          }
          ++i;
        }
        continue;
      }
    }
  }
  return content;
}

unsigned long Config::string_to_ip(const std::string &ip_string) {

  if (ip_string == "localhost") {
    return string_to_ip("127.0.0.1");
  }
  if (ip_string == "default") {
    return INADDR_ANY;
  }
  unsigned long s_addr = 0;
  int cur_8_bits;
  char dummy;
  std::istringstream ip_stream(ip_string);

  for (int i = 0; i < 4; ++i) {
    if (!(ip_stream >> cur_8_bits) || cur_8_bits < 0 || cur_8_bits >= 256) {
      throw InvalidConfig("Incorrect ip address: " + ip_string);
    }
    s_addr <<= 8;
    s_addr += cur_8_bits;
    if (!(ip_stream >> dummy) && i < 3) {
      throw InvalidConfig("Incorrect ip address: " + ip_string);
    }
  }
  return s_addr;
}

std::vector<std::string> Config::split_string(const std::string &s) {
  std::vector<std::string> result;
  std::istringstream iss(s);
  std::string tmp_s;
  while (iss >> tmp_s) {
    result.push_back(tmp_s);
  }
  return result;
}

std::string Config::get_value_safely(const std::string &s) {
  if (key_exists(s)) {
    return (*this)[s].get_content();
  }
  return ("");
}
