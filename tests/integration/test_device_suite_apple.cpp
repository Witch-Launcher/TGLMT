#include "../../apps/aquarium/device_tests.cpp"
#include "tglmt/Context.h"
#include <cstdio>
#include <string>

int main() {
    tglmt::Context ctx("apple");
    tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_device_suite_apple SKIP: no metal device\n");
        return 0;
    }
    std::string summary = RunDeviceTests(ctx, [](const char* line) {
        printf("%s\n", line);
    });
    if (summary.find(" 0 fail,") == std::string::npos) {
        printf("test_device_suite_apple FAIL: %s\n", summary.c_str());
        return 1;
    }
    printf("test_device_suite_apple PASS\n");
    return 0;
}
