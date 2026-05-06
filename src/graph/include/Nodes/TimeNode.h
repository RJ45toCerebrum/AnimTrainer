// Created by Tyler on 5/6/2026.
#pragma once

#include "NodeCompute.h"

START_NAMESPACE(ATGraph)

// output format: [App Time, Delta Time]
class TimeNode : public INodeCompute
{
public:
    static constexpr std::string_view kNodeName = "TimeNode";
    static constexpr NodeTypeID kNodeTypeId = nodeTypeHash(kNodeName);

    void compute(const NodeRecord &nodeRecord, DataStore &dStore) override;
    void initDataSlotDefaultValue(DataSlot &dataSlot, const AttributeDescriptor &attrDescriptor) const override;
    NDESC constexpr std::span<const AttributeDescriptor> inputAttrSchema() const override;
    NDESC constexpr std::span<const AttributeDescriptor> outputAttrSchema() const override;
    NDESC bool alwaysCompute() const override;
    NDESC constexpr std::string_view nodeName() const override;
    NDESC constexpr NodeTypeID nodeTypeID() const override;
};

END_NAMESPACE