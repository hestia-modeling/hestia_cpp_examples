#include "components/number_consumer.h"
#include "components/number_producer.h"

#include <hestia/toolbox/testbenches/cpp_test_bench.h>

int main(int argc, char* argv[]) {
    // Instantiate our test bench
    hestia::CppTestBench test_bench{{
         {"consumer", hestia::CreateComponent<NumberConsumer>},
         {"producer", hestia::CreateComponent<NumberProducer>}
     },
     {}
    };

    // Lets add a clock
    test_bench.AddDomain("clk", 10);

    // Names of our components
    const std::string consumer_name = "consumer";
    const std::string producer_name = "producer";

    // Configure our producer
    test_bench.SetParameter(hestia::FrameworkType::COMPONENT, producer_name, "num_transactions", "100");
    test_bench.SetParameter(hestia::FrameworkType::COMPONENT, producer_name, "min_number", "0");
    test_bench.SetParameter(hestia::FrameworkType::COMPONENT, producer_name, "max_number", "100");

    // Create our components
    test_bench.CreateComponent("consumer", consumer_name);
    test_bench.CreateComponent("producer", producer_name);

    // Connect our components
    hestia::ConnectionParameters connection_params{};
    connection_params.is_timed = true;
    connection_params.domain = "clk";
    test_bench.CreateConnection(producer_name, "out", consumer_name, "in", connection_params);

    // Validate the design
    test_bench.Validate();

    // Give everything a chance to setup
    test_bench.Setup();

    // Clock until no longer busy
    while (test_bench.Clock(1)) {}

    // Tear down the design
    test_bench.TearDown();

    return 0;
}