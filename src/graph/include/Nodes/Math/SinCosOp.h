// Created by Tyler on 5/6/2026.
#pragma once

#include "NodeCompute.h"

START_NAMESPACE(ATGraph)

class SinCosOp final : public INodeCompute
{
public:
    static constexpr std::string_view kNodeName = "SinCosOp";
    static constexpr NodeTypeID kNodeTypeID = nodeTypeHash(kNodeName);

    void compute(const NodeRecord &nodeRecord, DataStore &dStore) override;
    NDESC bool alwaysCompute() const override;
    void initDataSlotDefaultValue(DataSlot &dataSlot, const AttributeDescriptor &attrDescriptor) const override;
    NDESC bool changeAttributeDataType(const NodeRecord &nodeRecord, AttributeDataType concreteType,
        AttrID inputAttr, DataStore &dStore) override;
    NDESC bool setUnpluggedInputAttrData(AttrID inputAttrID, std::span<const std::byte> data,
        AttributeDataType concreteType, const NodeRecord& nodeRecord, DataStore& dStore) override;
    NDESC const std::span<const AttributeDescriptor> inputAttrSchema() const override;
    NDESC const std::span<const AttributeDescriptor> outputAttrSchema() const override;
    NDESC const std::string_view nodeName() const override;
    NDESC const NodeTypeID nodeTypeID() const override;
};

END_NAMESPACE
