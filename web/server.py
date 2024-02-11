#!/usr/bin/env python3
from http.server import HTTPServer, SimpleHTTPRequestHandler, test


class CORSRequestHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory="public", **kwargs)

    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        SimpleHTTPRequestHandler.end_headers(self)


if __name__ == "__main__":
    test(CORSRequestHandler, HTTPServer, port=8000)
