#include "net.h"

#include "log.h"
#include "utils.h"

#include <alloca.h>
#include <endian.h>
#include <stdint.h>
#include <string.h>

typedef uint8_t code_t;
typedef uint64_t payload_size_t;
typedef uint64_t arg_t;
/* Payload conversion */

static inline arg_t host_to_network(arg_t host)
{
        return htobe64(host);
}

static inline arg_t network_to_host(arg_t net)
{
        return be64toh(net);
}

static arg_t *abort_ton(const unsigned char *payload, arg_t args[])
{
        struct packet_payload_abort *p = (struct packet_payload_abort *)payload;
        args[0] = host_to_network(p->error_number);
        return args;
}
static size_t abort_toh(const arg_t args[], unsigned char *payload)
{
        struct packet_payload_abort p;
        p.error_number = (int32_t)network_to_host(args[0]);
        memcpy(payload, &p, sizeof(p));
        return sizeof(p);
}

static arg_t *settings_ton(const unsigned char *payload, arg_t args[])
{
        struct packet_payload_settings *p
                = (struct packet_payload_settings *)payload;
        args[0] = host_to_network(p->fs_block_size);
        return args;
}
static size_t settings_toh(const arg_t args[], unsigned char *payload)
{
        struct packet_payload_settings p;
        p.fs_block_size = (uint64_t)network_to_host(args[0]);
        memcpy(payload, &p, sizeof(p));
        return sizeof(p);
}

static arg_t *set_perm_modes_ton(const unsigned char *payload, arg_t args[])
{
        struct packet_payload_set_perm_modes *p
                = (struct packet_payload_set_perm_modes *)payload;
        args[0] = host_to_network(p->mode);
        return args;
}
static size_t set_perm_modes_toh(const arg_t args[], unsigned char *payload)
{
        struct packet_payload_set_perm_modes p;
        p.mode = (mode_t)network_to_host(args[0]);
        memcpy(payload, &p, sizeof(p));
        return sizeof(p);
}

static arg_t *set_owner_ton(const unsigned char *payload, arg_t args[])
{
        struct packet_payload_set_owner *p
                = (struct packet_payload_set_owner *)payload;
        args[0] = host_to_network(p->uid);
        args[1] = host_to_network(p->gid);
        return args;
}
static size_t set_owner_toh(const arg_t args[], unsigned char *payload)
{
        struct packet_payload_set_owner p;
        p.uid = (uid_t)network_to_host(args[0]);
        p.gid = (gid_t)network_to_host(args[1]);
        memcpy(payload, &p, sizeof(p));
        return sizeof(p);
}

static arg_t *set_timestamps_ton(const unsigned char *payload, arg_t args[])
{
        struct packet_payload_timestamps *p
                = (struct packet_payload_timestamps *)payload;
        args[0] = host_to_network(p->atim.tv_sec);
        args[1] = host_to_network(p->atim.tv_nsec);
        args[2] = host_to_network(p->mtim.tv_sec);
        args[3] = host_to_network(p->mtim.tv_nsec);
        return args;
}
static size_t set_timestamps_toh(const arg_t args[], unsigned char *payload)
{
        struct packet_payload_timestamps p;
        p.atim.tv_sec = network_to_host(args[0]);
        p.atim.tv_nsec = network_to_host(args[1]);
        p.mtim.tv_sec = network_to_host(args[2]);
        p.mtim.tv_nsec = network_to_host(args[3]);
        memcpy(payload, &p, sizeof(p));
        return sizeof(p);
}

static const struct conv_data {
        enum packet_msg_code code;
        int arg_count;
        arg_t *(*to_network)(const unsigned char *, uint64_t *);
        size_t (*to_host)(const uint64_t *, unsigned char *);
} PAYLOAD_CONV[] = {
        {
                .code = MSG_ABORT,
                .arg_count = 1,
                .to_network = abort_ton,
                .to_host = abort_toh,
        },
        {
                .code = MSG_SETTINGS,
                .arg_count = 1,
                .to_network = settings_ton,
                .to_host = settings_toh,
        },
        {
                .code = MSG_SET_PERM_MODES,
                .arg_count = 1,
                .to_network = set_perm_modes_ton,
                .to_host = set_perm_modes_toh,
        },
        {
                .code = MSG_SET_OWNER,
                .arg_count = 2,
                .to_network = set_owner_ton,
                .to_host = set_owner_toh,
        },
        {
                .code = MSG_SET_TIMESTAMPS,
                .arg_count = 4,
                .to_network = set_timestamps_ton,
                .to_host = set_timestamps_toh,
        },
};

const struct conv_data *find_conversion_data(enum packet_msg_code code)
{
        for (const struct conv_data *e = &PAYLOAD_CONV[0]; e->code < MSG_COUNT;
             ++e) {
                if (e->code == code)
                        return e;
        }
        return NULL;
}
/* Communication */

bool packet_net_read_code(int fd, enum packet_msg_code *code_ptr)
{
        code_t code;
        if (utils_read(fd, sizeof(code), &code) != sizeof(code))
                return false;
        *code_ptr = code;
        return true;
}

bool packet_net_read_size(int fd, size_t *size_ptr)
{
        payload_size_t size;
        if (utils_read(fd, sizeof(size), &size) != sizeof(size))
                return false;

        *size_ptr = (size_t)be64toh(size);
        return true;
}

bool packet_net_read_payload(int fd, struct packet *packet)
{
        if (packet->payload_size == 0)
                return true;
        if (utils_read(fd, packet->payload_size, packet->payload)
            != (ssize_t)packet->payload_size)
                return false;

        const struct conv_data *conv = find_conversion_data(packet->code);
        if (conv != NULL)
                packet->payload_size = conv->to_host(
                        (const arg_t *)packet->payload, packet->payload);

        return true;
}

static bool packet_net_send_code(int fd, enum packet_msg_code code)
{
        code_t c = (code_t)code;
        return utils_write(fd, sizeof(c), &c) == sizeof(c);
}

static bool packet_net_send_size(int fd, size_t size)
{
        payload_size_t s = size;
        s = htobe64(s);
        return utils_write(fd, sizeof(s), &s) == sizeof(s);
}

bool packet_net_send(int fd, enum packet_msg_code code, size_t payload_size,
                     const unsigned char payload[payload_size])
{
        if (!packet_net_send_code(fd, code))
                return false;
        if (payload_size == 0)
                return packet_net_send_size(fd, payload_size);

        const struct conv_data *conv = find_conversion_data(code);

        if (conv != NULL && conv->arg_count > 0) {
                payload_size = conv->arg_count * sizeof(uint64_t);
                arg_t *args = alloca(payload_size);
                payload = (unsigned char *)conv->to_network(payload, args);
        }

        return packet_net_send_size(fd, payload_size)
               && utils_write(fd, payload_size, payload)
                          == (ssize_t)payload_size;
}
