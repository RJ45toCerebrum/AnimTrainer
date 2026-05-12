// Created by Tyler on 5/7/2026.
#include "Nodes/Math/MulOp.h"
#include <iostream>

START_NAMESPACE(ATGraph)

static constexpr std::string_view kLeftOperandName("LeftOperand");
static constexpr std::string_view kRightOperandName("RightOperand");
static constexpr std::string_view kOutputAttrName("Output");


void MulOp::compute(const NodeRecord& nodeRecord, DataStore& dStore)
{
    const AttrID leftOperandAttrID = nodeRecord.inputAttrIDs.at(0);
    const AttrID rightOperandAttrID = nodeRecord.inputAttrIDs.at(1);
    const AttrID outputAttrID = nodeRecord.outputAttrIDs.at(0);

    const std::span<const float> rawLeftOperandData = dStore.read<float>(leftOperandAttrID);
    const std::span<const float> rawRightOperandData = dStore.read<float>(rightOperandAttrID);
    if (rawLeftOperandData.size() != rawRightOperandData.size())
    {
        std::cerr << "[MulOp::compute] MulOp requires the left and right operand data count to be equal" << std::endl;
        return;
    }
    std::span<float> writeBufferArea = dStore.prepareWrite<float>(outputAttrID, rawLeftOperandData.size());
    for (int i = 0; i < writeBufferArea.size(); ++i)
        writeBufferArea[i] = rawLeftOperandData[i] * rawRightOperandData[i];
}

void MulOp::initDataSlotDefaultValue(DataSlot& dataSlot, const AttributeDescriptor& attrDescriptor) const
{
    // in the case of MulOp, the only supported type is Float
    assert(attrDescriptor.supportedTypes == AttributeDataType::Float);
    dataSlot.updateConcreteType(AttributeDataType::Float);
}

bool MulOp::changeAttributeDataType(const NodeRecord& nodeRecord,
    AttributeDataType concreteType, AttrID inputAttr, DataStore& dStore)
{
    return false;
}

const std::span<const AttributeDescriptor> MulOp::inputAttrSchema() const
{
    static constexpr AttributeDescriptor kLeftOperandAttrDescriptor(
        kLeftOperandName, AttributeDataType::Float, AttributeDirection::Input);
    static constexpr AttributeDescriptor kRightOperandAttrDescriptor(
        kRightOperandName, AttributeDataType::Float, AttributeDirection::Input);
    static constexpr std::array kInputAttrs = {kLeftOperandAttrDescriptor, kRightOperandAttrDescriptor};
    return kInputAttrs;
}

const std::span<const AttributeDescriptor> MulOp::outputAttrSchema() const
{
    static constexpr AttributeDescriptor kOutputAttrDescriptor(
        kOutputAttrName, AttributeDataType::Float, AttributeDirection::Output);
    static constexpr std::array kOutputAttrs = {kOutputAttrDescriptor};
    return kOutputAttrs;
}

bool MulOp::alwaysCompute() const
{
    return false;
}
const std::string_view MulOp::nodeName() const
{
    return kNodeName;
}
const NodeTypeID MulOp::nodeTypeID() const
{
    return kNodeTypeId;
}

END_NAMESPACE
