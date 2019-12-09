#ifndef FIRST_SOC_MEMORY_PROCESSOR_H
#define FIRST_SOC_MEMORY_PROCESSOR_H

#include "functional/transactions/instruction.h"

#include <hestia/component/component_base.h>

#include <hestia/connection/transaction_handler.h>
#include <hestia/memory/i_memory.h>
#include <hestia/toolbox/transactions/memory_request.h>
#include <hestia/toolbox/transactions/memory_response.h>
#include <hestia/port/read_port.h>
#include <hestia/port/write_port.h>
#include <hestia/toolbox/connections/fifo.h>
#include <functional/functional_processor_library.h>

/**
 * Similar to our FunctionalProcessor, but with the addition of adding real memory requests to a memory module
 * instead of directly accessing memory.
 */
class MemoryBoundProcessor : public hestia::ComponentBase {
public:

    explicit MemoryBoundProcessor(const hestia::ComponentInit& init);
    ~MemoryBoundProcessor() override = default;

    [[nodiscard]] bool Validate() const noexcept override { return true; }

private:

    // Ports

    hestia::ReadPort<hestia::IMemory::Address> m_doorbell;
    hestia::WritePort<hestia::MemoryRequest> m_instruction_fetch;
    hestia::ReadPort<hestia::MemoryResponse> m_instruction_return;
    hestia::WritePort<hestia::MemoryRequest> m_data_request;
    hestia::ReadPort<hestia::MemoryResponse> m_data_return;

    // Internal Connections
    hestia::Fifo<Instruction> m_decoded_instruction_fifo;

    // Handlers
    hestia::TransactionHandler m_doorbell_handler;
    void CheckDoorbell();
    hestia::TransactionHandler m_instruction_return_handler;
    void InstructionReturn();
    hestia::TransactionHandler m_operand_back_pressure_handler;
    hestia::TransactionHandler m_operand_response_handler;
    void OperandReturn();
    hestia::TransactionHandler m_write_back_back_pressure_handler;

    // Functional Library
    FunctionalProcessorLibrary m_functional_library;

    // Bookkeeping logic

    std::deque<hestia::MemoryRequest> m_operand_requests; /*!< Outstanding operand requests in case of back pressure */
    std::deque<hestia::MemoryRequest> m_write_back_requests; /*!< Outstanding write back requests in case of back pressure */

    // Counters
    hestia::Counter m_memory_fetches;
    hestia::Counter m_doorbell_rings;

    // Utility Functions
    void SendOperandRequests();
    void SendWriteBackRequests();

};


#endif //FIRST_SOC_MEMORY_PROCESSOR_H
