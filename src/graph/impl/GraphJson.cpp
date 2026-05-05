// Created by Tyler on 5/4/2026.
#include "GraphJson.h"
#include <set>

START_NAMESPACE(ATGraph)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(JsonNodeConnectionData, nodeName, inputAttrIndex, outputAttrIndex);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(JsonNodeGraphData, nodeName, nodeTypeName, connectionData);

std::vector<JsonNodeGraphData> parseGraphJSON(const std::string& json)
{
    const auto parsedJson = nlohmann::json::parse(json);
    const std::vector<JsonNodeGraphData> jsonData = parsedJson.get<std::vector<JsonNodeGraphData>>();
    std::set<std::string> nodeNameSet;
    for (const auto& entry : jsonData)
    {
        const auto [itr, success] = nodeNameSet.insert(entry.nodeName);
        if (!success)
            throw JsonGraphDataException("Repeat Node name in json string. Each node should only have one entry");
    }
    for (const auto& entry : jsonData)
    {
        for (const auto& connectionData : entry.connectionData)
        {
            if (not nodeNameSet.contains(connectionData.nodeName))
                throw JsonGraphDataException("Node name in connection data does not have node entry.");
        }
    }
    return jsonData;
}

END_NAMESPACE