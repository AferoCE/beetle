# Testing Beetle "hub mode" via command-line interface

The **Beetle** daemon communicates with it's consumers (usually **hubby**) via a TCP socket connection. You can use this same socket connection to interact with Beetle to verify it's functionality when talking to Afero devices.  See also "**Testing Beetle "peripheral mode" via command-line interface**" in this same directory.

Build **beetle** through your normal process. Launch **beetle** with the command line:

```
sudo beetle -m cen -D -d
```

This forces **beetle** to start up in "central" (hub) mode, increases the debugging level (-D), and forks beetle into the background to release the terminal (-d).

You will notice that **beetle** starts listening on local TCP port **6969**:

```
$ netstat -anp | grep 6969
tcp        0      0 127.0.0.1:6969          0.0.0.0:*               LISTEN      834/beetle      
```


### Connecting to the Beetle daemon

On the local system, you can use the **telnet** utility to connect to **beetle**'s local port. **Telnet** is not installed by default on some Linux distributions, so you may need to install the **telnet** package to install the utility.

Connect to **beetle** with the command:

```
telnet localhost 6969
```

This will connect you to the **beetle** instance you started previously. If you have any Afero devices powered on and nearby, you will immediately start seeing BTLE advertising packets scrolling through **beetle**'s output:

```
$ telnet localhost 6969
Trying ::1...
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
adv fe:44:e0:f6:ea:bf d202010123074ed62ca57101 -44
adv cc:53:40:e0:ec:57 d2020101236548d72ca57101c5dd60 -48
adv d1:f6:38:f4:a0:88 d2020101238373009b376801339494 -59
adv fe:44:e0:f6:ea:bf d202010123074ed62ca57101 -61
adv d3:37:9f:91:c2:55 d2020001234026d62ca57101ba4a76 -41
adv e2:f0:87:90:a6:0b d2020001231029c9062b370170a308 -52
```

The output is the keyword "adv", then the BT MAC address of the device, then a string that starts "d2020" followed by the DeviceID, then the RSSI. You will need the MAC address to communicate via **beetle**'s commands, so note the MAC address of one or more devices that you want to interact with.

If you don't see advertisements, check that you have at least one Afero device powered on and within range of your system. If you have devices powered on and don't see any advertisements, check that central.c:handle_advertisement() is being called and trace back from there if it isn't.

### Connecting to the Beetle daemon without all the Advertisement spam

Once you've selected a couple of MAC addresses, you may way to restart your connection to **beetle** to filter out the constant flow of advertising messages so you can interact with individual devices.

Quit your **telnet** session by pressing Ctrl-] then typing "quit" at the telnet command prompt:

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


### Connecting to an Afero device

A complete list of **beetle** commands can be referenced on [The AferoCE GitHub](https://github.com/AferoCE/beetle/blob/master/docs/commands.md). Here we will show a few examples of the more useful ones.


Connect to a device with the command:

```
con fe:44:e0:f6:ea:bf
```

The hex address on the command line is the MAC address of the device listed in the advertising messages. The connect command will return a status code and file descriptor number (4 hex digits). This FD is what you will use to interact with the device with other **beetle** commands.

```
con fe:44:e0:f6:ea:bf
con fe:44:e0:f6:ea:bf 000a 0000
```

In this connection the status code returned is '000a' and tno FD is returned (meaning connection failed).

If you see this message, try connecting again:

```
con fe:44:e0:f6:ea:bf
con fe:44:e0:f6:ea:bf 000a 0000

con fe:44:e0:f6:ea:bf
con fe:44:e0:f6:ea:bf 0000 0007
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

### Interacting with a connected device

Once connected, there are 5 commands you can use to interact with the device:

 * dis - disconnect from the connected device
 * kat - dump list of attributes from device
 * rea - read a specific attribute from device
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
kat 0007 0000 fffd 0020
kat 0007 0000 fffe 0008
kat 0007 0000 fffb 0020
kat 0007 0000 fffc 0008
kat 0007 0000 fff7 0014
kat 0007 0000 fff8 0002
kat 0007 0000 fff9 0010
kat 0007 0000 fffa 0004
kat 0007 0000 fe00 0004
kat 0007 0000 fdff 0018
kat 0007 0000 fe00 0002
kat 0007 0000 0000
```

This command lists Bluetooth characteristics transmitted over GATT. The details of these attributes are Afero-internal and are **write-ony**, so as long as you can see them and can write individual values (below) then this part of the communication path is working.

```
wri 7 fffd 01234
```

This attempts to write 0x01234 to characteristic 0xfffd, which should fail trying to write to a real device. The command should complete with error 0004:

```
wri 7 fffd 0004
```

This completes basic verification of peripheral mode and hub mode communication.


