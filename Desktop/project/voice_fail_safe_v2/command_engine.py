import sounddevice as sd
import torch
import re
import json
import os
import numpy as np
from transformers import WhisperProcessor, WhisperForConditionalGeneration
from difflib import SequenceMatcher
from resemblyzer import VoiceEncoder

try:
    from epics import caput, caget
    EPICS_AVAILABLE = True
except ImportError:
    EPICS_AVAILABLE = False


MODEL_ID = "openai/whisper-tiny"
SAMPLE_RATE = 16000
DURATION = 5
CONFIG_PATH = "data/command.json"
USER_DB = "data/users.json"
EMB_DIR = "data/embeddings"


class CommandEngine:

    def __init__(self, message_callback=None):

        self.message_callback = message_callback

        self.processor = WhisperProcessor.from_pretrained(MODEL_ID)
        self.model = WhisperForConditionalGeneration.from_pretrained(MODEL_ID)
        self.model.eval()

        self.encoder = VoiceEncoder("cpu")

        self.speaker_threshold = 0.65
        self.speaker_margin = 0.01

        try:
            with open(CONFIG_PATH, "r") as f:
                config = json.load(f)
        except Exception as e:
            # Fallback to defaults if the config file is missing or invalid
            print(f"Warning: Error loading {CONFIG_PATH}: {str(e)}")
            config = {}

        self.command_threshold = config.get("command_threshold", 0.68)
        self.commands = config.get("commands", [])

        self.current_user = None

        self.log("Command engine initialized.")

    # ---------------------------------------
    def log(self, message):
        if self.message_callback:
            self.message_callback(message)

    # ---------------------------------------
    def start_session(self, authenticated_user):

        self.current_user = authenticated_user
        self.log("Session started.")
        return {"status": "session_started"}

    # ---------------------------------------
    def end_session(self):

        self.current_user = None
        self.log("Session ended.")
        return {"status": "session_ended"}

    # ---------------------------------------
    def record_audio(self):

        self.log("Recording command...")
        audio = sd.rec(
            int(DURATION * SAMPLE_RATE),
            samplerate=SAMPLE_RATE,
            channels=1,
            dtype="float32"
        )
        sd.wait()

        return audio.flatten()

    # ---------------------------------------
    def cosine_similarity(self, a, b):
        denom = np.linalg.norm(a) * np.linalg.norm(b)
        if denom == 0:
            return 0.0
        return np.dot(a, b) / denom

    # ---------------------------------------
    def transcribe(self, audio):

        inputs = self.processor(
            audio,
            sampling_rate=SAMPLE_RATE,
            return_tensors="pt"
        )

        forced_decoder_ids = self.processor.get_decoder_prompt_ids(
            language="en",
            task="transcribe"
        )

        with torch.no_grad():
            predicted_ids = self.model.generate(
                inputs.input_features,
                forced_decoder_ids=forced_decoder_ids,
                do_sample=False,
                temperature=0.0
            )

        text = self.processor.batch_decode(
            predicted_ids,
            skip_special_tokens=True
        )[0]

        return text.lower().strip()
    # ---------------------------------------
    def clean_text(self, text):
        text = text.lower()
        text = re.sub(r'[^\w\s]', '', text)   # remove punctuation
        text = re.sub(r'\s+', ' ', text)     # collapse multiple spaces
        return text.strip()
    # ---------------------------------------
    def process_command(self):

        if not self.current_user:
            return {"status": "no_active_session"}

        # -----------------------------
        # Record
        # -----------------------------
        audio = self.record_audio()
        audio = audio / (np.max(np.abs(audio)) + 1e-6)

        # -----------------------------
        # Re-verify Speaker
        # -----------------------------
        try:
            input_embedding = self.encoder.embed_utterance(audio)
        except Exception as e:
            self.log(f"Embedding error: {str(e)}")
            return {"status": "embedding_error"}

        try:
            with open(USER_DB, "r") as f:
                users = json.load(f)
        except Exception:
            return {"status": "db_error"}

        scores = []

        for username, user_data in users.items():

            emb_path = os.path.join(EMB_DIR, user_data["voice_emb"])

            if not os.path.exists(emb_path):
                continue

            stored_embedding = np.load(emb_path)
            score = self.cosine_similarity(input_embedding, stored_embedding)
            scores.append((score, username))

        if not scores:
            self.log("No embeddings found.")
            return {"status": "no_embeddings"}

        scores.sort(key=lambda x: x[0], reverse=True)

        best_score, best_username = scores[0]
        second_score = scores[1][0] if len(scores) > 1 else 0.0

        self.log(f"Similarity: {best_score:.3f}")

        if best_username != self.current_user["name"]:
            self.log("Unauthorized speaker.")
            return {"status": "unauthorized_speaker"}

        if best_score < self.speaker_threshold:
            self.log("Speaker below threshold.")
            return {"status": "low_confidence"}

        if (best_score - second_score) < self.speaker_margin:
            self.log("Speaker ambiguous.")
            return {"status": "ambiguous_speaker"}

        # -----------------------------
        # Transcribe Command
        # -----------------------------
        command_text = self.clean_text(self.transcribe(audio))
        self.log(f"Command recognized: {command_text}")

        # -----------------------------
        # Match Command
        # -----------------------------
        for cmd in self.commands:
            cmd_text = self.clean_text(cmd["name"])
            score = SequenceMatcher(None, command_text, cmd_text).ratio()

            if score >= self.command_threshold:

                response = cmd.get("response", "Executing command")
                self.log(response)

                epics_info = cmd.get("epics", {})

                # No EPICS defined → logical command only
                if not epics_info.get("pv"):
                    return {
                        "status": "command_executed",
                        "command": cmd["name"],
                        "message": response
                    }

                # Simulation mode
                if not EPICS_AVAILABLE:
                    self.log("(SIMULATION MODE)")
                    return {
                        "status": "command_simulated",
                        "command": cmd["name"],
                        "message": response
                    }

                try:
                    initial_pv = caget(epics_info["pv"])
                    caput(epics_info["pv"], epics_info["value"])
                    final_pv = caget(epics_info["pv"])

                    self.log("Execution success.")
                    return {
                        "status": "command_executed",
                        "command": cmd["name"],
                        "pv_name": epics_info["pv"],
                        "pv_value": epics_info["value"],
                        "message": response
                    }

                except Exception as e:
                    self.log(f"EPICS ERROR: {str(e)}")
                    return {"status": "epics_error"}

        # If no command matched
        self.log("Command not recognized.")
        return {"status": "command_not_recognized"}