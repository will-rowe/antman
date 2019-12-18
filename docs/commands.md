ANTMAN uses a [configuration file](the-config.md) to store runtime parameters.

## Initialisation

Start by adding a white list:

```bash
antman --setWhiteList=misc/data/NiV_6_Malaysia.fasta
```

We can also update the watch directory:

```bash
antman --setWatchDir=/Users/willrowe/Desktop
```

Or change the log file:

```bash
antman --setLog=newlog.txt
```

## Start/stop the daemon

To start:

```bash
antman --start
```

Or you can provide all the previous commands in one:

```bash
antman --setWhiteList=misc/data/NiV_6_Malaysia.fasta --setWatchDir=/Users/willrowe/Desktop antman --setLog=newlog.txt --start
```

To stop:

```bash
antman --stop
```

## Notes


* The order you provide the flags doesn't matter. The commands will always follow a hierarchy: stop, config changes, start.
* Any config changes will implicitly first stop any running daemon before making changes. If this happens, the daemon will then be restarted (unless --stop was included in the command).