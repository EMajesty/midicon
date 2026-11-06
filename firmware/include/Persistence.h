#pragma once
#include "Config.h"
namespace midicon {
class Persistence {
  public:
    static bool load(Configuration &);
    static bool save(const Configuration &);
};
} // namespace midicon
