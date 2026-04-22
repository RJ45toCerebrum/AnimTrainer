// Created by Tyler on 4/21/2026.
#pragma once

#include "common.h"
#include "ATSceneNode.h"
#include <unordered_map>
#include <functional>
#include <random>

START_NAMESPACE(ATScene)

class ATSceneGraph
{
    static std::unordered_map<NodeTypeID, std::unique_ptr<ISceneNodeFactory>> _factories;
    uint64_t _sceneHash = 0;
    NodeID _nextID = 0;
    std::unordered_map<NodeID, std::unique_ptr<ATSceneNode>> _nodes;

private:
    ATSceneGraph()
    {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dis;
        _sceneHash = dis(gen);
    }
    ~ATSceneGraph();

    NodeID nextNodeId();
    friend Observer<ATSceneNode> getSceneNode(const ATAttributeHandle& ah);

public:
    NodeID createNode(NodeTypeID typeId, std::string name);
    bool deleteNode(NodeID nodeId);
    [[nodiscard]] AttrHandleArray getNodeOutputHandles(NodeID nodeId) const;
    [[nodiscard]] AttrHandleArray getNodeInputHandles(NodeID nodeId) const;
    [[nodiscard]] bool isValidAttrHandle(const ATAttributeHandle& attrHandle) const;
    [[nodiscard]] bool isDirtyAttr(const ATAttributeHandle& attrHandle) const;
    [[nodiscard]] std::expected<bool,std::string> isInputAttrPlugged(const ATAttributeHandle& attrHandle) const;

    // this causes evaluation of the graph; should check because this can fail
    [[nodiscard]] AttributeData getData(const ATAttributeHandle& attrHandle) const;

    [[nodiscard]] bool willFormCycle(const ATAttributeHandle& outputHandle, const ATAttributeHandle& inputHandle) const;
    bool connect(const ATAttributeHandle& outputHandle, const ATAttributeHandle& inputHandle);
    bool disconnect(const ATAttributeHandle& inputHandle);

    // TODO: create iterator. This will do for now...
    void forEachNodeType(NodeTypeID typeId, const std::function<void(ATSceneNode&)>& visitor) const;

    // --- Node factory registry ---
    static void registerNodeType(NodeTypeID typeId, std::unique_ptr<ISceneNodeFactory> factoryPtr);
};

std::unique_ptr<ATSceneGraph> createATSceneGraph();
std::optional<std::reference_wrapper<ATSceneGraph>> getSceneGraph();

END_NAMESPACE