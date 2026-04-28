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

// Attributes are solely owned by the scene node.
using AttributePtr = std::unique_ptr<ATAttribute>;

class ATSceneNode
{
    friend class ATSceneGraph;

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

    [[nodiscard]] int32_t getAttributeDataCount(ATAttributeHandle ah) const;
    [[nodiscard]] AttributeDataType getAttrDataType(ATAttributeHandle ah) const;
    [[nodiscard]] int32_t getInputAttributeCount() const;
    [[nodiscard]] int32_t getOutputAttributeCount() const;
    [[nodiscard]] AttrHandleArray getInputAttrs() const;
    [[nodiscard]] AttrHandleArray getOutputAttrs() const;
    [[nodiscard]] AttrHandleArray getInputAttrs(AttributeDataType attrDataType) const;
    [[nodiscard]] AttrHandleArray getOutputAttrs(AttributeDataType attrDataType) const;
    [[nodiscard]] AttrHandleArray getDirtyInputAttrs() const;
    // this queries if a specific attribute is dirty.
    [[nodiscard]] bool isDirty(ATAttributeHandle ah) const;
    [[nodiscard]] bool isValidAttrHandle(ATAttributeHandle ah) const;

    /// NOTE: Will return false if ah points to output handle and return false if input handle not plugged
    [[nodiscard]] bool isPlugged(ATAttributeHandle ah) const;
    [[nodiscard]] bool setUnpluggedInputAttrData(ATAttributeHandle ah, AttributeData attrData);

protected:
    [[nodiscard]] ATAttribute& getAttributeRef(ATAttributeHandle ah) const;
    [[nodiscard]] ATAttribute& getAttributeRefUnchecked(ATAttributeHandle ah) const;
    // DOES NOT cause re-evaluation of graph. the attribute must be clean for this call to work...
    [[nodiscard]] AttributeData getAttributeData(ATAttributeHandle ah) const;

    /// where each node type implements node specific logic.
    /// This is called by the Scene Graph when graph evaluation occurs.
    /// i.e. when program entity calls getData on attribute handle.
    virtual void compute(ATAttributeHandle ah) = 0;

    bool plugAttribute(ATAttributeHandle thisNodeAttribute, ATAttributeHandle otherNodeAttribute);
    bool unplugAttribute(ATAttributeHandle ah);
    void markInputDirty(ATAttributeHandle ah);
    void markOutputsDirty();
    void markInputClean(ATAttributeHandle ah);
    void markOutputClean(ATAttributeHandle ah);

    /// ALL nodes should register their attributes in the constructor body AND ONLY constructor body.
    ATAttributeHandle registerInputAttribute(AttributePtr newAttribute);
    ATAttributeHandle registerOutputAttribute(AttributePtr newAttribute);

private:
    [[nodiscard]] ATAttributeHandle convertToHandle(AttributeDirection dataFlow, uint16_t index) const;
    [[nodiscard]] ATAttributeHandle getUpstreamHandle(ATAttributeHandle inputAttribute) const;
};

class ISceneNodeFactory
{
public:
    // TODO: make ability to pass through args via std::forward...
    virtual std::unique_ptr<ATSceneNode> createSceneNode(NodeID newNodeID, const std::string_view& name) = 0;
    virtual ~ISceneNodeFactory() = default;
    [[nodiscard]] virtual NodeTypeID getNodeTypeID() const = 0;
};


END_NAMESPACE
