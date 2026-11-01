#pragma once
#include <functional>
#include <string>

namespace tglmt { class Context; }

std::string RunDeviceTests(tglmt::Context& ctx,
                           const std::function<void(const char*)>& log);
