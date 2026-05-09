#include <sys/poll.h>
#include <algorithm>
#include <vector>

#include "core/utils.hpp"

bool removePollFD(std::vector<pollfd>& pollfds, int target)
{
  std::vector<pollfd>::iterator it {
    std::find_if(pollfds.begin(), pollfds.end(),
    [&](const pollfd& pfd){
      return target == pfd.fd;
    }
  )};
  if (it != pollfds.end())
  {
    pollfds.erase(it);
    return true;
  }
  return false;
}

