// Created by Tyler on 5/6/2026.
#include "Nodes/Math/VectorOp.h"
#include "AttributeTypeHandler.h"
#include "ATMath.h"

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
    const auto leftSize = dStore.elementCount(leftOperandAttr);
    const auto rightSize = dStore.elementCount(rightOperandAttr);
    if (leftSize != rightSize)
    {
        std::cerr << "[VectorOp::compute] left and right operands must have same count: " <<
                     '[' << leftSize << "," << rightSize << ']' << std::endl;
        return;
    }
    const int vectorOpData = dStore.readSingle<int>(vectorOpAttrID);
    const auto vectorOp = static_cast<const VectorOpType>(vectorOpData);
    if (vectorOp == VectorOpType::VectorAdd)
        computeVectorAdd(nodeRecord, dStore);
    else if (vectorOp == VectorOpType::VectorSub)
        computeVectorSub(nodeRecord, dStore);
    else if (vectorOp == VectorOpType::VectorScale)
        computeVectorScale(nodeRecord, dStore);
    else if (vectorOp == VectorOpType::VectorDot)
        computeVectorDot(nodeRecord, dStore);
    else if (vectorOp == VectorOpType::VectorAngle)
        computeVectorAngle(nodeRecord,dStore);
    else
        std::cerr << "[VectorOp::compute] Unknown vector operation type" << std::endl;
}

void VectorOp::computeVectorScale(const NodeRecord& nodeRecord, DataStore& dStore)
{
    const AttrID leftOperandAttr = nodeRecord.inputAttrIDs.at(1);
    const AttrID rightOperandAttr = nodeRecord.inputAttrIDs.at(2);
    const AttrID outputAttrID = nodeRecord.outputAttrIDs.at(0);
    const AttributeDataType dataType = dStore.getConcreteType(rightOperandAttr);
    attributeVecTypeHandler(dataType, [&]<AttributeTypeConcept T>()
    {
        // left side always scaler
        const auto leftOperandData = dStore.read<float>(leftOperandAttr);
        const std::span<const T> rightOperandData = dStore.read<T>(rightOperandAttr);
        assert(leftOperandData.size() == rightOperandData.size());
        std::span<T> writeBuffer = dStore.prepareWrite<T>(outputAttrID, leftOperandData.size());
        for (int i = 0; i < leftOperandData.size(); ++i)
            writeBuffer[i] = leftOperandData[i] * rightOperandData[i];
    });
}
void VectorOp::computeVectorAdd(const NodeRecord& nodeRecord, DataStore& dStore)
{
    const AttrID leftOperandAttr = nodeRecord.inputAttrIDs.at(1);
    const AttrID rightOperandAttr = nodeRecord.inputAttrIDs.at(2);
    const AttrID outputAttrID = nodeRecord.outputAttrIDs.at(0);
    const AttributeDataType dataType = dStore.getConcreteType(rightOperandAttr);
    attributeVecTypeHandler(dataType, [&]<AttributeTypeConcept T>()
    {
        const std::span<const T> leftOperandData = dStore.read<T>(leftOperandAttr);
        const std::span<const T> rightOperandData = dStore.read<T>(rightOperandAttr);
        assert(leftOperandData.size() == rightOperandData.size());
        std::span<T> writeBuffer = dStore.prepareWrite<T>(outputAttrID, leftOperandData.size());
        for (int i = 0; i < leftOperandData.size(); ++i)
            writeBuffer[i] = leftOperandData[i] + rightOperandData[i];
    });
}
void VectorOp::computeVectorSub(const NodeRecord& nodeRecord, DataStore& dStore)
{
    const AttrID leftOperandAttr = nodeRecord.inputAttrIDs.at(1);
    const AttrID rightOperandAttr = nodeRecord.inputAttrIDs.at(2);
    const AttrID outputAttrID = nodeRecord.outputAttrIDs.at(0);
    const AttributeDataType dataType = dStore.getConcreteType(rightOperandAttr);
    attributeVecTypeHandler(dataType, [&]<AttributeTypeConcept T>()
    {
        const std::span<const T> leftOperandData = dStore.read<T>(leftOperandAttr);
        const std::span<const T> rightOperandData = dStore.read<T>(rightOperandAttr);
        assert(leftOperandData.size() == rightOperandData.size());
        std::span<T> writeBuffer = dStore.prepareWrite<T>(outputAttrID, leftOperandData.size());
        for (int i = 0; i < leftOperandData.size(); ++i)
            writeBuffer[i] = leftOperandData[i] - rightOperandData[i];
    });
}
void VectorOp::computeVectorDot(const NodeRecord& nodeRecord, DataStore& dStore)
{
    const AttrID leftOperandAttr = nodeRecord.inputAttrIDs.at(1);
    const AttrID rightOperandAttr = nodeRecord.inputAttrIDs.at(2);
    const AttrID outputAttrID = nodeRecord.outputAttrIDs.at(0);
    const AttributeDataType dataType = dStore.getConcreteType(rightOperandAttr);
    attributeVecTypeHandler(dataType, [&]<AttributeTypeConcept T>()
    {
        const std::span<const T> leftOperandData = dStore.read<T>(leftOperandAttr);
        const std::span<const T> rightOperandData = dStore.read<T>(rightOperandAttr);
        assert(leftOperandData.size() == rightOperandData.size());
        std::span<float> writeBuffer = dStore.prepareWrite<float>(outputAttrID, leftOperandData.size());
        for (int i = 0; i < leftOperandData.size(); ++i)
            writeBuffer[i] = glm::dot(leftOperandData[i], rightOperandData[i]);
    });
}
void VectorOp::computeVectorAngle(const NodeRecord& nodeRecord, DataStore& dStore)
{
    const AttrID leftOperandAttr = nodeRecord.inputAttrIDs.at(1);
    const AttrID rightOperandAttr = nodeRecord.inputAttrIDs.at(2);
    const AttrID outputAttrID = nodeRecord.outputAttrIDs.at(0);
    const AttributeDataType dataType = dStore.getConcreteType(rightOperandAttr);
    attributeVecTypeHandler(dataType, [&]<AttributeTypeConcept T>()
    {
        const std::span<const T> leftOperandData = dStore.read<T>(leftOperandAttr);
        const std::span<const T> rightOperandData = dStore.read<T>(rightOperandAttr);
        assert(leftOperandData.size() == rightOperandData.size());
        std::span<float> writeBuffer = dStore.prepareWrite<float>(outputAttrID, leftOperandData.size());
        for (int i = 0; i < leftOperandData.size(); ++i)
        {
            if (glm::dot(leftOperandData[i], leftOperandData[i]) < 5 * std::numeric_limits<float>::epsilon())
                writeBuffer[i] = 0.0f;
            if (glm::dot(rightOperandData[i], rightOperandData[i]) < 5 * std::numeric_limits<float>::epsilon())
                writeBuffer[i] = 0.0f;

            writeBuffer[i] = ATMath::angleBetween(leftOperandData[i], rightOperandData[i]);
        }
    });
}

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
        dStore.updateAttributeType(nodeRecord.inputAttrIDs[kVectorOpLeftOperandInputIndex], AttributeDataType::Float);
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
        if(rawInputOpTypeData.size() != 1)
        {
            std::cerr << "[VectorOp::setUnpluggedInputAttrData] Vector Op Type input only supports a single operation" << std::endl;
            return false;
        }
        const auto inputOpType = static_cast<const VectorOpType>(rawInputOpTypeData[0]);
        if (inputOpType == currentOpType)
            return true;

        // IF the op type is changing, there is a possibility the other input types must change.
        // For example: IF there is a change from VectorAdd -> VectorScale; the left operand must be a scaler value.
        // This means we must check if we have any connections. Unlike changeAttributeDataType,
        // setUnpluggedInputAttrData can be called while this nodes other attributes have connections.
        // What this means is: users must disconnect the nodes then change the op type before performing this operation.
        for (const AttrID iAid : nodeRecord.inputAttrIDs)
        {
            if (dStore.doesAttrHaveConnection(iAid))
            {
                std::cerr << "[VectorOp::setUnpluggedInputAttrData] Can not vector op type "
                             "while node has connections" << std::endl;
                return false;
            }
        }
        for (const AttrID oAid : nodeRecord.outputAttrIDs)
        {
            if (dStore.doesAttrHaveConnection(oAid))
            {
                std::cerr << "[VectorOp::setUnpluggedInputAttrData] Can not vector op type "
                "while node has connections" << std::endl;
                return false;
            }
        }

        dStore.writeSingle<int>(inputAttrID, rawInputOpTypeData[0]);
        // we are not done as we need to ensure the types make sense for this operation.
        const AttrID leftOperandAID = nodeRecord.inputAttrIDs[kVectorOpLeftOperandInputIndex];
        const AttrID rightOperandAID = nodeRecord.inputAttrIDs[kVectorOpRightOperandInputIndex];
        const AttrID outputAID = nodeRecord.outputAttrIDs[kVectorOpOutputIndex];
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
    // NOTE: only the left operand supports the float for vector scale op
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
