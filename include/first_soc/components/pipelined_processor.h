#ifndef FIRST_SOC_PIPELINE_PROCESSOR_H
#define FIRST_SOC_PIPELINE_PROCESSOR_H

#include "functional/transactions/instruction.h"
#include "timing_devices/pipeline_stage.h"

#include <hestia/component/component_base.h>

#include <hestia/connection/transaction_handler.h>
#include <hestia/memory/i_memory.h>
#include <hestia/toolbox/transactions/memory_request.h>
#include <hestia/toolbox/transactions/memory_response.h>
#include <hestia/port/read_port.h>
#include <hestia/port/write_port.h>
#include <hestia/toolbox/connections/fifo.h>
#include <functional/functional_processor_library.h>

class PipelinedProcessor : public hestia::ComponentBase {
public:

    explicit PipelinedProcessor(const hestia::ComponentInit& init);
    ~PipelinedProcessor() override = default;

    [[nodiscard]] bool Validate() const noexcept override { return true; }

private:

    void SendOperandRequests();
    void SendWriteBackRequests();

    // Ports

    // Doorbell port where we will be told where to fetch our application
    hestia::ReadPort<hestia::IMemory::Address> m_doorbell;
    hestia::WritePort<hestia::MemoryRequest> m_instruction_fetch;
    hestia::ReadPort<hestia::MemoryResponse> m_instruction_return;
    hestia::WritePort<hestia::MemoryRequest> m_data_request;
    hestia::ReadPort<hestia::MemoryResponse> m_data_return;

    // Internal Connections
    PipelineStage<hestia::MemoryRequest> m_fetcher;
    PipelineStage<hestia::MemoryResponse> m_decoder;
    PipelineStage<Instruction> m_executor;
    PipelineStage<Instruction> m_write_back;


    // Handlers
    hestia::TransactionHandler m_doorbell_handler;
    void CheckDoorbell();
    hestia::TransactionHandler m_fetcher_back_pressure_handler;
    hestia::TransactionHandler m_fetcher_handler;
    void Fetch();
    hestia::TransactionHandler m_instruction_return_handler;
    void InstructionReturn();
    hestia::TransactionHandler m_decoder_handler;
    void Decode();
    hestia::TransactionHandler m_operand_back_pressure_handler;
    hestia::TransactionHandler m_operand_response_handler;
    void OperandReturn();
    hestia::TransactionHandler m_executor_handler;
    void Execute();
    hestia::TransactionHandler m_write_back_handler;
    void WriteBack();
    hestia::TransactionHandler m_write_back_back_pressure_handler;

    // Functional Library
    FunctionalProcessorLibrary m_functional_library;

    // Bookkeeping logic

    std::deque<hestia::MemoryRequest> m_operand_requests;
    std::deque<hestia::MemoryRequest> m_write_back_requests;
    std::deque<hestia::IMemory::Address> m_destination_registers;
    std::deque<hestia::IMemory::Address> m_destination_addresses;

    bool application_terminated = true;

    // Counters
    hestia::Counter m_memory_fetches;
    hestia::Counter m_doorbell_rings;

    void ProcessFetch();

    bool HazardCheck(const Instruction &instruction);
};


#endif //FIRST_SOC_PIPELINE_PROCESSOR_H
