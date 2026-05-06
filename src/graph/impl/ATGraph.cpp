// Created by Tyler on 4/29/2026.
#include "Graph.h"

#include <queue>
#include <stack>
#include <random>
#include <iostream>
#include <set>

START_NAMESPACE(ATGraph)


SceneGraph::GraphPtr SceneGraph::_instance = nullptr;
std::unordered_map<NodeTypeID, NodePtr> SceneGraph::_nodeTypeMap;
std::unordered_map<std::string_view, NodeTypeID> SceneGraph::_nodeNameToTypeID;

void SceneGraph::registerNodeType(NodePtr nodeCompute)
{
    const NodeTypeID nodeTypeID = nodeCompute->nodeTypeID();
    const std::string_view nodeName = nodeCompute->nodeName();
    const auto fitr = _nodeTypeMap.find(nodeTypeID);
    if (fitr != _nodeTypeMap.end())
    {
        std::cerr << "[SceneGraph::registerNodeType] attempt to register the same node type" << std::endl;
        return;
    }
    const auto [_,success] =
        _nodeTypeMap.insert({nodeTypeID, std::move(nodeCompute)});
    if (!success)
    {
        std::cerr << "[SceneGraph::registerNodeType] Failed to insert node." << std::endl;
        return;
    }
    _nodeNameToTypeID.insert({nodeName, nodeTypeID});
}

std::optional<NodeTypeID> SceneGraph::nodeTypeID(const std::string_view nodeName)
{
    const auto fitr = _nodeNameToTypeID.find(nodeName);
    if (fitr == _nodeNameToTypeID.end())
        return std::nullopt;
    return fitr->second;
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
    if (_nameToNodeID.contains(name))
    {
        std::cerr << "[SceneGraph::registerNodeType] Node with name " << name <<
            " already present in graph. Node names need to be unique" << std::endl;
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
        nodeCompute.initDataSlotDefaultValue(newDataSlot, iDesc);
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
        nodeCompute.initDataSlotDefaultValue(newDataSlot, outDesc);
        _dataSlots.push_back(std::move(newDataSlot));
        assert(_attributeRecords.size() == _dataSlots.size());
    }

    assert(newNodeRecord.inputAttrIDs.size() == newNodeRecord.lastSeenVersions.size());
    _nodeRecords.push_back(std::move(newNodeRecord));
    _nodeNames.emplace_back(name);
    _nameToNodeID.insert({name, newNodeRecordID});
    assert(_nodeRecords.size() == _nodeNames.size());

    _topoChanged = true;
    return {newNodeRecordID, typeID};
}

NodeHandle SceneGraph::getNodeHandle(const std::string_view nodeName) const
{
    const auto fitr = _nameToNodeID.find(nodeName);
    if (fitr == _nameToNodeID.end())
        return {};
    const NodeID nodeID = fitr->second;
    assert(isValidNodeID(nodeID));
    const NodeRecord& record = _nodeRecords[nodeID];
    return {nodeID, record.typeID};
}

// For initial implementation, we require the graph to be empty.
// TODO: remove empty graph constraint.
bool SceneGraph::buildFromGraphJson(const std::vector<JsonNodeGraphData>& graphData)
{
    // TODO: do not allow partial construction if something goes wrong.
    // 1) make command queue.
    // 2) if something goes wrong, undo the all commands that came before.
    // for now, it just leave it partial and exit early.

    assert(not _nodeRecords.empty());
    // Index 0 is always the invalid index.
    if (_nodeRecords.size() > 1)
    {
        std::cerr << "Graph must be empty for valid call to SceneGraph::buildFromGraphJson" << std::endl;
        return false;
    }
    // first create all the nodes.
    for (const JsonNodeGraphData& nodeData : graphData)
    {
        const auto fitr = _nodeNameToTypeID.find(nodeData.nodeTypeName);
        if (fitr == _nodeNameToTypeID.end())
        {
            std::cerr << "[SceneGraph::buildFromGraphJson] Unable to convert node type name to node type ID"
                << ". Was this node type registered (registerNodeType)?" << std::endl;
            return false;
        }
        const NodeTypeID nodeTypeID = fitr->second;
        const NodeHandle newNodeHandle = createNode(nodeTypeID, nodeData.nodeName);
        if (not newNodeHandle.isValid())
        {
            std::cerr << "[SceneGraph::buildFromGraphJson] Failed to create node with name: "
            << nodeData.nodeName << std::endl;
            return false;
        }
    }
    for (const JsonNodeGraphData& nodeData : graphData)
    {
        const std::string& fromNodeName = nodeData.nodeName;
        const auto fromNodeIDItr = _nameToNodeID.find(fromNodeName);
        // the node handle was valid, so if this fails, something is wrong with graph.
        assert(fromNodeIDItr != _nameToNodeID.end());
        const NodeID fromNodeID = fromNodeIDItr->second;
        for (const auto& conData : nodeData.connectionData)
        {
            const std::string& toNodeName = conData.nodeName;
            const auto toNodeIDItr = _nameToNodeID.find(toNodeName);
            assert(toNodeIDItr != _nameToNodeID.end());
            const NodeID toNodeID = toNodeIDItr->second;
            const auto outputAttrIDOpt =
                fromNodeAttributeIndex(fromNodeID, conData.outputAttrIndex, AttributeDirection::Output);
            if (not outputAttrIDOpt)
            {
                std::cerr << "[SceneGraph::buildFromGraphJson] Unable to find the output attribute ID " <<
                             "corresponding to outputAttrIndex: " << conData.outputAttrIndex <<
                                 ". Make sure to verify the indices in the json. Unable to connect attributes" << std::endl;
                return false;
            }
            const auto inputAttrIDOpt =
                fromNodeAttributeIndex(toNodeID, conData.inputAttrIndex, AttributeDirection::Input);
            if (not inputAttrIDOpt)
            {
                std::cerr << "[SceneGraph::buildFromGraphJson] Unable to find the input attribute ID " <<
                    "corresponding to inputAttrIndex: " << conData.inputAttrIndex <<
                        ". Make sure to verify the indices in the json. Unable to connect attributes" << std::endl;
                return false;
            }
            const AttrID outputAttrID = outputAttrIDOpt.value();
            const AttrID inputAttrID = inputAttrIDOpt.value();
            if (not connect(outputAttrID, inputAttrID))
            {
                std::cerr << "[SceneGraph::buildFromGraphJson] Failed to connect the attributes of node pair: "
                << '[' << fromNodeName << ',' << toNodeName << ']' << std::endl;
                return false;
            }
        }
    }
    return true;
}

bool SceneGraph::deleteNode(const NodeID nodeID)
{
    throw std::logic_error("[SceneGraph::deleteNode] Node ID not implemented");
    _topoChanged = true;
}

bool SceneGraph::canConnect(const AttrID outputAttr, const AttrID inputAttr) const
{
    if (not isValidInputAttrID(inputAttr))
    {
        std::cerr << "[SceneGraph::connect] Invalid input attribute " << inputAttr << std::endl;
        return false;
    }
    if (not isValidOutputAttrID(outputAttr))
    {
        std::cerr << "[SceneGraph::connect] Invalid input attribute ID " << inputAttr << std::endl;
        return false;
    }
    const AttributeRecord& inputAttrRec = _attributeRecords[inputAttr];
    const AttributeRecord& outputAttrRec = _attributeRecords[outputAttr];
    if (inputAttrRec.type != outputAttrRec.type)
    {
        std::cerr << "[SceneGraph::connect] The attributes being connected have incompatible types" << std::endl;
        return false;
    }
    if (inputAttrRec.hasInputSources())
    {
        if (inputAttrRec.upstream == outputAttr)
        {
            const auto fitr = outputAttrRec.findOutputSource(inputAttr);
            assert(fitr != outputAttrRec.downstream.end());
            std::cerr << "[SceneGraph::connect] Input attribute already has outputAttr plugged in" << std::endl;
        }
        else
        {
            std::cerr << "[SceneGraph::connect] Input attribute already has a source plugged in" << std::endl;
        }
        return false;
    }
    const auto fitr = outputAttrRec.findOutputSource(inputAttr);
    assert(fitr == outputAttrRec.downstream.end());
    if (willFormCycle(outputAttr, inputAttr))
    {
        std::cerr << "[SceneGraph::connect] Unable to connect attributes because it will form a cycle" << std::endl;
        return false;
    }
    return true;
}

bool SceneGraph::willFormCycle(const AttrID outputAttr, const AttrID inputAttr) const
{
    const AttributeRecord& inputAttrRec = _attributeRecords.at(inputAttr);
    const AttributeRecord& outputAttrRec = _attributeRecords.at(outputAttr);
    assert(not inputAttrRec.isTombstone());
    const NodeID inputOwnerID = inputAttrRec.owner;
    const NodeID outputOwnerID = outputAttrRec.owner;
    std::stack<NodeID> nodeStack;
    std::set<NodeID> visitedSet;
    // strange error on push. Use emplace instead
    // Clangd: In template: constexpr variable '_Is_pointer_address_convertible<unsigned int, unsigned int>' must be initialized by a constant expression
    nodeStack.emplace(inputOwnerID);
    visitedSet.insert(inputOwnerID);
    while (not nodeStack.empty())
    {
        const NodeID curNodeID = nodeStack.top();
        nodeStack.pop();
        if (curNodeID == outputOwnerID)
            return true;

        const NodeRecord& cnr = _nodeRecords.at(curNodeID);
        assert(not cnr.isTombstone());
        for (const AttrID curOutAttr : cnr.outputAttrIDs)
        {
            const AttributeRecord& attrRec = _attributeRecords.at(curOutAttr);
            assert(attrRec.owner == curNodeID);
            assert(attrRec.isOutputAttr());
            for (const AttrID downstreamAttr : attrRec.downstream)
            {
                const AttributeRecord& downstreamAttrRec = _attributeRecords.at(downstreamAttr);
                const auto [itr, success] =
                    visitedSet.insert(downstreamAttrRec.owner);
                if (success)
                    nodeStack.push(downstreamAttrRec.owner);
            }
        }
    }
    return false;
}

bool SceneGraph::connect(const AttrID outputAttr, const AttrID inputAttr)
{
    if (not canConnect(outputAttr, inputAttr))
        return false;

    AttributeRecord& outputAttrRec = _attributeRecords[outputAttr];
    AttributeRecord& inputAttrRec = _attributeRecords[inputAttr];
    inputAttrRec.upstream = outputAttr;
    outputAttrRec.downstream.push_back(inputAttr);

    // once input attribute plugged in, the data source now exists in the upstream data slot.
    // claim the current data slot memory
    DataSlot& currentAttrData = _dataSlots[inputAttr];
    currentAttrData.bytes.clear();
    currentAttrData.bytes.shrink_to_fit();

    _topoChanged = true;
    return true;
}

bool SceneGraph::disconnect(const AttrID outputAttr, const AttrID inputAttr)
{
    if (not isValidInputAttrID(inputAttr))
    {
        std::cerr << "[SceneGraph::disconnect] The input attribute passed in is invalid." << std::endl;
        return false;
    }
    if (not isValidOutputAttrID(outputAttr))
    {
        std::cerr << "[SceneGraph::disconnect] The output attribute passed in is invalid." << std::endl;
        return false;
    }
    AttributeRecord& inputAttrRec = _attributeRecords[inputAttr];
    AttributeRecord& outputAttrRec = _attributeRecords[outputAttr];
    // are they actually connected?
    if (inputAttrRec.upstream != outputAttr)
    {
        const auto fitr = outputAttrRec.findOutputSource(inputAttr);
        assert(fitr == outputAttrRec.downstream.end());
        std::cerr << "[SceneGraph::disconnect] The input attribute has no incoming connection from output attribute. "
        << "They should be connected for a valid disconnect call." << std::endl;
        return false;
    }
    // IF the types do not match here, something went horribly wrong.
    assert(inputAttrRec.type == outputAttrRec.type);
    // assert because this means the graph is straight broken
    const auto fitr = outputAttrRec.findOutputSource(inputAttr);
    assert(fitr != outputAttrRec.downstream.end());

    inputAttrRec.upstream = kInvalidAttr;
    outputAttrRec.downstream.erase(fitr);
    _topoChanged = true;
    return true;
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

bool SceneGraph::isValidInputAttrID(const AttrID attrID) const
{
    if (attrID >= _attributeRecords.size())
        return false;

    const AttributeRecord& attrRecord = _attributeRecords[attrID];
    if (attrRecord.isTombstone())
        return false;

    return attrRecord.isInputAttr();
}

bool SceneGraph::isValidOutputAttrID(const AttrID attrID) const
{
    if (attrID >= _attributeRecords.size())
        return false;

    const AttributeRecord& attrRecord = _attributeRecords[attrID];
    if (attrRecord.isTombstone())
        return false;

    return attrRecord.isOutputAttr();
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

std::optional<AttrID> SceneGraph::fromNodeAttributeIndex(const NodeID nodeID, const int attrIndex, const AttributeDirection dir) const
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
    uint32_t nodeCount = 0;
    for (NodeID nid = 0; nid < _nodeRecords.size(); nid++)
    {
        const NodeRecord& nRec = _nodeRecords[nid];
        if (nRec.isTombstone())
            continue;

        nodeCount++;
        assert(nRec.inputAttrIDs.size() < std::numeric_limits<int16_t>::max());
        int16_t upstreamDeg = 0;
        for (const AttrID aid : nRec.inputAttrIDs)
        {
            if (hasUpstreamInput(aid))
                upstreamDeg++;
        }
        inDegMap.insert({nid, upstreamDeg});
        if (upstreamDeg == 0)
            topoOrderQueue.emplace(nid);
    }
    // It's possible for 0 nodes to have an in-degree of 0. This is recoverable, however, I do not attempt this now.
    // TODO: recover from missing topological island error
    // For each topological island, there should be at least 1 node present (but not necessarily 1).
    assert(not topoOrderQueue.empty() && "When attempting to rebuild evaluation order, 0 nodes had a in-degree of 0.");

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
    assert(_evaluationOrder.size() == nodeCount && "Not every node is in the evaluation order vector. "
                                                   "This means some nodes could not be computed.");
    _topoChanged = false;
}

END_NAMESPACE
