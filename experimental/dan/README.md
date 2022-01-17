# WebSocket proof of concept

Instructions (tested on Linux only so far):

1. Install Datoviz
2. `source setup_env.sh`
3. `cd experimental/dan/`
4. Start the server: `python raster.py server`
5. In another terminal, start the client: `python raster.py client`
6. There should be two png images in the current directory.

Notes:

* The client should keep the websocket connection opened as long as the session is not over. When the `client()` function returns, the connection is closed and the server destroys all GPU objects.

Current limitations (work in progress):

* The window size is fixed (todo: implement board resizing request)
* The GPU buffer ("dat") size is fixed to a maximum size of 10M spikes (todo: implement dat resizing request)
