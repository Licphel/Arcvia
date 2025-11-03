#pragma once
#include <string>

#include "core/def.h"

namespace arc {

void cvt_u32_(const std::string& u8_str, std::u32string* u32_tmp);
void cvt_u8_(const std::u32string& u32_str, std::string* u8_tmp);

}  // namespace arc
