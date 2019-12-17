The `configuration file` is used by **ANTMAN** to keep track of the daemon, adjust settings and set the watch directory etc.

### The format

The configuration file is in JSON format:

```json
{
  "filename": "/tmp/.antman.config",
  "created": "2019-12-17:1420",
  "modified": "2019-12-17:1420",
  "current_log_file": "./antman-2019-12-17-1420.log",
  "watch_directory": "/var/lib/MinKNOW/data/reads",
  "pid": -1,
  "k_size": 7,
  "sketch_size": 128,
  "bloom_fp_rate": 0.000000,
  "bloom_max_elements": 100000
}
```

### How to change the location

The location of the configuration file must be set at compile time. The easiest way is to edit line 22 of `configure.ac`, then run:

```
./autogen.sh
./configure CFLAGS="-I/usr/local/include" LDFLAGS="-L/usr/local/lib"
make
make check
make install
```

If the default location is annoying, please let me know and I'll add the config file path as a CLI flag.