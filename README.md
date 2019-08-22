# A.N.T.M.A.N

Automated Nanopore Tracking, Modification + Analysis daemoN

> very much a WIP (incl. the backronym)

## What is it

At the moment, it doesn't do much...

It will run in the background and register any new FASTQ files that appear in a given directory (and sub directories).

FASTQ files are checked and added to a processing queue. You can start / stop antman and tell it where to watch.

Next up, it will query/update a LIMS and kick of specific workflows.

## Install

```bash
make clean
make test
make
```

## Commands

```bash
antman -start
antman -stop
antman -setWatchDir /path/to/some/dir
```
