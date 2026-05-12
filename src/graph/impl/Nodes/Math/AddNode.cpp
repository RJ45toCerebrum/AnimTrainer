// Created by Tyler on 5/9/2026.
#include "Nodes/Math/AddNode.h"
#include <iostream>

START_NAMESPACE(ATGraph)

static constexpr uint16_t kLeftAttrIndexer = 0;
static constexpr std::string_view kLeftAttrName = "Left";
static constexpr uint16_t kRightAttrIndexer = 1;
static constexpr std::string_view kRightAttrName = "Right";

static constexpr AttributeDataType kSupportedTypes =
    AttributeDataType::Float | AttributeDataType::Int | AttributeDataType::Vec2 | AttributeDataType::Vec3 | AttributeDataType::Vec4;


template<AttributeTypeConcept DT>
void computeAdd(const AttrID leftAttrID, const AttrID rightAttrID, const AttrID outputAttrID, DataStore& dStore)
{
    const std::span<const DT> leftOperandInput = dStore.read<DT>(leftAttrID);
    const std::span<const DT> rightOperandInput = dStore.read<DT>(rightAttrID);
    if (leftOperandInput.size() != rightOperandInput.size())
    {
        std::cerr << "[AddNode::compute] left and right operands do not have the same size. "
                     "Is this intentional? Output will output size of the min of operand data" << std::endl;
    }
    const std::size_t elementCount = std::min(leftOperandInput.size(), rightOperandInput.size());
    const std::span<DT> writeBuffer = dStore.prepareWrite<DT>(outputAttrID, elementCount);
    for (std::size_t i = 0; i < elementCount; ++i)
        writeBuffer[i] = leftOperandInput[i] + rightOperandInput[i];
}

void AddNode::compute(const NodeRecord& nodeRecord, DataStore& dStore)
{
    assert(nodeRecord.inputAttrIDs.size() == 2);
    assert(nodeRecord.outputAttrIDs.size() == 1);
    const AttrID leftAttr = nodeRecord.inputAttrIDs[kLeftAttrIndexer];
    const AttrID rightAttr = nodeRecord.inputAttrIDs[kRightAttrIndexer];
    const AttrID outputAttrID = nodeRecord.outputAttrIDs[0];

    const AttributeDataType leftAttrDT = dStore.getConcreteType(leftAttr);
    assert(leftAttrDT == dStore.getConcreteType(rightAttr) and
        "[AddNode::compute] AddNode compute requires both operands to be of same type.");

    switch (leftAttrDT)
    {
        case AttributeDataType::Float:
            computeAdd<float>(leftAttr, rightAttr, outputAttrID, dStore);
            break;
        case AttributeDataType::Int:
            computeAdd<int>(leftAttr, rightAttr, outputAttrID, dStore);
            break;
        case AttributeDataType::Vec2:
            computeAdd<glm::vec2>(leftAttr, rightAttr, outputAttrID, dStore);
            break;
        case AttributeDataType::Vec3:
            computeAdd<glm::vec3>(leftAttr, rightAttr, outputAttrID, dStore);
            break;
        case AttributeDataType::Vec4:
            computeAdd<glm::vec4>(leftAttr, rightAttr, outputAttrID, dStore);
            break;
        default:
            throw std::runtime_error("[AddNode::compute] Unknown attribute type.");
    }
}

void AddNode::initDataSlotDefaultValue(DataSlot& dataSlot, const AttributeDescriptor& attrDescriptor) const
{
    dataSlot.updateConcreteType(AttributeDataType::Float);
}

bool AddNode::changeAttributeDataType(const NodeRecord& nodeRecord,
    const AttributeDataType concreteType, const AttrID inputAttr, DataStore& dStore)
{
    dStore.updateAttributeType(nodeRecord.inputAttrIDs[kLeftAttrIndexer], concreteType);
    dStore.updateAttributeType(nodeRecord.inputAttrIDs[kRightAttrIndexer], concreteType);
    dStore.updateAttributeType(nodeRecord.outputAttrIDs.at(0), concreteType);
    return true;
}

const std::span<const AttributeDescriptor> AddNode::inputAttrSchema() const
{
    static constexpr AttributeDescriptor kLeftFloatAttrDesc(kLeftAttrName, kSupportedTypes, AttributeDirection::Input);
    static constexpr AttributeDescriptor kRightFloatAttrDesc(kRightAttrName, kSupportedTypes, AttributeDirection::Input);
    static constexpr std::array<const AttributeDescriptor, 2> inputAttrs = {kLeftFloatAttrDesc, kRightFloatAttrDesc};
    return inputAttrs;
}

const std::span<const AttributeDescriptor> AddNode::outputAttrSchema() const
{
    static constexpr AttributeDescriptor kOutFloatAttrDesc("Out", kSupportedTypes, AttributeDirection::Output);
    static constexpr std::array<const AttributeDescriptor, 1> outputAttrs = {kOutFloatAttrDesc};
    return outputAttrs;
}

bool AddNode::alwaysCompute() const
{
    return false;
}
const std::string_view AddNode::nodeName() const
{
    return kNodeName;
}
const NodeTypeID AddNode::nodeTypeID() const
{
    return kNodeTypeId;
}

END_NAMESPACE