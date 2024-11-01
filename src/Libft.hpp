#pragma once

#include <string>

class Libft {
private:
  Libft();
  Libft(Libft const &other);
  ~Libft();
  Libft &operator=(const Libft &obj);

public:
  /**
   * @brief std::atoi
   *
   * @param s
   * @return int
   */
  static int ft_atoi(const std::string &s);

public:
  /**
   * @brief std::tolower
   *
   * @param s
   * @return int
   */
  static char tolower(int c);

  /**
   * @brief std::toupper
   *
   * @param s
   * @return int
   */
  static char toupper(int c);

  /**
   * @brief http header to env var
   * 
   * @param c 
   * @return char 
   */
  static char toenv(int c);

  /**
   * @brief number to its string repr
   *
   * @param number
   * @return std::string
   */
  static std::string ft_itos(int number);

  /**
   * @brief std::memset
   *
   * @param dst
   * @param len
   * @param byte
   */
  static void ft_memset(void *dst, int len, unsigned char byte);
};
