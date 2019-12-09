
#include "applications/loop_application.h"

#include <hestia/memory/memory_manager.h>
#include <functional/functional_processor_library.h>
#include <random>

uint64_t LoopApplication::m_write_back_register = 0;
uint64_t LoopApplication::m_count_register = 1;

LoopApplication::LoopApplication(const hestia::ComponentInit &init) :
        Manageable(hestia::FrameworkType::COMPONENT, init.name),
        hestia::ComponentBase(init),
        // Port
        m_doorbell(CreatePortInit("doorbell")),
        // Memory
        m_memory(m_init.memories->GetMemory(GetParam("memory_name"))),
        m_num_iterations(GetUintParam("num_iterations")),
        m_num_ops_per_iteration(GetUintParam("num_ops_per_iteration")),
        m_instruction_type_mode(GetParam("mode")) {}

void LoopApplication::Setup() noexcept {
    // Create Surface
    m_write_back_address = m_memory->Allocate(1);
    std::vector<hestia::IMemory::Data> surface;
    // Loop over creating the correct number of operations
    for (uint64_t i = 0; i < m_num_ops_per_iteration; i++) {
        AddInstruction(CreateInstruction(), surface);
    }
    // Add the loop logic
    AddInstruction(LoopLogicInstructions(), surface);
    // End program
    AddInstruction(CreateENDPRGMInstruction(), surface);
    // Allocate the surface and set it
    m_application_start_address = m_memory->Allocate(surface.size());
    m_memory->Set(m_application_start_address, surface.data(), surface.size());
    // Write the address out through our doorbell port
    m_doorbell.Write(m_application_start_address);
}

void LoopApplication::AddInstruction(
        const Instruction& instruction,
        std::vector<hestia::IMemory::Data> &surface) {

    auto encoded_values = FunctionalProcessorLibrary::Encode(instruction);
    for (auto const& value : encoded_values) {
        surface.emplace_back(value);
    }
}

void LoopApplication::AddInstruction(const std::vector<Instruction> &instructions, std::vector<hestia::IMemory::Data> &surface) {
    for(auto const& instruction : instructions) {
        AddInstruction(instruction, surface);
    }
}

// 2 + 3 set result at memory location = 2
Instruction LoopApplication::CreateInstruction() {
    if(m_instruction_type_mode == "alu") {
        return CreateAddInstruction();
    } else if (m_instruction_type_mode == "memory") {
        return CreateMoveInstruction();
    } else if (m_instruction_type_mode == "split") {
        m_generate_alu = !m_generate_alu;
        if(m_generate_alu) {
            return CreateAddInstruction();
        } else {
            return CreateMoveInstruction();
        }
    } else if (m_instruction_type_mode == "random") {
        static auto gen = std::bind(std::uniform_int_distribution<>(0,1), std::default_random_engine());
        if(gen()) {
            return CreateAddInstruction();
        } else {
            return CreateMoveInstruction();
        }
    }
    assert(false);
}

// 2 + 3 set result at memory location = 2
Instruction LoopApplication::CreateAddInstruction() {
    Instruction instruction{};
    instruction.opcode = Opcode::ADD;
    instruction.operands.resize(2);
    instruction.operands[0].type = Operand::Type::EMBEDDED;
    instruction.operands[0].value = 2;
    instruction.operands[1].type = Operand::Type::EMBEDDED;
    instruction.operands[1].value = 3;
    instruction.result.type = Result::Type::REGISTER;
    instruction.result.location = m_write_back_register;
    return instruction;
}

// 2 + 3 set result at memory location = 2
Instruction LoopApplication::CreateMoveInstruction() {
    Instruction instruction{};
    instruction.opcode = Opcode::MOVE;
    instruction.operands.resize(2);
    instruction.operands[0].type = Operand::Type::INDIRECT_MEMORY_REGISTER;
    instruction.operands[0].location = m_write_back_register;
    instruction.result.type = Result::Type::MEMORY;
    instruction.result.location = m_write_back_address;
    return instruction;
}

Instruction LoopApplication::CreateENDPRGMInstruction() {
    Instruction instruction{};
    instruction.opcode = Opcode::ENDPRGM;
    return instruction;
}

std::vector<Instruction> LoopApplication::LoopLogicInstructions() {
    std::vector<Instruction> instructions(3);
    auto& increment = instructions[0];
    increment.opcode = Opcode::INCREMENT;
    increment.operands.resize(1);
    increment.operands[0].type = Operand::Type::REGISTER;
    increment.operands[0].location = m_count_register;
    increment.result.location = m_count_register;
    increment.result.type = Result::Type::REGISTER;
    auto& compare = instructions[1];
    compare.opcode = Opcode::COMPARE;
    compare.operands.resize(2);
    compare.operands[0].type = Operand::Type::REGISTER;
    compare.operands[0].location = m_count_register;
    compare.operands[1].type = Operand::Type::EMBEDDED;
    compare.operands[1].value = m_num_iterations;
    auto& jump = instructions[2];
    jump.opcode = Opcode::JUMP_LESS;
    jump.operands.resize(1);
    jump.operands[0].type = Operand::Type::EMBEDDED;
    jump.operands[0].value = 1;
    return instructions;
}


