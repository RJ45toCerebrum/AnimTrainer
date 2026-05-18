// Created by Tyler on 5/2/2026.

#include <type_traits>
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
        return ds.bytesAllocated();
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

std::vector<int> generateRandomInts(const size_t count)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(1,100);

    std::vector<int> result;
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
    EXPECT_TRUE(isConcreteTypeSupported(leftAttrInfo.supportedTypes, AttributeDataType::Float));
    EXPECT_TRUE(isConcreteTypeSupported(rightAttrInfo.supportedTypes, AttributeDataType::Float));

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

    EXPECT_TRUE(graphRef.connect(outputAttrID, inputAttrID) == GraphConnectionQueryInfo::IsConnected);
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
    const NodeHandle rootNodeHandle = graphRef.getNodeHandle("Node0");
    EXPECT_TRUE(rootNodeHandle.isValid());
    const NodeHandle tailNodeHandle = graphRef.getNodeHandle("Node3");
    EXPECT_TRUE(tailNodeHandle.isValid());

    const auto inputAttrIDOpt =
        rootNodeHandle.fromNodeAttributeIndex(0, ATGraph::AttributeDirection::Input);
    const AttrID inputAttrID = inputAttrIDOpt.value();
    const auto outputAttrIDOpt =
        tailNodeHandle.fromNodeAttributeIndex(0, ATGraph::AttributeDirection::Output);
    const AttrID outputAttrID = outputAttrIDOpt.value();

    EXPECT_TRUE(graphRef.connect(outputAttrID, inputAttrID) == GraphConnectionQueryInfo::WillFormCycle);
}

TEST_F(GraphTestFixture, DataSlot)
{
    EXPECT_TRUE(_graphInstance.has_value());

    DataSlot testDataSlot;
    // bool
    {
        testDataSlot.updateConcreteType(AttributeDataType::Bool);
        EXPECT_TRUE(testDataSlot.getConcreteType() == AttributeDataType::Bool);
        const std::span<const bool> data = testDataSlot.readAsSpan<bool>();
        EXPECT_TRUE(data.size() == 1);

        EXPECT_TRUE(data[0] == false);

        std::span<bool> writeBuffer = testDataSlot.prepareWrite<bool>(3);
        EXPECT_TRUE(writeBuffer.size() == 3);
        writeBuffer[0] = false;
        writeBuffer[1] = true;
        writeBuffer[2] = true;

        const std::span<const bool> boolData = testDataSlot.readAsSpan<bool>();
        EXPECT_TRUE(boolData.size() == 3);
        EXPECT_TRUE(boolData[0] == false);
        EXPECT_TRUE(boolData[1] == true);
        EXPECT_TRUE(boolData[2] == true);
    }
    // float
    {
        testDataSlot.updateConcreteType(AttributeDataType::Float);
        EXPECT_FALSE(testDataSlot.getConcreteType() == AttributeDataType::Bool);
        EXPECT_TRUE(testDataSlot.getConcreteType() == AttributeDataType::Float);
        const std::span<const float> data = testDataSlot.readAsSpan<float>();
        EXPECT_TRUE(data.size() == 1);
        EXPECT_TRUE(data[0] == 0.0f);

        const auto rfs = generateRandomFloats(7);
        std::span<float> writeBuffer = testDataSlot.prepareWrite<float>(rfs.size());
        EXPECT_TRUE(writeBuffer.size() == rfs.size());
        for (int i = 0; i < rfs.size(); i++)
            writeBuffer[i] = rfs.at(i);

        const std::span<const float> readData = testDataSlot.readAsSpan<float>();
        EXPECT_TRUE(readData.size() == rfs.size());
        for (int i = 0; i < rfs.size(); i++)
            EXPECT_TRUE(readData[i] == rfs.at(i));
    }

    // int
    {
        testDataSlot.updateConcreteType(AttributeDataType::Int);
        EXPECT_TRUE(testDataSlot.getConcreteType() == AttributeDataType::Int);
        const std::span<const int> data = testDataSlot.readAsSpan<int>();
        EXPECT_TRUE(data.size() == 1);
        EXPECT_TRUE(data[0] == 0);

        const auto ris = generateRandomInts(7);
        std::span<int> writeBuffer = testDataSlot.prepareWrite<int>(ris.size());
        EXPECT_TRUE(writeBuffer.size() == ris.size());
        for (int i = 0; i < ris.size(); i++)
            writeBuffer[i] = ris.at(i);

        const std::span<const int> readData = testDataSlot.readAsSpan<int>();
        EXPECT_TRUE(readData.size() == ris.size());
        for (int i = 0; i < ris.size(); i++)
            EXPECT_TRUE(readData[i] == ris.at(i));
    }

    // Vec3
    {
        testDataSlot.updateConcreteType(AttributeDataType::Vec3);
        EXPECT_TRUE(testDataSlot.getConcreteType() == AttributeDataType::Vec3);
        const std::span<const glm::vec3> data = testDataSlot.readAsSpan<glm::vec3>();
        EXPECT_TRUE(data.size() == 1);
        EXPECT_TRUE(data[0] == glm::vec3(0.0f, 0.0f, 0.0f));

        constexpr int vectorCount = 7;
        const auto rfs = generateRandomFloats(vectorCount * 3);
        std::span<glm::vec3> writeBuffer = testDataSlot.prepareWrite<glm::vec3>(vectorCount);
        EXPECT_TRUE(writeBuffer.size() == rfs.size() / 3);
        for (int i = 0; i < vectorCount; i++)
        {
            glm::vec3& v = writeBuffer[i];
            v.x = rfs[(i * 3) + 0];
            v.y = rfs[(i * 3) + 1];
            v.z = rfs[(i * 3) + 2];
        }

        const std::span<const glm::vec3> readData = testDataSlot.readAsSpan<glm::vec3>();
        EXPECT_TRUE(readData.size() == rfs.size() / 3);
        for (int i = 0; i < vectorCount; i++)
        {
            const glm::vec3 v = readData[i];
            EXPECT_TRUE(
                v.x == rfs.at((i * 3) + 0) and
                v.y == rfs.at((i * 3) + 1) and
                v.z == rfs.at((i * 3) + 2));
        }
    }
}


END_NAMESPACE