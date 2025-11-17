# Start a very simple web server that shows a web page which loads/runs the
# WASM file given as the first parameter
from http.server import HTTPServer, SimpleHTTPRequestHandler
import mimetypes


mimetypes.add_type("application/javascript", ".mjs")
mimetypes.add_type("application/wasm", ".wasm")


class NoCacheHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/":
            self.send_response(302)
            self.send_header("Location", "/index.html")
            self.end_headers()
            return

        return super().do_GET()

    def translate_path(self, path: str) -> str:
        if path == "/index.html":
            path = "/playground/playground.html"
        return super().translate_path(path)

    def end_headers(self):
        # Disable caching for files served locally
        self.send_header("Cache-Control", "no-store")
        self.send_header("Pragma", "no-cache")
        self.send_header("Expires", "0")
        super().end_headers()


if __name__ == "__main__":
    server = HTTPServer(("localhost", 8000), NoCacheHandler)
    print("Serving on http://localhost:8000")
    server.serve_forever()
