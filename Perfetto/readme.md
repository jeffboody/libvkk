Perfetto Usage
==============

Capture a trace
---------------

1. Run start-perfetto.sh
2. Launch https://ui.perfetto.dev/
3. Record new trace
4. Select Android, Graphics, ADB + WebSocket
5. Connect to your device
6. Start/Stop tracing

Analyze a trace
---------------

Copy important traces to a new workspace.

1. CPU Scheduling
2. GPU
3. App QueuePresentKHR thread and GPU Completion
4. SurfaceFlinger Composite thread
5. SurfaceFlinger Expected Timeline and Actual Timeline
6. SurfaceFlinger BufferTX for the app

Check the following events to verify frame pacing.

1. Check Actual Timeline for missed deadlines/janks
2. Check buffer number queued by QueuePresentKHR
3. Compare with buffer number latched by SurfaceFlinger

BufferTX increases when a buffer is queued and decreses
when a buffer is latched. It can take values from 0-2
depending on the Choreographer timeline selected and
application rendering characteristics. Typical values of
BufferTX for a one frame latency timeline are 0-1 while a
two frame latency timeline are 1-2. Trivial applications
such as Gears can cause these values to increase by 1.

Dropped frames may occur when SurfaceFlinger tries to latch
a buffer and the application has not queued the buffer by
the deadline or when the GPU cannot complete rendering
before the presentation deadline.
