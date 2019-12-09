#include "functional_processor_library.h"

#include <hestia/parameter/parameter_manager.h>

#include <cassert>
#include <utility>

static const auto FrameworkType = hestia::FrameworkType::COMPONENT;

static const auto MAX_OPERANDS = 2;

FunctionalProcessorLibrary::FunctionalProcessorLibrary(std::string name, const hestia::Init &init) :
    hestia::Manageable(FrameworkType, std::move(name)),
    m_registers(init.params->GetUintParam(FrameworkType, GetName(), "num_registers")),
    m_counters(GetName(), this, init),
    m_logger(hestia::LoggerInit{*init.logging_manager, FrameworkType, GetName()}),
    m_instruction_trace(hestia::LoggerInit{*init.logging_manager, FrameworkType, GetName() + ".instructions"}) {

}

void FunctionalProcessorLibrary::SetApplicationStart(hestia::IMemory::Address address) {
    assert(m_program_counter == 0);
    ++m_counters.applications.started;
    m_program_counter = address;
    m_logger.Log(hestia::LoggingType::INFO, "Received Doorbell for application at address: ");
    m_logger.LogLn(hestia::LoggingType::INFO, std::to_string(m_program_counter).c_str());
    m_instruction_trace.LogLn(hestia::LoggingType::DEBUG,
                              "Opcode  |  Operand 0 (Type) (Location) | Operand 1 (Type) (Location) | "
                              "Result (Type) (Location) | Flags before | Flags after");
}


hestia::MemoryRequest FunctionalProcessorLibrary::Fetch() {
    hestia::MemoryRequest request{};
    request.address = m_program_counter;
    request.size = 1;
    ++m_counters.instructions.fetched;
    m_logger.Log(hestia::LoggingType::DEBUG, "Fetching instruction at address: ");
    m_logger.LogLn(hestia::LoggingType::DEBUG, std::to_string(m_program_counter).c_str());
    return request;
}


Instruction FunctionalProcessorLibrary::Decode(const hestia::MemoryResponse& response) {
    auto instruction = response.data[0];
    Instruction result{};
    result.opcode = static_cast<Opcode>(static_cast<uint16_t>(instruction));
    result.operands.resize(GetDetails(result.opcode).num_operands);
    OperandEncodedType operand_types = instruction >> 16u;
    OperandMetadata operand_meta_data = instruction >> 40u;
    for (size_t i = 0; i < result.operands.size(); i++) {
        auto pos = i * 8;
        result.operands[i].type = static_cast<Operand::Type>((operand_types.to_ulong() >> pos) & 0xFFu);
        auto meta_data = (operand_meta_data.to_ulong() >> pos) & 0xFFu;
        switch(result.operands[i].type) {
            case Operand::Type::REGISTER:
                result.operands[i].location = meta_data;
                break;
            case Operand::Type::CONSTANT:
                break;
            case Operand::Type::INDIRECT_MEMORY_REGISTER:
                result.operands[i].location = meta_data;
                break;
            case Operand::Type::EMBEDDED:
                result.operands[i].value = meta_data;
                result.operands[i].status = Operand::Status::GATHERED;
                break;
        }
    }
    result.result.type = operand_types[16] ? Result::Type::REGISTER : Result::Type::NONE;
    result.result.type = operand_types[17] ? Result::Type::MEMORY : result.result.type;
    result.result.location = (operand_meta_data.to_ulong() >> 16u) & 0xFFu;
    return result;
}

std::deque<hestia::MemoryRequest> FunctionalProcessorLibrary::GatherOperands(Instruction &instruction) {
    ++m_counters.instructions.decoded;
    ++m_program_counter;
    std::deque<hestia::MemoryRequest> requests;
    for (auto &op : instruction.operands) {
        ++m_counters.operands.gathered;
        switch (op.type) {
            case Operand::Type::REGISTER:
                ++m_counters.operands.registers;
                op.value = m_registers[op.location];
                op.status = Operand::Status::GATHERED;
                break;
            case Operand::Type::CONSTANT: {
                ++m_counters.operands.constants;
                op.status = Operand::Status::REQUESTED;
                hestia::MemoryRequest request{};
                request.address = m_program_counter;
                request.size = 0;
                requests.emplace_back(request);
                ++m_program_counter;
                break;
            }
            case Operand::Type::INDIRECT_MEMORY_REGISTER: {
                ++m_counters.operands.indirect_memories;
                op.status = Operand::Status::REQUESTED;
                hestia::MemoryRequest request{};
                request.address = m_registers[op.location];
                request.size = 1;
                requests.emplace_back(request);
                break;
            }
            case Operand::Type::EMBEDDED:
                ++m_counters.operands.embedded;
                break;
        }
    }
    return requests;
}

void FunctionalProcessorLibrary::ProcessOperandMemoryResponses(Instruction &instruction, std::deque<hestia::MemoryResponse> &responses) {
    for (auto& response : responses) {
        for (auto& op : instruction.operands) {
            if (op.status == Operand::Status::REQUESTED) {
                op.value = response.data[0];
                op.status = Operand::Status::GATHERED;
                break;
            }
        }
    }
}

std::string to_string(Opcode op) {
    switch (op){
        case Opcode::MOVE:
            return "MOVE";
        case Opcode::ADD:
            return "ADD";
        case Opcode::SUBTRACT:
            return "SUBTRACT";
        case Opcode::MULTIPLY:
            return "MULTIPLY";
        case Opcode::DIVIDE:
            return "DIVIDE";
        case Opcode::INCREMENT:
            return "INCREMENT";
        case Opcode::DECREMENT:
            return "DECREMENT";
        case Opcode::COMPARE:
            return "COMPARE";
        case Opcode::JUMP:
            return "JUMP";
        case Opcode::JUMP_LESS:
            return "JUMP_LESS";
        case Opcode::RETURN:
            return "RETURN";
        case Opcode::ENDPRGM:
            return "ENDPRGM";
    }
}

std::string to_string(const std::vector<Operand>& operands) {
    std::string result;
    for (auto const& operand: operands) {
        result += std::to_string(operand.value);
        switch(operand.type) {
            case Operand::Type::REGISTER:
                result += " R ";
                break;
            case Operand::Type::CONSTANT:
                result += " C ";
                break;
            case Operand::Type::INDIRECT_MEMORY_REGISTER:
                result += " I ";
                break;
            case Operand::Type::EMBEDDED:
                result += " E ";
                break;
        }
        result += std::to_string(operand.location) + " | ";
    }

    for (size_t i = 0; i < MAX_OPERANDS - operands.size(); i++) {
        result += "   N/a   |";
    }
    return result;
}

std::string to_string(const Result& result) {
    std::string string;
    string += std::to_string(result.value);
    switch(result.type) {
        case Result::Type::REGISTER:
            string += " R ";
            break;
        case Result::Type::MEMORY:
            string += " M ";
            break;
        case Result::Type::NONE:
            string += " N/A ";
            break;
    }
    string += std::to_string(result.location) + " | ";
    return string;
}

std::string to_string(const Flags& flags) {
    std::string string;
    string += flags.sign ? "1" : "0";
    string += flags.zero ? "1" : "0";
    string += flags.parity ? "1" : "0";
    string += flags.carry ? "1" : "0";
    return string;
}

std::string to_string(Instruction& instruction, Flags& before_flags) {
    std::string result;
    result += to_string(instruction.opcode) + " | ";
    result += to_string(instruction.operands) + " | ";
    result += to_string(instruction.result) + " | ";
    result += to_string(before_flags) + " | ";
    result += to_string(instruction.result.flags);
    return result;
}

void FunctionalProcessorLibrary::Execute(Instruction &instruction) {
    ++m_counters.instructions.executed;
    std::vector<hestia::MemoryRequest> results{};
    auto flags = m_flags;
    switch(GetDetails(instruction.opcode).type) {
        case OpcodeDetails::Type::MEMORY:
            ExecuteMemory(instruction);
            break;
        case OpcodeDetails::Type::ALU:
            ExecuteAlu(instruction);
            m_flags = instruction.result.flags;
            break;
        case OpcodeDetails::Type::BRANCH:
            ExecuteControl(instruction);
            break;
    }
    m_instruction_trace.LogLn(hestia::LoggingType::DEBUG, to_string(instruction, flags).c_str());

}

std::deque<hestia::MemoryRequest> FunctionalProcessorLibrary::WriteBack(Instruction &instruction) {
    ++m_counters.instructions.written_back;
    std::deque<hestia::MemoryRequest> requests{};
    switch(instruction.result.type) {
        case Result::Type::REGISTER:
            m_registers[instruction.result.location] = instruction.result.value;
            break;
        case Result::Type::MEMORY: {
            hestia::MemoryRequest request{};
            request.type = hestia::MemoryRequest::Type::WRITE;
            request.address = instruction.result.location;
            request.data.emplace_back(instruction.result.value);
            request.size = 1;
            requests.emplace_back(request);
        }
        case Result::Type::NONE:
            break;
    }
    return requests;
}

void FunctionalProcessorLibrary::ExecuteMemory(Instruction &instruction) {
    switch (instruction.opcode) {
        case Opcode::MOVE:
            instruction.result.value = instruction.operands[0].value;
            break;
        default:
            assert(false);
    }
}

void FunctionalProcessorLibrary::ExecuteAlu(Instruction &instruction) {
    auto opcode = instruction.opcode;
    auto &operands = instruction.operands;
    auto &result = instruction.result;
    assert(operands.size() == GetDetails(opcode).num_operands);
    switch (opcode) {
        case Opcode::ADD:
            assert(operands.size() == GetDetails(opcode).num_operands);
            result.value = operands[0].value + operands[1].value;
            result.flags.carry = AdditionOverflow(operands[0].value, operands[1].value);
            break;
        case Opcode::SUBTRACT:
            result.value = operands[0].value - operands[1].value;
            result.flags.carry = SubtractionOverflow(operands[0].value, operands[1].value);
            break;
        case Opcode::MULTIPLY:
            result.value = operands[0].value * operands[1].value;
            result.flags.carry = MultiplicationOverflow(operands[0].value, operands[1].value);
            break;
        case Opcode::DIVIDE:
            assert(operands[1].value != 0);
            result.value = operands[0].value / operands[1].value;
            result.flags.carry = false;
            break;
        case Opcode::INCREMENT:
            result.value = operands[0].value + 1;
            result.flags.carry = operands[0].value > result.value;
            break;
        case Opcode::DECREMENT:
            result.value = operands[0].value - 1;
            result.flags.carry = operands[0].value < result.value;
            break;
        case Opcode::COMPARE:
            if (operands[0].value == operands[1].value) {
                result.flags.zero = true;
                result.flags.carry = false;
            } else if (operands[0].value < operands[1].value) {
                result.flags.zero = false;
                result.flags.carry = true;
            } else {
                result.flags.zero = false;
                result.flags.carry = false;
            }
            return;
        default:
            break;
    }
    result.flags.sign = result.value < 0;
    result.flags.zero = result.value == 0;
    result.flags.parity =
            std::bitset<sizeof(result.value)>(result.value).count() == (sizeof(result.value) / 2);

}

void FunctionalProcessorLibrary::ExecuteControl(Instruction &instruction) {
    switch(instruction.opcode) {
        case Opcode::JUMP:
            break;
        case Opcode::JUMP_LESS:
            if(m_flags.carry) {
                m_program_counter = instruction.operands[0].value;
            }
            break;
        case Opcode::ENDPRGM:
            ++m_counters.applications.terminated;
            m_program_counter = 0;
            break;
        default:
            assert(false);
            break;
    }
}

bool FunctionalProcessorLibrary::AdditionOverflow(int64_t a, int64_t b) {
    // Positive Overflow
    if (a > 0 && b > 0 && a + b < 0) {
        return true;
        // Negative Overflow
    } else if (a < 0 && b < 0 && a + b > 0) {
        return true;
    }
    return false;
}

bool FunctionalProcessorLibrary::SubtractionOverflow(int64_t a, int64_t b) {
    // Positive Overflow
    if (a > 0 && b < 0 && a - b < 0) {
        return true;
        // Negative Overflow
    } else if (a < 0 && b > 0 && a - b > 0) {
        return true;
    }
    return false;
}

bool FunctionalProcessorLibrary::MultiplicationOverflow(int64_t a, int64_t b) {
    if (a == 0 || b == 0) {
        return false;
    } else {
        return a != (a * b) / b;
    }
}

std::vector<uint64_t> FunctionalProcessorLibrary::Encode(const Instruction &instruction) {
    std::bitset<64> encoded_instruction = static_cast<uint16_t>(instruction.opcode);;
    auto operand_types = EncodeOperandTypes(instruction);
    auto operand_meta_data = EncodeOperandMetaData(instruction);

    for (size_t i = 0; i < operand_types.size(); i++) {
        encoded_instruction[16 + i] = operand_types[i];
    }

    for (size_t i = 0; i < operand_types.size(); i++) {
        encoded_instruction[16 + operand_types.size() + i] = operand_meta_data[i];
    }

    std::vector<uint64_t> encoded_values;
    encoded_values.emplace_back(encoded_instruction.to_ulong());

    for (auto const& op : instruction.operands) {
        if (op.type == Operand::Type::CONSTANT) {
            encoded_values.emplace_back(op.value);
        }
    }
    return encoded_values;
}

auto FunctionalProcessorLibrary::EncodeOperandTypes(const Instruction &instruction) -> OperandEncodedType {
    OperandEncodedType result = 0;
    assert(instruction.operands.size() <= 2);
    for (size_t i = 0; i < instruction.operands.size(); i++) {
        auto pos = i * 8;
        switch(instruction.operands[i].type) {
            case Operand::Type::REGISTER:
                break;
            case Operand::Type::CONSTANT:
                result[pos].flip();
                break;
            case Operand::Type::INDIRECT_MEMORY_REGISTER:
                result[pos + 1].flip();
                break;
            case Operand::Type::EMBEDDED:
                result[pos].flip();
                result[pos + 1].flip();
                break;
        }
    }
    switch (instruction.result.type) {
        case Result::Type::NONE:
            break;
        case Result::Type::REGISTER:
            result[16].flip();
            break;
        case Result::Type::MEMORY:
            result[17].flip();
            break;
    }
    return result;}

auto FunctionalProcessorLibrary::EncodeOperandMetaData(const Instruction &instruction) -> OperandMetadata {
    OperandMetadata result = 0;
    assert(instruction.operands.size() <= 2);
    for (size_t i = 0; i < instruction.operands.size(); i++) {
        auto pos = i * 8;
        switch(instruction.operands[i].type) {
            case Operand::Type::REGISTER:
                result |= (instruction.operands[i].location & 0xFFu) << pos;
                break;
            case Operand::Type::CONSTANT:
                break;
            case Operand::Type::INDIRECT_MEMORY_REGISTER:
                result |= (instruction.operands[i].location & 0xFFu) << pos;
                break;
            case Operand::Type::EMBEDDED:
                result |= (static_cast<uint64_t>(instruction.operands[i].value) & 0xFFu) << pos;
                break;
        }
    }
    if(instruction.result.type != Result::Type::NONE) {
        result |= ((instruction.result.location & 0xFFu) << 16u);
    }
    return result;
}

FunctionalProcessorLibrary::Counters::Counters(const std::string &name, hestia::Manageable* owner,  const hestia::Init &init) :
        instructions("instructions.", owner, init),
        operands("operands.", owner, init),
        applications("applications.", owner, init) {}

FunctionalProcessorLibrary::Counters::Application::Application(const std::string &name, hestia::Manageable* owner, const hestia::Init &init) :
        started(name + "started", owner, init),
        terminated(name + "terminated", owner, init) {}

FunctionalProcessorLibrary::Counters::Operand::Operand(const std::string &name, hestia::Manageable *owner, const hestia::Init &init) :
        gathered(name + "gathered", owner, init),
        registers(name + "registers", owner, init),
        constants(name + "constants", owner, init),
        indirect_memories(name + "indirect_memories", owner, init),
        embedded(name + "embedded", owner, init){}

FunctionalProcessorLibrary::Counters::Instruction::Instruction(const std::string &name, hestia::Manageable *owner, const hestia::Init &init) :
        fetched(name + "fetched", owner, init),
        decoded(name + "decoded", owner, init),
        executed(name + "executed", owner, init),
        written_back(name + "written_back", owner, init) {}
