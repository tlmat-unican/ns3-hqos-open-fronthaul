/*
 * Copyright (c) 2007, 2014 University of Washington
 *               2015 Universita' degli Studi di Napoli Federico II
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors:  Stefano Avallone <stavallo@unina.it>
 *           Tom Henderson <tomhend@u.washington.edu>
 */

#include "marker-queue-disc.h"

#include "ns3/log.h"
#include "ns3/object-factory.h"
#include "ns3/queue.h"
#include "ns3/socket.h"
#include "ns3/ipv4-queue-disc-item.h"
#include "ns3/udp-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-l4-protocol.h"


#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/test.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MarkerQueueDisc");

NS_OBJECT_ENSURE_REGISTERED(MarkerQueueDisc);

TypeId
MarkerQueueDisc::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MarkerQueueDisc")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<MarkerQueueDisc>()
            .AddAttribute("MarkingQueue",
                          "It can be used in order to map dscp marking with the queues",
                          MapQueueValue(MapQueue{{8080, 0x2E},{8081, 4}}),
                          MakeMapQueueAccessor(&MarkerQueueDisc::markingMap),
                          MakeMapQueueChecker())
            .AddAttribute("MaxSize",
                          "The maximum number of packets accepted by this queue disc.",
                          QueueSizeValue(QueueSize("10024p")),
                          MakeQueueSizeAccessor(&QueueDisc::SetMaxSize, &QueueDisc::GetMaxSize),
                          MakeQueueSizeChecker());
    return tid;
}

MarkerQueueDisc::MarkerQueueDisc()
    : QueueDisc(QueueDiscSizePolicy::MULTIPLE_QUEUES, QueueSizeUnit::PACKETS)
{
    NS_LOG_FUNCTION(this);
}

MarkerQueueDisc::~MarkerQueueDisc()
{
    NS_LOG_FUNCTION(this);
}



bool
MarkerQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this << item);

    if (GetCurrentSize() >= GetMaxSize())
    {
        NS_LOG_LOGIC("Queue disc limit exceeded -- dropping packet");
        DropBeforeEnqueue(item, LIMIT_EXCEEDED_DROP);
        return false;
    }

    int band = 0;
    Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);
    Ipv4Header ipHeader = ipItem->GetHeader();
    Ptr<Packet> writablePacket = ipItem->GetPacket()->Copy(); // Copia para no modificar el original

    Ipv4Header ipv4Header;
    UdpHeader udpHeader;

    // Parse the IPv4 header
    writablePacket->RemoveHeader(ipv4Header);
    // Parse the UDP header
    writablePacket->RemoveHeader(udpHeader);
    
    // Obtain the source port
    uint16_t udpDestPort = udpHeader.GetDestinationPort();

    // Get an iterator pointing to the first element in the map
    std::map<int, int>::iterator it = markingMap.begin();

    // Initialize a variable to track whether the port was found in the range
    bool portFound = false;
 
    // Iterate over the map to find the port within the range
    while (it != markingMap.end()) {
        // Check if the port is within the range
        if (udpDestPort >= it->first && udpDestPort < (it->first + 100)) {
            // If found, set the corresponding DSCP value
            ipHeader.SetDscp(ns3::Ipv4Header::DscpType(it->second));
            band = 0;
            portFound = true;
            break; // Exit the loop once found
        }
        ++it;
    }

    // If the port was not found in the range, assign a default DSCP value (e.g., CS4)
    if (!portFound) {
        NS_LOG_INFO("Src Port");
        ipHeader.SetDscp(Ipv4Header::DSCP_CS4);
        band = 0;
    }
    
  
    Ptr<Packet> modpkt = ipItem->GetPacket()->Copy();
    modpkt->RemoveHeader(ipHeader);
    Ptr<QueueDiscItem> modifiedQueueItem = Create<Ipv4QueueDiscItem>(modpkt, ipItem->GetAddress(), ipItem->GetProtocol(), ipHeader);


    bool retval = GetInternalQueue(band)->Enqueue(modifiedQueueItem);
    if (!retval)
    {
        NS_LOG_WARN("Packet enqueue failed. Check the size of the internal queues");
    }

   

    return retval;
}

Ptr<QueueDiscItem>
MarkerQueueDisc::DoDequeue()
{
    
    NS_LOG_FUNCTION(this);

    Ptr<QueueDiscItem> item;

    // Dequeue from the single internal queue
    // if ((item = GetInternalQueue(0)->Dequeue()))
    // {
    //     NS_LOG_LOGIC("Dequeued: " << item);
    //     NS_LOG_LOGIC("Number of packets in the queue: " << GetInternalQueue(0)->GetNPackets());
    //     return item;
    // }

    
    for (uint32_t i = 0; i < GetNInternalQueues(); i++)
    {
        if ((item = GetInternalQueue(i)->Dequeue()))
        {
            NS_LOG_LOGIC("Popped from band " << i << ": " << item);
            NS_LOG_LOGIC("Number packets band " << i << ": " << GetInternalQueue(i)->GetNPackets());
            return item;
        }
    }

    NS_LOG_LOGIC("Queue empty");
    return item;
}

Ptr<const QueueDiscItem>
MarkerQueueDisc::DoPeek()
{
    NS_LOG_FUNCTION(this);

    Ptr<const QueueDiscItem> item;

    for (uint32_t i = 0; i < GetNInternalQueues(); i++)
    {
        if ((item = GetInternalQueue(i)->Peek()))
        {
            NS_LOG_LOGIC("Peeked from band " << i << ": " << item);
            NS_LOG_LOGIC("Number packets band " << i << ": " << GetInternalQueue(i)->GetNPackets());
            return item;
        }
    }

    NS_LOG_LOGIC("Queue empty");
    return item;
}

bool
MarkerQueueDisc::CheckConfig()
{
    NS_LOG_FUNCTION(this);
    if (GetNQueueDiscClasses() > 0)
    {
        NS_LOG_ERROR("MarkerQueueDisc cannot have classes");
        return false;
    }

    if (GetNPacketFilters() != 0)
    {
        NS_LOG_ERROR("MarkerQueueDisc needs no packet filter");
        return false;
    }

    if (GetNInternalQueues() == 0)
    {
        // create 3 DropTail queues with GetLimit() packets each
        ObjectFactory factory;
        factory.SetTypeId("ns3::DropTailQueue<QueueDiscItem>");
        factory.Set("MaxSize", QueueSizeValue(GetMaxSize()));
        AddInternalQueue(factory.Create<InternalQueue>());
        AddInternalQueue(factory.Create<InternalQueue>());
        AddInternalQueue(factory.Create<InternalQueue>());
        AddInternalQueue(factory.Create<InternalQueue>());
    }

    if (GetNInternalQueues() != 4)
    {
        NS_LOG_ERROR("MarkerQueueDisc needs 3 internal queues");
        return false;
    }

    if (GetInternalQueue(0)->GetMaxSize().GetUnit() != QueueSizeUnit::PACKETS ||
        GetInternalQueue(1)->GetMaxSize().GetUnit() != QueueSizeUnit::PACKETS ||
        GetInternalQueue(2)->GetMaxSize().GetUnit() != QueueSizeUnit::PACKETS ||
        GetInternalQueue(3)->GetMaxSize().GetUnit() != QueueSizeUnit::PACKETS )
    {
        NS_LOG_ERROR("MarkerQueueDisc needs 3 internal queues operating in packet mode");
        return false;
    }

    for (uint8_t i = 0; i < 4; i++)
    {
        if (GetInternalQueue(i)->GetMaxSize() < GetMaxSize())
        {
            NS_LOG_ERROR(
                "The capacity of some internal queue(s) is less than the queue disc capacity");
            return false;
        }
    }

    return true;
}

void
MarkerQueueDisc::InitializeParams()
{
    NS_LOG_FUNCTION(this);
}

} // namespace ns3
