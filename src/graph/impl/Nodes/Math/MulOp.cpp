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
    assert(attrDescriptor.type == AttributeDataType::Float);
    constexpr std::array defaultBuffer = {0.0f};
    dataSlot.writeAsSpan<float>(defaultBuffer);
}


constexpr std::span<const AttributeDescriptor> MulOp::inputAttrSchema() const
{
    static constexpr AttributeDescriptor kLeftOperandAttrDescriptor(
        kLeftOperandName, AttributeDataType::Float, AttributeDirection::Input);
    static constexpr AttributeDescriptor kRightOperandAttrDescriptor(
        kRightOperandName, AttributeDataType::Float, AttributeDirection::Input);
    static constexpr std::array kInputAttrs = {kLeftOperandAttrDescriptor, kRightOperandAttrDescriptor};
    return kInputAttrs;
}

constexpr std::span<const AttributeDescriptor> MulOp::outputAttrSchema() const
{
    static constexpr AttributeDescriptor kOutputAttrDescriptor(kOutputAttrName, AttributeDataType::Float, AttributeDirection::Output);
    static constexpr std::array kOutputAttrs = {kOutputAttrDescriptor};
    return kOutputAttrs;
}

bool MulOp::alwaysCompute() const
{
    return false;
}
constexpr std::string_view MulOp::nodeName() const
{
    return kNodeName;
}
constexpr NodeTypeID MulOp::nodeTypeID() const
{
    return kNodeTypeId;
}

END_NAMESPACE
