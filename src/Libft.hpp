#ifndef LIBFT_HPP
#define LIBFT_HPP

#include <string>

class	Libft {
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

  /**
   * @brief ip string to binary ip representation
   * 
   * equivalent to inet_pton()
   * @param s 
   * @return int 
   */
  static int ft_atoip(const std::string &s);

  /**
   * @brief number to its string repr
   * 
   * @param number 
   * @return std::string 
   */
  static std::string ft_itos(int number);
};

#endif // LIBFT_HPP
