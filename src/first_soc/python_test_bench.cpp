#include "components/memory_bound_processor.h"
#include "components/functional_processor.h"
#include "components/performant_processor.h"
#include "components/pipelined_processor.h"
#include "applications/simple_application.h"
#include "applications/loop_application.h"
#include "observers/doorbell.h"

#include <hestia/toolbox/components/memory.h>
#include <hestia/testbench/cpp_test_bench.h>
#include <hestia/testbench/python/test_bench.h>

class ProcessorTestBench : public hestia::CppTestBench {
public:
    ProcessorTestBench() : hestia::CppTestBench() {
        AddComponentFactories({
            {"functional_processor", hestia::CreateComponent<FunctionalProcessor>},
            {"memory_bound_processor", hestia::CreateComponent<MemoryBoundProcessor>},
            {"performant_processor", hestia::CreateComponent<PerformantProcessor>},
            {"pipelined_processor", hestia::CreateComponent<PipelinedProcessor>},
            {"simple_driver", hestia::CreateComponent<SimpleApplication>},
            {"loop_driver", hestia::CreateComponent<LoopApplication>},
            {"memory", hestia::CreateComponent<hestia::MemoryComponent>}
      });
    }
};

hestia::ITestBench* CreateTestBench() noexcept {
    return new ProcessorTestBench();
}
