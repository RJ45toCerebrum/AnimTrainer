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

static constexpr int kVectorOpTypeInputIndex = 0;
static constexpr int kVectorOpLeftOperandInputIndex = 1;
static constexpr int kVectorOpRightOperandInputIndex = 2;
static constexpr int kVectorOpOutputIndex = 0;


void VectorOp::compute(const NodeRecord& nodeRecord, DataStore& dStore)
{
    const AttrID vectorOpAttrID = nodeRecord.inputAttrIDs.at(0);
    const AttrID leftOperandAttr = nodeRecord.inputAttrIDs.at(1);
    const AttrID rightOperandAttr = nodeRecord.inputAttrIDs.at(2);
    const AttrID outputAttrID = nodeRecord.outputAttrIDs.at(0);

    if (dStore.elementCount(leftOperandAttr) != dStore.elementCount(rightOperandAttr))
    {
        std::cerr << "[VectorOp::compute] left and right operands must have same count" << std::endl;
        return;
    }

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

void VectorOp::computeVectorScale(const NodeRecord& nodeRecord, DataStore& dStore)
{
    const AttrID leftOperandAttr = nodeRecord.inputAttrIDs.at(1);
    const AttrID rightOperandAttr = nodeRecord.inputAttrIDs.at(2);
    const AttrID outputAttrID = nodeRecord.outputAttrIDs.at(0);



    const AttributeDataType dataType = dStore.getConcreteType(rightOperandAttr);
    if (dataType == AttributeDataType::Vec2)
    {

    }
    else if (dataType == AttributeDataType::Vec3)
    {

    }
    else
    {

    }
}

void VectorOp::computeVectorAdd(const NodeRecord &nodeRecord, DataStore &dStore) {}
void VectorOp::computeVectorSub(const NodeRecord &nodeRecord, DataStore &dStore) {}
void VectorOp::computeVectorDot(const NodeRecord &nodeRecord, DataStore &dStore) {}
void VectorOp::computeVectorAngle(const NodeRecord &nodeRecord, DataStore &dStore) {}

void VectorOp::initDataSlotDefaultValue(DataSlot& dataSlot, const AttributeDescriptor& attrDescriptor) const
{
    // this method should only be called when node first created
    assert(dataSlot.getConcreteType() == AttributeDataType::Invalid);
    if (attrDescriptor.name == kVectorOpTypeAttrName)
    {
        dataSlot.updateConcreteType(AttributeDataType::Int);
        const auto writeBuffer = dataSlot.prepareWrite<int>(1);
        writeBuffer[0] = static_cast<int>(VectorOpType::VectorScale);
    }
    else if (attrDescriptor.name == kLeftOperandAttrName)
    {
        dataSlot.updateConcreteType(AttributeDataType::Float);
    }
    else
    {
        dataSlot.updateConcreteType(AttributeDataType::Vec3);
    }
}

bool VectorOp::changeAttributeDataType(const NodeRecord& nodeRecord,
    const AttributeDataType concreteType, const AttrID inputAttr, DataStore& dStore)
{
    const int attrIndex = nodeRecord.getAttrIndex(inputAttr);
    assert(attrIndex > -1);
    if (attrIndex == kVectorOpTypeInputIndex)
    {
        // only one allowed type; and nothing to do just exit.
        assert(concreteType == AttributeDataType::Int);
        return true;
    }
    if (concreteType == AttributeDataType::Float)
    {
        // we must convert the VectorOp to VectorScale as that is the only valid configuration
        // Additionally, we only support float on left operand to keep things simpler.
        assert(attrIndex == kVectorOpLeftOperandInputIndex);
        const AttrID vectorOpAttrID = nodeRecord.inputAttrIDs[kVectorOpTypeInputIndex];
        dStore.writeSingle(vectorOpAttrID, static_cast<int>(VectorOpType::VectorScale));
        dStore.updateAttributeType(inputAttr, concreteType);
        return true;
    }
    // all other vector operations require both operands to be the same type
    // we just default to vector add op type.
    const AttrID vectorOpAttrID = nodeRecord.inputAttrIDs[kVectorOpTypeInputIndex];
    dStore.writeSingle(vectorOpAttrID, static_cast<int>(VectorOpType::VectorAdd));
    dStore.updateAttributeType(nodeRecord.inputAttrIDs[kVectorOpLeftOperandInputIndex], concreteType);
    dStore.updateAttributeType(nodeRecord.inputAttrIDs[kVectorOpRightOperandInputIndex], concreteType);
    return true;
}

bool VectorOp::setUnpluggedInputAttrData(const AttrID inputAttrID, const std::span<const std::byte> data,
    const AttributeDataType concreteType, const NodeRecord& nodeRecord, DataStore& dStore)
{
    const int attrIndex = nodeRecord.getAttrIndex(inputAttrID);
    assert(attrIndex != -1 and
        "[VectorOp::setUnpluggedAttrData] Node record does not contain an input attribute ID."
        " This can happen if the caller of NodeHandle passed in input attr ID of different node or output attr id.");

    if (attrIndex == kVectorOpTypeInputIndex)
    {
        assert(concreteType == AttributeDataType::Int);
        // read single because this input should only ever have 1 value that applies to all inputs.
        const int currentRawVectorOpType = dStore.readSingle<int>(inputAttrID);
        const auto currentOpType = static_cast<const VectorOpType>(currentRawVectorOpType);
        const std::span<const int> rawInputOpTypeData = DataSlot::convert<int>(data);
        assert(rawInputOpTypeData.size() == 1);
        const auto inputOpType = static_cast<const VectorOpType>(rawInputOpTypeData[0]);
        if (inputOpType == currentOpType)
            return true;

        // IF the op type is changing, there is a possibility the other input types must change.
        // For example: IF there is a change from VectorAdd -> VectorScale; the left or right operand must be a scaler value.
        // This means we must check if we have any connections. We only allow changes to VectorOpType when there is
        // no existing connections.
        for (const AttrID iAid : nodeRecord.inputAttrIDs)
        {
            const AttributeRecord& attrRec = dStore.getAttributeRecord(iAid);
            if (attrRec.hasInputSources())
                return false;
        }
        for (const AttrID oAid : nodeRecord.outputAttrIDs)
        {
            const AttributeRecord& attrRec = dStore.getAttributeRecord(oAid);
            if (attrRec.hasOutputSources())
                return false;
        }

        dStore.writeSingle<int>(inputAttrID, rawInputOpTypeData[0]);
        // we are not done as we need to ensure the types make sense for this operation.
        const AttrID leftOperandAID = nodeRecord.inputAttrIDs[kVectorOpLeftOperandInputIndex];
        const AttrID rightOperandAID = nodeRecord.inputAttrIDs[kVectorOpRightOperandInputIndex];
        const AttrID outputAID = nodeRecord.outputAttrIDs[kVectorOpOutputIndex];
        // NOTE: after changing the vector op type, we always reset the default values for that vector operation.
        // For VectorAdd its vec3 + vec3
        if (inputOpType == VectorOpType::VectorAdd or inputOpType == VectorOpType::VectorSub)
        {
            dStore.updateAttributeType(leftOperandAID, AttributeDataType::Vec3);
            dStore.updateAttributeType(rightOperandAID, AttributeDataType::Vec3);
            dStore.updateAttributeType(outputAID, AttributeDataType::Vec3);
        }
        else if (inputOpType == VectorOpType::VectorScale)
        {
            dStore.updateAttributeType(leftOperandAID, AttributeDataType::Float);
            dStore.updateAttributeType(rightOperandAID, AttributeDataType::Vec3);
            dStore.updateAttributeType(outputAID, AttributeDataType::Vec3);
        }
        else if (inputOpType == VectorOpType::VectorDot or inputOpType == VectorOpType::VectorAngle)
        {
            dStore.updateAttributeType(leftOperandAID, AttributeDataType::Vec3);
            dStore.updateAttributeType(rightOperandAID, AttributeDataType::Vec3);
            dStore.updateAttributeType(outputAID, AttributeDataType::Float);
        }
        else
            return false;
    }
    else if (attrIndex == kVectorOpLeftOperandInputIndex)
    {
        // unlike the vector op type input, we do not attempt changing types.
        const AttrID leftOperandAID = nodeRecord.inputAttrIDs[kVectorOpLeftOperandInputIndex];
        const AttributeDataType currentType = dStore.getConcreteType(leftOperandAID);
        if (concreteType != currentType)
        {
            std::cerr << "[VectorOp::setUnpluggedInputAttrData] Input data type does not match "
                         "the current attribute concrete type of left operand" << std::endl;
            return false;
        }
        dStore.writeRawBytes(leftOperandAID, data);
    }
    else
    {
        const AttrID rightOperandAID = nodeRecord.inputAttrIDs[kVectorOpRightOperandInputIndex];
        const AttributeDataType currentType = dStore.getConcreteType(rightOperandAID);
        if (concreteType != currentType)
        {
            std::cerr << "[VectorOp::setUnpluggedInputAttrData] Input data type does not match "
                         "the current attribute concrete type of right operand" << std::endl;
            return false;
        }
        dStore.writeRawBytes(rightOperandAID, data);
    }
    return true;
}

const std::span<const AttributeDescriptor> VectorOp::inputAttrSchema() const
{
    static constexpr AttributeDataType kSupportedLeftOperandTypes =
        AttributeDataType::Float | AttributeDataType::Vec2 | AttributeDataType::Vec3 | AttributeDataType::Vec4;
    static constexpr AttributeDataType kSupportedRightOperandTypes =
        AttributeDataType::Vec2 | AttributeDataType::Vec3 | AttributeDataType::Vec4;

    static constexpr AttributeDescriptor vectorOpTypeIntAttr(
        kVectorOpTypeAttrName, AttributeDataType::Int, AttributeDirection::Input);
    static constexpr AttributeDescriptor leftOperandAttr(
        kLeftOperandAttrName, kSupportedLeftOperandTypes, AttributeDirection::Input);
    static constexpr AttributeDescriptor rightOperandAttr(
        kRightOperandAttrName, kSupportedRightOperandTypes, AttributeDirection::Input);
    static constexpr std::array<const AttributeDescriptor, 3> inputAttrs = {vectorOpTypeIntAttr, leftOperandAttr, rightOperandAttr};
    return inputAttrs;
}
const std::span<const AttributeDescriptor> VectorOp::outputAttrSchema() const
{
    static constexpr AttributeDataType kSupportedOutputTypes =
        AttributeDataType::Float | AttributeDataType::Vec2 | AttributeDataType::Vec3 | AttributeDataType::Vec4;
    static constexpr AttributeDescriptor vectorOpOutputAttr(
        kVectorOpOutputAttrName, kSupportedOutputTypes, AttributeDirection::Output);
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
