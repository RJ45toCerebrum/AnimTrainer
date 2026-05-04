// Created by Tyler on 4/30/2026.
#pragma once

#include "common.h"
#include "NodeCompute.h"
#include <stdexcept>
#include <memory>
#include <unordered_map>
#include <expected>

START_NAMESPACE(ATGraph)

using NodePtr = std::unique_ptr<INodeCompute>;
using GraphRef = std::optional<std::reference_wrapper<class SceneGraph>>;

class NodeTypeNotFound : public std::runtime_error
{
    NodeTypeID _invalidTypeId;
public:
    explicit NodeTypeNotFound(const NodeTypeID invalidTypeId) :
        std::runtime_error("Node Factory not found"), _invalidTypeId(invalidTypeId)
    {}
};


class SceneGraph final
{
    // NodeHandle should never reach directly into graph types;
    // ONLY call private methods.
    friend class NodeHandle;
    // TODO: figure out why this is not working as expected with gtest fixture.
    friend class GraphTestFixture;

    using GraphPtr = std::unique_ptr<SceneGraph>;
    using NodePtr = std::unique_ptr<INodeCompute>;

    static constexpr int kInvalidNodeAddress = 0;
    static constexpr int kTimeNodeAddress = 1;

    // only ever one instance of the graph at a time and only changes on scene load.
    static GraphPtr _instance;
    static std::unordered_map<NodeTypeID, NodePtr> _nodeTypeMap;

    std::vector<NodeRecord> _nodeRecords;
    // for initial implementation; _attributeRecords.size() should always equal _dataSlots.size()
    // In other words, if a nodes input attribute becomes plugged, that node now gathers data from
    // upstream data slot. Which makes that nodes input attr data slot a tombstone;
    std::vector<AttributeRecord> _attributeRecords;
    std::vector<DataSlot> _dataSlots;

    uint64_t _sceneHash = 0;
    bool _topoChanged = true;
    // this should only ever change when there are structural changes to graph.
    // create/delete, connect/disconnect
    std::vector<NodeID> _evaluationOrder;
    // _nodeNames.size() should always equal _nodeRecords.size()
    std::vector<std::string> _nodeNames;

public:
    SceneGraph();
    ~SceneGraph() = default;

    // TODO: make command queue for methods that make graph structural changes.
    NodeHandle createNode(NodeTypeID typeID, std::string_view name);
    bool deleteNode(NodeID nodeID);
    /// Important Note on connection:
    /// Cycle detection occurs during graph evaluation. This means a connection call can succeed but
    /// forms a cycles which is invalid. An error log will occur and the connection will be removed.
    bool connect(AttrID outputAttr, AttrID inputAttr);
    bool disconnect(AttrID outputAttr, AttrID inputAttr);

    void evaluate();
    NDESC bool topologyChanged() const;

    static void registerNodeType(NodePtr nodeCompute);
    static SceneGraph& instance();
    static void destroy();

private:
    NDESC bool isValidAttrID(AttrID attrID) const;
    NDESC bool isValidInputAttrID(AttrID attrID) const;
    NDESC bool isValidOutputAttrID(AttrID attrID) const;
    NDESC bool isValidNodeID(NodeID nodeID) const;
    NDESC bool isTombstone(NodeID nodeID) const;
    /// will return false if it's not input attribute or if input attr has no upstream input
    NDESC bool hasUpstreamInput(AttrID attrID) const;
    NDESC uint64_t getAttrComputeCode(AttrID aid) const;
    NDESC bool getAttrInfo(NodeID nodeID, AttributeDirection dir, std::span<AttrInfo> attrInfos) const;
    NDESC std::vector<AttrInfo> getAttrInfo(NodeID nodeID, AttributeDirection dir) const;
    NDESC bool nodeNeedsCompute(const NodeRecord& node) const;
    NDESC bool canConnect(AttrID outputAttr, AttrID inputAttr) const;
    NDESC bool willFormCycle(AttrID outputAttr, AttrID inputAttr) const;
    void rebuildEvaluationOrder();

    NDESC std::span<const std::byte> getData(AttrID attrID) const;
    bool setUnpluggedInputAttrData(AttrID attrID, std::span<const std::byte> data);
    // maps the node id and an Attribute Index -> attribute ID.
    // The attributeID is a unique globally identifying ID where the attribute index only identifies it within a node.
    NDESC std::optional<AttrID> fromNodeAttributeIndex(NodeID nodeID, int attrIndex, AttributeDirection dir) const;
};

class NDESC NodeHandle final
{
    const NodeID _nodeID;
    const NodeTypeID _nodeTypeID;

public:
    NodeHandle(const NodeID nodeID, const NodeTypeID nodeTypeID) :
        _nodeID(nodeID), _nodeTypeID(nodeTypeID)
    {}
    NodeHandle() :
        _nodeID(kInvalidNodeID), _nodeTypeID(kInvalidNodeTypeID)
    {}
    NDESC bool isValid() const
    {
        if (_nodeID == kInvalidNodeID or _nodeTypeID == kInvalidNodeTypeID)
            return false;
        const SceneGraph& gr = SceneGraph::instance();
        return gr.isValidNodeID(_nodeID);
    }
    NDESC NodeID nodeID() const
    {
        return _nodeID;
    }
    NDESC NodeTypeID nodeTypeID() const
    {
        return _nodeTypeID;
    }
    /// Maps the Attribute Index => AttrID
    /// AttrID is globally identifying ID and Attribute index is the indexer into attr arrays of node record.
    NDESC std::optional<AttrID> fromNodeAttributeIndex(const int attrIndex, const AttributeDirection dir) const
    {
        const SceneGraph& gr = SceneGraph::instance();
        return gr.fromNodeAttributeIndex(_nodeID, attrIndex, dir);
    }

    // std::array to avoid heap; this will fail if template param N is not the actual attr count for this node.
    template<int N>
    bool inputAttrInfo(std::array<AttrInfo,N>& attrInfo) const
    {
        const SceneGraph& graphRef = SceneGraph::instance();
        return graphRef.getAttrInfo(_nodeID, AttributeDirection::Input, attrInfo);
    }
    template<int N>
    void outputAttrInfo(std::array<AttrInfo,N>& attrInfo) const
    {
        const SceneGraph& graphRef = SceneGraph::instance();
        return graphRef.getAttrInfo(_nodeID, AttributeDirection::Output, attrInfo);
    }

    NDESC std::vector<AttrInfo> inputAttrInfo() const
    {
        const SceneGraph& graphRef = SceneGraph::instance();
        return graphRef.getAttrInfo(_nodeID, AttributeDirection::Input);
    }

    /// NOT thread safe. This is not an issue currently, as the graph will be running on main thread where
    /// data is grabbed, BUT this will become a problem in the future.
    /// BE CAREFUL with this. NEVER store these. IF you are not using the data immediately
    /// after this call, you MUST copy the data. This method does not copy the data. The span points directly
    /// into the DataSlots internal buffer. IF data slot reallocated, this is a dangling pointer. Use wisely.
    template<AttributeTypeConcept T>
    std::span<const T> getData(const AttrID attrID) const
    {
        const SceneGraph& gr = SceneGraph::instance();
        // TODO: should make the graph getData method a template...
        const AttributeRecord& arec = gr._attributeRecords[attrID];
        assert(arec.owner == _nodeID);
        constexpr AttributeDataType attrType = enumFromAttrType<T>();
        assert(arec.type == attrType);
        const std::span<const std::byte> byteData = gr.getData(attrID);
        return DataSlot::convert<T>(byteData);
    }

    template<AttributeTypeConcept T>
    bool setUnpluggedInputAttrData(const AttrID attrID, const std::span<const T> data)
    {
        SceneGraph& gr = SceneGraph::instance();
        assert(gr.isValidAttrID(attrID));
        assert(gr.isValidNodeID(_nodeID));
        // TODO: need to fix. violation of my rule. This will due for now.
        const AttributeRecord& arec = gr._attributeRecords[attrID];
        // never allow setting of attribute if it does not belong to the node this handle belongs to.
        assert(arec.owner == _nodeID);
        constexpr AttributeDataType attrType = enumFromAttrType<T>();
        assert(arec.type == attrType);
        const std::span<const std::byte> byteData = DataSlot::convert(data);
        return gr.setUnpluggedInputAttrData(attrID, byteData);
    }

    template<AttributeTypeConcept T>
    bool setUnpluggedInputAttrData(const AttrID attrID, const T data)
    {
        std::span<const T> dataSpan(&data, 1);
        return setUnpluggedInputAttrData(attrID, dataSpan);
    }

    template<AttributeTypeConcept T>
    bool setUnpluggedInputByIndex(const int attrIndex, std::span<const T> data)
    {
        const SceneGraph& gr = SceneGraph::instance();
        const auto attrIDOpt = gr.fromNodeAttributeIndex(_nodeID, attrIndex, AttributeDirection::Input);
        if (not attrIDOpt)
            return false;
        const AttrID attrID = attrIDOpt.value();
        return setUnpluggedInputAttrData(attrID, data);
    }

    template<AttributeTypeConcept T>
    bool setUnpluggedInputByIndex(const int attrIndex, const T data)
    {
        const std::span<const T> dataSpan(&data, 1);
        return setUnpluggedInputByIndex(attrIndex, dataSpan);
    }
};

END_NAMESPACE