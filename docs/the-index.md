# The index

> WIP

## BIGSI

## Construction

The BIGSI index will be constructed from the white list multifasta file. Each sequence in the multifasta will be decomposed to k-mers; the k-mers from a sequence will be run through a new bloom filter which is then stored in a map (coupled with the fasta header for that sequence). Once all sequences are processed, all the bloom filter bit vectors will be inserted into the BIGSI data structure.

> make sure you have run `--setWhiteList` first so ANTMAN knows where the multifasta is - it will warn you if this isn't done

To run the index construction:

`antman --index`

## Current notes/ideas

`processRef` function will take the multifasta file and iterate over the sequences

these are then passed to the `sketchSequence` function, that collects k-mers and can then populate a bloom filter or KMV sketch

> parallelisation: try iterating the file and sending each seq to the worker pool, along with the sketchSequence function
> this would require downstream merging for BIGSI etc. to be thread safe

`processRef` will then collect bloom filter from each sequence and can add it to BIGSI

so:

- init a BIGSI
- iterate over a multifasta
- populate a bloom filter per sequence
- assign a colour to the bloom filter, record this and the sequence header/id
- add the bloom filter to the BIGSI
  - this needs a thread safe `insert` function?

current:

- init a BIGSI
- insert bloom filters
  - done via a map of seqID -> bloom filter
  - will assign a colour and create the lookup maps
  - end up with:
    - map seqID->bit vector
    - map seqID->colour(int)
    - array seqIDs (indexed by colour)
    - checks for duplicate seqIDs only
- once all bloom filters inserted, run the index function
  - this will take slices from all the bit vectors
  - store them as new maps?
  - can get rid of original bit vectors

```
need to check the insert code is keeping track of bv, seqID, colour relationships
need to write the index function
```

once this is done, we can chunk the multifasta:

- each chunk sent to a thread that will create a map of bloom filters; one per seq
- then do a thread safe add to the BIGSI, or wait on all threads
- then index the BIGSI

implementation notes:

- once inserted, you can get rid of the input bit vectors (BIGSI has copied them)
- once indexed, the BIGSI can't be added to

## To do

- add update functionality to index
- add de-noising of k-mers
- add proper parameterisation of index
- add serialisation
- try different indexing schemes (COBS, RAMBO etc.)
- improve index construction
  - move away from the multifasta idea?
  - split bloom filter work across threads
- add in minimizer option
- add bloom filter code to bigsi code as there is a lot of duplication
