#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("UdpSimulation");

int main(int argc, char *argv[]) {
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
    LogComponentEnable("UdpServer", LOG_LEVEL_INFO);

    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);


    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    UdpServerHelper server1(9);
    ApplicationContainer serverApps1 = server1.Install(nodes.Get(1));
    serverApps1.Start(Seconds(1.0));
    serverApps1.Stop(Seconds(10.0));

    UdpServerHelper server2(10);
    ApplicationContainer serverApps2 = server2.Install(nodes.Get(0));
    serverApps2.Start(Seconds(1.0));
    serverApps2.Stop(Seconds(10.0));

    UdpClientHelper client1(interfaces.GetAddress(1), 9);
    client1.SetAttribute("MaxPackets", UintegerValue(100));
    client1.SetAttribute("Interval", TimeValue(Seconds(0.1)));
    client1.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps1 = client1.Install(nodes.Get(0));
    clientApps1.Start(Seconds(2.0));
    clientApps1.Stop(Seconds(10.0));

    UdpClientHelper client2(interfaces.GetAddress(0), 10);
    client2.SetAttribute("MaxPackets", UintegerValue(100));
    client2.SetAttribute("Interval", TimeValue(Seconds(0.15))); 
    client2.SetAttribute("PacketSize", UintegerValue(512));    

    ApplicationContainer clientApps2 = client2.Install(nodes.Get(1));
    clientApps2.Start(Seconds(2.0));
    clientApps2.Stop(Seconds(10.0));

    pointToPoint.EnablePcap("udp-simulation", devices.Get(0));
    pointToPoint.EnablePcap("udp-simulation", devices.Get(1));
    
    AsciiTraceHelper ascii;
    pointToPoint.EnableAsciiAll(ascii.CreateFileStream("udp-simulation.tr"));

    AnimationInterface anim("udp-animation.xml");
    anim.SetConstantPosition(nodes.Get(0), 10.0, 10.0);
    anim.SetConstantPosition(nodes.Get(1), 40.0, 10.0);

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
