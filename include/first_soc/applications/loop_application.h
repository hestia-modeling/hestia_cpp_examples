
#ifndef FIRST_SOC_APPLICATIONS_ALU_LOOP_APPLICATION_H
#define FIRST_SOC_APPLICATIONS_ALU_LOOP_APPLICATION_H

#include <functional/transactions/instruction.h>

#include <hestia/component/component_base.h>

#include <hestia/memory/i_memory.h>
#include <hestia/port/write_port.h>

class LoopApplication : public hestia::ComponentBase {
public:
    explicit LoopApplication(const hestia::ComponentInit &init);

    void Setup() noexcept override;

    [[nodiscard]] bool Validate() const noexcept override { return true; }

private:
    /**
     * Create an instruction based of run time params
     * @return A Newly Created Instruction to be placed in loop
     */
    Instruction CreateInstruction();
    Instruction CreateAddInstruction();
    Instruction CreateMoveInstruction();


    /**
     * Create an End program instruction
     * @return Instruction with ENDPRGM as an opcode
     */
    static Instruction CreateENDPRGMInstruction();

    std::vector<Instruction> LoopLogicInstructions();

    /**
     * Will encode and add the instruction to a vector of memory
     * @param instruction The instruction to be encoded
     * @param surface The surface to append the instruction to
     */
    static void AddInstruction(const Instruction& instruction, std::vector<hestia::IMemory::Data>& surface);
    static void AddInstruction(const std::vector<Instruction>& instructions, std::vector<hestia::IMemory::Data>& surface);

    hestia::WritePort<hestia::IMemory::Address> m_doorbell;

    hestia::IMemory* m_memory;

    const uint64_t m_num_iterations;
    const uint64_t m_num_ops_per_iteration;

    hestia::IMemory::Address m_write_back_address = 0;
    hestia::IMemory::Address m_application_start_address = 0;
    static uint64_t m_write_back_register;
    static uint64_t m_count_register;

    const std::string m_instruction_type_mode;

    bool m_generate_alu = false;
};
#endif //FIRST_SOC_APPLICATIONS_ALU_LOOP_APPLICATION_H
