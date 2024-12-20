#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("BusTopology");

int main(int argc, char *argv[]) {
    Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(137));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("14kb/s"));

    uint32_t nNodes = 4;

    NS_LOG_INFO("Build bus topology.");
    NodeContainer nodes;
    nodes.Create(nNodes);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices[nNodes - 1];
    for (uint32_t i = 0; i < nNodes - 1; i++) {
        devices[i] = pointToPoint.Install(nodes.Get(i), nodes.Get(i + 1));
    }

    NS_LOG_INFO("Install internet stack on all nodes.");
    InternetStackHelper internet;
    internet.Install(nodes);

    NS_LOG_INFO("Assign IP Addresses.");
    Ipv4AddressHelper address;
    Ipv4InterfaceContainer interfaces[nNodes - 1];
    for (uint32_t i = 0; i < nNodes - 1; i++) {
        std::string baseIP = "10.1." + std::to_string(i + 1) + ".0";
        address.SetBase(baseIP.c_str(), "255.255.255.0");
        interfaces[i] = address.Assign(devices[i]);
    }

    NS_LOG_INFO("Create applications.");
    uint16_t port = 50000;

    Address sinkAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", sinkAddress);
    ApplicationContainer sinkApp = packetSinkHelper.Install(nodes.Get(nNodes - 1));
    sinkApp.Start(Seconds(1.0));
    sinkApp.Stop(Seconds(10.0));

    OnOffHelper onOffHelper("ns3::TcpSocketFactory", Address());
    onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    AddressValue remoteAddress(InetSocketAddress(interfaces[nNodes - 2].GetAddress(1), port));
    onOffHelper.SetAttribute("Remote", remoteAddress);

    ApplicationContainer sourceApp = onOffHelper.Install(nodes.Get(0));
    sourceApp.Start(Seconds(1.0));
    sourceApp.Stop(Seconds(10.0));

    NS_LOG_INFO("Enable static global routing.");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    NS_LOG_INFO("Enable tracing.");
    AsciiTraceHelper ascii;
    pointToPoint.EnableAsciiAll(ascii.CreateFileStream("bus-topology.tr"));
    pointToPoint.EnablePcapAll("bus");

    AnimationInterface anim("bus-topology.xml");
    anim.SetMaxPktsPerTraceFile(5000);

    double xPos = 50.0;
    double yPos = 250.0;
    double xDelta = 150.0;

    for (uint32_t i = 0; i < nNodes; ++i) {
        anim.SetConstantPosition(nodes.Get(i), xPos + i * xDelta, yPos);
    }

    NS_LOG_INFO("Run Simulation.");
    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_INFO("Done.");
    return 0;
}

