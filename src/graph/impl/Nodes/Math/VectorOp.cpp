// Created by Tyler on 5/6/2026.
#include "Nodes/Math/VectorOp.h"
#include "ATMath.h"

#include <glm/vec2.hpp>
#include <iostream>
#include <ostream>

START_NAMESPACE(ATGraph)

static constexpr std::string_view kVectorOpTypeAttrName = "VectorOpType";
static constexpr std::string_view kLeftOperandAttrName = "LeftOperand";
static constexpr std::string_view kRightOperandAttrName = "RightOperand";
static constexpr std::string_view kVectorOpOutputAttrName = "VectorOpOutput";
static constexpr AttributeDataType kSupportedOperandTypes =
    AttributeDataType::Vec2 | AttributeDataType::Vec3 | AttributeDataType::Vec4;


void VectorOp::compute(const NodeRecord& nodeRecord, DataStore& dStore)
{
    const AttrID vectorOpAttrID = nodeRecord.inputAttrIDs.at(0);
    const AttrID leftOperandAttr = nodeRecord.inputAttrIDs.at(1);
    const AttrID rightOperandAttr = nodeRecord.inputAttrIDs.at(2);
    const AttrID outputAttrID = nodeRecord.outputAttrIDs.at(0);

    const std::span<const float> leftOperandRawData = dStore.read<float>(leftOperandAttr);
    const std::size_t leftOperandSize = leftOperandRawData.size();
    const std::span<const float> rightOperandRawData = dStore.read<float>(rightOperandAttr);
    const std::size_t rightOperandSize = rightOperandRawData.size();
    if (leftOperandSize == 0 or rightOperandSize == 0 or leftOperandSize > 4 or rightOperandSize > 4)
    {
        // later, I can make a node that is more general than this for large scale operations
        std::cerr << "VectorOp::compute: One of the operands has 0 data to compute with" << std::endl;
        return;
    }
    std::array<float,4> resultBuffer = {0.0f,0.0f,0.0f,0.0f};

    const int vectorOpData = dStore.readSingle<int>(vectorOpAttrID);
    const auto vectorOp = static_cast<const VectorOpType>(vectorOpData);
    if (vectorOp == VectorOpType::VectorAdd)
    {
        if (leftOperandSize != rightOperandSize)
        {
            // later, I can make a node that is more general than this for large scale operations
            std::cerr << "VectorOp::compute: Vector operands must have same dimensions and " <<
               " have dimensions <= 4" << std::endl;
            return;
        }

        for (int i = 0; i < leftOperandSize; ++i)
            resultBuffer[i] = leftOperandRawData[i] + rightOperandRawData[i];

        const std::span<const float> resultData(resultBuffer.data(), leftOperandSize);
        dStore.write(outputAttrID, resultData);
    }
    else if (vectorOp == VectorOpType::VectorSub)
    {
        if (leftOperandSize != rightOperandSize)
        {
            // later, I can make a node that is more general than this for large scale operations
            std::cerr << "VectorOp::compute: Vector subtraction requires both vectors to have same dimensions" << std::endl;
            return;
        }

        for (int i = 0; i < leftOperandSize; ++i)
            resultBuffer[i] = leftOperandRawData[i] - rightOperandRawData[i];

        const std::span<const float> resultData(resultBuffer.data(), leftOperandSize);
        dStore.write(outputAttrID, resultData);
    }
    else if (vectorOp == VectorOpType::VectorScale)
    {
        if (leftOperandRawData.size() == 1)
        {
            const float scaler = leftOperandRawData[0];
            for (int i = 0; i < rightOperandRawData.size(); ++i)
                resultBuffer[i] = scaler * rightOperandRawData[i];

            const std::span<const float> resultData(resultBuffer.data(), rightOperandRawData.size());
            dStore.write(outputAttrID, resultData);
        }
        else if (rightOperandRawData.size() == 1)
        {
            const float scaler = rightOperandRawData[0];
            for (int i = 0; i < leftOperandRawData.size(); ++i)
                resultBuffer[i] = scaler * leftOperandRawData[i];

            const std::span<const float> resultData(resultBuffer.data(), leftOperandRawData.size());
            dStore.write(outputAttrID, resultData);
        }
        else
        {
            std::cerr << "VectorOp::compute: For valid vector scale operation, one of the operands must be of size 1 " << std::endl;
            return;
        }
    }
    else if (vectorOp == VectorOpType::VectorDot)
    {
        if (leftOperandSize != rightOperandSize)
        {
            std::cerr << "VectorOp::compute: For valid vector dot operation, both operands must have same dimensions" << std::endl;
            return;
        }

        float dotProduct = 0.0f;
        for (int i = 0; i < leftOperandRawData.size(); ++i)
            dotProduct += leftOperandRawData[i] * rightOperandRawData[i];

        const std::span<const float> resultData(&dotProduct, 1);
        dStore.write(outputAttrID, resultData);
    }
    else if (vectorOp == VectorOpType::VectorAngle)
    {
        if (leftOperandSize != rightOperandSize)
        {
            std::cerr << "VectorOp::compute: For valid vector angle operation, both operands must have same dimensions" << std::endl;
            return;
        }

        float angle = 0.0f;
        if (leftOperandSize == 2)
        {
            const glm::vec2 left(leftOperandRawData[0], leftOperandRawData[1]);
            const glm::vec2 right(rightOperandRawData[0], rightOperandRawData[1]);
            angle = ATMath::angleBetween(left, right);
        }
        else if (leftOperandSize == 3)
        {
            const glm::vec3 left(leftOperandRawData[0], leftOperandRawData[1], leftOperandRawData[2]);
            const glm::vec3 right(rightOperandRawData[0], rightOperandRawData[1],rightOperandRawData[2]);
            angle = ATMath::angleBetween(left, right);
        }
        else
        {
            const glm::vec4 left(leftOperandRawData[0], leftOperandRawData[1], leftOperandRawData[2],leftOperandRawData[3]);
            const glm::vec4 right(rightOperandRawData[0], rightOperandRawData[1],rightOperandRawData[2],rightOperandRawData[3]);
            angle = ATMath::angleBetween(left, right);
        }
        const std::span<const float> resultData(&angle, 1);
        dStore.write(outputAttrID, resultData);
    }
}

void VectorOp::initDataSlotDefaultValue(DataSlot& dataSlot, const AttributeDescriptor& attrDescriptor) const
{
    // vec3 assumed.
    dataSlot.updateConcreteType(AttributeDataType::Vec3);
}

bool VectorOp::changeAttributeDataType(const NodeRecord& nodeRecord,
    const AttributeDataType concreteType, const AttrID inputAttr, DataStore& dStore)
{
    dStore.updateAttributeType(inputAttr, concreteType);
    return true;
}

const std::span<const AttributeDescriptor> VectorOp::inputAttrSchema() const
{
    static constexpr AttributeDescriptor vectorOpTypeIntAttr(
        kVectorOpTypeAttrName, AttributeDataType::Int, AttributeDirection::Input);
    static constexpr AttributeDescriptor leftOperandAttr(
        kLeftOperandAttrName, kSupportedOperandTypes, AttributeDirection::Input);
    static constexpr AttributeDescriptor rightOperandAttr(
        kRightOperandAttrName, kSupportedOperandTypes, AttributeDirection::Input);
    static constexpr std::array<const AttributeDescriptor, 3> inputAttrs = {vectorOpTypeIntAttr, leftOperandAttr, rightOperandAttr};
    return inputAttrs;
}

const std::span<const AttributeDescriptor> VectorOp::outputAttrSchema() const
{
    static constexpr AttributeDescriptor vectorOpOutputAttr(
        kVectorOpOutputAttrName, AttributeDataType::Float, AttributeDirection::Output);
    static constexpr std::array<const AttributeDescriptor, 1> outputAttrs = {vectorOpOutputAttr};
    return outputAttrs;
}

bool VectorOp::alwaysCompute() const
{
    return false;
}
const std::string_view VectorOp::nodeName() const
{
    return kNodeName;
}
const NodeTypeID VectorOp::nodeTypeID() const
{
    return kNodeTypeID;
}

END_NAMESPACE
