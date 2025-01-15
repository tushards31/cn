#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/tcp-header.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TCPCongestionExample");

int main (int argc, char *argv[])
{
    bool enableFlowMonitor = true;
    Time::SetResolution (Time::NS);
    
    LogComponentEnable ("TCPCongestionExample", LOG_LEVEL_INFO);
    LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("TcpSocketBase", LOG_LEVEL_INFO);

    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1024));
    Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (1));
    Config::SetDefault ("ns3::TcpSocketBase::WindowScaling", BooleanValue (true));
    Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
    
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1024*1024));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1024*1024));

    NodeContainer nodes;
    nodes.Create (4);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    Ptr<ConstantPositionMobilityModel> mobility1 = nodes.Get(0)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> mobility2 = nodes.Get(1)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> mobility3 = nodes.Get(2)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> mobility4 = nodes.Get(3)->GetObject<ConstantPositionMobilityModel>();

    mobility1->SetPosition(Vector(0.0, 0.0, 0.0));
    mobility2->SetPosition(Vector(50.0, 0.0, 0.0));
    mobility3->SetPosition(Vector(100.0, 0.0, 0.0));
    mobility4->SetPosition(Vector(150.0, 0.0, 0.0));

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("2Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("5ms"));
    pointToPoint.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("100p"));

    NetDeviceContainer devices = pointToPoint.Install (nodes.Get (0), nodes.Get (1));
    NetDeviceContainer devices2 = pointToPoint.Install (nodes.Get (1), nodes.Get (2));
    NetDeviceContainer devices3 = pointToPoint.Install (nodes.Get (2), nodes.Get (3));

    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign (devices);

    address.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces2 = address.Assign (devices2);

    address.SetBase ("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces3 = address.Assign (devices3);

    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));

    uint16_t port = 9;
    OnOffHelper onoff ("ns3::TcpSocketFactory", 
                      InetSocketAddress (interfaces3.GetAddress (1), port));
    onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=2]"));
    onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute ("DataRate", StringValue ("1Mbps"));
    onoff.SetAttribute ("PacketSize", UintegerValue (1000));
    onoff.SetAttribute ("MaxBytes", UintegerValue (0));

    ApplicationContainer sourceApps = onoff.Install (nodes.Get (0));
    sourceApps.Start (Seconds (1.0));
    sourceApps.Stop (Seconds (20.0));

    PacketSinkHelper sink ("ns3::TcpSocketFactory",
                          InetSocketAddress (Ipv4Address::GetAny (), port));
    ApplicationContainer sinkApps = sink.Install (nodes.Get (3));
    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop (Seconds (20.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    if (enableFlowMonitor)
    {
        flowMonitor = flowHelper.InstallAll();
    }
    
    AnimationInterface anim ("tcp-congestion.xml");
    anim.SetMobilityPollInterval (Seconds (0.1));
    anim.SetConstantPosition (nodes.Get(0), 0.0, 0.0);
    anim.SetConstantPosition (nodes.Get(1), 50.0, 0.0);
    anim.SetConstantPosition (nodes.Get(2), 100.0, 0.0);
    anim.SetConstantPosition (nodes.Get(3), 150.0, 0.0);
    
    anim.EnablePacketMetadata (true);
    anim.EnableIpv4L3ProtocolCounters (Seconds (0), Seconds (20));

    anim.UpdateNodeDescription(nodes.Get(0), "TCP Sender");
    anim.UpdateNodeDescription(nodes.Get(1), "Router 1");
    anim.UpdateNodeDescription(nodes.Get(2), "Router 2");
    anim.UpdateNodeDescription(nodes.Get(3), "TCP Receiver");
    
    anim.UpdateNodeColor(nodes.Get(0), 255, 0, 0);
    anim.UpdateNodeColor(nodes.Get(1), 0, 0, 255);
    anim.UpdateNodeColor(nodes.Get(2), 0, 0, 255);
    anim.UpdateNodeColor(nodes.Get(3), 0, 255, 0);

    AsciiTraceHelper ascii;
    pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("tcp-congestion.tr"));
    pointToPoint.EnablePcapAll ("tcp-congestion");

    Simulator::Stop (Seconds (20.0));
    Simulator::Run ();

    if (enableFlowMonitor)
    {
        flowMonitor->SerializeToXmlFile("tcp-flow-monitor.xml", true, true);
    }

    Simulator::Destroy ();
    return 0;
}
