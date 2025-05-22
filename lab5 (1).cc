#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

void RunScenario(uint32_t numNodes, std::string mobilityModel)
{
    NS_LOG_UNCOND("===== Running Scenario with " << numNodes << " nodes =====");

    // Tạo node
    NodeContainer nodes;
    nodes.Create(numNodes);

    // Cấu hình WiFi
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(channel.Create());

    WifiMacHelper mac;
    WifiHelper wifi;
    mac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install(wifiPhy, mac, nodes);

    // Tắt RTS/CTS
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("999999"));

    // Cấu hình Mobility
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
    else
    {
        mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                      "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=100.0]"),
                                      "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=100.0]"));

        mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                                  "Mode", StringValue("Time"),
                                  "Time", TimeValue(Seconds(1.0)),
                                  "Speed", StringValue("ns3::UniformRandomVariable[Min=1.0|Max=5.0]"),
                                  "Bounds", RectangleValue(Rectangle(-100, 100, -100, 100)));
    }
    mobility.Install(nodes);

    // Cấu hình giao thức mạng
    InternetStackHelper stack;
    stack.Install(nodes);
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Cấu hình server trên node 0
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApp = echoServer.Install(nodes.Get(0));
    serverApp.Start(Seconds(10.0));
    serverApp.Stop(Seconds(100.0));

    // Cấu hình client gửi packet
    uint32_t packetSize = 1024;
    uint32_t maxPackets = 100;
    double interval = 1.0;

    for (uint32_t i = 1; i < numNodes; i++)
    {
        UdpEchoClientHelper echoClient(interfaces.GetAddress(0), 9);
        echoClient.SetAttribute("MaxPackets", UintegerValue(maxPackets));
        echoClient.SetAttribute("Interval", TimeValue(Seconds(interval)));
        echoClient.SetAttribute("PacketSize", UintegerValue(packetSize));

        ApplicationContainer clientApp = echoClient.Install(nodes.Get(i));
        clientApp.Start(Seconds(10.0 + i * 0.1));
        clientApp.Stop(Seconds(100.0));
    }

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(100.0));
    Simulator::Run();

    // **Lưu dữ liệu vào file XML**
    flowMonitor->SerializeToXmlFile("flowmonitor_results.xml", true, true);

    Simulator::Destroy();
}

void RunAllScenarios()
{
    for (uint32_t numNodes = 2; numNodes <= 30; numNodes++)
    {
        RunScenario(numNodes, "ConstantPositionMobilityModel");
    }
}

int main(int argc, char *argv[])
{
    CommandLine cmd;
    cmd.Parse(argc, argv);
    RunAllScenarios();
    return 0;
}
