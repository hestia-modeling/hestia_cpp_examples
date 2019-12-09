#ifndef FIRST_SOC_FUNCTIONAL_PROCESSOR_LIBRARY_H
#define FIRST_SOC_FUNCTIONAL_PROCESSOR_LIBRARY_H

#include <hestia/base/manageable.h>

#include "transactions/instruction.h"

#include <hestia/base/init.h>
#include <hestia/counter/counter.h>
#include <hestia/log/logger.h>
#include <hestia/memory/i_memory.h>
#include <hestia/toolbox/transactions/memory_response.h>

#include <deque>

/**
 * The Functional Processor Library handles all of the low level details of the instruction cycle from
 * start of the application until it's termination.
 */
class FunctionalProcessorLibrary : public hestia::Manageable {
public:

    FunctionalProcessorLibrary(std::string name, const hestia::Init& init);

    /**
     * Set the initial address of the application
     */
    void SetApplicationStart(hestia::IMemory::Address);

    /**
     * Generates a Memory request for the next instruction
     * @return Memory request with address pointing at the location for the next address
     */
    hestia::MemoryRequest Fetch();

    /**
     * Processes a memory response containing the encoded instruction.
     * @param response Memory response containing the instruction
     * @return Valid Instruction set to Decoded stage
     */
    Instruction Decode(const hestia::MemoryResponse& response);

    /**
     * Gathers operands from various possible locations. For constant / indirect memory based operands
     * the return value will contain the necessary memory requests needed to gather the remain operands
     * @param instruction A Decoded Instruction
     * @return Memory requests if any that need to be processed to get values for remaining operands
     */
    std::deque<hestia::MemoryRequest> GatherOperands(Instruction& instruction);

    /**
     * Processes the responses of the Memory requests from Gather Operands and sets the appropriate values on newly
     * gathered operands.
     * @param instruction A Decoded Instruction
     * @param responses  Responses for remaining operands. Does support setting only a subset of the operands. Responses must be in order.
     */
    void ProcessOperandMemoryResponses(Instruction& instruction, std::deque<hestia::MemoryResponse>& responses);

    /**
     * Executes the logic for the instruction
     * @param instruction A fully gathered instruction
     */
    void Execute(Instruction& instruction);

    /**
     * Writes back the result and generates memory requests if needed
     * @param instruction
     * @return Memory request with result
     */
    std::deque<hestia::MemoryRequest> WriteBack(Instruction& instruction);

    /**
     * Validates that funclib has at least 1 register
     * @return True if has at least 1 register
     */
    bool Validate() const noexcept override { return !m_registers.empty(); }

    /***
     * Encode an instruction into raw bytes for easier storing in simulated memory
     * @param instruction A valid Instruction
     * @return Raw bytes representing the instruction
     */
    static std::vector<uint64_t> Encode(const Instruction& instruction);

private:

    using OpcodeEncodedType = std::bitset<16>;
    using OperandEncodedType = std::bitset<24>; // One byte per Operand + Result
    using OperandMetadata = std::bitset<24>; // One byte per Operand + Result

    static OperandEncodedType EncodeOperandTypes(const Instruction& instruction);
    static OperandMetadata EncodeOperandMetaData(const Instruction& instruction);

    static bool AdditionOverflow(int64_t a, int64_t b);
    static bool SubtractionOverflow(int64_t a, int64_t b);
    static bool MultiplicationOverflow(int64_t a, int64_t b);

    static void ExecuteMemory(Instruction& instruction);
    static void ExecuteAlu(Instruction& instruction);
    void ExecuteControl(Instruction& instruction);

    struct Counters {
        struct Instruction {
            hestia::Counter fetched;
            hestia::Counter decoded;
            hestia::Counter executed;
            hestia::Counter written_back;

            Instruction(const std::string& name, hestia::Manageable* owner, const hestia::Init& init);
        } instructions;

        struct Operand {
            hestia::Counter gathered;
            hestia::Counter registers;
            hestia::Counter constants;
            hestia::Counter indirect_memories;
            hestia::Counter embedded;

            Operand(const std::string& name, hestia::Manageable* owner, const hestia::Init& init);
        } operands;

        struct Application {
            hestia::Counter started;
            hestia::Counter terminated;

            Application(const std::string& name, hestia::Manageable* owner, const hestia::Init& init);
        } applications;

        Counters(const std::string& name, hestia::Manageable* owner, const hestia::Init& init);
    } m_counters;

    hestia::IMemory::Address m_program_counter = 0;

    using Register = hestia::IMemory::Data;
    std::vector<Register> m_registers;
    Flags m_flags{};

    hestia::Logger m_logger;
    hestia::Logger m_instruction_trace;

};

#endif //FIRST_SOC_FUNCTIONAL_PROCESSOR_LIBRARY_H
