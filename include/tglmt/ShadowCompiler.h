#pragma once

#include <string>
#include <utility>
#include <vector>

namespace tglmt::gl {

bool ShadowCompilerEnabled();
void ShadowValidateCompile(const std::string& source, unsigned stage, unsigned shaderId);
void ShadowValidateLink(const std::vector<std::pair<unsigned, std::string>>& stages);

}
