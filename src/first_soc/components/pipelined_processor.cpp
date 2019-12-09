#include "pipelined_processor.h"

#include <hestia/memory/memory_manager.h>
#include <functional/functional_processor_library.h>


PipelinedProcessor::PipelinedProcessor(const hestia::ComponentInit &init) :
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

    m_doorbell_handler.SetHandler(m_init, std::bind(&PipelinedProcessor::CheckDoorbell, this));
    m_doorbell_handler << m_doorbell;

    m_fetcher_back_pressure_handler.SetHandler(m_init, std::bind(&PipelinedProcessor::Fetch, this));

    m_fetcher_handler.SetHandler(m_init, std::bind(&PipelinedProcessor::ProcessFetch, this));
    m_fetcher_handler << m_fetcher.GetReadable();

    m_instruction_return_handler.SetHandler(m_init, std::bind(&PipelinedProcessor::InstructionReturn, this));
    m_instruction_return_handler << m_instruction_return;
    m_instruction_return_handler << m_fetcher.GetReadable();


    m_decoder_handler.SetHandler(m_init, std::bind(&PipelinedProcessor::Decode, this));
    m_decoder_handler << m_decoder.GetReadable();

    m_operand_response_handler.SetHandler(m_init, std::bind(&PipelinedProcessor::OperandReturn, this));
    m_operand_response_handler << m_data_return;

    m_operand_back_pressure_handler.SetHandler(m_init, std::bind(&PipelinedProcessor::SendOperandRequests, this));

    m_executor_handler.SetHandler(m_init, std::bind(&PipelinedProcessor::Execute, this));
    m_executor_handler << m_executor.GetReadable();

    m_write_back_handler.SetHandler(m_init, std::bind(&PipelinedProcessor::WriteBack, this));
    m_write_back_handler << m_write_back.GetReadable();

    m_write_back_back_pressure_handler.SetHandler(m_init, std::bind(&PipelinedProcessor::SendWriteBackRequests, this));

}

void PipelinedProcessor::CheckDoorbell() {
    // Read our doorbell
    ++m_doorbell_rings;
    m_functional_library.SetApplicationStart(m_doorbell.Read());
    application_terminated = false;
    Fetch();
}

void PipelinedProcessor::Fetch() {
    if (m_fetcher.WriteValid() && !application_terminated) {
        m_logger.LogLn(hestia::LoggingType::INFO, "Sending To Fetcher");
        m_fetcher.Write(m_functional_library.Fetch());
    } else {
        m_logger.LogLn(hestia::LoggingType::INFO, "Back pressured by fetcher");
        m_fetcher.NotifyOnWriteable(m_fetcher_back_pressure_handler.GetId());
    }
}

void PipelinedProcessor::ProcessFetch() {
    if (m_fetcher.ReadValid() && m_fetcher.Peek().status == hestia::MemoryRequest::Status::PENDING && m_instruction_fetch.WriteValid()) {
        m_logger.LogLn(hestia::LoggingType::INFO, "Fetching");
        ++m_memory_fetches;
        m_fetcher.Peek().status = hestia::MemoryRequest::Status::SENT;
        m_instruction_fetch.Write(m_fetcher.Peek());
    }
    if (m_fetcher.ReadValid() && !m_instruction_fetch.WriteValid()) {
        m_logger.LogLn(hestia::LoggingType::INFO, "Back Pressured by instruction fetch");
        m_instruction_fetch.NotifyOnWriteable(m_fetcher_handler.GetId());
    }
}

void PipelinedProcessor::InstructionReturn() {
    while (m_instruction_return.ReadValid() && m_fetcher.ReadValid() && m_decoder.WriteValid()) {
        m_logger.LogLn(hestia::LoggingType::INFO, "Sending to Decoder");
        m_decoder.Write(m_instruction_return.Read());
    }
    if (m_instruction_return.ReadValid() && !m_decoder.WriteValid()) {
        m_logger.LogLn(hestia::LoggingType::INFO, "Back pressured by decoder");
        m_decoder.NotifyOnWriteable(m_instruction_return_handler.GetId());
    }
}

void PipelinedProcessor::Decode() {
    while (m_decoder.ReadValid() && m_fetcher.ReadValid() && m_operand_requests.empty() && m_executor.WriteValid()) {
        auto response = m_decoder.Peek();
        auto instruction = m_functional_library.Decode(response);
        if(instruction.opcode == Opcode::ENDPRGM) {
            application_terminated = true;
        }

        if(HazardCheck(instruction)) {
            m_decoder.Read();
            m_fetcher.Read();
            m_operand_requests = m_functional_library.GatherOperands(instruction);
            m_executor.Write(instruction);
            switch(instruction.result.type) {
                case Result::Type::NONE:
                    break;
                case Result::Type::REGISTER:
                    m_destination_registers.emplace_back(instruction.result.location);
                    break;
                case Result::Type::MEMORY:
                    m_destination_addresses.emplace_back(instruction.result.location);
                    break;
            }
            m_logger.LogLn(hestia::LoggingType::INFO, "Sending to executor");
            SendOperandRequests();
            if (GetDetails(instruction.opcode).type != OpcodeDetails::Type::BRANCH) {
                m_logger.LogLn(hestia::LoggingType::INFO, "Fetching because of branch");
                Fetch();
            } else {
                m_logger.LogLn(hestia::LoggingType::INFO, "Branching");
            }
        } else {
            m_logger.LogLn(hestia::LoggingType::INFO, "Hit Memory Hazard Stalling");
            break;
        }
    }
    if(m_decoder.ReadValid() && !m_executor.WriteValid()) {
        m_logger.LogLn(hestia::LoggingType::INFO, "Back pressured by executor");
        m_executor.NotifyOnWriteable(m_decoder_handler.GetId());
    }
}

void PipelinedProcessor::SendOperandRequests() {
    while (!m_operand_requests.empty() && m_data_request.WriteValid()) {
        m_logger.LogLn(hestia::LoggingType::INFO, "Requesting Operand");
        ++m_memory_fetches;
        m_data_request.Write(m_operand_requests.front(), m_operand_requests.front().size);
        m_operand_requests.pop_front();
    }
    if (!m_operand_requests.empty()) {
        m_logger.LogLn(hestia::LoggingType::INFO, "Back Pressured Waiting on Operand");
        m_data_request.NotifyOnWriteable(m_operand_back_pressure_handler.GetId());
    }
}

void PipelinedProcessor::OperandReturn() {
    std::deque<hestia::MemoryResponse> responses{};
    while(m_data_return.ReadValid()) {
        responses.emplace_back(m_data_return.Read());
    }
    m_logger.LogLn(hestia::LoggingType::INFO, "Received Operands");
    m_functional_library.ProcessOperandMemoryResponses(m_executor.Peek(), responses);
    if (m_executor.Peek().OperandsGathered()) {
        Execute();
    }
}

void PipelinedProcessor::Execute() {
    while (m_executor.ReadValid() && m_executor.Peek().OperandsGathered() && m_write_back.WriteValid()) {
        auto instruction = m_executor.Read();
        m_functional_library.Execute(instruction);
        m_write_back.Write(instruction);
        m_logger.LogLn(hestia::LoggingType::INFO, "Executed");
        if(GetDetails(instruction.opcode).type == OpcodeDetails::Type::BRANCH) {
            Fetch();
        }
    }
    if (m_executor.ReadValid() && m_executor.Peek().OperandsGathered() && !m_write_back.WriteValid()) {
        m_logger.LogLn(hestia::LoggingType::INFO, "Back Pressured by Write Back");
        m_executor.NotifyOnWriteable(m_executor_handler.GetId());
    }
}

void PipelinedProcessor::WriteBack() {
    while (m_write_back.ReadValid()) {
        auto instruction = m_write_back.Read();
        m_write_back_requests = std::move(m_functional_library.WriteBack(instruction));
        m_logger.LogLn(hestia::LoggingType::INFO, "Written Back");
        SendWriteBackRequests();
        switch(instruction.result.type) {
            case Result::Type::NONE:
                break;
            case Result::Type::REGISTER:
                m_destination_registers.pop_front();
                Decode();
                break;
            case Result::Type::MEMORY:
                m_destination_addresses.pop_front();
                Decode();
                break;
        }
    }
}

void PipelinedProcessor::SendWriteBackRequests() {
    while(!m_write_back_requests.empty() && m_data_request.WriteValid()) {
        ++m_memory_fetches;
        m_data_request.Write(m_write_back_requests.front(), m_write_back_requests.front().size);
        m_write_back_requests.pop_front();
    }
    if (!m_write_back_requests.empty()) {
        m_data_request.NotifyOnWriteable(m_write_back_back_pressure_handler.GetId());
    }
}

bool PipelinedProcessor::HazardCheck(const Instruction& instruction) {
    for (auto& op : instruction.operands) {
        switch (op.type) {
            case Operand::Type::REGISTER: {
                for (auto &destination_register : m_destination_registers) {
                    if (destination_register == op.location) {
                        return false;
                    }
                }
                break;
            }
            case Operand::Type::CONSTANT:
                break;
            case Operand::Type::INDIRECT_MEMORY_REGISTER:
                for (auto &destination_register : m_destination_registers) {
                    if (destination_register == op.location) {
                        return false;
                    }
                }
                for (auto& destination_address : m_destination_addresses) {
                    if (destination_address == m_destination_registers[op.location]) {
                        return false;
                    }
                }
                break;
            case Operand::Type::EMBEDDED:
                break;
        }
    }
    return true;
}