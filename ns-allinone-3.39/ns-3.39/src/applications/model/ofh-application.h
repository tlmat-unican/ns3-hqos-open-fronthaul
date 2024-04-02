//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: George F. Riley<riley@ece.gatech.edu>
//

// ns3 - On/Off Data Source Application class
// George F. Riley, Georgia Tech, Spring 2007
// Adapted from ApplicationOnOff in GTNetS.

#ifndef OFH_APPLICATION_H
#define OFH_APPLICATION_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/seq-ts-size-header.h"
#include "ns3/traced-callback.h"

namespace ns3
{

class Address;
class RandomVariableStream;
class Socket;

/**
 * \ingroup applications
 * \defgroup onoff OfhApplication
 *
 * This traffic generator follows an On/Off pattern: after
 * Application::StartApplication
 * is called, "On" and "Off" states alternate. The duration of each of
 * these states is determined with the onTime and the offTime random
 * variables. During the "Off" state, no traffic is generated.
 * During the "On" state, cbr traffic is generated. This cbr traffic is
 * characterized by the specified "data rate" and "packet size".
 */
/**
 * \ingroup onoff
 *
 * \brief Generate traffic to a single destination according to an
 *        OnOff pattern.
 *
 * This traffic generator follows an On/Off pattern: after
 * Application::StartApplication
 * is called, "On" and "Off" states alternate. The duration of each of
 * these states is determined with the onTime and the offTime random
 * variables. During the "Off" state, no traffic is generated.
 * During the "On" state, cbr traffic is generated. This cbr traffic is
 * characterized by the specified "data rate" and "packet size".
 *
 * Note:  When an application is started, the first packet transmission
 * occurs _after_ a delay equal to (packet size/bit rate).  Note also,
 * when an application transitions into an off state in between packet
 * transmissions, the remaining time until when the next transmission
 * would have occurred is cached and is used when the application starts
 * up again.  Example:  packet size = 1000 bits, bit rate = 500 bits/sec.
 * If the application is started at time 3 seconds, the first packet
 * transmission will be scheduled for time 5 seconds (3 + 1000/500)
 * and subsequent transmissions at 2 second intervals.  If the above
 * application were instead stopped at time 4 seconds, and restarted at
 * time 5.5 seconds, then the first packet would be sent at time 6.5 seconds,
 * because when it was stopped at 4 seconds, there was only 1 second remaining
 * until the originally scheduled transmission, and this time remaining
 * information is cached and used to schedule the next transmission
 * upon restarting.
 *
 * If the underlying socket type supports broadcast, this application
 * will automatically enable the SetAllowBroadcast(true) socket option.
 *
 * If the attribute "EnableSeqTsSizeHeader" is enabled, the application will
 * use some bytes of the payload to store an header with a sequence number,
 * a timestamp, and the size of the packet sent. Support for extracting
 * statistics from this header have been added to \c ns3::PacketSink
 * (enable its "EnableSeqTsSizeHeader" attribute), or users may extract
 * the header via trace sources.  Note that the continuity of the sequence
 * number may be disrupted across On/Off cycles.
 */
class OfhApplication : public Application
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    OfhApplication();

    ~OfhApplication() override;

    /**
     * \brief Set the total number of bytes to send.
     *
     * Once these bytes are sent, no packet is sent again, even in on state.
     * The value zero means that there is no limit.
     *
     * \param maxBytes the total number of bytes to send
     */
    void SetMaxBytes(uint64_t maxBytes);

    /**
     * \brief Return a pointer to associated socket.
     * \return pointer to associated socket
     */
    Ptr<Socket> GetSocket() const;

    /**
     * \brief Assign a fixed random variable stream number to the random variables
     * used by this model.
     *
     * \param stream first stream index to use
     * \return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

  protected:
    void UserDoDispose();
    void ControlDoDispose();

  private:

    void StartApplication(); // Called at time specified by Start
    void ControlStopApplication();  // Called at time specified by Stop

    // helpers
    /**
     * \brief Cancel all pending events.
     */
    void ControlCancelEvents();

    // Event handlers
    /**
     * \brief Start an On period
     */
    void ControlStartSending();
    /**
     * \brief Start an Off period
     */
    void ControlStopSending();
    /**
     * \brief Send a packet
     */
    void ControlSendPacket();








    
    void UserStopApplication();  // Called at time specified by Stop

    // helpers
    /**
     * \brief Cancel all pending events.
     */
    void UserCancelEvents();

    // Event handlers
    /**
     * \brief Start an On period
     */
    void UserStartSending();
    /**
     * \brief Start an Off period
     */
    void UserStopSending();
    /**
     * \brief Send a packet
     */
    void UserSendPacket();

    // USER-Plane 

    Ptr<Socket> m_u_socket;                //!< Associated socket
    Address m_u_peer;                      //!< Peer address
    Address m_u_local;                     //!< Local address to bind to
    bool m_u_connected;                    //!< True if connected
    Ptr<RandomVariableStream> m_u_onTime;  //!< rng for On Time
    Ptr<RandomVariableStream> m_u_offTime; //!< rng for Off Time
    DataRate m_u_cbrRate;                  //!< Rate that data is generated
    DataRate m_u_cbrRateFailSafe;          //!< Rate that data is generated (check copy)
    uint32_t m_u_pktSize;                  //!< Size of packets
    uint32_t m_u_residualBits;             //!< Number of generated, but not sent, bits
    Time m_u_lastStartTime;                //!< Time last packet sent
    uint64_t m_u_maxBytes;                 //!< Limit total number of bytes sent
    uint64_t m_u_totBytes;                 //!< Total bytes sent so far
    EventId m_u_startStopEvent;            //!< Event id for next start or stop event
    EventId m_u_sendEvent;                 //!< Event id of pending "send packet" event
    uint32_t m_u_seq{0};                   //!< Sequence
    Ptr<Packet> m_u_unsentPacket;          //!< Unsent packet cached for future attempt
    bool tx_finished = false;

   // USER-Plane 

    Ptr<Socket> m_c_socket;                //!< Associated socket
    Address m_c_peer;                      //!< Peer address
    Address m_c_local;                     //!< Local address to bind to
    bool m_c_connected;                    //!< True if connected
    Ptr<RandomVariableStream> m_c_onTime;  //!< rng for On Time
    Ptr<RandomVariableStream> m_c_offTime; //!< rng for Off Time
    DataRate m_c_cbrRate;                  //!< Rate that data is generated
    DataRate m_c_cbrRateFailSafe;          //!< Rate that data is generated (check copy)
    uint32_t m_c_pktSize;                  //!< Size of packets
    uint32_t m_c_residualBits;             //!< Number of generated, but not sent, bits
    Time m_c_lastStartTime;                //!< Time last packet sent
    uint64_t m_c_maxBytes;                 //!< Limit total number of bytes sent
    uint64_t m_c_totBytes;                 //!< Total bytes sent so far
    EventId m_c_startStopEvent;            //!< Event id for next start or stop event
    EventId m_c_sendEvent;                 //!< Event id of pending "send packet" event
    uint32_t m_c_seq{0};                   //!< Sequence
    Ptr<Packet> m_c_unsentPacket;          //!< Unsent packet cached for future attempt


    // Global things
    bool m_enableSeqTsSizeHeader{false}; //!< Enable or disable the use of SeqTsSizeHeader
    TypeId m_tid;                        //!< Type of the socket used
    /// Traced Callback: transmitted packets.
    TracedCallback<Ptr<const Packet>> m_txTrace;

    /// Callbacks for tracing the packet Tx events, includes source and destination addresses
    TracedCallback<Ptr<const Packet>, const Address&, const Address&> m_txTraceWithAddresses;

    /// Callback for tracing the packet Tx events, includes source, destination, the packet sent,
    /// and header
    TracedCallback<Ptr<const Packet>, const Address&, const Address&, const SeqTsSizeHeader&>
        m_txTraceWithSeqTsSize;

  private:
    /**
     * \brief Schedule the next packet transmission
     */
    void UserScheduleNextTx();
    /**
     * \brief Schedule the next On period start
     */
    void UserScheduleStartEvent();
    /**
     * \brief Schedule the next Off period start
     */
    void UserScheduleStopEvent();
    /**
     * \brief Handle a Connection Succeed event
     * \param socket the connected socket
     */

     /**
     * \brief Schedule the next packet transmission
     */
    void ControlScheduleNextTx();
    /**
     * \brief Schedule the next On period start
     */
    void ControlScheduleStartEvent();
    /**
     * \brief Schedule the next Off period start
     */
    void ControlScheduleStopEvent();
    /**
     * \brief Handle a Connection Succeed event
     * \param socket the connected socket
     */



    void UserConnectionSucceeded(Ptr<Socket> socket);
    /**
     * \brief Handle a Connection Failed event
     * \param socket the not connected socket
     */
    void UserConnectionFailed(Ptr<Socket> socket);



    void ControlConnectionSucceeded(Ptr<Socket> socket);
    /**
     * \brief Handle a Connection Failed event
     * \param socket the not connected socket
     */
    void ControlConnectionFailed(Ptr<Socket> socket);
};

} // namespace ns3

#endif /* ONOFF_APPLICATION_H */