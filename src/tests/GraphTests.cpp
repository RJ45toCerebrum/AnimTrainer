// Created by Tyler on 5/2/2026.

#include <numeric>
#include <gtest/gtest.h>
#include <print>
#include <random>
#include <fstream>

#include "Graph.h"
#include "GraphJson.h"
#include "Nodes/Math/AddNode.h"

START_NAMESPACE(ATGraph)

using ATGraph::SceneGraph;
using ATGraph::NodeHandle;
using ATGraph::AddNode;
using ATGraph::AttrInfo;
using ATGraph::AttrID;
using ATGraph::AttributeDataType;
using ATGraph::JsonNodeGraphData;
using ATGraph::JsonNodeConnectionData;

// any logic that requires inspecting the internal state of the graph must exist directly in this class.
class GraphTestFixture : public ::testing::Test
{
public:
    std::optional<std::reference_wrapper<SceneGraph>> _graphInstance;

protected:
    void SetUp() override
    {
        std::println("Starting graph instance...");
        _graphInstance = SceneGraph::instance();
        EXPECT_TRUE(_graphInstance.has_value());
        // register the nodes types...
        ATGraph::NodePtr addNodePtr = std::make_unique<AddNode>();
        SceneGraph::registerNodeType(std::move(addNodePtr));
    }
    void TearDown() override
    {
        std::println("Stopping graph instance...");
        SceneGraph::destroy();
    }

    NDESC bool AllNodesNeedCompute() const
    {
        const SceneGraph& graphRef = _graphInstance.value();
        for (const NodeRecord& nr : graphRef._nodeRecords)
        {
            // skip tombstones
            if (nr.typeID == kInvalidNodeTypeID)
                continue;
            if (not graphRef.nodeNeedsCompute(nr))
                return false;
        }
        return true;
    }

    NDESC bool AllNodesNoCompute() const
    {
        const SceneGraph& graphRef = _graphInstance.value();
        for (const NodeRecord& nr : graphRef._nodeRecords)
        {
            if (nr.typeID == kInvalidNodeTypeID)
                continue;
            if (graphRef.nodeNeedsCompute(nr))
                return false;
        }
        return true;
    }

    NDESC std::size_t getDataSlotMemSizeForAttribute(const AttrID attrID) const
    {
        const SceneGraph& graphRef = _graphInstance.value();
        const DataSlot& ds = graphRef._dataSlots.at(attrID);
        return ds.bytes.size();
    }
};


std::vector<float> generateRandomFloats(const size_t count)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution dis(0.0f, 10.0f);

    std::vector<float> result;
    result.reserve(count);
    for (size_t i = 0; i < count; ++i)
        result.push_back(dis(gen));

    return result;
}

std::vector<JsonNodeGraphData> getDefaultGraphJson()
{
    const std::filesystem::path pathToTestJson = std::filesystem::path(PROJECT_ROOT_DIR) / "resources/graph_json_test.json";
    EXPECT_TRUE(std::filesystem::exists(pathToTestJson));
    std::ifstream jsonFileStream(pathToTestJson);
    EXPECT_TRUE(jsonFileStream.is_open());
    const std::string jsonContent((std::istreambuf_iterator<char>(jsonFileStream)),
        std::istreambuf_iterator<char>());
    return ATGraph::parseGraphJSON(jsonContent);
}


TEST(ExampleSuite, ExampleTest)
{
    EXPECT_EQ(1 + 1, 2);
}

TEST_F(GraphTestFixture, CreateNodeTest)
{
    EXPECT_TRUE(_graphInstance.has_value());
    SceneGraph& graphRef = _graphInstance.value();
    NodeHandle newNodeHandle = graphRef.createNode(AddNode::kNodeTypeId, "NodeTest");
    EXPECT_TRUE(newNodeHandle.isValid());

    const std::vector<AttrInfo> inputAttrInfo = newNodeHandle.inputAttrInfo();
    const AttrInfo leftAttrInfo = inputAttrInfo[0];
    const AttrInfo rightAttrInfo = inputAttrInfo[1];

    EXPECT_TRUE(leftAttrInfo.type == AttributeDataType::Float);
    EXPECT_TRUE(rightAttrInfo.type == AttributeDataType::Float);

    const AttrID leftFloatAttr = leftAttrInfo.attrID;
    const AttrID rightFloatAttr = rightAttrInfo.attrID;
    EXPECT_TRUE(newNodeHandle.setUnpluggedInputAttrData<float>(leftFloatAttr, 1.0f));
    EXPECT_TRUE(newNodeHandle.setUnpluggedInputAttrData(rightFloatAttr, 2.0f));
}

TEST_F(GraphTestFixture, NodeConnectionTest)
{
    EXPECT_TRUE(_graphInstance.has_value());
    SceneGraph& graphRef = _graphInstance.value();

    const std::vector<float> rfs = generateRandomFloats(3);
    const float expectedResult = std::accumulate(rfs.begin(), rfs.end(), 0.0f);

    NodeHandle nh0 = graphRef.createNode(AddNode::kNodeTypeId, "Node0");
    EXPECT_TRUE(nh0.isValid());
    nh0.setUnpluggedInputByIndex(0, rfs[0]);
    nh0.setUnpluggedInputByIndex(1, rfs[1]);
    const auto outAttrOpt = nh0.fromNodeAttributeIndex(0, ATGraph::AttributeDirection::Output);
    EXPECT_TRUE(outAttrOpt.has_value());
    const AttrID outputAttrID = outAttrOpt.value();

    NodeHandle nh1 = graphRef.createNode(AddNode::kNodeTypeId, "Node1");
    EXPECT_TRUE(nh1.isValid());
    EXPECT_TRUE(nh1.setUnpluggedInputByIndex(1, rfs[2]));
    const auto inputAttrOpt = nh1.fromNodeAttributeIndex(0, ATGraph::AttributeDirection::Input);
    EXPECT_TRUE(inputAttrOpt.has_value());
    const AttrID inputAttrID = inputAttrOpt.value();

    EXPECT_TRUE(graphRef.connect(outputAttrID, inputAttrID));
    EXPECT_TRUE(graphRef.topologyChanged());
    // after making a connection, we expect the memory in the input data slot should be of size 0
    EXPECT_TRUE(getDataSlotMemSizeForAttribute(inputAttrID) == 0);

    const auto resultAttrIDOpt = nh1.fromNodeAttributeIndex(0, ATGraph::AttributeDirection::Output);
    EXPECT_TRUE(resultAttrIDOpt.has_value());
    const AttrID resultAttrID = resultAttrIDOpt.value();

    graphRef.evaluate();

    EXPECT_TRUE(not graphRef.topologyChanged());

    const auto data = nh1.getData<float>(resultAttrID);
    EXPECT_TRUE(data.size() == 1);
    const float asbDiff = std::abs(data[0] - expectedResult);
    EXPECT_TRUE( asbDiff <= std::numeric_limits<float>::epsilon() );

    // test disconnection...
    EXPECT_TRUE(graphRef.disconnect(outputAttrID, inputAttrID));
    EXPECT_TRUE(graphRef.topologyChanged());
    EXPECT_TRUE(getDataSlotMemSizeForAttribute(inputAttrID) == sizeof(float));

    graphRef.evaluate();
}

TEST_F(GraphTestFixture, GraphJsonParsing)
{
    std::vector<JsonNodeGraphData> nodes = getDefaultGraphJson();
    const JsonNodeGraphData& gd = nodes[0];
    EXPECT_TRUE(gd.nodeName == "Node0");
}

TEST_F(GraphTestFixture, JsonToGraph)
{
    EXPECT_TRUE(_graphInstance.has_value());

    const std::vector<JsonNodeGraphData> jsonNodes = getDefaultGraphJson();

    SceneGraph& graphRef = _graphInstance.value();
    EXPECT_TRUE(graphRef.buildFromGraphJson(jsonNodes));
    NodeHandle rootNodeHandle = graphRef.getNodeHandle("Node0");
    EXPECT_TRUE(rootNodeHandle.isValid());
    NodeHandle tailNodeHandle = graphRef.getNodeHandle("Node3");
    EXPECT_TRUE(tailNodeHandle.isValid());

    const std::vector<float> rfs = generateRandomFloats(2);
    EXPECT_TRUE(rootNodeHandle.setUnpluggedInputByIndex(0, rfs[0]));
    EXPECT_TRUE(rootNodeHandle.setUnpluggedInputByIndex(1, rfs[1]));

    graphRef.evaluate();

    const auto rawData = tailNodeHandle.getOutputAttrData<float>(0);
    EXPECT_TRUE(rawData.size() == 1);
    const float result = rawData[0];
    const float expectedResult = 2 * std::accumulate(rfs.begin(), rfs.end(), 0.0f);
    EXPECT_TRUE(std::abs(result - expectedResult) < 10 * std::numeric_limits<float>::epsilon());
}

// On the second evaluation, no nodes should need compute
TEST_F(GraphTestFixture, NeedComputeTest)
{
    const std::vector<JsonNodeGraphData> jsonNodes = getDefaultGraphJson();
    SceneGraph& graphRef = _graphInstance.value();
    EXPECT_TRUE(graphRef.buildFromGraphJson(jsonNodes));

    const std::vector<float> rfs = generateRandomFloats(2);
    NodeHandle rootNodeHandle = graphRef.getNodeHandle("Node0");
    EXPECT_TRUE(rootNodeHandle.isValid());
    EXPECT_TRUE(rootNodeHandle.setUnpluggedInputByIndex(0, rfs[0]));
    EXPECT_TRUE(rootNodeHandle.setUnpluggedInputByIndex(1, rfs[1]));

    EXPECT_TRUE(AllNodesNeedCompute());
    graphRef.evaluate();
    EXPECT_TRUE(AllNodesNoCompute());
}

TEST_F(GraphTestFixture, DetectCycle)
{
    EXPECT_TRUE(_graphInstance.has_value());

    const std::vector<JsonNodeGraphData> jsonNodes = getDefaultGraphJson();

    SceneGraph& graphRef = _graphInstance.value();
    EXPECT_TRUE(graphRef.buildFromGraphJson(jsonNodes));

    // create intentional cycle
    NodeHandle rootNodeHandle = graphRef.getNodeHandle("Node0");
    EXPECT_TRUE(rootNodeHandle.isValid());
    NodeHandle tailNodeHandle = graphRef.getNodeHandle("Node3");
    EXPECT_TRUE(tailNodeHandle.isValid());

    const auto inputAttrIDOpt =
        rootNodeHandle.fromNodeAttributeIndex(0, ATGraph::AttributeDirection::Input);
    const AttrID inputAttrID = inputAttrIDOpt.value();
    const auto outputAttrIDOpt =
        tailNodeHandle.fromNodeAttributeIndex(0, ATGraph::AttributeDirection::Output);
    const AttrID outputAttrID = outputAttrIDOpt.value();

    EXPECT_FALSE(graphRef.connect(outputAttrID, inputAttrID));
}

END_NAMESPACE