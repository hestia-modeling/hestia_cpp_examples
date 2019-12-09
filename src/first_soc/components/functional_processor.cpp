#include "functional_processor.h"

#include <hestia/memory/memory_manager.h>

FunctionalProcessor::FunctionalProcessor(const hestia::ComponentInit &init) :
        hestia::Manageable(hestia::FrameworkType::COMPONENT, init.name),
        hestia::ComponentBase(init),
        // Ports
        m_doorbell(CreatePortInit("doorbell")),
        // Handlers
        m_doorbell_handler("doorbell_handler", this, m_init),
        // Functional Library
        m_functional_library(init.name + ".functional", m_init),
        // Memory and Params
        m_memory(m_init.memories->GetMemory(GetParam("memory_name"))),
        // Counters
        m_memory_fetches("memory_fetches", this, m_init),
        m_doorbell_rings("doorbell_rings", this, m_init) {

    m_doorbell_handler.SetHandler(m_init, std::bind(&FunctionalProcessor::CheckDoorbell, this));
    m_doorbell_handler << m_doorbell;
}

void FunctionalProcessor::CheckDoorbell() {
    // Read our doorbell
    m_functional_library.SetApplicationStart(m_doorbell.Read());
    ++m_doorbell_rings;
    // Run until hit ENDPRGRM
    while(true) {
        // Run through our instruction cycle
        // Fetch
        auto instruction_request = m_functional_library.Fetch();
        auto instruction_response = FetchMemory(instruction_request);
        // Decode and Gather
        auto instruction = m_functional_library.Decode(instruction_response);
        auto operand_requests = m_functional_library.GatherOperands(instruction);
        auto operand_responses = FetchMemory(operand_requests);
        m_functional_library.ProcessOperandMemoryResponses(instruction, operand_responses);
        // Execute
        m_functional_library.Execute(instruction);
        // Write back result
        auto write_backs = m_functional_library.WriteBack(instruction);
        FetchMemory(write_backs);
        // Terminate the application if hit end program sequence
        if (instruction.opcode == Opcode::ENDPRGM) {
            break;
        }
    }
}

std::deque<hestia::MemoryResponse> FunctionalProcessor::FetchMemory(std::deque<hestia::MemoryRequest>& requests) {
    std::deque<hestia::MemoryResponse> results;
    for (auto& request : requests) {
        results.emplace_back(std::move(FetchMemory(request)));
    }
    return results;
}

hestia::MemoryResponse FunctionalProcessor::FetchMemory(hestia::MemoryRequest& request) {
    ++m_memory_fetches;
    if(request.type == hestia::MemoryRequest::Type::WRITE) {
        m_memory->Set(request.address, request.data.data(), request.data.size());
    }
    return {std::move(m_memory->Get(request.address, request.size)), request};
}
