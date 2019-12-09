#ifndef FIRST_SOC_INSTRUCTION_H
#define FIRST_SOC_INSTRUCTION_H

#include "isa.h"

#include <bitset>
#include <cstdint>
#include <vector>

/**
 * Contains not only the actual value of the operand but also metadata about
 * where to source the value from.
 */
struct Operand {

    /**
     * Describes the source type of an operand
     */
    enum class Type : uint8_t {
        REGISTER = 0, // Our operand comes from a register
        CONSTANT = 1, // Our operand is in memory sequentially behind instruction
        INDIRECT_MEMORY_REGISTER = 2, // The address of our operand is in a register
        EMBEDDED = 3 // Our operand is embedded into the opcode meta data
    };

    /**
     * During the gathering process we will provide a Status flag, which will help keep track of what
     * stage of gathering an operand is in.
     */
    enum class Status : uint8_t {
        DECODED = 0,
        REQUESTED = 1,
        GATHERED = 3
    };

    // Metadata about the operand
    Type     type     = Type::REGISTER;
    Status   status   = Status::DECODED;
    uint64_t location = 0;
    // The value that will be utilized by the processor
    int64_t value    = 0;
};

struct Flags {
    bool sign = false;
    bool zero = false;
    bool parity = false;
    bool carry = false;
};

/**
 * Contains not only the actual value produced by the execution unit
 * but also metadata about where to send the result.
 */
struct Result {
    /**
     * Describes the destination type of the opcode if the type is of ALU
     */
    enum class Type : uint8_t {
        NONE = 0,
        REGISTER = 1,
        MEMORY = 2
    };

    // Meta data about the result
    Type type = Type::NONE;
    uint64_t location = 0;
    // Actual result created by the processor
    int64_t value = 0;
    Flags flags{};
};

/**
 * Our abstracted Instruction class. The model will make use of this class to pass instructions from one
 * stage to another.
 */
struct Instruction {

    enum class Phase : uint8_t {
        FETCHED = 0,
        DECODED = 1,
        EXECUTED = 2,
        FINISHED = 3,
    };

    Opcode opcode = Opcode::ENDPRGM;
    std::vector<Operand> operands;
    Phase phase = Phase::FETCHED;
    uint8_t size = 0;
    Result result{};

    bool OperandsGathered() const;

};


#endif //FIRST_SOC_INSTRUCTION_H
