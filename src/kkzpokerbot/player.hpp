#ifndef __PLAYER_HPP__
#define __PLAYER_HPP__

#include <boost/asio.hpp>

using boost::asio::ip::tcp;

extern std::map<std::string, double> preflop_strs;
extern std::map<std::string, double> flop_avgs;
extern std::map<std::string, double> flop_devs;
extern std::map<std::string, double> turn_avgs;
extern std::map<std::string, double> turn_devs;
extern std::map<std::string, double> river_avgs;
extern std::map<std::string, double> river_devs;;

class Player {
 public:
  Player();
  void run(tcp::iostream &stream);
};

#endif  // __PLAYER_HPP__
