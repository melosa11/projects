# Dropbox

Program for synchronizing two folders. Similar to `rsync`

## Usage
```
Usage: ./dropbox (-S|--server) [OPTIONS] TARGET
       ./dropbox (-C|--client) [OPTIONS] HOST SOURCE

DESCRIPTION:
If the program is run as client, then it will send files from
SOURCE to HOST. Additionally if -o isn't set, the client starts
watching changes in the SOURCE and synchronize them with the server.
If the program is run as server, then it will listen for the
client requests.

OPTIONS:
    -p,--port=N       The port at which the server is listening.

    -C,--client       Starts program as client.

    -o,--one-shot     Synchronizes the folders and ends.

    -S,--server       Starts program as server.

    -f,--force        Allows synchronization with non-empty TARGET folder.


    -h                Shows this message.

    -v,--verbose      Prints every file/object it works with.

    -n,--foreground   Runs in foreground.

    -d,--debug        Sets DEBUG log level.

    -q,--quiet        Sets WARNING log level.

    -s,--sparse       Saves files with holes effectively.
```
## Coding Style

See `CodingStyle.md`.

## How to compile?

```shell
$ make build
```

## Tests

[CUT - C Unit Tests][1] is used as testing framework.

```shell
$ make tests
```

[1]: https://github.com/spito/testing
