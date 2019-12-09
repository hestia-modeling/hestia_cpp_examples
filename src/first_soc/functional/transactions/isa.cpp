
#include "transactions/isa.h"

#include <cassert>

std::unordered_map<Opcode, OpcodeDetails> details;

void SetupDetails();

const std::unordered_map<Opcode, OpcodeDetails>& GetDetails() {
    if (details.empty()) {
       SetupDetails();
    }
    return details;
}

const OpcodeDetails& GetDetails(Opcode op) {
    if (details.empty()) {
        SetupDetails();
    }
    assert(details.find(op) != details.end());
    return details[op];
}

void SetupMemoryDetails();
void SetupALUDetails();
void SetupControlDetails();


void SetupDetails() {
    SetupMemoryDetails();
    SetupALUDetails();
    SetupControlDetails();
}


void SetupMemoryDetails() {
    details[Opcode::MOVE] = {OpcodeDetails::Type::MEMORY, 1};
}

void SetupALUDetails() {
    details[Opcode::ADD] = {OpcodeDetails::Type::ALU, 2};
    details[Opcode::INCREMENT] = {OpcodeDetails::Type::ALU, 1};
    details[Opcode::COMPARE] = {OpcodeDetails::Type::ALU, 2};
}

void SetupControlDetails() {
    details[Opcode::ENDPRGM] = {OpcodeDetails::Type::BRANCH, 0};
    details[Opcode::JUMP_LESS] = {OpcodeDetails::Type::BRANCH, 1};
}

