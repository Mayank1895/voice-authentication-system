import os
import json


class UserManager:

    def __init__(self):

        base_dir = os.path.dirname(os.path.abspath(__file__))

        self.data_dir = os.path.join(base_dir, "data")
        self.user_db = os.path.join(self.data_dir, "users.json")

        os.makedirs(self.data_dir, exist_ok=True)

        if not os.path.exists(self.user_db):
            with open(self.user_db, "w") as f:
                json.dump({}, f, indent=4)

    def load_users(self):

        try:
            with open(self.user_db, "r") as f:
                return json.load(f)

        except Exception:
            return {}

    def save_users(self, users):

        with open(self.user_db, "w") as f:
            json.dump(users, f, indent=4)

    def add_user(self, username, embedding_file):

        users = self.load_users()

        if username in users:
            return False, "User already exists"

        if len(users) == 0:
            role = "admin"
            authorized = True
        else:
            role = "standard"
            authorized = False

        users[username] = {
            "voice_emb": embedding_file,
            "role": role,
            "authorized": authorized
        }

        self.save_users(users)

        return True, "User added"

    def list_users(self):
        return self.load_users()

    def authorize_user(self, username):

        users = self.load_users()

        if username not in users:
            return False, "User not found"

        users[username]["authorized"] = True

        self.save_users(users)

        return True, "User authorized"

    def unauthorize_user(self, username):

        users = self.load_users()

        if username not in users:
            return False, "User not found"

        users[username]["authorized"] = False

        self.save_users(users)

        return True, "User unauthorized"

    def remove_user(self, username):

        users = self.load_users()

        if username not in users:
            return False, "User not found"

        del users[username]

        self.save_users(users)

        return True, "User removed"