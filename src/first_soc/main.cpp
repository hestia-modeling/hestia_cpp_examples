#include "components/memory_bound_processor.h"
#include "components/functional_processor.h"
#include "components/performant_processor.h"
#include "components/pipelined_processor.h"
#include "applications/simple_application.h"
#include "applications/loop_application.h"
#include "observers/doorbell.h"

#include <hestia/toolbox/components/memory.h>
#include <hestia/testbench/cpp_test_bench.h>

int main(int argc, char* argv[]) {
    // Instantiate our test bench
    hestia::CppTestBench test_bench{};
    test_bench.AddComponentFactories({
        {"functional_processor", hestia::CreateComponent<FunctionalProcessor>},
        {"memory_bound_processor", hestia::CreateComponent<MemoryBoundProcessor>},
        {"performant_processor", hestia::CreateComponent<PerformantProcessor>},
        {"pipelined_processor", hestia::CreateComponent<PipelinedProcessor>},
        {"simple_driver", hestia::CreateComponent<SimpleApplication>},
        {"loop_driver", hestia::CreateComponent<LoopApplication>},
        {"memory", hestia::CreateComponent<hestia::MemoryComponent>}
    });
    test_bench.AddObserverFactories({
        {"doorbell", hestia::CreateObserver<DoorbellDumper>}
    });

    bool build_functional = argc == 1;
    bool build_memory_bound = argc == 2;
    bool build_performant = argc == 3;
    bool build_pipelined = argc == 4;

    test_bench.AddDomain("clk", 1);
    const std::string memory_name = "mem";
    test_bench.CreateMemory(memory_name, {hestia::MemoryParameters::Type::LINEAR, 1024});

    // Names of our components
    const std::string processor_name = "processor";
    const std::string application_name = "simple_application";
    const std::string memory_component_name = "ram";

    test_bench.SetParameter(hestia::FrameworkType::COMPONENT, application_name, "memory_name", memory_name);
    test_bench.SetParameter(hestia::FrameworkType::COMPONENT, application_name, "num_ops_per_iteration", "5");
    test_bench.SetParameter(hestia::FrameworkType::COMPONENT, application_name, "num_iterations", "2");
    test_bench.SetParameter(hestia::FrameworkType::COMPONENT, application_name, "mode", "memory");

    test_bench.SetParameter(hestia::FrameworkType::COMPONENT, processor_name + ".functional", "num_registers", "10");
    test_bench.SetParameter(hestia::FrameworkType::COMPONENT, processor_name, "memory_name", memory_name);
    test_bench.SetParameter(hestia::FrameworkType::COMPONENT, memory_component_name, "memory_name", memory_name);

    hestia::ConnectionParameters connection_parameters{};
    connection_parameters.is_timed = true;
    connection_parameters.domain = "clk";
    connection_parameters.is_observable = true;

    // Create our components
    if(build_functional) {
        test_bench.CreateComponent("functional_processor", processor_name);
    } else {
        if (build_memory_bound) {
            test_bench.CreateComponent("memory_bound_processor", processor_name);
        } else {
            test_bench.SetConnectionParameters("processor.fetcher.processor.fetcher", connection_parameters);
            test_bench.SetConnectionParameters("processor.decoder.processor.decoder", connection_parameters);
            test_bench.SetConnectionParameters("processor.executor.processor.executor", connection_parameters);
            test_bench.SetConnectionParameters("processor.write_back.processor.write_back", connection_parameters);
            if(build_performant) {
                test_bench.CreateComponent("performant_processor", processor_name);
            } else {
                test_bench.CreateComponent("pipelined_processor", processor_name);
            }
        }
        test_bench.CreateComponent("memory", memory_component_name);
    }

    test_bench.CreateComponent("loop_driver", application_name);

    test_bench.CreateConnection(application_name, "doorbell", processor_name, "doorbell", connection_parameters);
    if(!build_functional) {
        test_bench.CreateConnection(processor_name, "instruction_request", memory_component_name, "requests", connection_parameters);
        test_bench.CreateConnection(processor_name, "data_request", memory_component_name, "requests", connection_parameters);
        test_bench.CreateConnection(memory_component_name, "responses", processor_name, "instruction_response", connection_parameters);
        test_bench.CreateConnection(memory_component_name, "responses", processor_name, "data_response", connection_parameters);
    }
    const std::string sampler_name = "csv_sampler";
    if(build_functional) {
        test_bench.SetParameter(hestia::FrameworkType::SAMPLER, sampler_name, "file", "functional_counters.csv");
    } else if (build_memory_bound) {
        test_bench.SetParameter(hestia::FrameworkType::SAMPLER, sampler_name, "file", "memory_counters.csv");
    } else if (build_performant) {
        test_bench.SetParameter(hestia::FrameworkType::SAMPLER, sampler_name, "file", "performant_counters.csv");
    } else {
        test_bench.SetParameter(hestia::FrameworkType::SAMPLER, sampler_name, "file", "pipelined_counters.csv");
    }

    test_bench.SetParameter(hestia::FrameworkType::SAMPLER, sampler_name, "domain", "clk");
    test_bench.SetParameter(hestia::FrameworkType::SAMPLER, sampler_name, "sample_rate", "5");
    test_bench.CreateSampler(sampler_name);
    test_bench.AttachCountersToSampler(sampler_name, ".*instructions.*");

    test_bench.CreateSink("console_sink");
    test_bench.AttachLoggerToSink(hestia::FrameworkType::COMPONENT, processor_name + ".functional", "console_sink");

    hestia::SinkParameters instruction_sink_params;
    instruction_sink_params.type = hestia::SinkType::FILE;
    instruction_sink_params.file = "instructions.log";

    test_bench.CreateSink("instruction_sink", instruction_sink_params);
    test_bench.AttachLoggerToSink(hestia::FrameworkType::COMPONENT, processor_name + ".functional.instructions", "instruction_sink");

    // Validate the design
    if(!test_bench.Validate()) {
        printf("Model failed to validate");
        exit(1);
    }

    // Give everything a chance to setup
    test_bench.Setup();

    // Clock until no longer busy
    while (test_bench.Clock(1)) {}

    // Tear down the design
    test_bench.TearDown();

    return 0;
}