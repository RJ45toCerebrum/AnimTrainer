// Created by Tyler on 4/29/2026.
#include "Graph.h"

#include <queue>
#include <random>
#include <iostream>

START_NAMESPACE(ATGraph)

SceneGraph::GraphPtr SceneGraph::_instance = nullptr;
std::unordered_map<NodeTypeID, NodePtr> SceneGraph::_nodeTypeMap;

void SceneGraph::registerNodeType(NodePtr nodeCompute)
{
    const NodeTypeID nodeTypeID = nodeCompute->nodeTypeID();
    const auto fitr = _nodeTypeMap.find(nodeTypeID);
    if (fitr != _nodeTypeMap.end())
    {
        std::cerr << "[SceneGraph::registerNodeType] attempt to register the same node type" << std::endl;
        return;
    }
    if (const auto [_,success] =
        _nodeTypeMap.insert({nodeTypeID, std::move(nodeCompute)}); !success)
        std::cerr << "[SceneGraph::registerNodeType] Failed to insert node." << std::endl;
}

SceneGraph& SceneGraph::instance()
{
    if (not _instance)
        _instance = std::make_unique<SceneGraph>();
    return *_instance;
}

void SceneGraph::destroy()
{
    if (not _instance)
    {
        std::cerr << "[SceneGraph::destroy] attempting to destroy graph when none exists" << std::endl;
        return;
    }
    _instance.reset();
}


SceneGraph::SceneGraph()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    _sceneHash = dis(gen);

    NodeRecord invalidRecord;
    _nodeRecords.push_back(std::move(invalidRecord));
    _nodeNames.emplace_back("InvalidNodeID");
}

NodeHandle SceneGraph::createNode(const NodeTypeID typeID, const std::string_view name)
{
    const auto fitr = _nodeTypeMap.find(typeID);
    if (fitr == _nodeTypeMap.end())
    {
        std::cerr << "[SceneGraph::registerNodeType] Node type " << typeID << " not found" << std::endl;
        return {};
    }
    const INodeCompute& nodeCompute = *fitr->second;
    assert(typeID == nodeCompute.nodeTypeID());

    const NodeID newNodeRecordID = _nodeRecords.size();
    assert(newNodeRecordID != kInvalidNodeID);
    NodeRecord newNodeRecord;
    newNodeRecord.typeID = typeID;

    const std::span<const AttributeDescriptor> inputAttrDescriptors = nodeCompute.inputAttrSchema();
    for (const AttributeDescriptor& iDesc : inputAttrDescriptors)
    {
        AttributeRecord inputAttrRecord;
        inputAttrRecord.owner = newNodeRecordID;
        inputAttrRecord.direction = iDesc.direction;
        inputAttrRecord.type = iDesc.type;
        inputAttrRecord.upstream = kInvalidAttr;

        _attributeRecords.push_back(std::move(inputAttrRecord));
        newNodeRecord.inputAttrIDs.push_back(_attributeRecords.size() - 1);
        newNodeRecord.lastSeenVersions.push_back(0);

        // every attr needs a data slot
        DataSlot newDataSlot;
        // NOTE how I intentionally mismatch the data slot version and lastSeenVersions
        // this is important as we want to compute to be called initially.
        newDataSlot.version = newNodeRecord.lastSeenVersions.back() + 1;
        _dataSlots.push_back(std::move(newDataSlot));
        assert(_attributeRecords.size() == _dataSlots.size());
    }
    const std::span<const AttributeDescriptor> outputAttrDescriptors = nodeCompute.outputAttrSchema();
    for (const AttributeDescriptor& outDesc : outputAttrDescriptors)
    {
        AttributeRecord outAttrRecord;
        outAttrRecord.owner = newNodeRecordID;
        outAttrRecord.direction = outDesc.direction;
        outAttrRecord.type = outDesc.type;
        outAttrRecord.upstream = kInvalidAttr;

        _attributeRecords.push_back(std::move(outAttrRecord));
        newNodeRecord.outputAttrIDs.push_back(_attributeRecords.size() - 1);

        DataSlot newDataSlot;
        newDataSlot.version = 1;
        _dataSlots.push_back(std::move(newDataSlot));
        assert(_attributeRecords.size() == _dataSlots.size());
    }

    assert(newNodeRecord.inputAttrIDs.size() == newNodeRecord.lastSeenVersions.size());
    _nodeRecords.push_back(std::move(newNodeRecord));
    _nodeNames.emplace_back(name);
    assert(_nodeRecords.size() == _nodeNames.size());

    _topoChanged = true;
    const NodeID nid = _nodeRecords.size() - 1;
    return {nid, typeID};
}

bool SceneGraph::deleteNode(const NodeID nodeID)
{
    throw std::logic_error("[SceneGraph::deleteNode] Node ID not implemented");
    _topoChanged = true;
}

bool SceneGraph::connect(const AttrID outputAttr, const AttrID inputAttr)
{
    if (not isValidAttrID(outputAttr))
    {
        std::cerr << "[SceneGraph::connect] Invalid output attribute ID " << outputAttr << std::endl;
        return false;
    }
    if (not isValidAttrID(inputAttr))
    {
        std::cerr << "[SceneGraph::connect] Invalid input attribute ID " << inputAttr << std::endl;
        return false;
    }
    AttributeRecord& inputAttrRec = _attributeRecords[inputAttr];
    AttributeRecord& outputAttrRec = _attributeRecords[outputAttr];
    if (inputAttrRec.type != outputAttrRec.type)
    {
        std::cerr << "[SceneGraph::connect] The attributes being connected have incompatible types" << std::endl;
        return false;
    }
    if (not inputAttrRec.isInputAttr())
    {
        std::cerr << "[SceneGraph::connect] The input attribute passed in is actually an output attr" << std::endl;
        return false;
    }
    if (inputAttrRec.hasInputSources())
    {
        std::cerr << "[SceneGraph::connect] Input attribute already has a source plugged in" << std::endl;
        return false;
    }
    // something went wrong because input attribute does not have the upstream source
    // but the output attribute already has a downstream of inputAttr. Something went wrong with disconnect?
    assert(not outputAttrRec.hasOutputSource(inputAttr));

    inputAttrRec.upstream = outputAttr;
    outputAttrRec.downstream.push_back(inputAttr);
    _topoChanged = true;
    return true;
}

bool SceneGraph::disconnect(const AttrID outputAttr, const AttrID inputAttr)
{
    throw std::logic_error("[SceneGraph::deleteNode] Node ID not implemented");
    _topoChanged = true;
}

void SceneGraph::evaluate()
{
    // TODO: 1) perform any changes necessary from command queue
    //if topo changes rebuild eval order
    if (_topoChanged)
        rebuildEvaluationOrder();

    // eval; all nodes use the same data store but read/write diff locations within the store.
    // The data store takes care of where to read/write to.
    DataStore nodeDataStore(_attributeRecords, _dataSlots);
    for (const NodeID nodeID : _evaluationOrder)
    {
        const NodeRecord& nr = _nodeRecords[nodeID];
        assert(not nr.isTombstone());
        const NodeTypeID nodeType = nr.typeID;
        const auto fitr = _nodeTypeMap.find(nodeType);
        assert(fitr != _nodeTypeMap.end());
        if (nodeNeedsCompute(nr))
        {
            INodeCompute& nodeCompute = *fitr->second;
            nodeCompute.compute(nr, nodeDataStore);
        }
    }
}

bool SceneGraph::topologyChanged() const
{
    return _topoChanged;
}

bool SceneGraph::isValidNodeID(const NodeID nodeID) const
{
    if (nodeID == kInvalidNodeID or nodeID >= _nodeRecords.size())
        return false;
    return not _nodeRecords[nodeID].isTombstone();
}

/// NOTE: an attribute is still NOT considered a tombstone IF
/// its plugged and data is retrieved upstream because the attribute can be unplugged.
/// Attributes or Nodes are only considered tombstone on deletion.
bool SceneGraph::isValidAttrID(const AttrID attrID) const
{
    // an attribute can exist at index 0.
    return attrID < _attributeRecords.size() and not _attributeRecords[attrID].isTombstone();
}

bool SceneGraph::isTombstone(const NodeID nodeID) const
{
    assert(nodeID != kInvalidNodeID and nodeID < _nodeRecords.size());
    const NodeRecord& nr = _nodeRecords[nodeID];
    return nr.isTombstone();
}

bool SceneGraph::hasUpstreamInput(const AttrID attrID) const
{
    return isValidAttrID(attrID) and _attributeRecords[attrID].hasInputSources();
}

bool SceneGraph::getAttrInfo(const NodeID nodeID, const AttributeDirection dir, std::span<AttrInfo> attrInfos) const
{
    if (not isValidNodeID(nodeID))
        return false;

    const NodeRecord& nr = _nodeRecords[nodeID];
    const std::vector<AttrID>& attrIDs = dir == AttributeDirection::Input ? nr.inputAttrIDs : nr.outputAttrIDs;

    if (attrInfos.size() != attrIDs.size())
        return false;

    for (int i = 0; i < attrInfos.size(); ++i)
    {
        AttrInfo& attrInfo = attrInfos[i];
        const AttrID attrID = attrIDs[i];
        assert(isValidAttrID(attrID));
        const AttributeRecord& arec = _attributeRecords[attrID];
        attrInfo.type = arec.type;
        attrInfo.attrID = attrID;
    }
    return true;
}

std::vector<AttrInfo> SceneGraph::getAttrInfo(const NodeID nodeID, const AttributeDirection dir) const
{
    if (not isValidNodeID(nodeID))
    {
        std::cerr << "[SceneGraph::getAttrInfo] Attempting to get attribute info of invalid node" << std::endl;
        return {};
    }
    const NodeRecord& nr = _nodeRecords[nodeID];
    const std::vector<AttrID>& attrIDs = dir == AttributeDirection::Input ? nr.inputAttrIDs : nr.outputAttrIDs;
    std::vector<AttrInfo> attrInfos(attrIDs.size());
    for (int i = 0; i < attrInfos.size(); ++i)
    {
        AttrInfo& attrInfo = attrInfos[i];
        const AttrID attrID = attrIDs[i];
        assert(isValidAttrID(attrID));
        const AttributeRecord& arec = _attributeRecords[attrID];
        attrInfo.type = arec.type;
        attrInfo.attrID = attrID;
    }
    return attrInfos;
}

std::span<const std::byte> SceneGraph::getData(const AttrID attrID) const
{
    assert(isValidAttrID(attrID));
    const DataSlot& ds = _dataSlots[attrID];
    return {ds.bytes.data(), ds.bytes.size()};
}

bool SceneGraph::setUnpluggedInputAttrData(const AttrID attrID, const std::span<const std::byte> data)
{
    if (not isValidAttrID(attrID))
    {
        std::cerr << "Invalid attribute ID: " << attrID << std::endl;
        return false;
    }
    const AttributeRecord& ar = _attributeRecords[attrID];
    if (not ar.isInputAttr() or ar.hasSources())
        return false;

    DataSlot& ds = _dataSlots[attrID];
    ds.writeRawBytes(data);
    return true;
}

std::optional<AttrID> SceneGraph::fromNodeAttributeIndex(const NodeID nodeID, const int attrIndex, AttributeDirection dir) const
{
    if (not isValidNodeID(nodeID))
    {
        std::cerr << "Invalid node ID: " << nodeID << std::endl;
        return std::nullopt;
    }
    const NodeRecord& nr = _nodeRecords[nodeID];
    const std::vector<AttrID>& attrIDs = dir == AttributeDirection::Input ? nr.inputAttrIDs : nr.outputAttrIDs;
    if (attrIndex < 0 or attrIndex >= attrIDs.size())
        return std::nullopt;

    return attrIDs[attrIndex];
}

// This accounts for if the attribute is plugged in...
NDESC uint64_t SceneGraph::getAttrComputeCode(const AttrID aid) const
{
    const AttributeRecord& ar = _attributeRecords[aid];
    if (not ar.hasInputSources())
        return _dataSlots[aid].version;

    const AttrID uid = ar.upstream;
    assert(isValidAttrID(uid));
    const AttributeRecord& upstreamAttrRecord = _attributeRecords[uid];
    const auto matchUpIdToDownID = [aid](const AttrID id) -> bool
    {
        return id == aid;
    };
    assert(std::ranges::any_of(upstreamAttrRecord.downstream, matchUpIdToDownID));
    const auto& [_, version] = _dataSlots[uid];
    return version;
}

bool SceneGraph::nodeNeedsCompute(const NodeRecord& node) const
{
    // if the last version seen by node is not same as upstream version, need recompute
    for (int attrIndex = 0; attrIndex < node.inputAttrIDs.size(); ++attrIndex)
    {
        const AttrID aid = node.inputAttrIDs[attrIndex];
        assert(isValidAttrID(aid));
        if (getAttrComputeCode(aid) != node.lastSeenVersions[attrIndex])
            return true;
    }
    return false;
}

void SceneGraph::rebuildEvaluationOrder()
{
    std::unordered_map<NodeID, int16_t> inDegMap;
    std::queue<NodeID> topoOrderQueue;
    for (NodeID nid = 0; nid < _nodeRecords.size(); nid++)
    {
        const NodeRecord& nRec = _nodeRecords[nid];
        if (nRec.isTombstone())
            continue;

        assert(nRec.inputAttrIDs.size() < std::numeric_limits<int16_t>::max());
        int16_t upstreamDeg = 0;
        for (const AttrID aid : nRec.inputAttrIDs)
        {
            if (hasUpstreamInput(aid))
                upstreamDeg++;
        }
        inDegMap.insert({nid, upstreamDeg});
        if (upstreamDeg == 0)
            topoOrderQueue.emplace(nid); // not push because I am getting strange pointer error
    }

    _evaluationOrder.clear();
    while (not topoOrderQueue.empty())
    {
        const NodeID nid = topoOrderQueue.front();
        topoOrderQueue.pop();
        _evaluationOrder.push_back(nid);

        const NodeRecord& nRec = _nodeRecords[nid];
        assert(not nRec.isTombstone());
        for (const AttrID attrID : nRec.outputAttrIDs)
        {
            const AttributeRecord& attrRec = _attributeRecords[attrID];
            for (const AttrID dsID : attrRec.downstream)
            {
                assert(isValidAttrID(dsID));
                const NodeID downstreamOwner = _attributeRecords[dsID].owner;
                const auto fitr = inDegMap.find(downstreamOwner);
                assert(fitr != inDegMap.end() and fitr->second != 0);
                fitr->second--;
                if (fitr->second == 0)
                    topoOrderQueue.push(downstreamOwner);
            }
        }
    }
    _topoChanged = false;
}

END_NAMESPACE
