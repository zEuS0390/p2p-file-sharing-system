#ifndef NETWORK_UTILS_HPP
#define NETWORK_UTILS_HPP

#include <sys/poll.h>
#include <vector>

bool removePollFD(std::vector<pollfd>& pollfds, int target);

#endif
