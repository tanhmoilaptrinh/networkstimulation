#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FinalSimulation");

// Hàm thiết lập chung cho các kịch bản
void RunScenario(std::string scenarioName, uint32_t numServers, uint32_t numClients, std::string mobilityModel)
{
    NS_LOG_UNCOND("===== Running " << scenarioName << " =====");

    uint32_t totalNodes = numServers + numClients;
    NodeContainer nodes;
    nodes.Create(totalNodes);

    // ⚡ Cấu hình WiFi (Ad-hoc Mode)
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(channel.Create());

    WifiMacHelper mac;
    WifiHelper wifi;
    mac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install(wifiPhy, mac, nodes);

    // 📌 Tắt RTS/CTS (theo yêu cầu bài toán)
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("999999"));

    // 🚀 Cấu hình Mobility Model
    MobilityHelper mobility;
    if (mobilityModel == "ConstantPositionMobilityModel")
    {
        mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                      "MinX", DoubleValue(0.0),
                                      "MinY", DoubleValue(0.0),
                                      "DeltaX", DoubleValue(5.0),
                                      "DeltaY", DoubleValue(10.0),
                                      "GridWidth", UintegerValue(3),
                                      "LayoutType", StringValue("RowFirst"));
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    }
    else if (mobilityModel == "RandomWalk2dMobilityModel")
{
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
        "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=100.0]"),
        "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=100.0]")
    );

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
        "Mode", StringValue("Time"),
        "Time", TimeValue(Seconds(1.0)),  // Thay đổi hướng sau mỗi giây
        "Speed", StringValue("ns3::UniformRandomVariable[Min=1.0|Max=5.0]"),
        "Bounds", RectangleValue(Rectangle(-100, 100, -100, 100))
    );
}
    mobility.Install(nodes);

    // 🌐 Cấu hình Internet
    InternetStackHelper stack;
    stack.Install(nodes);
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // 📡 Cấu hình Server
    for (uint32_t i = 0; i < numServers; i++)
    {
        UdpEchoServerHelper echoServer(9);
        ApplicationContainer serverApp = echoServer.Install(nodes.Get(i));
        serverApp.Start(Seconds(0.0));
        serverApp.Stop(Seconds(200.0));
    }

    // 📡 Cấu hình Client
    uint32_t packetSize = 1024;
    uint32_t maxPackets = 200;
    double interval = 1.0;
    
    for (uint32_t i = 0; i < numClients; i++)
    {
        uint32_t serverIndex = i % numServers; // Chia đều client vào các server
        UdpEchoClientHelper echoClient(interfaces.GetAddress(serverIndex), 9);
        echoClient.SetAttribute("MaxPackets", UintegerValue(maxPackets));
        echoClient.SetAttribute("Interval", TimeValue(Seconds(interval)));
        echoClient.SetAttribute("PacketSize", UintegerValue(packetSize));

        ApplicationContainer clientApp = echoClient.Install(nodes.Get(numServers + i));
        clientApp.Start(Seconds(1.0));
        clientApp.Stop(Seconds(200.0));
    }

    // 📊 Cấu hình FlowMonitor
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(200.0));
    Simulator::Run();

    // 📝 Xuất dữ liệu FlowMonitor
    std::string fileName = "flowmonitor_results_" + scenarioName + ".csv";
    std::ofstream outFile(fileName);
    outFile << "Flow ID,Tx Packets,Rx Packets,Lost Packets,Loss Rate (%)\n";
    flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    auto stats = flowMonitor->GetFlowStats();
    
    for (auto it = stats.begin(); it != stats.end(); ++it)
    {
        uint32_t tx = it->second.txPackets;
        uint32_t rx = it->second.rxPackets;
        uint32_t lost = tx - rx;
        double rate = (tx > 0) ? (lost * 100.0 / tx) : 0.0;
        outFile << it->first << "," << tx << "," << rx << "," << lost << "," << rate << "\n";
    }
    outFile.close();

    Simulator::Destroy();
}

// 📌 Chạy tất cả các kịch bản
void RunAllScenarios()
{
    RunScenario("Scenario1", 1, 29, "ConstantPositionMobilityModel");
    RunScenario("Scenario2", 5, 25, "ConstantPositionMobilityModel");
    RunScenario("Scenario3", 1, 29, "RandomWaypointMobilityModel");
    RunScenario("Scenario4", 5, 25, "RandomWaypointMobilityModel");
}

int main(int argc, char *argv[])
{
    CommandLine cmd;
    cmd.Parse(argc, argv);

    RunAllScenarios();

    return 0;
}
