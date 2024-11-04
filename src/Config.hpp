#pragma once

#include <exception>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include <climits>

using std::string;
using std::vector;

/*
Usage: construct Config from a file, and then call something like
config["key1"].get_vec["key2"][1]["key2"].unwrap() to get values
*/
class Config {
public:
  Config();
  /**
   * @brief Return content string.
   *
   * @throw std::invalid_argument if the content is not a string.
   * @return std::string
   */
  std::string unwrap();

  /**
   * @brief Get the content_ string.
   *
   * @return std::string
   */
  std::string get_content();

  /**
   * @brief Get config by key.
   *
   * If there are multiple entries with the same key, returns the last one.
   * @throw std::out_of_range if the key is not found
   * @param key
   * @return Config
   */
  Config operator[](const std::string &key);

  /**
   * @brief Return vector of configs by key.
   *
   * One Config entry for each key entry.
   * Doesn't throw if the vec is empty.
   * @param key
   * @return vector<Config>
   */
  vector<Config> get_vec(const std::string &key);

  /**
   * @brief Parse a new Config object from a file.
   *
   * @throw Config::InvalidConfig if failed to open/failed to parse.
   * @param filename
   */
  Config(const std::string &filename);

  Config(const Config& other);

  Config& operator=(const Config& other);

  /**
   * @brief Checks if the key is present.
   *
   * @param key
   * @return true
   * @return false
   */
  bool key_exists(const std::string &key);

  /**
   * @brief Convert config to key:value map.
   *
   * @throw Config::InvalidConfig if unable to parse the IP address.
   * @return std::map<string, string>
   */
  std::map<string, string> get_content_dict();

  /**
   * @brief Convert string to ulong ip address.
   *
   * @param ip_string
   * @return unsigned long
   */
  static unsigned long string_to_ip(const std::string &ip_string);

  /**
   * @brief Split string by spaces.
   *
   * @param s
   * @return std::vector<std::string>
   */
  static std::vector<std::string> split_string(const std::string &s);

  /**
   * @brief Get value by key, return empty string if not found.
   *
   * @note Equivalent to (*this)[s].unwrap() but doesn't throw if the key
   * is not found.
   * @param s
   * @return std::string
   */
  std::string get_value_safely(const std::string &s);

private:
  Config(const std::string &content, bool /* dummy */);
  static string remove_spaces(const string &s);
  void throw_if_invalid();
  void get_value(const std::string &s);
  string eat_obj(const string &s);
  void search_linklist(string s);
  string eat_link(string s);
  string eat_value(const string &s);

  int depth_;
  string content_;
  string key_to_find_;
  string key_found_;
  vector<string> values_found_;

public:
  const static constexpr int client_default_max_body_size = (2 << 10) * (2 << 10) * 5; // 32KB
};

class InvalidConfig : public std::runtime_error {
public:
  InvalidConfig(const std::string &msg) : std::runtime_error(msg) {}
};

/*
grammar.

OBJ: {LINKLIST}
VALUE: STR | OBJ
LINK: STR:VALUE
LINKLIST: empty | LINK(,LINK)*
STR: "some text"
*/
