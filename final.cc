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

NS_LOG_COMPONENT_DEFINE("FinalMultiScenario");

// =============================
// = 1) Scenario 1 (Static, Light Load)
void Scenario1(uint32_t nNodes, uint32_t packetSize)
{
    NS_LOG_UNCOND("===== SCENARIO 1: STATIC, LIGHT LOAD =====");
    NS_LOG_UNCOND("Number of Nodes: " << nNodes);

    // Tạo node
    NodeContainer nodes;
    nodes.Create(nNodes);

    // WiFi
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(channel.Create());

    WifiMacHelper mac;
    WifiHelper wifi;
    mac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install(wifiPhy, mac, nodes);

    // Mobility: Static (Constant Position)
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(5.0),
                                  "DeltaY", DoubleValue(10.0),
                                  "GridWidth", UintegerValue(3),
                                  "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Internet
    InternetStackHelper stack;
    stack.Install(nodes);
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // 20% Sender, 80% Receiver
    uint32_t numSenders = nNodes * 0.2;
    uint32_t numReceivers = nNodes - numSenders;

    // Setup ứng dụng (UDP Echo)
    for (uint32_t i = 0; i < numReceivers; i++)
    {
        UdpEchoServerHelper echoServer(9);
        ApplicationContainer serverApp = echoServer.Install(nodes.Get(numSenders + i));
        serverApp.Start(Seconds(0.0));
        serverApp.Stop(Seconds(20.0));
    }

    for (uint32_t i = 0; i < numSenders; i++)
    {
        // Chọn receiver ngẫu nhiên
        uint32_t receiver = numSenders + (std::rand() % numReceivers);
        UdpEchoClientHelper echoClient(interfaces.GetAddress(receiver), 9);

        // Gửi nhẹ
        uint32_t maxPackets = 100 + (std::rand() % 500); // 100-600
        echoClient.SetAttribute("MaxPackets", UintegerValue(maxPackets));
        echoClient.SetAttribute("Interval", TimeValue(Seconds(0.5)));
        echoClient.SetAttribute("PacketSize", UintegerValue(packetSize));

        ApplicationContainer clientApp = echoClient.Install(nodes.Get(i));
        clientApp.Start(Seconds(1.0));
        clientApp.Stop(Seconds(20.0));
    }

    // FlowMonitor
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(20.0));
    Simulator::Run();

    // Xuất dữ liệu
    flowMonitor->SerializeToXmlFile("flowmonitor_Scenario1.xml", true, true);
    std::ofstream outFile("flowmonitor_results_Scenario1.csv");
    outFile << "Flow ID,Tx Packets,Rx Packets,Lost Packets,Loss Rate (%)\n";
    flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    auto stats = flowMonitor->GetFlowStats();
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        uint32_t tx = it->second.txPackets;
        uint32_t rx = it->second.rxPackets;
        uint32_t lost = tx - rx;
        double rate = (tx > 0) ? (lost * 100.0 / tx) : 0.0;
        outFile << it->first << "," << tx << "," << rx << "," << lost << "," << rate << "\n";
    }
    outFile.close();

    Simulator::Destroy();
}

// =============================
// = 2) Scenario 2 (Static, Heavy Load + Interference)
void Scenario2(uint32_t nNodes, uint32_t packetSize)
{
    NS_LOG_UNCOND("===== SCENARIO 2: STATIC, HEAVY LOAD + INTERFERENCE =====");
    NS_LOG_UNCOND("Number of Nodes: " << nNodes);

    // Tạo node
    NodeContainer nodes;
    nodes.Create(nNodes);

    // WiFi
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(channel.Create());

    // Giảm công suất phát để mô phỏng nhiễu
    Config::SetDefault("ns3::WifiPhy::TxPowerStart", DoubleValue(1.0));
    Config::SetDefault("ns3::WifiPhy::TxPowerEnd", DoubleValue(1.0));

    WifiMacHelper mac;
    WifiHelper wifi;
    mac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install(wifiPhy, mac, nodes);

    // Mobility: Static
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(5.0),
                                  "DeltaY", DoubleValue(10.0),
                                  "GridWidth", UintegerValue(3),
                                  "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Internet
    InternetStackHelper stack;
    stack.Install(nodes);
    Ipv4AddressHelper address;
    address.SetBase("10.2.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // 50% Sender, 50% Receiver
    uint32_t numSenders = nNodes / 2;
    uint32_t numReceivers = nNodes - numSenders;

    // Setup ứng dụng (UDP Echo)
    for (uint32_t i = 0; i < numReceivers; i++)
    {
        UdpEchoServerHelper echoServer(9);
        ApplicationContainer serverApp = echoServer.Install(nodes.Get(numSenders + i));
        serverApp.Start(Seconds(0.0));
        serverApp.Stop(Seconds(50.0));
    }

    for (uint32_t i = 0; i < numSenders; i++)
    {
        // Chọn receiver ngẫu nhiên
        uint32_t receiver = numSenders + (std::rand() % numReceivers);
        UdpEchoClientHelper echoClient(interfaces.GetAddress(receiver), 9);

        // Gửi nhiều hơn
        uint32_t maxPackets = 1000 + (std::rand() % 2000); // 1000-3000
        echoClient.SetAttribute("MaxPackets", UintegerValue(maxPackets));
        echoClient.SetAttribute("Interval", TimeValue(Seconds(0.1))); // Tải cao
        echoClient.SetAttribute("PacketSize", UintegerValue(packetSize));

        ApplicationContainer clientApp = echoClient.Install(nodes.Get(i));
        clientApp.Start(Seconds(1.0));
        clientApp.Stop(Seconds(50.0));
    }

    // FlowMonitor
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(50.0));
    Simulator::Run();

    // Xuất dữ liệu
    flowMonitor->SerializeToXmlFile("flowmonitor_Scenario2.xml", true, true);
    std::ofstream outFile("flowmonitor_results_Scenario2.csv");
    outFile << "Flow ID,Tx Packets,Rx Packets,Lost Packets,Loss Rate (%)\n";
    flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    auto stats = flowMonitor->GetFlowStats();
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        uint32_t tx = it->second.txPackets;
        uint32_t rx = it->second.rxPackets;
        uint32_t lost = tx - rx;
        double rate = (tx > 0) ? (lost * 100.0 / tx) : 0.0;
        outFile << it->first << "," << tx << "," << rx << "," << lost << "," << rate << "\n";
    }
    outFile.close();

    Simulator::Destroy();
}

// =============================
// = 3) Scenario 3 (Dynamic, Random Waypoint)
void Scenario3(uint32_t nNodes, uint32_t packetSize)
{
    NS_LOG_UNCOND("===== SCENARIO 3: DYNAMIC, RANDOM WAYPOINT =====");
    NS_LOG_UNCOND("Number of Nodes: " << nNodes);

    // Tạo node
    NodeContainer nodes;
    nodes.Create(nNodes);

    // WiFi
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(channel.Create());

    WifiMacHelper mac;
    WifiHelper wifi;
    mac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install(wifiPhy, mac, nodes);

    // Mobility: Random Waypoint
    MobilityHelper mobility;
    // Cần position allocator
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
        "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=100.0]"),
        "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=100.0]")
    );
    
    mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
        "Speed", StringValue("ns3::UniformRandomVariable[Min=1.0|Max=5.0]"),
        "Pause", StringValue("ns3::ConstantRandomVariable[Constant=2.0]"),
        "PositionAllocator", StringValue("ns3::RandomRectanglePositionAllocator")
    );    
    mobility.Install(nodes);

    // Internet
    InternetStackHelper stack;
    stack.Install(nodes);
    Ipv4AddressHelper address;
    address.SetBase("10.3.3.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // 50% Sender, 50% Receiver
    uint32_t numSenders = nNodes / 2;
    uint32_t numReceivers = nNodes - numSenders;

    // Setup CBR traffic (UDP OnOff)
    for (uint32_t i = 0; i < numReceivers; i++)
    {
        // Mở "server" cổng 8080 (UDP EchoServer cũ cũng được, nhưng ta dùng OnOff => cbr)
        UdpEchoServerHelper echoServer(9);
        ApplicationContainer serverApp = echoServer.Install(nodes.Get(numSenders + i));
        serverApp.Start(Seconds(0.0));
        serverApp.Stop(Seconds(75.0));
    }

    for (uint32_t i = 0; i < numSenders; i++)
    {
        uint32_t receiver = numSenders + (std::rand() % numReceivers);

        // CBR traffic
        OnOffHelper cbrClient("ns3::UdpSocketFactory", InetSocketAddress(interfaces.GetAddress(receiver), 9));
        cbrClient.SetConstantRate(DataRate("1Mbps"));
        cbrClient.SetAttribute("PacketSize", UintegerValue(packetSize));
        ApplicationContainer clientApp = cbrClient.Install(nodes.Get(i));
        clientApp.Start(Seconds(2.0));
        clientApp.Stop(Seconds(75.0));
    }

    // FlowMonitor
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(75.0));
    Simulator::Run();

    // Xuất dữ liệu
    flowMonitor->SerializeToXmlFile("flowmonitor_Scenario3.xml", true, true);
    std::ofstream outFile("flowmonitor_results_Scenario3.csv");
    outFile << "Flow ID,Tx Packets,Rx Packets,Lost Packets,Loss Rate (%)\n";
    flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    auto stats = flowMonitor->GetFlowStats();
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        uint32_t tx = it->second.txPackets;
        uint32_t rx = it->second.rxPackets;
        uint32_t lost = tx - rx;
        double rate = (tx > 0) ? (lost * 100.0 / tx) : 0.0;
        outFile << it->first << "," << tx << "," << rx << "," << lost << "," << rate << "\n";
    }
    outFile.close();

    Simulator::Destroy();
}

// =============================
// = 4) Scenario 4 (Dynamic, RandomWalk, High Load TCP)
void Scenario4(uint32_t nNodes, uint32_t packetSize)
{
    NS_LOG_UNCOND("===== SCENARIO 4: DYNAMIC, RANDOM WALK, HIGH LOAD TCP =====");
    NS_LOG_UNCOND("Number of Nodes: " << nNodes);

    // Tạo node
    NodeContainer nodes;
    nodes.Create(nNodes);

    // WiFi
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(channel.Create());

    WifiMacHelper mac;
    WifiHelper wifi;
    mac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install(wifiPhy, mac, nodes);

    // Mobility: RandomWalk2d
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                  "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=100.0]"),
                                  "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=100.0]"));
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));
    mobility.Install(nodes);

    // Internet
    InternetStackHelper stack;
    stack.Install(nodes);
    Ipv4AddressHelper address;
    address.SetBase("10.4.4.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // 80% Sender, 20% Receiver
    uint32_t numSenders = nNodes * 0.8;
    uint32_t numReceivers = nNodes - numSenders;

    // Setup High Load TCP traffic (OnOff)
    for (uint32_t i = 0; i < numReceivers; i++)
    {
        // Mở "server" cổng 8080
        UdpEchoServerHelper echoServer(9); // or we can do a PacketSink
        ApplicationContainer serverApp = echoServer.Install(nodes.Get(numSenders + i));
        serverApp.Start(Seconds(0.0));
        serverApp.Stop(Seconds(100.0));
    }

    for (uint32_t i = 0; i < numSenders; i++)
    {
        uint32_t receiver = numSenders + (std::rand() % numReceivers);

        OnOffHelper tcpClient("ns3::TcpSocketFactory", InetSocketAddress(interfaces.GetAddress(receiver), 9));
        tcpClient.SetConstantRate(DataRate("2Mbps"));
        tcpClient.SetAttribute("PacketSize", UintegerValue(packetSize));
        ApplicationContainer clientApp = tcpClient.Install(nodes.Get(i));
        clientApp.Start(Seconds(2.0));
        clientApp.Stop(Seconds(100.0));
    }

    // FlowMonitor
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(100.0));
    Simulator::Run();

    // Xuất dữ liệu
    flowMonitor->SerializeToXmlFile("flowmonitor_Scenario4.xml", true, true);
    std::ofstream outFile("flowmonitor_results_Scenario4.csv");
    outFile << "Flow ID,Tx Packets,Rx Packets,Lost Packets,Loss Rate (%)\n";
    flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    auto stats = flowMonitor->GetFlowStats();
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        uint32_t tx = it->second.txPackets;
        uint32_t rx = it->second.rxPackets;
        uint32_t lost = tx - rx;
        double rate = (tx > 0) ? (lost * 100.0 / tx) : 0.0;
        outFile << it->first << "," << tx << "," << rx << "," << lost << "," << rate << "\n";
    }
    outFile.close();

    Simulator::Destroy();
}

// =============================
// = MAIN
int main(int argc, char *argv[])
{
    uint32_t packetSize = 512;
    std::string scenarioType = "Scenario3";  // Mặc định Scenario1
    CommandLine cmd;
    cmd.AddValue("scenario", "Scenario type (Scenario1, Scenario2, Scenario3, Scenario4)", scenarioType);
    cmd.Parse(argc, argv);

    // Chạy từ 2 đến 30 node
    for (uint32_t n = 2; n <= 30; n++)
    {
        NS_LOG_UNCOND("***************************************");
        NS_LOG_UNCOND("Simulation with " << n << " nodes, " << scenarioType);

        if (scenarioType == "Scenario1") {
            Scenario1(n, packetSize);
        }
        else if (scenarioType == "Scenario2") {
            Scenario2(n, packetSize);
        }
        else if (scenarioType == "Scenario3") {
            Scenario3(n, packetSize);
        }
        else if (scenarioType == "Scenario4") {
            Scenario4(n, packetSize);
        }
        else {
            NS_LOG_UNCOND("Invalid scenario! Exiting...");
            return 0;
        }
    }

    return 0;
}
