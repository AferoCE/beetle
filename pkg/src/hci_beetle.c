#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/l2cap.h>

#include "beetle.h"
#include "hci_beetle.h"
#include "utils.h"

#define MAX_KILLED_CONNECTIONS 16

/*
 * Wrappers for bluetooth HCI
 */

int bhci_open(bhci_t *bhci, const char *interface_name) {
    bhci_clear_callbacks(bhci);

    int dev_id = interface_name ? hci_devid(interface_name) : hci_get_route(NULL);
    if (dev_id < 0) {
        log_failure("hci_devid/hci_get_route");
        return -1;
    }

    /* try to get own address */
    if (hci_devba(dev_id, &bhci->own_address) < 0) {
        log_failure("hci_devba");
        return -1;
    }
    bhci->fd = hci_open_dev(dev_id);
    if (bhci->fd < 0) {
        log_failure("hci_open_dev");
        return -1;
    }
    return 0;
}

void bhci_clear_callbacks(bhci_t *bhci) {
    bhci->on_disconnect = NULL;
    bhci->on_disconnect_arg = NULL;
    bhci->on_connect = NULL;
    bhci->on_connect_arg = NULL;
    bhci->on_advertisement = NULL;
    bhci->on_advertisement_arg = NULL;
}

void bhci_close(bhci_t *bhci) {
    close(bhci->fd);
    bhci_clear_callbacks(bhci);
    bhci->fd = -1;
    memset(&bhci->own_address, 0, sizeof(bhci->own_address));
}

int bhci_set_scan_enable(bhci_t *bhci, uint8_t enable) {
    le_set_scan_enable_cp scan_cp;

    memset(&scan_cp, 0, sizeof(scan_cp));
    scan_cp.enable = enable;
	scan_cp.filter_dup = 0;

    if (hci_send_cmd(bhci->fd, OGF_LE_CTL, OCF_LE_SET_SCAN_ENABLE, sizeof(scan_cp), &scan_cp) < 0) {
        log_failure("bhci_set_scan_enable");
        return -1;
    }
    return 0;
}

int bhci_set_advertising_enable(bhci_t *bhci, uint8_t enable) {
    le_set_advertise_enable_cp adv_cp;

    memset(&adv_cp, 0, sizeof(adv_cp));
    adv_cp.enable = enable;

    if (hci_send_cmd(bhci->fd, OGF_LE_CTL, OCF_LE_SET_ADVERTISE_ENABLE, sizeof(adv_cp), &adv_cp) < 0) {
        log_failure("bhci_set_scan_enable");
        return -1;
    }
    return 0;
}

int bhci_disconnect(bhci_t *bhci, uint16_t handle) {
    disconnect_cp dis_cp;

    memset(&dis_cp, 0, sizeof(dis_cp));
    dis_cp.handle = htobs(handle);
    dis_cp.reason = HCI_CONNECTION_TERMINATED;

    if (hci_send_cmd(bhci->fd, OGF_LINK_CTL, OCF_DISCONNECT, sizeof(dis_cp), &dis_cp) < 0) {
        log_failure("bhci_disconnect");
        return -1;
    }
    return 0;
}

/*
 * clear all filters, and set the scan parameters.
 */
int bhci_set_scan_parameters(bhci_t *bhci, int interval, int window) {
    le_set_scan_parameters_cp scan_cp;
    struct hci_filter flt;

    hci_filter_clear(&flt);
    hci_filter_all_ptypes(&flt);
    hci_filter_all_events(&flt);
    if (setsockopt(bhci->fd, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
        log_failure("HCI_FILTER");
        return -1;
    }

    memset(&scan_cp, 0, sizeof(scan_cp));
    scan_cp.type = 0;
    scan_cp.interval = htobs(interval);
    scan_cp.window = htobs(window);
    scan_cp.own_bdaddr_type = 0;
    scan_cp.filter = 0;

    if (hci_send_cmd(bhci->fd, OGF_LE_CTL, OCF_LE_SET_SCAN_PARAMETERS, sizeof(scan_cp), &scan_cp) < 0) {
        log_failure("bhci_set_scan_parameters");
        return -1;
    }
    return 0;
}

int bhci_kill_all_connections(bhci_t *bhci) {
    struct hci_conn_list_req *cl;
    int i;

    cl = (struct hci_conn_list_req *) malloc(MAX_KILLED_CONNECTIONS * sizeof(struct hci_conn_info) + sizeof(*cl));
    cl->dev_id = 0;
    cl->conn_num = MAX_KILLED_CONNECTIONS;

    if (ioctl(bhci->fd, HCIGETCONNLIST, (void *) cl)) {
        free(cl);
        log_failure("HCIGETCONNLIST");
        return -1;
    }

    for (i = 0; i < cl->conn_num; i++) {
        char addr[18];
        addr2str(&cl->conn_info[i].bdaddr, addr);
        syslog(LOG_INFO, "Disconnecting %s", addr);
        bhci_disconnect(bhci, btohs(cl->conn_info[i].handle));
    }

    free(cl);
    return 0;
}

int bhci_device_connect(bhci_t *bhci, const char *addr, uint8_t addr_type) {
    le_create_connection_cp create_conn_cp;

    /* build the UART packet directly */
    memset(&create_conn_cp, 0, sizeof(create_conn_cp));
    create_conn_cp.interval = htobs(0x00a0);
    create_conn_cp.window = htobs(0x00a0);
    create_conn_cp.initiator_filter = 0;
    create_conn_cp.peer_bdaddr_type = addr_type;
    str2ba(addr, &create_conn_cp.peer_bdaddr);
    create_conn_cp.own_bdaddr_type = LE_PUBLIC_ADDRESS;
    create_conn_cp.min_interval = htobs(0x0013);
    create_conn_cp.max_interval = htobs(0x0013);
    create_conn_cp.latency = htobs(0x0009);
    create_conn_cp.supervision_timeout = htobs(0x0190);
    create_conn_cp.min_ce_length = htobs(0x0000);
    create_conn_cp.max_ce_length = htobs(0x0000);

    if (hci_send_cmd(bhci->fd, OGF_LE_CTL, OCF_LE_CREATE_CONN, sizeof(create_conn_cp), &create_conn_cp) < 0) {
        log_failure("bhci_device_connect");
        return -1;
    }
    return 0;
}

/* cancel whatever the current connection is */
int bhci_cancel_device_connect(bhci_t *bhci) {
    if (hci_send_cmd(bhci->fd, OGF_LE_CTL, OCF_LE_CREATE_CONN_CANCEL, 0, NULL) < 0) {
        log_failure("bhci_cancel_device_connect");
        return -1;
    }
    return 0;
}

int bhci_listen(bhci_t *bhci) {
    int fd = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (fd < 0) {
        log_failure("bhci_listen:socket");
        return -1;
    }

    /* set up the local address */
    struct sockaddr_l2 localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.l2_family = AF_BLUETOOTH;
    localAddr.l2_cid = htobs(0x04);
    localAddr.l2_bdaddr_type = BDADDR_LE_PUBLIC;
    localAddr.l2_bdaddr = bhci->own_address;

    if (bind(fd, (struct sockaddr *) &localAddr, sizeof(localAddr)) < 0) {
        log_failure("bhci_listen:bind");
        close(fd);
        return -1;
    }

    struct bt_security sec;
    memset(&sec, 0, sizeof(sec));
    sec.level = BT_SECURITY_LOW;
    if (setsockopt(fd, SOL_BLUETOOTH, BT_SECURITY, &sec, sizeof(sec)) < 0) {
        log_failure("bhci_listen:setsockopt");
        close(fd);
        return -1;
    }

    if (listen(fd, 1) < 0) {
        log_failure("bhci_listen:listen");
        close(fd);
        return -1;
    }

    return fd;
}

int bhci_set_advertising_parameters(bhci_t *bhci, uint16_t min_interval, uint16_t max_interval) {
    le_set_advertising_parameters_cp adv_cp;

    /* build the UART packet directly */
    memset(&adv_cp, 0, sizeof(adv_cp));
    adv_cp.min_interval = htobs(min_interval);
    adv_cp.max_interval = htobs(max_interval);
    adv_cp.advtype = 0;
    adv_cp.own_bdaddr_type = LE_PUBLIC_ADDRESS;
    adv_cp.chan_map = 7;
    adv_cp.filter = 0;

    if (hci_send_cmd(bhci->fd, OGF_LE_CTL, OCF_LE_SET_ADVERTISING_PARAMETERS, sizeof(adv_cp), &adv_cp) < 0) {
        log_failure("bhci_set_advertising_parameters");
        return -1;
    }
    return 0;
}

int bhci_set_advertising_data(bhci_t *bhci, uint8_t *data, int len) {
    le_set_advertising_data_cp data_cp;

    /* build the UART packet directly */
    memset(&data_cp, 0, sizeof(data_cp));
    data_cp.length = len;
    memcpy(&data_cp.data, data, len);
    if (hci_send_cmd(bhci->fd, OGF_LE_CTL, OCF_LE_SET_ADVERTISING_DATA, sizeof(data_cp), &data_cp) < 0) {
        log_failure("bhci_set_advertising_data");
        return -1;
    }
    return 0;
}

/* open a non-blocking L2CAP socket to a device */
int bhci_l2cap_connect(const char* addr, int cid, int addr_type) {
    if (g_debug >= 1) {
        syslog(LOG_DEBUG, "bhci_l2cap_connect: %s %d", addr, addr_type);
    }

    int fd = socket(PF_BLUETOOTH, SOCK_SEQPACKET | SOCK_NONBLOCK, BTPROTO_L2CAP);
    if (fd < 0) {
        log_failure("bhci_l2cap_connect:socket");
        return -1;
    }

    /* set the socket type to l2cap */
    struct sockaddr_l2 sockAddr;
    memset(&sockAddr, 0, sizeof(sockAddr));
    sockAddr.l2_family = AF_BLUETOOTH;
    sockAddr.l2_cid = htobs(cid);

    if (bind(fd, (struct sockaddr*) &sockAddr, sizeof(sockAddr)) < 0) {
        log_failure("bind");
        return -1;
    }

    /* connect to the device */
    str2ba(addr, &sockAddr.l2_bdaddr);
    sockAddr.l2_bdaddr_type = (addr_type ? BDADDR_LE_RANDOM : BDADDR_LE_PUBLIC);

    if (connect(fd, (struct sockaddr *) &sockAddr, sizeof(sockAddr)) < 0) {
        if (errno != EINPROGRESS) {
            log_failure("connect");
            return -1;
        }
    }
    return fd;
}

/* call me when the HCI socket polls as "readable" */
void bhci_read(bhci_t *bhci) {
    uint8_t buffer[HCI_MAX_FRAME_SIZE];
    char hex_data[HCI_MAX_FRAME_SIZE * 2 + 1];
    ssize_t len = recv(bhci->fd, buffer, sizeof(buffer), 0);

    if (len <= 0) {
        log_failure("HCI read");
        return;
    }
    if (len < 3) {
        syslog(LOG_ERR, "ignoring truncated bluetooth packet (len=%zi)", len);
        return;
    }

    if (buffer[0] != HCI_EVENT_PKT) return;
    uint8_t event = buffer[1];
    void *data = &buffer[3];
    int data_len = len - 3;

    switch (event) {
        case EVT_CMD_STATUS:
            if (data_len >= sizeof(evt_cmd_status)) {
                evt_cmd_status *status = data;
                syslog(LOG_DEBUG, "evt_cmd_status %02x %04x", status->status, htobs(status->opcode));
            }
            break;

        case EVT_CMD_COMPLETE:
            if (data_len >= sizeof(evt_cmd_complete)) {
                evt_cmd_complete *cc = data;
                syslog(LOG_DEBUG, "evt_cmd_complete %02x %04x", cc->ncmd, htobs(cc->opcode));
            }
            break;

        case EVT_NUM_COMP_PKTS:
            if (data_len >= 1) {
                uint8_t i = 0, count = ((uint8_t *)data)[0];
                data++, len--;
                while (i < count && len >= 4) {
                    uint16_t handle = btohs(*(uint16_t *)data);
                    data += 2, len -= 2;
                    uint16_t completed = btohs(*(uint16_t *)data);
                    data += 2, len -= 2;
                    syslog(LOG_DEBUG, "completed packet handle=%i count=%i", handle, completed);
                }
            }
            break;

        case EVT_DISCONN_COMPLETE:
            if (data_len >= sizeof(evt_disconn_complete)) {
                evt_disconn_complete *disconn = data;
                syslog(LOG_INFO, "DISCONNECT hci_handle=%d status=0x%02x reason=0x%02x",
                    btohs(disconn->handle), disconn->status, disconn->reason);
                if (bhci->on_disconnect) bhci->on_disconnect(bhci, disconn, bhci->on_disconnect_arg);
            }
            break;

        case EVT_LE_META_EVENT:
            if (data_len > 0) {
                uint8_t le_event = *(uint8_t *)data;
                data++, data_len--;
                switch (le_event) {
                    case EVT_LE_CONN_COMPLETE:
                        if (data_len >= sizeof(evt_le_connection_complete)) {
                            evt_le_connection_complete *conn = data;
                            char addr[BT_ADDR_SIZE];
                            addr2str(&conn->peer_bdaddr, addr);
                            syslog(LOG_INFO, "LE_CONNECT: addr=%s handle=%d", addr, btohs(conn->handle));
                            if (bhci->on_connect) bhci->on_connect(bhci, conn, addr, bhci->on_connect_arg);
                        }
                        break;

                    case EVT_LE_ADVERTISING_REPORT:
                        if (bhci->on_advertisement) bhci->on_advertisement(bhci, data, len, bhci->on_advertisement_arg);
                        break;

                    case EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE:
                        if (data_len >= sizeof(evt_le_read_remote_used_features_complete)) {
                            evt_le_read_remote_used_features_complete *features = data;
                            syslog(LOG_DEBUG, "connect complete status=%i handle=%i",
                                features->status, btohs(features->handle));
                        }
                        break;

                    default:
                        syslog(LOG_DEBUG, "hci %s", data2hex(hex_data, sizeof(hex_data), buffer, len));
                }
            }
            break;

        default:
            syslog(LOG_DEBUG, "hci %s", data2hex(hex_data, sizeof(hex_data), buffer, len));
    }
}
