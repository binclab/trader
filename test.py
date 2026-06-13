#!/usr/bin/env python3

import os
import websocket
import ssl

def on_message(ws, message):
    print("Received:", message)

def on_error(ws, error):
    print("Error:", error)

def on_close(ws, close_status_code, close_msg):
    print("Closed:", close_status_code, close_msg)

def on_open(ws):
    print("Connection opened")
    ws.send('{"action":"exchange_code","code":"test","code_verifier":"test"}')

url = "wss://tazi.binclab.com/trader/oauth"

if os.geteuid() == 0:
    url = "wss://127.0.0.1:5000/trader/oauth"

ws = websocket.WebSocketApp(
    url,
    on_message=on_message,
    on_error=on_error,
    on_close=on_close
)
ws.run_forever(sslopt={"cert_reqs": ssl.CERT_NONE})
