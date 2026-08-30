#!/bin/bash

# https://ui.perfetto.dev/#!/record/target
if [[ ! -f "./tracebox" ]]; then
	curl -LO get.perfetto.dev/tracebox
	chmod +x ./tracebox
fi

adb start-server
./tracebox websocket_bridge
