# Appendix J – CAN Support

## CAN Support

MMBasic on the Colour Maximite 2 exposes the native support for CAN provided by the STM32H743 chip as the MMBasic CAN command.

A CAN transceiver such as the SN65HVD230 CAN Transceiver is required to interface to the actual physical CAN bus. It connects to 3.3V and GND plus the nominated CAN Tx and CAN Rx pins on the Colour Maximite 2. The CANL and CANH connectors go to the physical CAN bus. You can exercise the CAN command in loopback mode without having the transceiver attached.

![CAN transceiver](pdf:173:424){width=6}

The STM32H743 chip supports both the Classical CAN frame type and also the newer CAN FD (CAN Flexible Data-rate) frame type. The improvements are:

- CAN FD supports up to 64 data bytes per data frame vs. 8 data bytes for Classical CAN.
- CAN FD supports dual bit rates: the nominal (arbitration) bit-rate limited to 1 Mbit/s as in Classical CAN, and the data bit-rate, which depends on the network topology/transceivers.
- CAN FD uses an improved cyclic redundancy check (CRC) and the "protected stuff-bit counter", which lower the risk of undetected errors.

The following MMBasic commands provide access to the CAN controller in the STM32H743 chip. There are some variations to the syntax between Classical CAN and CAN FD. These common parameters apply to their use in the following commands:

{widths: 20,80}
| Parameter | Description |
|---|---|
| id, id1, id2 | These refer to either an 11bit (0-7FF) or 29bit (0-1FFFFFFF) identifier for a CAN Frame. id2 may also refer to a bit mask in the CAN FILTER command. |
| eid | 0 indicates 11bit id (STDID) is used/expected, 1 indicates a 29bit id (EXTID) is used/expected |
| rtr | Indicates a Remote Frame. i.e. dlc is 0 and no data. These are not used much any more. |
| dlc | Indicates the size of the data in the CAN message. (0-8) (Classical Frame Type) |
| msg | Contains 8 bytes of data, only those indicated by dlc are valid, the rest are set to 00. |
| ret | Is the return value for the command. |

## CAN OPEN index, speed, mode

This command opens the CAN interface indicated by index. This configures the CAN based on predefined pin allocations as shown below.

{widths: 15,15,15,55}
| Index | CAN Rx | CAN Tx | Notes |
|---|---|---|---|
| 1 | pin 5 | pin 3 | Rx shares I2C SCK, Tx shares I2C SDA |
| 2 | pin 19 | pin 40 | Rx shares SPI MOSI, Tx shares SPI2 CLK |

The speed indicates the desired CAN speed. Allowed values are 0 (special case), 125000, 250000, 500000 and 1000000. The sampling point is set to 80%. The speed is not directly given to the CAN hardware, a set of parameters is provided that will achieve the desired CAN speed and sample point. This short form of the CAN OPEN opens the CAN with a Frame Type of Classical.

To set a different speed for Classical CAN or use CAN FD the special case 0 must be used and the syntax as described below is required to specify the parameters to obtain the desired speed, frame type and sample points.

The mode parameter determines the mode to be configured:

{widths: 10,20,70}
| Mode | Operation | Notes |
|---|---|---|
| 0 | Normal | Normal operation |
| 1 | Internal Loop Back | This mode can be used for a loopback test. No data is placed on or received from the physical CAN Bus, however internally the transmitted data is also seen as received data. The transceiver does not need to be attached. |
| 2 | External Loop Back | This mode can be used for a loopback test. The transmitted data is also placed on the CAN Bus and seen by other nodes if the transceiver is attached and connected to the CAN Bus. |
| 3 | Bus Monitoring | This mode does not place data on the physical CAN Bus, but does receive data from the CAN Bus. Can be used to monitor a live system. |

In Normal mode the CAN sends and reads messages via the CAN bus using the attached CAN transceiver. It does not see its own messages. In Loop Back mode the CAN sees its own messages as if they were received from the CAN bus. The CAN transceiver does not need to be attached. This mode is useful for testing/developing and checking your filters work as expected. In this mode no messages are placed on or received from the CAN bus if it is attached.

The CAN OPEN command also sets a default Global Filter that accepts all messages into the FIFO1 buffer (see CAN FILTER GLOBAL below). This will start as soon as a CAN START command is issued. If you want to apply alternate filters to restrict what messages are accepted you enter them after the CAN OPEN command and before the CAN START command.

The following long forms of the CAN OPEN command, used when the speed is set to 0, allow the user to provide the prescaler, segment1, segment2 and SJW (SyncJumpWidth) values that allow the detailed selection of the speed and sampling point. The base clock speed is 40MHz and an online calculator (search for "Bit Timing Calculator for CAN FD") allows the determination of the values for various CAN speeds and sampling points.

The syntax below is used to set a Frame Type of Classical CAN to a desired speed and sample point:

```
CAN OPEN index,0,mode,prescaler,seg1,seg2,sjw
```

The syntax below is used to set a Frame Type of FD CAN to a desired speed and sample point. It also defines additional parameters to set the speed and sample point of the data segment, sets whether Bit Rate Switching is used and the data size to be used in the FD CAN frame (8, 12, 16, 20, 24, 32, 48 or 64):

```
CAN OPEN index,0,mode,prescaler,seg1,seg2,sjw,brs,dsize,dprescaler,dseg1,dseg2,dsjw
```

The parameters prescaler, seg1, seg2, sjw, dprescaler, dseg1, dseg2 and dsjw are the values obtained from the Bit Timing Calculator for CAN FD. brs indicates whether Bit Rate Switching is used (0 or 1). dsize indicates the maximum data size (8 to 64). These values give the common speeds:

![CAN bit timing](pdf:174:428){width=16}

## CAN FILTER GLOBAL id, eid, idr, eidr

The Global filter determines how each of the possible four types of CAN messages are handled. id and eid respectively apply to STDID and EXTID messages. They determine what happens to messages that do not match any filter after all the filters are checked. idr and eidr determine whether remote frames (i.e. rtr=1) are accepted. They can have values as below.

{widths: 50,50}
| id and eid (normal frame rtr=0) | idr and eidr (remote frames i.e. rtr=1) |
|---|---|
| 0 = Accept unmatched and pass to RxFIFO0 | 0 = Pass to filters for possible matching. |
| 1 = Accept unmatched and pass to RxFIFO1 | 1 = Reject message, don't attempt matching. |
| 2 = Reject message |  |

When a CAN OPEN command is issued a default Global Filter is set so that all messages are accepted to RxFIFO1. i.e. the equivalent of CAN FILTER GLOBAL 1,1,1,1

## CAN FILTER index, eid, type, config, id1, id2

Filters are used to let the CAN hardware do the work to discard messages you are not interested in, without using the MMBasic resources to analyse them. There are 32 filters configured for each of STDID and EXTID. They are tested in order of the index.

- index is the filter number 0-31, these are applied in order.
- eid is 0 for STDID filters and 1 for EXTID filters.
- type is the filter type: 0 is a range filter matching ids in the range id1 to id2; 1 is a filter containing two IDs (id1, id2) to be matched; 2 is a classic filter where id1 is the filter and id2 is the mask; 3 is a range filter for EXTIDs (eid 1) in which the extended ID mask is not applied.
- config values: 0 will disable the filter, i.e. it is ignored; 1 will accept a matched message into Rx FIFO0; 2 will accept a matched message into Rx FIFO1; 3 will reject the message.

The following filter will accept STDID messages with ids 235-300 into FIFO0:

```
CAN FILTER 0, 0, 0, 1, &H235, &H300
```

## CAN START

After the CAN is configured and the filters are set the CAN START command tells the CAN to start accepting messages and to accept messages to send. The CAN START command MUST be issued to start operation.

## CAN STOP

The CAN STOP command can be issued to stop sending and receiving messages. You could issue this command if you wanted to pause the reception of messages.

## CAN SEND

```
CAN SEND id, eid, rtr, dlc, msg, ret         ' Classical CAN
CAN SEND id, eid, rtr, dlc, msg(), ret       ' CAN FD
```

This command places a message in the next available transmit buffer and it is sent in order. There are 32 transmit buffers configured, so there is no need to configure a queue in MMBasic. If no free buffer is available then the ret value is 1.

If Frame Type of CAN FD is used then:

- If brs=1 in the CAN OPEN command Bit Rate Switching will be used.
- msg() must be a suitably dimensioned array for the data e.g. INTEGER msg(7) for 64 bytes.
- dlc must not exceed the datasize set in the CAN OPEN command.
- dlc can have values 0-8, 12, 16, 20, 24, 32, 48 or 64.

## CAN READ

```
CAN READ fifo, id, eid, rtr, dlc, msg, fmi, ret     ' Classical CAN
CAN READ fifo, id, eid, rtr, dlc, msg(), fmi, ret   ' CAN FD
```

This command reads a message from either FIFO0 or FIFO1 depending on the value of fifo and populates id, eid, rtr, dlc, msg and fmi with the details from the received CAN Frame. Each of the FIFO queues is configured with 32 buffers, so there is no need to configure a queue within MMBasic.

If Frame Type of CAN FD is used then:

- msg() must be a suitably dimensioned array. e.g. INTEGER msg(7) for 64 bytes.
- dlc must not exceed the datasize set in the CAN OPEN command.
- dlc can expect values 0-8, 12, 16, 20, 24, 32, 48 or 64.

ret is 0 if no messages are available else the number of messages available. The application needs to continually call this command in the main DO:LOOP as this implementation does not use interrupts and expects that messages are polled. Only one message is retrieved for each CAN READ command.

fmi is the Filter Match Index. It indicates which filter matched the message. The fmi is the filter number used when adding the filter. There are two numbering sequences numbered sequentially starting from 0 for each of STDID and EXTID filters. Using the fmi is an alternate method of analysing the incoming message, i.e. know which filter matched it to see what to do with it. The typical method is to analyse the ID to identify a particular message.

## CAN CLOSE

This command closes the CAN and releases the pins.

## Identifying a Node

If you are implementing a CAN system it is useful to be able to uniquely identify a node e.g. serial no. MM.INFO(ID) will return a unique 96bit ID for the processor as a hexadecimal string. This, whilst guaranteed to be unique, is too long to easily pass in a single CAN message.

MM.INFO(ID48) will return an integer with a 48 bit hash of this 96 bit ID which while not guaranteed to be unique will most probably be unique within any MMBasic systems you have. This can be placed in 6 bytes of a CAN message to identify a node and have two bytes left for the return of an allocated shorter node number to be used within the system.
