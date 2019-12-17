## The easy way

This will be with conda and brew, which will happen once there is a release.

I'll also look at making a static binary but this might be tricky due to how we are currently using `fswatch`.

## The less easy way

You can compile from source, once you have taken care of the dependencies.

1. Get the dependencies

Currently there is just [libfswatch](https://github.com/emcrisostomo/fswatch). It's easiest to get this with a package manager:

```bash
brew install fswatch
```

Alternatively, you can follow the instructions at the [fswatch repo](https://github.com/emcrisostomo/fswatch) or use the install script from the **ANTMAN** repo:

```bash
./scripts/install-fswatch.sh
```

2. Compile ANTMAN

Just run the usual C autotools steps, making sure to point out where libfswatch is:

```bash
./configure CFLAGS="-I/usr/local/include" LDFLAGS="-L/usr/local/lib"
make
make check
make install
```

3. Run some more tests

**ANTMAN** has some unit tests, which are run in the previous step (`make check`). There are also some system tests which check that **ANTMAN** installed correctly:

```bash
./run-antman-tests.py
```