#pragma once

#include <stdint.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/l2cap.h>

typedef struct bhci {
    int fd;
    bdaddr_t own_address;

    /* callbacks */
    void (*on_disconnect)(struct bhci *bhci, evt_disconn_complete *disconn, void *arg);
    void *on_disconnect_arg;
    void (*on_connect)(struct bhci *bhci, evt_le_connection_complete *conn, char *addr, void *arg);
    void *on_connect_arg;
    void (*on_advertisement)(struct bhci *bhci, uint8_t *data, int len, void *arg);
    void *on_advertisement_arg;
} bhci_t;

int bhci_open(bhci_t *bhci, const char *interface_name);
void bhci_clear_callbacks(bhci_t *bhci);
void bhci_close(bhci_t *bhci);
int bhci_set_scan_enable(bhci_t *bhci, uint8_t enable);
int bhci_set_advertising_enable(bhci_t *bhci, uint8_t enable);
int bhci_disconnect(bhci_t *bhci, uint16_t handle);
int bhci_set_scan_parameters(bhci_t *bhci, int interval, int window);
int bhci_kill_all_connections(bhci_t *bhci);
int bhci_device_connect(bhci_t *bhci, const char *addr, uint8_t addr_type);
int bhci_cancel_device_connect(bhci_t *bhci);
int bhci_listen(bhci_t *bhci);
int bhci_set_advertising_parameters(bhci_t *bhci, uint16_t min_interval, uint16_t max_interval);
int bhci_set_advertising_data(bhci_t *bhci, uint8_t *data, int len);
int bhci_l2cap_connect(const char* addr, int cid, int addr_type);
void bhci_read(bhci_t *bhci);

#define bhci_enable_scan(bhci) bhci_set_scan_enable(bhci, 0x01)
#define bhci_disable_scan(bhci) bhci_set_scan_enable(bhci, 0x00)

#define bhci_enable_advertising(bhci) bhci_set_advertising_enable(bhci, 0x01)
#define bhci_disable_advertising(bhci) bhci_set_advertising_enable(bhci, 0x00)

#define bhci_on_disconnect(bhci, callback, arg) do { \
    bhci->on_disconnect = callback; \
    bhci->on_disconnect_arg = arg; \
} while (0)

#define bhci_on_connect(bhci, callback, arg) do { \
    bhci->on_connect = callback; \
    bhci->on_connect_arg = arg; \
} while (0)

#define bhci_on_advertisement(bhci, callback, arg) do { \
    bhci->on_advertisement = callback; \
    bhci->on_advertisement_arg = arg; \
} while (0)
