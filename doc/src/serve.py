#!/usr/bin/env python3

# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

"Stupid simple HTTP server to host a site at a particular baseurl"

import argparse
import contextlib
import http.server
import mimetypes
import os.path
import shutil
import socket
import urllib.parse
from pathlib import Path


class RemappingHTTPRequestHandler(http.server.BaseHTTPRequestHandler):
    "Request handler that hosts the given directory at a particular path"

    protocol = "HTTP/1.0"

    def __init__(self, *args, directory: Path, base_path: str = "", **kwargs) -> None:
        self._directory = directory
        self._base_path = "/" + base_path.lstrip("/")
        super().__init__(*args, **kwargs)

    def do_HEAD(self) -> None:  # pylint: disable=invalid-name
        "Service a HEAD request"
        with self._do_work():
            pass

    def do_GET(self) -> None:  # pylint: disable=invalid-name
        "Service a GET request"
        with self._do_work() as f:
            if f is not None:
                shutil.copyfileobj(f, self.wfile)

    @contextlib.contextmanager
    def _do_work(self):
        "Common handler code for GET and HEAD requests"
        if not self.path.startswith(self._base_path):
            self.send_error(http.HTTPStatus.FORBIDDEN, "Outside hosted base path")
            yield None
            return
        raw_path = self.path.removeprefix(self._base_path).split("#")[0].split("?")[0]
        raw_path = os.path.normpath(urllib.parse.unquote(raw_path))
        path = self._directory / raw_path.lstrip("/")

        if path.is_dir():
            url = urllib.parse.urlsplit(self.path)
            if not url.path.endswith(("/", "%2f", "%2F")):
                # Redirect to the version ending in a slash
                self.send_response(http.HTTPStatus.MOVED_PERMANENTLY)
                new_url = url._replace(path=url.path + "/")
                self.send_header("Location", urllib.parse.urlunsplit(new_url))
                self.send_header("Content-Length", "0")
                self.end_headers()
                yield None
                return

            # Otherwise load the index.html, or fail
            path = path / "index.html"
            if not path.exists():
                self.send_error(
                    http.HTTPStatus.NOT_FOUND, "index.html not found in directory"
                )
                yield None
                return

        if not path.exists():
            self.send_error(http.HTTPStatus.NOT_FOUND, "File not found")
            yield None
            return

        with path.open("rb") as f:
            mtype, encoding = mimetypes.guess_type(path)
            stat = os.fstat(f.fileno())

            self.send_response(http.HTTPStatus.OK)
            self.send_header("Content-Type", mtype or "application/octet-stream")
            if encoding is not None:
                self.send_header("Content-Encoding", encoding)
            self.send_header("Content-Length", f"{stat.st_size:d}")
            self.end_headers()
            yield f


def main():
    "Main function"
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-p", "--port", type=int, default=8000, help="TCP port to serve on"
    )
    parser.add_argument("directory", type=Path)
    parser.add_argument("base_path")
    args = parser.parse_args()

    class Server(http.server.ThreadingHTTPServer):
        "Server wrapper to pass extra kwargs to the RequestHandler"

        def finish_request(self, request, client_address):
            self.RequestHandlerClass(
                request,
                client_address,
                self,
                directory=args.directory,
                base_path=args.base_path,
            )

    Server.address_family, _, _, _, addr = next(
        iter(
            socket.getaddrinfo(
                "localhost", args.port, type=socket.SOCK_STREAM, flags=socket.AI_PASSIVE
            )
        )
    )
    with Server(addr, RemappingHTTPRequestHandler) as httpd:
        _, port = httpd.socket.getsockname()[:2]
        print(
            f"Serving {args.directory} at http://localhost:{port}/{args.base_path.lstrip('/')} ..."
        )
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n^C received, stopping server")
            return


if __name__ == "__main__":
    main()
