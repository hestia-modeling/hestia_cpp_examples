
#include "performant_processor.h"

#include <hestia/memory/memory_manager.h>
#include <functional/functional_processor_library.h>


PerformantProcessor::PerformantProcessor(const hestia::ComponentInit &init) :
        hestia::Manageable(hestia::FrameworkType::COMPONENT, init.name),
        hestia::ComponentBase(init),
        // Ports
        m_doorbell(CreatePortInit("doorbell")),
        m_instruction_fetch(CreatePortInit("instruction_request")),
        m_instruction_return(CreatePortInit("instruction_response")),
        m_data_request(CreatePortInit("data_request")),
        m_data_return(CreatePortInit("data_response")),
        // Internal Connections
        m_fetcher("fetcher", this, m_init),
        m_decoder("decoder", this, m_init),
        m_executor("executor", this, m_init),
        m_write_back("write_back", this, m_init),
        // Handlers
        m_doorbell_handler("doorbell_handler", this, m_init),
        m_fetcher_back_pressure_handler("fetcher_back_pressure_handler", this, m_init),
        m_fetcher_handler("fetcher_handler", this, m_init),
        m_instruction_return_handler("instruction_return", this, m_init),
        m_decoder_handler("decoder_handler", this, m_init),
        m_operand_back_pressure_handler("operand_back_pressure_handler", this, m_init),
        m_operand_response_handler("operand_response_handler", this, m_init),
        m_executor_handler("executor_handler", this, m_init),
        m_write_back_handler("write_back_handler", this, m_init),
        m_write_back_back_pressure_handler("write_back_back_pressure_handler", this, m_init),
        // Functional Library
        m_functional_library(init.name + ".functional", m_init),
        // Counters
        m_memory_fetches("memory_fetches", this, m_init),
        m_doorbell_rings("doorbell_rings", this, m_init)  {

    m_doorbell_handler.SetHandler(m_init, std::bind(&PerformantProcessor::CheckDoorbell, this));
    m_doorbell_handler << m_doorbell;

    m_fetcher_back_pressure_handler.SetHandler(m_init, std::bind(&PerformantProcessor::Fetch, this));

    m_fetcher_handler.SetHandler(m_init, std::bind(&PerformantProcessor::ProcessFetch, this));
    m_fetcher_handler << m_fetcher.GetReadable();

    m_instruction_return_handler.SetHandler(m_init, std::bind(&PerformantProcessor::InstructionReturn, this));
    m_instruction_return_handler << m_instruction_return;

    m_decoder_handler.SetHandler(m_init, std::bind(&PerformantProcessor::Decode, this));
    m_decoder_handler << m_decoder.GetReadable();

    m_operand_response_handler.SetHandler(m_init, std::bind(&PerformantProcessor::OperandReturn, this));
    m_operand_response_handler << m_data_return;

    m_operand_back_pressure_handler.SetHandler(m_init, std::bind(&PerformantProcessor::SendOperandRequests, this));

    m_executor_handler.SetHandler(m_init, std::bind(&PerformantProcessor::Execute, this));
    m_executor_handler << m_executor.GetReadable();

    m_write_back_handler.SetHandler(m_init, std::bind(&PerformantProcessor::WriteBack, this));
    m_write_back_handler << m_write_back.GetReadable();

    m_write_back_back_pressure_handler.SetHandler(m_init, std::bind(&PerformantProcessor::SendWriteBackRequests, this));

}

void PerformantProcessor::CheckDoorbell() {
    // Read our doorbell
    ++m_doorbell_rings;
    m_functional_library.SetApplicationStart(m_doorbell.Read());
    Fetch();
}

void PerformantProcessor::Fetch() {
    if (m_fetcher.WriteValid()) {
        m_fetcher.Write(m_functional_library.Fetch());
    } else {
        m_fetcher.NotifyOnWriteable(m_fetcher_back_pressure_handler.GetId());
    }
}

void PerformantProcessor::ProcessFetch() {
    while (m_fetcher.ReadValid() && m_instruction_fetch.WriteValid()) {
        ++m_memory_fetches;
        m_instruction_fetch.Write(m_fetcher.Read());
    }
    if (m_fetcher.ReadValid() && !m_instruction_fetch.WriteValid()) {
        m_instruction_fetch.NotifyOnWriteable(m_fetcher_handler.GetId());
    }
}

void PerformantProcessor::InstructionReturn() {
    while (m_instruction_return.ReadValid() && m_decoder.WriteValid()) {
        m_decoder.Write(m_instruction_return.Read());
    }
    if (m_instruction_return.ReadValid() && !m_decoder.WriteValid()) {
        m_decoder.NotifyOnWriteable(m_instruction_return_handler.GetId());
    }
}

void PerformantProcessor::Decode() {
    while (m_decoder.ReadValid() && m_executor.WriteValid()) {
        auto response = m_decoder.Read();
        auto instruction = m_functional_library.Decode(response);
        m_operand_requests = m_functional_library.GatherOperands(instruction);
        m_executor.Write(instruction);
        SendOperandRequests();
    }
    if(m_decoder.ReadValid() && !m_executor.WriteValid()) {
        m_executor.NotifyOnWriteable(m_decoder_handler.GetId());
    }
}

void PerformantProcessor::SendOperandRequests() {
    while (!m_operand_requests.empty() && m_data_request.WriteValid()) {
        ++m_memory_fetches;
        m_data_request.Write(m_operand_requests.front(), m_operand_requests.front().size);
        m_operand_requests.pop_front();
    }
    if (!m_operand_requests.empty()) {
        m_data_request.NotifyOnWriteable(m_operand_back_pressure_handler.GetId());
    }
}

void PerformantProcessor::OperandReturn() {
    std::deque<hestia::MemoryResponse> responses{};
    while(m_data_return.ReadValid()) {
        responses.emplace_back(m_data_return.Read());
    }
    m_functional_library.ProcessOperandMemoryResponses(m_executor.Peek(), responses);
    if (m_executor.Peek().OperandsGathered()) {
        Execute();
    }
}

void PerformantProcessor::Execute() {
    while (m_executor.ReadValid() && m_executor.Peek().OperandsGathered() && m_write_back.WriteValid()) {
        auto instruction = m_executor.Read();
        m_functional_library.Execute(instruction);
        m_write_back.Write(instruction);
    }
    if (m_executor.ReadValid() && m_executor.Peek().OperandsGathered() && !m_write_back.WriteValid()) {
        m_executor.NotifyOnWriteable(m_executor_handler.GetId());
    }
}

void PerformantProcessor::WriteBack() {
    while (m_write_back.ReadValid()) {
        auto instruction = m_write_back.Read();
        m_write_back_requests = std::move(m_functional_library.WriteBack(instruction));
        if(m_write_back_requests.empty() && instruction.opcode != Opcode::ENDPRGM) {
            Fetch();
        } else {
            SendWriteBackRequests();
        }
    }
}

void PerformantProcessor::SendWriteBackRequests() {
    bool did_work = false;
    while(!m_write_back_requests.empty() && m_data_request.WriteValid()) {
        ++m_memory_fetches;
        m_data_request.Write(m_write_back_requests.front(), m_write_back_requests.front().size);
        m_write_back_requests.pop_front();
        did_work = true;
    }
    if (!m_write_back_requests.empty()) {
        m_data_request.NotifyOnWriteable(m_write_back_back_pressure_handler.GetId());
    } else if(did_work) {
        Fetch();
    }
}
