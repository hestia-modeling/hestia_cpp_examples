#ifndef FIRST_SOC_FUNCTIONAL_PROCESSOR_H
#define FIRST_SOC_FUNCTIONAL_PROCESSOR_H

#include <hestia/component/component_base.h>

#include <functional/functional_processor_library.h>
#include <functional/transactions/instruction.h>

#include <hestia/connection/transaction_handler.h>
#include <hestia/memory/i_memory.h>
#include <hestia/port/read_port.h>

/**
 * Receives a doorbell from its doorbell port and processes the entire
 * application in a single cycle.
 */
class FunctionalProcessor : public hestia::ComponentBase {
public:

    explicit FunctionalProcessor(const hestia::ComponentInit& init);
    ~FunctionalProcessor() override = default;

    [[nodiscard]] bool Validate() const noexcept override { return true; }

private:

    hestia::ReadPort<hestia::IMemory::Address> m_doorbell; /*!< Our Doorbell port >*/

    hestia::IMemory* m_memory; /*!< Our simulated memory model >*/

    hestia::TransactionHandler m_doorbell_handler; /*!< Our Handler for responding to doorbells >*/
    void CheckDoorbell();

    /**
     * Helper functions for fetching data from our simulated memory model
     */
    std::deque<hestia::MemoryResponse> FetchMemory(std::deque<hestia::MemoryRequest>& requests);
    hestia::MemoryResponse FetchMemory(hestia::MemoryRequest& requests);

    /**
     * Our functional library that will handle most of the work
     */
    // Functional Library
    FunctionalProcessorLibrary m_functional_library;

    // Counters
    hestia::Counter m_memory_fetches; /*!< Count how many memory requests we have made >*/
    hestia::Counter m_doorbell_rings; /*!< Count how many doorbell requests we have processed >*/
};

#endif //FIRST_SOC_FUNCTIONAL_PROCESSOR_H