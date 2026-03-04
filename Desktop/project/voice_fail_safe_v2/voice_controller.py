from verification_engine import VerificationEngine
from command_engine import CommandEngine
from enrollment_engine import EnrollmentEngine
from user_management import UserManager
from datetime import datetime

print("VOICE CONTROLLER FILE:", __file__)


class VoiceSystemController:

    def __init__(self, console_callback=None, audit_callback=None):

        self.console_callback = console_callback
        self.audit_callback = audit_callback

        self.verification_engine = VerificationEngine(
            message_callback=self.console_callback
        )

        self.command_engine = CommandEngine(
            message_callback=self.console_callback
        )

        self.enrollment_engine = EnrollmentEngine(
            message_callback=self.console_callback
        )

        self.user_manager = UserManager()

        self.current_user = None

    # ---------------------------------------
    # Console Logger
    # ---------------------------------------
    def log(self, message):
        if self.console_callback:
            self.console_callback(message)

    # ---------------------------------------
    # Audit Logger (SYSTEM LOG)
    # ---------------------------------------
    def audit_log(self, message):

        timestamp = datetime.now().strftime("%H:%M:%S")
        log_msg = f"[{timestamp}] {message}"

        # If Qt provided audit callback → send there
        if self.audit_callback:
            self.audit_callback(log_msg)

        # Otherwise fallback to console
        elif self.console_callback:
            self.console_callback(log_msg)

    # ---------------------------------------
    # Authentication
    # ---------------------------------------
    def authenticate(self):

        self.audit_log("Authentication attempt")

        result = self.verification_engine.authenticate_once()

        if result.get("status") == "auth_success":

            self.current_user = result["user"]

            self.audit_log(
                f"Authentication successful: {self.current_user['name']}"
            )

            self.log(
                f"Authenticated: {self.current_user['name']}"
            )

        return result

    # ---------------------------------------
    # Start Session
    # ---------------------------------------
    def start_session(self):

        if not self.current_user:
            return {"status": "no_authenticated_user"}

        self.audit_log(
            f"Session started for user: {self.current_user['name']}"
        )

        return self.command_engine.start_session(self.current_user)

    # ---------------------------------------
    # Process Command
    # ---------------------------------------
    def process_command(self):

        if not self.current_user:
            return {"status": "no_authenticated_user"}

        result = self.command_engine.process_command()

        username = self.current_user["name"]

        if result.get("status") == "command_executed":

            command = result.get("command", "unknown")

            self.audit_log(
                f"Command executed by {username}: {command}"
            )

            if result.get("pv_name") and result.get("pv_value") is not None:

                self.audit_log(
                    f"PV changed: {result['pv_name']} → {result['pv_value']}"
                )

        elif result.get("status") == "command_not_recognized":

            self.audit_log(
                f"Unknown command attempt by {username}"
            )

        return result

    # ---------------------------------------
    # End Session
    # ---------------------------------------
    def end_session(self):

        username = None

        if self.current_user:
            username = self.current_user["name"]

        self.command_engine.end_session()

        if username:
            self.audit_log(
                f"Session ended for user: {username}"
            )

        self.current_user = None

        return {"status": "session_ended"}

    # ---------------------------------------
    # Enrollment
    # ---------------------------------------
    def enroll(self, name):

        name = name.strip().lower()

        if not name:
            return {"status": "invalid_name"}

        users = self.user_manager.load_users()

        if len(users) != 0 and not self._is_admin():
            return {"status": "permission_denied"}

        result = self.enrollment_engine.enroll(name)

        if result.get("status") != "embedding_created":
            return result

        embedding_file = result["embedding_file"]

        success, message = self.user_manager.add_user(name, embedding_file)

        if not success:
            return {"status": "error", "message": message}

        self.log(f"User enrolled successfully: {name}")

        self.audit_log(
            f"User enrolled successfully: {name}"
        )

        return {"status": "enrollment_success"}

    # ---------------------------------------
    # User Management
    # ---------------------------------------
    def list_users(self):

        if not self._is_admin():
            return {"status": "permission_denied"}

        users = self.user_manager.list_users()

        return {
            "status": "success",
            "users": users
        }

    def authorize_user(self, username):

        if not self._is_admin():
            return {"status": "permission_denied"}

        success, message = self.user_manager.authorize_user(username)

        if success:
            return {"status": "user_authorized"}

        return {"status": "error", "message": message}

    def unauthorize_user(self, username):

        if not self._is_admin():
            return {"status": "permission_denied"}

        success, message = self.user_manager.unauthorize_user(username)

        if success:
            return {"status": "user_unauthorized"}

        return {"status": "error", "message": message}

    def remove_user(self, username):

        if not self._is_admin():
            return {"status": "permission_denied"}

        success, message = self.user_manager.remove_user(username)

        if success:
            return {"status": "user_removed"}

        return {"status": "error", "message": message}

    # ---------------------------------------
    # Role Check
    # ---------------------------------------
    def _is_admin(self):

        if not self.current_user:
            return False

        return self.current_user.get("role") == "admin"