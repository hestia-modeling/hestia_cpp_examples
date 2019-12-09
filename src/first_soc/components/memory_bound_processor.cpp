
#include "memory_bound_processor.h"

#include <hestia/memory/memory_manager.h>
#include <functional/functional_processor_library.h>


MemoryBoundProcessor::MemoryBoundProcessor(const hestia::ComponentInit &init) :
        hestia::Manageable(hestia::FrameworkType::COMPONENT, init.name),
        hestia::ComponentBase(init),
        // Ports
        m_doorbell(CreatePortInit("doorbell")),
        m_instruction_fetch(CreatePortInit("instruction_request")),
        m_instruction_return(CreatePortInit("instruction_response")),
        m_data_request(CreatePortInit("data_request")),
        m_data_return(CreatePortInit("data_response")),
        // Internal Connections
        m_decoded_instruction_fifo("decoded_instruction", this, m_init),
        // Handlers
        m_doorbell_handler("doorbell_handler", this, m_init),
        m_instruction_return_handler("instruction_return", this, m_init),
        m_operand_back_pressure_handler("operand_back_pressure_handler", this, m_init),
        m_operand_response_handler("operand_response_handler", this, m_init),
        m_write_back_back_pressure_handler("write_back_back_pressure_handler", this, m_init),
        // Functional Library
        m_functional_library(init.name + ".functional", m_init),
        // Counters
        m_memory_fetches("memory_fetches", this, m_init),
        m_doorbell_rings("doorbell_rings", this, m_init)  {

    m_doorbell_handler.SetHandler(m_init, std::bind(&MemoryBoundProcessor::CheckDoorbell, this));
    m_doorbell_handler << m_doorbell;

    m_instruction_return_handler.SetHandler(m_init, std::bind(&MemoryBoundProcessor::InstructionReturn, this));
    m_instruction_return_handler << m_instruction_return;

    m_operand_response_handler.SetHandler(m_init, std::bind(&MemoryBoundProcessor::OperandReturn, this));
    m_operand_response_handler << m_data_return;
    m_operand_response_handler << m_decoded_instruction_fifo.GetReadable();

    m_operand_back_pressure_handler.SetHandler(m_init, std::bind(&MemoryBoundProcessor::SendOperandRequests, this));

    m_write_back_back_pressure_handler.SetHandler(m_init, std::bind(&MemoryBoundProcessor::SendWriteBackRequests, this));

}

void MemoryBoundProcessor::CheckDoorbell() {
    // Read our doorbell
    ++m_doorbell_rings;
    m_functional_library.SetApplicationStart(m_doorbell.Read());
    ++m_memory_fetches;
    m_instruction_fetch.Write(m_functional_library.Fetch());
}

void MemoryBoundProcessor::InstructionReturn() {
    // Loop until unable to do any more work whether from stall or starve
    while (m_instruction_return.ReadValid() && m_decoded_instruction_fifo.WriteValid()) {
        auto response = m_instruction_return.Read();
        auto instruction = m_functional_library.Decode(response);
        // If instruction is end program just finish processing it.
        if(instruction.opcode == Opcode::ENDPRGM) {
            m_functional_library.Execute(instruction);
            m_functional_library.WriteBack(instruction);
        } else {
            m_operand_requests = std::move(m_functional_library.GatherOperands(instruction));
            SendOperandRequests();
            m_decoded_instruction_fifo.Write(instruction);
        }
    }
    // Check to see if we were back pressured and if so tell fifo to notify us when relieved.
    if(m_instruction_return.ReadValid() && !m_decoded_instruction_fifo.WriteValid()) {
        m_decoded_instruction_fifo.NotifyOnWriteable(m_instruction_return_handler.GetId());
    }
}

void MemoryBoundProcessor::SendOperandRequests() {
    // Loop until unable to do any more work whether from stall or starve
    while (!m_operand_requests.empty() && m_data_request.WriteValid()) {
        ++m_memory_fetches;
        m_data_request.Write(m_operand_requests.front(), m_operand_requests.front().size);
        m_operand_requests.pop_front();
    }
    // Check to see if we were back pressured and if so tell fifo to notify us when relieved.
    if (!m_operand_requests.empty()) {
        m_data_request.NotifyOnWriteable(m_operand_back_pressure_handler.GetId());
    }
}

void MemoryBoundProcessor::OperandReturn() {
    std::deque<hestia::MemoryResponse> responses{};
    // Gather up all of the operand responses
    while(m_data_return.ReadValid()) {
        responses.emplace_back(m_data_return.Read());
    }
    m_functional_library.ProcessOperandMemoryResponses(m_decoded_instruction_fifo.Peek(), responses);
    // If we have gathered our operands Execute the instruction
    if (m_decoded_instruction_fifo.ReadValid() && m_decoded_instruction_fifo.Peek().OperandsGathered()) {
        auto instruction = m_decoded_instruction_fifo.Read();
        m_functional_library.Execute(instruction);
        m_write_back_requests = std::move(m_functional_library.WriteBack(instruction));
        if(m_write_back_requests.empty()) {
            ++m_memory_fetches;
            m_instruction_fetch.Write(m_functional_library.Fetch());
        } else {
            SendWriteBackRequests();
        }
    }
}

void MemoryBoundProcessor::SendWriteBackRequests() {
    bool did_work = false;
    while(!m_write_back_requests.empty() && m_data_request.WriteValid()) {
        ++m_memory_fetches;
        m_data_request.Write(m_write_back_requests.front(), m_write_back_requests.front().size);
        m_write_back_requests.pop_front();
        did_work = true;
    }
    // Check to see if we processed all the results if not we were back pressured
    if (!m_write_back_requests.empty()) {
        m_data_request.NotifyOnWriteable(m_write_back_back_pressure_handler.GetId());
    } else if(did_work) { // If we did work and finished the instruction fetch the next one
        ++m_memory_fetches;
        m_instruction_fetch.Write(m_functional_library.Fetch());
    }
}
