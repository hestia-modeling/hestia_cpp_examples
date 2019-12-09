#ifndef FIRST_SOC_SIMPLE_APPLICATION_H
#define FIRST_SOC_SIMPLE_APPLICATION_H

#include <functional/transactions/instruction.h>

#include <hestia/component/component_base.h>

#include <hestia/memory/i_memory.h>
#include <hestia/port/write_port.h>

class SimpleApplication : public hestia::ComponentBase {
public:
    explicit SimpleApplication(const hestia::ComponentInit &init);

    void Setup() noexcept override;

    [[nodiscard]] bool Validate() const noexcept override { return true; }

private:
    /**
     * Will create an ADD instruction that has the operands embedded into the instruction
     * 2 + 2 store result at location 2 in memory
     * @return The add instruction with all of it's meta data
     */
    static Instruction CreateAddInstruction();

    /**
     * Create an End program instruction
     * @return Instruction with ENDPRGM as an opcode
     */
    static Instruction CreateENDPRGMInstruction();

    /**
     * Will encode and add the instruction to a vector of memory
     * @param instruction The instruction to be encoded
     * @param surface The surface to append the instruction to
     */
    static void AddInstruction(const Instruction& instruction, std::vector<hestia::IMemory::Data>& surface);

    hestia::WritePort<hestia::IMemory::Address> m_doorbell;

    hestia::IMemory* m_memory;
};



#endif //FIRST_SOC_SIMPLE_APPLICATION_H
