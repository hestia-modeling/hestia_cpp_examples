#ifndef FIRST_SOC_TIMING_DEVICES_PIPELINE_STAGE_H
#define FIRST_SOC_TIMING_DEVICES_PIPELINE_STAGE_H

#include <hestia/connection/internal_connection.h>

template<typename Transaction>
class PipelineStage : public hestia::InternalConnection<Transaction> {
public:

    using hestia::IInternalConnection::CreateInit;
    using hestia::IInternalConnection::CreateName;

    PipelineStage(const std::string& name, const hestia::Manageable* owner, const hestia::Init& init) :
            hestia::Manageable(hestia::FrameworkType::INTERNAL_CONNECTION, CreateName(owner, name) + ".pipeline_stage"),
            hestia::IActionable(init),
            hestia::InternalConnection<Transaction>(owner, CreateInit(init, owner, name)) {}

    using hestia::Manageable::GetName;

    using hestia::InternalConnection<Transaction>::GetCapacity;

    bool Validate() const noexcept override { return GetCapacity() == 1; }
};

#endif //FIRST_SOC_TIMING_DEVICES_PIPELINE_STAGE_H
