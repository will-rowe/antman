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
antman -setWatchDir /path/to/some/dir
antman -start
antman -stop
```

> ANTMAN is being actively worked on and more documentation is being added
