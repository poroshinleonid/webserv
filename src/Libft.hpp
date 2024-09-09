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
  static unsigned long ft_atoip(const std::string &s);

  /**
   * @brief number to its string repr
   * 
   * @param number 
   * @return std::string 
   */
  static std::string ft_itos(int number);

  /**
   * @brief ip from binary to string repr
   * 
   * @param s 
   * @return int 
   */
  static std::string ft_iptoa(int ip_repr);
  
  /**
   * @brief std::memset
   * 
   * @param dst 
   * @param len 
   * @param byte 
   */
  static void ft_memset(void *dst, int len, unsigned char byte);

};

#endif // LIBFT_HPP
