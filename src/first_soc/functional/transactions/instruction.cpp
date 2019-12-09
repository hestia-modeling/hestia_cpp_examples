
#include "transactions/instruction.h"

bool Instruction::OperandsGathered() const {
    for(auto const& operand : operands) {
        if(operand.status != Operand::Status::GATHERED) {
            return false;
        }
    }
    return true;
}