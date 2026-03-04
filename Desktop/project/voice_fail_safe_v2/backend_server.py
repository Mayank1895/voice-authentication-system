import sys
import json

from voice_controller import VoiceSystemController
from simple_logger import log, response, audit


class BackendServer:

    def __init__(self):

        self.controller = VoiceSystemController(console_callback=log, audit_callback=audit)

        response({"status": "backend_ready"})

    def handle_request(self, request):

        action = request.get("action")

        try:

            if action == "authenticate":
                result = self.controller.authenticate()

            elif action == "start_session":
                result = self.controller.start_session()

            elif action == "process_command":
                result = self.controller.process_command()

            elif action == "end_session":
                result = self.controller.end_session()

            elif action == "enroll":
                result = self.controller.enroll(request.get("name"))

            elif action == "list_users":
                result = self.controller.list_users()

            elif action == "authorize_user":
                result = self.controller.authorize_user(request.get("username"))

            elif action == "unauthorize_user":
                result = self.controller.unauthorize_user(request.get("username"))

            elif action == "remove_user":
                result = self.controller.remove_user(request.get("username"))

            else:
                result = {"status": "unknown_action"}

        except Exception as e:
            result = {"status": "error", "message": str(e)}

        response(result)


def main():

    server = BackendServer()

    while True:

        line = sys.stdin.readline()

        if not line:
            break

        try:

            request = json.loads(line.strip())
            server.handle_request(request)

        except Exception as e:
            response({"status": "invalid_request", "message": str(e)})


if __name__ == "__main__":
    main()