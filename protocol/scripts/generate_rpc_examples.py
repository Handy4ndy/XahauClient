#!/usr/bin/env python3
"""Deprecated entrypoint for live WSS generation.

Live WebSocket examples:  python protocol/scripts/generate_wss_examples.py  -> examples/wss/
Offline RPC examples:     hand-maintained under examples/rpc/

This script forwards to generate_wss_examples.py for backward compatibility.
"""
from pathlib import Path
import runpy

if __name__ == "__main__":
    runpy.run_path(str(Path(__file__).with_name("generate_wss_examples.py")), run_name="__main__")
