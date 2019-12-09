#include <components/number_consumer.h>
#include <components/number_producer.h>

#include <hestia/toolbox/testbenches/cpp_test_bench.h>
#include <hestia/toolbox/testbenches/python/test_bench.h>

class SandboxTestBench : public hestia::CppTestBench {
public:
    SandboxTestBench() : hestia::CppTestBench() {
        AddComponentFactories({
            {"number_consumer", hestia::CreateComponent<NumberConsumer>},
            {"number_producer", hestia::CreateComponent<NumberProducer>}
        });
    }
};

hestia::ITestBench* CreateTestBench() noexcept {
    return new SandboxTestBench();
}
