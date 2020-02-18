<div align="center">
    <img src="misc/antman-logo-with-text.png?raw=true?" alt="antman-logo" width="250">
    <h3><a>Automated Nanopore Tracking, Modification + Analysis daemoN</a></h3>
    <hr/>
    <a href="https://travis-ci.org/will-rowe/antman"><img src="https://travis-ci.org/will-rowe/antman.svg?branch=master" alt="travis"></a>
    <a href="https://antman.readthedocs.io/en/latest/?badge=latest"><img src="https://readthedocs.org/projects/antman/badge/?version=latest" alt="Documentation Status" /></a>
    <a href=""><img src="https://img.shields.io/badge/status-WIP-yellow" alt="project status" /></a>
    <hr/>
</div>

## What is it

At the moment, it's a work in progress...

ANTMAN is a system daemon that will collect nanopore reads as they are basecalled, check them against a whitelist and then do some more stuff (e.g. select and run specific Nextflow pipelines, update a LIMS, index reads).

Currently, the daemon will:

- watch a directory
- detect new FASTQ files and add to a processing queue
- sketch the reads (using KMV MinHash)
- run a containment search against a reference sequence

## Quickstart

```bash
antman -w /path/to/some/dir
antman start
antman stop
```

> ANTMAN is being actively worked on and more documentation is being added

NOTES:

- can only set k-mer, bloom setting etc. during sketch command
  - otherwise can get misamtches between dbs

TODO:

- a lot
  - priorities:
    - set up an efficient way to translate bigsi k-mer queries
- set up threads properly
  - sketch command needs threads
  - set up the default proc number intelligently
- update run-antman-tests.py to include test data

now working
daemon is picking up fastq files and is controlled via CLI nicely
daemon will create a new job for each file and send it to the threadpool, along with the function to run

now need to have a set of functions to run on fastq files. Idea is to combine / sort out the sequence.h and sketch.h files
the solution needs to be a function to read a fastq file, process each sequence, get the k-mers and add to bloom filter


the bigsi branch is nearly working - just need to sort out the DB use across threads. Try setting up the bdb environment properly
