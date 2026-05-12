// Created by Tyler on 5/6/2026.
#pragma once

#include "NodeCompute.h"

START_NAMESPACE(ATGraph)

/* Vector Operations Node:
 * The vector operations node can do a variety of vector operations such as:
 * 1) multiply vector by scalar.
 * 2) vector dot product
 * 3) Angle Between
 *
 * IT does NOT define cross product. There will be a different node for that.
 *
 *  The int attribute determines the operation type. And therefore the size and interpretation of the output.
 *  EXAMPLE: if the operation is VectorScale, the left attr is interpreted as a single float.
 *  IF the operation is VectorCross, the left attribute (left operand) is interpreted as a vector and the vectors
 *  must have the same dimensions.
 *
 * Input attrs:
 * [int, float[], vec]
 * Output attrs:
 * [float[]]
 */
class VectorOp : public INodeCompute
{
public:
    enum class VectorOpType : uint8_t
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
    NDESC const std::span<const AttributeDescriptor> inputAttrSchema() const override;
    NDESC const std::span<const AttributeDescriptor> outputAttrSchema() const override;
    NDESC const std::string_view nodeName() const override;
    NDESC const NodeTypeID nodeTypeID() const override;
};

END_NAMESPACE