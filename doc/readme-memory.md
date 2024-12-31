VKK Memory
==========

The following documentation describes the VKK memory
implementation using the Vulkan API. These details are
internal to the VKK implementation, however, they may be
useful for apps to understand how memory is allocated on
their behalf.

Memory may be allocated on the users behalf in system
memory, device memory or transient memory.

* System: System RAM
* Device: Graphics RAM
* Transient: Graphics Cache (optional)

Transient memory is commonly found on moble devices and may
be used to optimize some operations. For example, the depth
/MSAA buffers are typically not required after performing a
rendering operation and can be discarded. Mobile GPUs
typically render the screen in batches of small tiles using
small high performance memory caches. The transient memory
type allows the depth/MSAA buffers to be discarded after
rendering which eliminates a relatively expensive copy from
cached memory back to Graphics RAM.

User Allocation Types

* Buffers
  * Uniform (system memory)
  * Vertex (system memory)
  * Index (system memory)
  * Storage (device memory)
* Images (device memory on Linux only)

Internal Allocation Types

* Buffers
  * Xfer (system memory)
* Images
  * Depth (transient or device memory)
  * MSAA (device memory)

Default framebuffer images are allocated externally and are
not tracked by the memory manager.

The following sections describe the VKK memory
implementation.

* Memory Manager
* Memory Pool
* Memory Chunk
* Memory

For additional information, see the memory.dot diagram
(using xdot) which clarifies how the underlying Vulkan
functions are called and how synchronization is performed.

Memory Manager
--------------

The memory manager is an internal API responsible for
allocating, freeing, updating, and tracking memory (buffers
and images).

The memory manager is thread safe and optimized for
multithreaded performance.

Managed memory is not visible to the CC library memory
tracker. See the Tracking and Debugging section for more
details.

Memory Pool
-----------

A memory pool is used by the memory manager to perform one
or more allocations of a specific type and size.

A memory pool consists of one or more memory chunks which
are used to peform the actual allocation.

The memory pools must be locked when allocating or freeing
memory.

Memory Chunk
------------

A memory chunk is used by a memory pool to encapsulate a
Vulkan memory allocation and supports one or more memory
object suballocations (slots).

Memory pools/chunks provide advantages over individual
memory allocations including:

* Reduced allocation overhead
* Reduced memory fragmentation

The memory chunk must be locked before performing updates to
a suballocation (slot). Memory chunks are independent of all
other memory chunks, however, they share a set of mutex and
cond variables in the memory manager. An updater index is
assigned to reduce conflicts for this shared resource to
improve multithreaded performance. The optimal number of
updaters was determined experimentally.

Memory
------

A memory object is a handle which references a chunk
suballocation (slot) using an offset.

Tracking and Debugging
----------------------

Memory allocated by the memory manager may be tracked by
using the vkk\_engine\_memoryInfo() function.

	void vkk_engine_memoryInfo(vkk_engine_t* self,
	                           int verbose,
	                           vkk_memoryType_e type,
	                           vkk_memoryInfo_t* _info);

The memory info parameters returned include:

* count\_chunks: Number of Vulkan memory allocations
* count\_slots: Number of suballocations
* size\_chunks: Size of Vulkan memory allocated
* size\_slots: Size of suballocations used

Here is an example of the verbose output.

	I/142674/vkk: vkk_memoryManager_memoryInfo@853 MEMINFO: type=any, count_chunks=40, count_slots=36269, size_chunks=417333248, size_slots=281179904
	I/142674/vkk: vkk_memoryManager_memoryInfo@864 MEMINFO: type=system, count_chunks=22, count_slots=35853, size_chunks=157286400, size_slots=92750592
	I/142674/vkk: vkk_memoryManager_memoryInfo@864 MEMINFO: type=device, count_chunks=18, count_slots=416, size_chunks=260046848, size_slots=188429312
	I/142674/vkk: vkk_memoryManager_memoryInfo@864 MEMINFO: type=transient, count_chunks=0, count_slots=0, size_chunks=0, size_slots=0
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=system, count=256, stride=8192, chunk_count=1, chunk_size=2097152
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=230, usage=0.9
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=system, count=1024, stride=2048, chunk_count=1, chunk_size=2097152
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=828, usage=0.8
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=device, count=256, stride=8192, chunk_count=1, chunk_size=2097152
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=1, usage=0.0
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=device, count=256, stride=16384, chunk_count=1, chunk_size=4194304
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=40, usage=0.2
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=system, count=2048, stride=1024, chunk_count=1, chunk_size=2097152
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=1, usage=0.0
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=system, count=64, stride=262144, chunk_count=1, chunk_size=16777216
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=3, usage=0.0
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=system, count=512, stride=4096, chunk_count=1, chunk_size=2097152
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=92, usage=0.2
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=device, count=2, stride=8388608, chunk_count=1, chunk_size=16777216
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=1, usage=0.5
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=device, count=2048, stride=1024, chunk_count=1, chunk_size=2097152
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=5, usage=0.0
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=device, count=32, stride=524288, chunk_count=7, chunk_size=117440512
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=32, usage=1.0
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=32, usage=1.0
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=32, usage=1.0
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=32, usage=1.0
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=32, usage=1.0
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=32, usage=1.0
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=3, usage=0.1
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=device, count=64, stride=262144, chunk_count=3, chunk_size=50331648
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=64, usage=1.0
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=64, usage=1.0
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=35, usage=0.5
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=system, count=256, stride=65536, chunk_count=1, chunk_size=16777216
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=2, usage=0.0
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=system, count=256, stride=16384, chunk_count=1, chunk_size=4194304
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=6, usage=0.0
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=device, count=128, stride=131072, chunk_count=1, chunk_size=16777216
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=4, usage=0.0
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=system, count=4096, stride=512, chunk_count=1, chunk_size=2097152
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=2617, usage=0.6
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=system, count=1, stride=16777216, chunk_count=1, chunk_size=16777216
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=1, usage=1.0
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=system, count=8192, stride=256, chunk_count=4, chunk_size=8388608
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=8192, usage=1.0
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=8192, usage=1.0
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=8192, usage=1.0
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=5613, usage=0.7
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=system, count=128, stride=131072, chunk_count=1, chunk_size=16777216
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=2, usage=0.0
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=system, count=256, stride=32768, chunk_count=8, chunk_size=67108864
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=256, usage=1.0
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=256, usage=1.0
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=256, usage=1.0
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=256, usage=1.0
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=256, usage=1.0
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=256, usage=1.0
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=256, usage=1.0
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=90, usage=0.4
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=device, count=256, stride=65536, chunk_count=1, chunk_size=16777216
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=5, usage=0.0
	I/142674/vkk: vkk_memoryPool_memoryInfo@209 POOL: type=device, count=1, stride=16777216, chunk_count=2, chunk_size=33554432
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=1, usage=1.0
	I/142674/vkk: vkk_memoryChunk_memoryInfo@234 CHUNK: usecount=1, usage=1.0
