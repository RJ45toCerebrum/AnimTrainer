// Created by Tyler on 5/6/2026.
#pragma once

#include "NodeCompute.h"

START_NAMESPACE(ATGraph)

/// Input Description:
/// Input Index 0 => Vector Op Type
/// Input Index 1 => Left Operand
/// Input Index 2 -> Right Operand
class VectorOp final : public INodeCompute
{
public:
    enum class VectorOpType : int
    {
        VectorScale,
        VectorAdd,
        VectorSub,
        VectorDot,
        VectorAngle
    };

    static constexpr std::string_view kNodeName = "VectorOp";
    static constexpr NodeTypeID kNodeTypeID = nodeTypeHash(kNodeName);

    void compute(const NodeRecord& nodeRecord, DataStore& dStore) override;
    NDESC bool alwaysCompute() const override;
    void initDataSlotDefaultValue(DataSlot& dataSlot, const AttributeDescriptor& attrDescriptor) const override;
    NDESC bool changeAttributeDataType(const NodeRecord& nodeRecord, AttributeDataType concreteType,
        AttrID inputAttr, DataStore& dStore) override;
    NDESC bool setUnpluggedInputAttrData(AttrID inputAttrID, std::span<const std::byte> data,
        AttributeDataType concreteType, const NodeRecord& nodeRecord, DataStore& dStore) override;
    NDESC const std::span<const AttributeDescriptor> inputAttrSchema() const override;
    NDESC const std::span<const AttributeDescriptor> outputAttrSchema() const override;
    NDESC const std::string_view nodeName() const override;
    NDESC const NodeTypeID nodeTypeID() const override;

    //static bool setVectorOperation(class NodeHandle nh, VectorOpType opType);

private:
    static void computeVectorScale(const NodeRecord& nodeRecord, DataStore& dStore);
    static void computeVectorAdd(const NodeRecord& nodeRecord, DataStore& dStore);
    static void computeVectorSub(const NodeRecord& nodeRecord, DataStore& dStore);
    static void computeVectorDot(const NodeRecord& nodeRecord, DataStore& dStore);
    static void computeVectorAngle(const NodeRecord& nodeRecord, DataStore& dStore);
};

END_NAMESPACE