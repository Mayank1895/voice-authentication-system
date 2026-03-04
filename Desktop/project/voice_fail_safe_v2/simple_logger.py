from datetime import datetime
import sys


def log(message):
    sys.stdout.write(f"LOG::{message}\n")
    sys.stdout.flush()


def audit(message):
    sys.stdout.write(f"AUDIT::{message}\n")
    sys.stdout.flush()


def response(data):
    import json
    sys.stdout.write("RESP::" + json.dumps(data) + "\n")
    sys.stdout.flush()