/*
 * Copyright (c) 2017 Universita' degli Studi di Napoli Federico II
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
 */

#include "prio-queue-dscp-disc.h"





#include "ns3/log.h"
#include "ns3/object-factory.h"
#include "ns3/pointer.h"
#include "ns3/socket.h"
#include "ns3/ipv4-queue-disc-item.h"
#include "ns3/net-device-queue-interface.h"

#include <algorithm>
#include <iterator>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PrioQueueDscpDisc");

NS_OBJECT_ENSURE_REGISTERED(PrioQueueDscpDisc);



TypeId
PrioQueueDscpDisc::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::PrioQueueDscpDisc")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<PrioQueueDscpDisc>();

    return tid;
}

PrioQueueDscpDisc::PrioQueueDscpDisc()
    : QueueDisc(QueueDiscSizePolicy::NO_LIMITS)
{
    NS_LOG_FUNCTION(this);
}

PrioQueueDscpDisc::~PrioQueueDscpDisc()
{
    NS_LOG_FUNCTION(this);
}



bool
PrioQueueDscpDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this << item);

    
    uint32_t band = 0;

    Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);
    Ipv4Header ipHeader = ipItem->GetHeader();

    // Extract DSCP value from the IP header
    int dscp = ipHeader.GetDscp();
    // Map DSCP to band
    // Implement logic for mapping DSCP to bands here according to some RFC
   
    if (dscp == 46)
    {
        band = 0;
        NS_LOG_LOGIC("DSCP value: " << dscp << " band 0");
    }else{
        band = 1;
        NS_LOG_LOGIC("DSCP value: " << dscp << " band " << band << "- EF");
    }
    

    NS_ASSERT_MSG(band < GetNQueueDiscClasses(), "Selected band out of range");
    bool retval = GetQueueDiscClass(band)->GetQueueDisc()->Enqueue(item);
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " PRIO Enqueue: Number packets band " << band << ": " <<  GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets() << " from: " << ipHeader.GetSource());
    NS_LOG_LOGIC("Number packets band " << band << ": "
                                        << GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets());
    
    return retval;
}

Ptr<QueueDiscItem>
PrioQueueDscpDisc::DoDequeue()
{
    NS_LOG_FUNCTION(this);

    Ptr<QueueDiscItem> item;

    for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++)
    {
        if ((item = GetQueueDiscClass(i)->GetQueueDisc()->Dequeue()))
        {
            // NS_LOG_INFO("Popped from band " << i << ": " << item);
            NS_LOG_LOGIC("Number packets band "
                         << i << ": " << GetQueueDiscClass(i)->GetQueueDisc()->GetNPackets());
            NS_LOG_INFO(Simulator::Now().GetSeconds() << " PRIO Dequeue: Number packets band " << i << ": " <<  GetQueueDiscClass(i)->GetQueueDisc()->GetNPackets());
            return item;
        }
    }
   
    NS_LOG_LOGIC("Queue empty");
    return item;
}

Ptr<const QueueDiscItem>
PrioQueueDscpDisc::DoPeek()
{
    NS_LOG_FUNCTION(this);

    Ptr<const QueueDiscItem> item;

    for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++)
    {
        if ((item = GetQueueDiscClass(i)->GetQueueDisc()->Peek()))
        {
            NS_LOG_LOGIC("Peeked from band " << i << ": " << item);
            NS_LOG_LOGIC("Number packets band "
                         << i << ": " << GetQueueDiscClass(i)->GetQueueDisc()->GetNPackets());
            return item;
        }
    }

    NS_LOG_LOGIC("Queue empty");
    return item;
}

bool
PrioQueueDscpDisc::CheckConfig()
{
    NS_LOG_FUNCTION(this);
    if (GetNInternalQueues() > 0)
    {
        NS_LOG_ERROR("PrioQueueDscpDisc cannot have internal queues");
        return false;
    }

    if (GetNQueueDiscClasses() == 0)
    {
        // create 3 fifo queue discs
        ObjectFactory factory;
        factory.SetTypeId("ns3::FifoQueueDisc");
        for (uint8_t i = 0; i < 2; i++)
        {
            Ptr<QueueDisc> qd = factory.Create<QueueDisc>();
            qd->Initialize();
            Ptr<QueueDiscClass> c = CreateObject<QueueDiscClass>();
            c->SetQueueDisc(qd);
            AddQueueDiscClass(c);
        }
    }

    if (GetNQueueDiscClasses() < 2)
    {
        NS_LOG_ERROR("PrioQueueDscpDisc needs at least 2 classes");
        return false;
    }

    return true;
}

void
PrioQueueDscpDisc::InitializeParams()
{
    NS_LOG_FUNCTION(this);
}

} // namespace ns3
