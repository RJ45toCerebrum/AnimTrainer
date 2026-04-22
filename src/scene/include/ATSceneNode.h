//
// Created by Tyler on 4/20/2026.
//
#pragma once

#include "common.h"
#include "ATAttribute.h"

START_NAMESPACE(ATScene)

using NodeTypeID = uint64_t;

// this compile-time function is primarily used by node factory for type IDs.
// type ID should be a compile-time constant.
consteval NodeTypeID atHashString(const std::string_view str)
{
    constexpr uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325ULL;
    uint64_t hash = FNV_OFFSET_BASIS;
    for (const char c : str)
    {
        constexpr uint64_t FNV_PRIME = 0x00000100000001B3ULL;
        hash ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
        hash *= FNV_PRIME;
    }
    return hash;
}

class ATSceneNode
{
    friend class ATSceneGraph;

    // Attributes are solely owned by the scene node.
    using AttributePtr = std::unique_ptr<ATAttribute>;

    const NodeID _nodeID;
    std::string _name;
    std::vector<AttributePtr> _inputAttributes;
    std::vector<AttributePtr> _outputAttributes;

public:
    // TODO: pass in polymorphic pool resource.
    ATSceneNode(const NodeID nodeID, const std::string_view& name) :
        _nodeID(nodeID), _name(name)
    {
        _inputAttributes.reserve(3);
        _outputAttributes.reserve(3);
    }
    virtual ~ATSceneNode() = default;

    [[nodiscard]] NodeID getNodeID() const;
    [[nodiscard]] virtual NodeTypeID getNodeTypeID() const = 0;
    [[nodiscard]] const std::string& getName() const;
    void setName(const std::string& newName);

    [[nodiscard]] int32_t getAttributeDataCount(const ATAttributeHandle& ah) const;
    [[nodiscard]] AttributeDataType getAttrDataType(const ATAttributeHandle& ah) const;
    [[nodiscard]] int32_t getInputAttributeCount() const;
    [[nodiscard]] int32_t getOutputAttributeCount() const;
    [[nodiscard]] AttrHandleArray getInputAttrs() const;
    [[nodiscard]] AttrHandleArray getOutputAttrs() const;
    [[nodiscard]] AttrHandleArray getInputAttrs(AttributeDataType attrDataType) const;
    [[nodiscard]] AttrHandleArray getOutputAttrs(AttributeDataType attrDataType) const;
    // First value in pair => is the attr handle valid for this scene node?
    // second value => was it dirty?
    [[nodiscard]] bool isDirty(const ATAttributeHandle& ah) const;
    [[nodiscard]] bool isValidAttrHandle(const ATAttributeHandle& ah) const;
    /// NOTE: Will return false if ah points to output handle and return false if input handle not plugged
    [[nodiscard]] bool isPlugged(const ATAttributeHandle& ah) const;
    [[nodiscard]] bool setUnpluggedInputAttrData(const ATAttributeHandle& ah, const AttributeData& attrData);

protected:
    [[nodiscard]] const ATAttribute& getAttributeRef(const ATAttributeHandle& ah) const;
    [[nodiscard]] const ATAttribute& getAttributeRefUnchecked(const ATAttributeHandle& ah) const;
    /// where each node type implements node specific logic.
    /// This is called by the Scene Graph when graph evaluation occurs.
    /// i.e. when program entity calls getData on attribute handle.
    virtual void compute(const ATAttributeHandle& ah) = 0;
    void markOutputsDirty();
    void markInputDirty(const ATAttributeHandle& ah);
    /// ALL nodes should register their attributes in the constructor body AND ONLY constructor body.
    ATAttributeHandle registerInputAttribute(AttributePtr newAttribute);
    ATAttributeHandle registerOutputAttribute(AttributePtr newAttribute);

private:
    [[nodiscard]] ATAttributeHandle convertToHandle(AttributeDirection dataFlow, uint16_t index) const;
};

class ISceneNodeFactory
{
public:
    // TODO: make ability to pass through args via std::forward...
    virtual std::unique_ptr<ATSceneNode> createSceneNode() = 0;
    virtual ~ISceneNodeFactory() = default;
    [[nodiscard]] virtual NodeTypeID getNodeTypeID() const = 0;
};


END_NAMESPACE
