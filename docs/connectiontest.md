# Testing Beetle via command-line interface

The **Beetle** daemon communicates with it's consumers (usually **hubby**, but sometimes other **beetle** instances) via a TCP socket connection. You can use this same socket connection to interact with Beetle to verify it's functionality when talking to Afero devices.  In this document we will use two instances of **beetle** on two different boards to communicate with each other over BTLE using **beetle**'e "peripheral mode".

Build **beetle** through your normal process. Install it on two separate boards. One will act as a "peripheral" host, and one will act as a "hub" host.  

In this example we will call the "peripheral" host *system1* and the "hub" host *system2*.

On *system1*, launch **beetle** with the command line:

```
sudo beetle -m per -D -d
```

This forces **beetle** to start up in "peripheral" mode, increases the debugging level (-D), and forks beetle into the background to release the terminal (-d).


On *system2*, , launch **beetle** with the command line:

```
sudo beetle -m cen -D -d
```

This forces **beetle** to start up in "central" (hub) mode, increases the debugging level (-D), and forks beetle into the background to release the terminal (-d).

On both hosts, you will notice that **beetle** starts listening on local TCP port **6969**:

```
$ netstat -anp | grep 6969
tcp        0      0 127.0.0.1:6969          0.0.0.0:*               LISTEN      834/beetle      
```

We will be connecting to this TCP port on both systems to interact with them, and the two **beetle** daemons will communicate with each other over local BTLE connection.

### Connecting to the Beetle daemon

On the local system, you can use the **telnet** utility to connect to **beetle**'s local port. **Telnet** is not installed by default on some Linux distributions, so you may need to install the **telnet** package to install the utility.

On the *system1* system, connect to **beetle** with the command:

```
telnet localhost 6969 | grep -v "^adv"
```

This will connect you to the **beetle** instance you started previously and will filter out any advertising messages that are being broadcast by any other local BTLE devices.


On the *system2* system, connect to **beetle** with the command:

```
telnet localhost 6969 | grep "12345"
```

This will connect you to the **beetle** instance you started previously however it will only display any lines from telnet that contain the string "12345".  We're going to use that number in the advertisement packet from *system1* in just a minute, so for now just leave this session running the way it is.


On *system1*, start sending advertising packets over BLE with the command:

```
pad 0006 03c0 d202010123456789abcdef01
```

(TODO: explain the flags (0006) and appearance (03c0) bitfields.)

This starts advertising *system1* as an Afero device with a deviceID 0123456789abcdef.

On *system2* you will start seeing the advertising packets:

```
adv b8:27:eb:24:9d:99 d202010123456789abcdef01 -34
adv b8:27:eb:24:9d:99 d202010123456789abcdef01 -28
adv b8:27:eb:24:9d:99 d202010123456789abcdef01 -33
adv b8:27:eb:24:9d:99 d202010123456789abcdef01 -34
```

The output is the keyword "adv", then the BT MAC address of the device, then a string that starts "d20201" followed by the DeviceID, then the RSSI. You will need the MAC address of *system1*'s BTLE interface to communicate with it via **beetle**'s commands.

If you don't see advertisements, check that the pad command on the first system completed successfully (response "pad 0000"). If the commmand completed and you still don't see any advertisements, check that central.c:handle_advertisement() is being called and trace back from there if it isn't.

Note the MAC address shown in your output. We'll use it to connect the two systems together.


### Connecting to the Beetle daemon without all the Advertisement spam

Once you've found the MAC address of the first **beetle** system, you may way to restart your connection to **beetle** to filter out the constant flow of advertising messages so you can interact with individual devices.

Quit your **telnet** session on *system2* by pressing Ctrl-] then typing "quit" at the telnet command prompt:

```
^]quit
telnet> quit
Connection closed.
```

Now, restart another **telnet** session but use the **grep** command to filter out advertisement messages"

```
telnet localhost 6969 | grep -v "^adv"
```

### Unsolicited Events

In addition to the "adv" advertising messages you have seen, you may also recieve other unsolicited messages about devices. The two you may see are:

```
dis <file descriptor>
```

This message says that the device you were connected to has disconnected - this may be in response to a legitimate disconnect command, or the device may have dropped offline. When using beetle interactively like we are, it's not unusual for a BT device to disconnect after a while. If you get a disconnect message for the device you're working with, just reconnect to it.

```
not <file descriptor>
```

This is a notification message from the device you are connected to. These notifications may be enabled by the "nen" command while connected to a device.


### Connecting to the peripheral beetle instance

A complete list of **beetle** commands can be referenced on [The AferoCE GitHub](https://github.com/AferoCE/beetle/blob/master/docs/commands.md). Here we will show some simple communication between a **beetle** peripheral service and a **beetle** hub service.


On *system2*, connect to the MAC address of the *system1*'s **beetle** service we identified above:

```
con b8:27:eb:24:9d:99
```

The hex address on the command line is the MAC address of the device listed in the advertising messages. The connect command will return a status code and file descriptor number (4 hex digits). This FD is what you will use to interact with the device with other **beetle** commands.

```
con b8:27:eb:24:9d:99
con b8:27:eb:24:9d:99 000a 0000
```

In this connection the status code returned is '000a' and no FD is returned (meaning connection failed).

If you see this message, try connecting again:

```
con b8:27:eb:24:9d:99
con b8:27:eb:24:9d:99 000a 0000

con b8:27:eb:24:9d:99
con b8:27:eb:24:9d:99 0000 0007
```

This 2nd connection succeeded (status 0000) and the FD returned is "7".


Status codes can be found in **beetle.h**

```
#define STATUS_OK                0x00
#define STATUS_UNKNOWN_CONN      0x01
#define STATUS_UNKNOWN_ATTR      0x02
#define STATUS_PENDING           0x03
#define STATUS_NOT_PERMITTED     0x04
#define STATUS_IO_ERROR          0x05
#define STATUS_UNKNOWN_DEVICE    0x06
#define STATUS_ALREADY_CONNECTED 0x07
#define STATUS_CONN_LIST_FULL    0x08
#define STATUS_TIMED_OUT         0x09
#define STATUS_L2CAP_CONN_FAILED 0x0a
#define STATUS_NOT_CONNECTED     0x0b
#define STATUS_BLUETOOTH_ERROR   0x0c
#define STATUS_BAD_PARAM         0x0d
#define STATUS_ALREADY_IN_MODE   0x0e
#define STATUS_WRONG_MODE        0x0f
#define STATUS_DATABASE_FULL     0x10
#define STATUS_UNKNOWN_CMD       0x11
```

If you can't connect with a device, find the section in central.c that triggers the specific error above and diagnose from there.

On *system1* it will log the connection from the other beetle instance:

```
pco B8:27:EB:BB:96:ED
```

(In general, peripheral mode commands will be "p" plus the first two characters of the central mode command -- *con* results in a *pco*, *dis* results in a *pdi*, and so on.)

On the peripheral host *system1*, create some characteristics that you can interact with from the remote server:

```
pka fffe 0020
pka fffd 0008
pka 0000 0000
```

For each of these commands, **beetle** will return the command line back to you with a return code (listed above). They should all be "0000" (no error).


### Interacting with a connected beetle instance

Once connected, from *system2* there are 4 commands you can use to interact with the beetle instance on *system1*:

 * dis - disconnect from the connected device
 * kat - dump list of attributes from device
 * wri - write a specific attribute to the device
 * nen - enable notifications for an attribute change


Example responses:

```
dis 7
dis 0007 0000 0016
```

Disconnect from FD 7. Response *dis (FD) (status) (reason)". Status codes are from **beetle.h** above.

```
kat 7
kat 0007 0000 fffd 0008
kat 0007 0000 fffe 0020
kat 0007 0000 0000
```

This command lists Bluetooth characteristics transmitted over GATT. The data in these attributes is irrelevant since we just created them on the other system above. These characteristics are **write-only**. As long as you can see them and can write individual values (below) then this part of the communication path is working.

```
wri 7 fffd 01234567
```

This writes 0x01234567 to characteristic 0xfffd. On *system1* you should see the attribute being updated:

```
pwr fffd 01234567
```

This verifies that beetle was able to process the update.

```
nen 7 fffe 01
```

Enable notifications for attribute fffe. On *system1* you should see the notification enabled with the response:

```
pne fffe 0001
```

On *system1* send a notification for that attribute and watch *system2* receive it:

On *system1*

```
pin fffe 1234
```

On *system2* you should see:

```
not 0007 fffe 1234
```

This completes basic verification of peripheral mode and hub mode communication.
