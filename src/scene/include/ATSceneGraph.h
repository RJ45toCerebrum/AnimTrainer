// Created by Tyler on 4/21/2026.
#pragma once

#include "common.h"
#include "ATSceneNode.h"
#include <unordered_map>
#include <functional>
#include <random>
#include <expected>


START_NAMESPACE(ATScene)

class NodeFactoryNotFound : public std::runtime_error
{
    NodeTypeID _invalidTypeId;
public:
    explicit NodeFactoryNotFound(const NodeTypeID invalidTypeId) :
        std::runtime_error("Node Factory not found"), _invalidTypeId(invalidTypeId)
    {}
};

/// TODO: think about multi-threading
/// How do we get a separate physics steps
class ATSceneGraph final
{
    using SceneNodePtr = std::unique_ptr<ATSceneNode>;
    static constexpr int kInvalidNodeAddress = 0;
    static constexpr int kTimeNodeAddress = 1;

    // TODO: protect with mutex? When multiple threads come into play I will do thread safe check
    // oon various data structures. Too soon for that right now...
    static std::unordered_map<NodeTypeID, std::unique_ptr<ISceneNodeFactory>> _factories;

    uint64_t _sceneHash = 0;
    NodeID _nextID = 0;
    // node ids correspond to the index of the vector. Index 0 is NEVER used and should always be nullptr.
    // This is because node with ID 0 represents invalid node.
    std::vector<SceneNodePtr> _nodes;

private:
    friend Observer<ATSceneNode> getSceneNode(ATAttributeHandle ah);
    friend ATAttribute& getAttributeRefUnchecked(ATAttributeHandle ah);

    [[nodiscard]] bool isValidNodeID(const NodeID nodeId) const
    {
        if (nodeId == 0 or nodeId >= _nodes.size())
            return false;
        return _nodes[nodeId] != nullptr;
    }
    void evaluateGraph(ATAttributeHandle attrHandle);
    [[nodiscard]] ATAttribute& getAttributeRefUnchecked(ATAttributeHandle ah) const;

public:
    // dependency injection for node factories instead? Could be cleaner than registerNodeType static.
    // TODO: make constructor private
    ATSceneGraph();
    ~ATSceneGraph();

    NodeID createNode(NodeTypeID typeId, std::string_view name);
    bool deleteNode(NodeID nodeId);
    // TODO: Friendly name -> NodeID mapping
    [[nodiscard]] AttrHandleArray getNodeOutputHandles(NodeID nodeId) const;
    [[nodiscard]] AttrHandleArray getNodeInputHandles(NodeID nodeId) const;
    [[nodiscard]] bool isValidAttrHandle(ATAttributeHandle attrHandle) const;
    [[nodiscard]] bool isDirtyAttr(ATAttributeHandle attrHandle) const;
    [[nodiscard]] std::expected<bool,std::string> isInputAttrPlugged(ATAttributeHandle attrHandle) const;

    /// This causes evaluation of the graph; should check AttributeData because this can fail
    [[nodiscard]] std::expected<AttributeData,int> getData(ATAttributeHandle attrHandle);
    /// Any nodes connected to the default time node will be marked dirty
    void update();

    [[nodiscard]] bool willFormCycle(ATAttributeHandle outputHandle, ATAttributeHandle inputHandle) const;
    bool connect(ATAttributeHandle outputHandle, ATAttributeHandle inputHandle);
    bool disconnect(ATAttributeHandle inputHandle);

    // TODO: create iterator. This will do for now...
    //void forEachNodeType(NodeTypeID typeId, const std::function<void(ATSceneNode&)>& visitor) const;

    [[nodiscard]] bool checkReservedAddresses() const;
    // --- Node factory registry ---
    static void registerNodeType(NodeTypeID typeId, std::unique_ptr<ISceneNodeFactory> factoryPtr);
};

void createATSceneGraph();
void destroyATSceneGraph();
std::optional<std::reference_wrapper<ATSceneGraph>> getSceneGraph();

END_NAMESPACE