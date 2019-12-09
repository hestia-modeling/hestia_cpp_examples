#ifndef HESTIA_EXAMPLES_FIRST_SOC_ISA_H
#define HESTIA_EXAMPLES_FIRST_SOC_ISA_H

#include <cstdint>
#include <unordered_map>

enum class Opcode : uint16_t {
    MOVE = 0, // Move memory from one location to another (Memory / Register / Constant / etc...)
    ADD = 1, // Execute instruction on the ALU to add together the two operands
    SUBTRACT = 2,
    MULTIPLY = 3,
    DIVIDE = 4,
    INCREMENT = 5,
    DECREMENT = 6,
    COMPARE = 7,
    JUMP = 8, // Set the program counter to the result of the jump instruction
    JUMP_LESS = 9, // If carry flag is set after a compare instruction will jump to memory location.
    CALL = 9, // Push current program counter to the stack and jump to the call location
    RETURN = 10, // Pop off the stack and set the program counter to the result and.
    ENDPRGM = 0xFFu // Terminate the application
};

/***
 * Metadata described details about the opcode.
 */
struct OpcodeDetails {
    /***
     * Describes what kind of functionality the instruction requires.
     */
    enum class Type {
        MEMORY = 0,
        ALU = 1,
        BRANCH = 2
    };

    Type type = Type::MEMORY;
    uint8_t num_operands = 0;
};

/***
 * We will provide a map of all the opcodes to some metadata about that opcode
 * @return Map of opcode details
 */
const std::unordered_map<Opcode, OpcodeDetails>& GetDetails();

/***
 * Get details for a specific opcode.
 * @return
 */
const OpcodeDetails& GetDetails(Opcode);


#endif //HESTIA_EXAMPLES_FIRST_SOC_ISA_H
