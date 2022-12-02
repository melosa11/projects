#include "set.h"

#include "operation.h"

#include "log.h"

#include <sys/stat.h>
#include <sys/time.h>

enum packet_msg_code
server_set_timestamps(const struct server_file_info *file_info)
{
        const struct timeval access_time
                = { file_info->atim.tv_sec, file_info->atim.tv_nsec / 1000 };
        const struct timeval modification_time
                = { file_info->mtim.tv_sec, file_info->mtim.tv_nsec / 1000 };

        const struct timeval times[2] = { access_time, modification_time };
        if (futimes(file_info->filefd, times) == -1) {
                log_warning("futimes");
                return MSG_NOK;
        }
        syslog(LOG_DEBUG, "timestamps set successfully.");
        return MSG_OK;
}

enum packet_msg_code
server_set_perm_modes(const struct server_file_info *file_info)
{
        if (fchmod(file_info->filefd, file_info->mode) == -1) {
                log_warning("fchmod");
                return MSG_NOK;
        }
        syslog(LOG_DEBUG, "permissions set successfully.");
        return MSG_OK;
}

enum packet_msg_code server_set_owner(const struct server_file_info *file_info)
{
        if (fchown(file_info->filefd, file_info->uid, file_info->gid) == -1) {
                log_warning("fchown");
                return MSG_NOK;
        }
        syslog(LOG_DEBUG, "owner uid and gid set successfully.");
        return MSG_OK;
}
