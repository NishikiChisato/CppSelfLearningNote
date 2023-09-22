# B+ Tree Index

- [B+ Tree Index](#b-tree-index)
  - [Storage Models \& Compression](#storage-models--compression)
    - [Database Workload](#database-workload)
    - [Database Storage Models](#database-storage-models)
    - [Compression](#compression)
      - [Run-Length Compression](#run-length-compression)
      - [Bit-Packing Compression](#bit-packing-compression)
      - [Mostly Compression](#mostly-compression)
      - [Bitmap Compression](#bitmap-compression)
      - [Delta Compression](#delta-compression)
      - [Incremental Compression](#incremental-compression)
      - [Dirtionary Compression](#dirtionary-compression)
  - [Memory Management](#memory-management)
    - [Buffer Pool Manager](#buffer-pool-manager)
    - [Buffer Pool Optimizations](#buffer-pool-optimizations)
      - [Multiple Buffer Pools](#multiple-buffer-pools)
      - [Pre-fetching](#pre-fetching)
      - [Scan sharing](#scan-sharing)
      - [Page Cache](#page-cache)
    - [Buffer Replacement Policy](#buffer-replacement-policy)
      - [Laest Recently Used](#laest-recently-used)
      - [Clock](#clock)
      - [LRU-K](#lru-k)

## Storage Models & Compression

### Database Workload

* On-line Transaction Processing: Simple queries that only read/modify a small part of database, sometimes alse needing to read a entire tuple from database
* On-line Analytical Processing: Complex queries that read/modify a large portion of the database, usually needing to scan multiple tuple
* Hybird Transaction + Analytical Processing: The combination of OLTP and OLAP

![OLTP_OLAP](./img/OLTP_OLAP.png)

### Database Storage Models

DBMS store tuple in different ways that is batter for eithor OLTP or OLAP. The basic storage models are as follow:

* row storage(n-ary storage model, NSM): Simply store tuple with the entire attribute one after the other, which is ideal for OLTP.
* colume storage(decomposition storage model, DSM): DBMS contiguously store a single attribute(column) for all tuple, which is ideal for OLAP.

![NSM](./img/NSM.png)

Since OLTP tends to read entry tuple and do a lot of inserts, NSM is ideal. On the other hand, let's say we want to extract some attribute from multiple tuple to analysis, NSM would cause expensive overhead.

![NSM_Downside](./img/NSM_Downside.png)

So in this scenario, DBMS must use DSM to store tuple.

![DSM1](./img/DMS1.png)

![DSM2](./img/DSM2.png)

Therefore, we can extract some useful attribute of the tuple instead read all attribute of the tuple.

![DSM3](./img/DSM3.png)

### Compression

As an example of musql innoDB compression, which shown in the following figure.

![Compression1](./img/Compression1.png)

Mysql default page size is 16KB, so we compress it and round to the power of 2. In the front of the compressed page , called mod log,  is used to store update log. Let's say we want to update some tuple in compressed page, instead of decompressing it, we can add update log into mod log. When we want to evict certain compressed page, we would evict this page as well as mod log.

Let's say we want to read certain tuple without decompressing it first, this is ideal.

![Compression2](./img/Compression2.png)

As the shown of the firgre, when we want to specify where clause, in this example, is 'andy', if we can replace 'andy' to another something, in this scenario, we can select in compressed data without decompress it.

The rest of this chapter, we would introduce some compression algorithm.

#### Run-Length Compression

![Compression3](./img/Compression3.png)

We can compress data as a sequence of triplets, all of which contain the value of this triplet, the offset of this triplet and the number of value which triplet represents.

Further, if we can sort the original data before compression, we can get higher compression ratio.

![Compression4](./img/Compression4.png)

#### Bit-Packing Compression

![Compression5](./img/Compression5.png)

Bit-Packing is very intuitive, we can store the useful data instead of storing entire data.

#### Mostly Compression

![Compression6](./img/Compression6.png)

This is varient of the bit-packing compression. In original data, if exist a specail data, which is large than any other, we can replace this value with a specail marker. If we scan for this marker, we would find this value in another table.

#### Bitmap Compression

We can store a separate bitmap for special attribute, where its offset is corresponding to the tuple.

![Compression7](./img/Compression7.png)

It looks like this compression has high comression retio, but, to make matter worse, let's say we have 1e5 tuple, and we use this encoding for `int32`, the comparition as following:

* Decompression: $32\ bits\times 10^5$
* Compression: $INT32_{MAX}\ bits\times 10^5$

#### Delta Compression

For time data or another with same attribute, for each value, we can store its difference with the first value, as the following figure show:

![Compression8](./img/Compression8.png)

#### Incremental Compression

![Compression9](./img/Compression9.png)

In common prefix table, the first entry is null, for every another enrties, we consider what frefix portion of the front string I have. As an example, `robbed` has the prefix `rob` in the front string `rob`, `robbing` has the frefix `robb` in the front string `robbed`, the rest of string follow this patten.

#### Dirtionary Compression

![Compression10](./img/Compression10.png)

This compression method is suitable for our ideal compression, we can read compressed data without compress it. As an example of the follow figure show, we replace 'andy' with integer number `30`

![Compression11](./img/Compression11.png)

This compression is also supprot range query, as the shown of following figure:

![Compression12](./img/Compression12.png)

---

## Memory Management

*Storage model and Compression* answer the question about how DBMS represent database **in file** on disk, and *memory management* answer the question about how DBMS move and memory data front and back from disk.

![MemoryManagement1](./img/MemoryManagement1.png)

Initially, buffer pool is empty. If the above application, as the figure shown, is *execution engine*, wants to fetch `page 2`. Buffer pool first need to read *directory page* from disk, then read the required page.

### Buffer Pool Manager

![BufferPool1](./img/BufferPool1.png)

Page table maps `page id` to `frame id` as well as tracks each `page id` corresponding the `frame id`(maybe this `page` would not store in physical memory).

![BufferPool2](./img/BufferPool2.png)

Because of the page table is shared resource, so we need to use `latch` to protect it from accessing other thread when certain thread is accessing.

> In database, `lock` used to pretect the content of database(e.g. tuple, table), `latch` used to pretect the interal data structure(e.g. hashtable, regions of memory).

At this points, we need to clarify two concepts: `page table` and `page directory`.

* `page table`: used to map `page id` to `frame id`. It would be stored in memory without storing in disk
* `page directory`: used to track the location of every pages **in database file**. It must be record in disk, which allows DBMS can find certain page when restarts

### Buffer Pool Optimizations

#### Multiple Buffer Pools

DBMS can maintain mulitple buffer pools in each scenario: per-database buffer pool and per-page type buffer pool(page size vary from `1K` to `16K`).

Partitioning memory across multiple pool can reduse latch contention and improve locality. We have two approach to implement it. 

Every record ID has extending attribute, called *object ID*, and a mapping from object id to specific buffer pool can be maintainded. Another approach is *hashing*, which hashs page id to select a buffer pool.

![MultipleBufferPool1](./img/MultipleBufferPool1.png)

![MultipleBufferPool2](./img/MultipleBufferPool2.png)

#### Pre-fetching

![Prefetching1](./img/Prefetching1.png)

At first, BPM would fetch `page 0`, then when the query cursor points to `page 1`, BPM would fetch `page 1`. The query cursor may continue downword, so the BPM would prefetching `page 2` and `page 3`, as the following shown:

![Prefetching2](./img/Prefetching2.png)

Therefore, when query cursor move to `page 2`, BPM would not fetch this page

#### Scan sharing

![ScanSharing1](./img/ScanShared1.png)

When `Q1` cursor points to `page 3`, the content of buffer pool is `page 3, 1, 2` and `Q2` comes

![ScanSharing2](./img/ScanShared2.png)

In scan sharing, cursor of `Q2` would jump to cursor of `Q1` instead of scanning from the beginning. Then, `Q1` and `Q2` would scan downword together.

![ScanSharing3](./img/ScanShared3.png)

![ScanSharing4](./img/ScanShared4.png)

When the part of query of `Q2` finished, the cursor of `Q2` would jump to the beginning and scan those page that have not scanned.

![ScanSharing5](./img/ScanShared5.png)

#### Page Cache

![PageCache](./img/PageCache.png)

DBMS can fetch certain page from `file system`, at the between of `file system` and `disk`, OS would support a cache for those page, which can reduce the I/O form disk. But, because of redundent copys of page, different eviction policies, DBMS can bypass this cache layer by using `O_DIRECT`. 

### Buffer Replacement Policy

#### Laest Recently Used

LRU would maintain a timestamp of each access page, and it would evict the oldest timestamp.

#### Clock

The Clock policy is approximation of LRU without timestamp. This policy would organize page as circle buffer with 'clock hand'. Upon swapping, clock hand would chick the `page bit` is set to '1'. If this bit is '1', then set to '0'; otherwise, evict this page. The detail process is as follow:

![CLOCK](./img/CLOCK.png)

If the ref bit is zero, which suggest that this page has not been accessed before clock hand pointed to it this time.

#### LRU-K

The porblem of this two approachs is that, the cacahe would susceptible for *sequential flood*. Let's say a scenario, BPM has record thress pages, then a flood of page fetched by using BPM, those flood page would **only read once**. After the flood, we want to read the three pages from last time, but now, those three pages no longer in BPM. The sequential flood would pollute the cache in BPM.

The varient of LRU policy, called LRU-K, can handle this situation. LRU-K would track the number of access time is greater and equal than K, for those access times less than K, they would be evicted firstly.


