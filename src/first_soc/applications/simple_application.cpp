
#include "applications/simple_application.h"

#include <hestia/memory/memory_manager.h>
#include <functional/functional_processor_library.h>

SimpleApplication::SimpleApplication(const hestia::ComponentInit &init) :
        Manageable(hestia::FrameworkType::COMPONENT, init.name),
        hestia::ComponentBase(init),
        // Port
        m_doorbell(CreatePortInit("doorbell")),
        // Memory
        m_memory(m_init.memories->GetMemory(GetParam("memory_name"))) {}

void SimpleApplication::Setup() noexcept {
    // Create Surface
    std::vector<hestia::IMemory::Data> surface;
    AddInstruction(CreateAddInstruction(), surface);
    AddInstruction(CreateENDPRGMInstruction(), surface);
    surface.emplace_back(0); // Place for our result
    // Allocate the surface and set it
    auto address = m_memory->Allocate(surface.size());
    m_memory->Set(address, surface.data(), surface.size());
    // Write the address out through our doorbell port
    m_doorbell.Write(address);
}

void SimpleApplication::AddInstruction(
        const Instruction& instruction,
        std::vector<hestia::IMemory::Data> &surface) {

    auto encoded_values = FunctionalProcessorLibrary::Encode(instruction);
    for (auto const& value : encoded_values) {
        surface.emplace_back(value);
    }
}

// 2 + 3 set result at memory location = 2
Instruction SimpleApplication::CreateAddInstruction() {
    Instruction instruction{};
    instruction.opcode = Opcode::ADD;
    instruction.operands.resize(2);
    instruction.operands[0].type = Operand::Type::EMBEDDED;
    instruction.operands[0].value = 2;
    instruction.operands[1].type = Operand::Type::EMBEDDED;
    instruction.operands[1].value = 3;
    instruction.result.type = Result::Type::MEMORY;
    instruction.result.location = 2;
    return instruction;
}

Instruction SimpleApplication::CreateENDPRGMInstruction() {
    Instruction instruction{};
    instruction.opcode = Opcode::ENDPRGM;
    return instruction;
}
