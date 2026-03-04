import os
import json
import torch
import numpy as np
import sounddevice as sd
from transformers import WhisperProcessor, WhisperForConditionalGeneration
from resemblyzer import VoiceEncoder
from difflib import SequenceMatcher


USER_DB = "data/users.json"
EMB_DIR = "data/embeddings"

MODEL_ID = "openai/whisper-tiny"

SAMPLE_RATE = 16000
DURATION = 5

THRESHOLD = 0.70
MARGIN = 0.04

ACTIVATION_SENTENCE = "start voice assistant"


class VerificationEngine:

    def __init__(self, message_callback=None):

        self.message_callback = message_callback

        self.processor = WhisperProcessor.from_pretrained(MODEL_ID)
        self.model = WhisperForConditionalGeneration.from_pretrained(MODEL_ID)
        self.model.eval()

        self.encoder = VoiceEncoder("cpu")

    # ---------------------------------------
    # Unified Logger (UI Safe)
    # ---------------------------------------
    def log(self, message):
        if self.message_callback:
            self.message_callback(message)

    # ---------------------------------------
    def trim_silence(self, audio, threshold=0.01):
        mask = np.abs(audio) > threshold
        if np.sum(mask) == 0:
            return audio
        start = np.argmax(mask)
        end = len(audio) - np.argmax(mask[::-1])
        return audio[start:end]

    # ---------------------------------------
    def normalize_audio(self, audio):
        audio = self.trim_silence(audio)
        max_val = np.max(np.abs(audio)) + 1e-6
        return audio / max_val

    # ---------------------------------------
    def cosine_similarity(self, a, b):
        denom = np.linalg.norm(a) * np.linalg.norm(b)
        if denom == 0:
            return 0.0
        return np.dot(a, b) / denom

    # ---------------------------------------
    def phrase_similarity(self, a, b):
        return SequenceMatcher(None, a.lower(), b.lower()).ratio()

    # ---------------------------------------
    def record_audio(self):

        self.log(f'Say: "{ACTIVATION_SENTENCE}"')
        self.log(f"Recording for {DURATION} seconds...")

        audio = sd.rec(
            int(DURATION * SAMPLE_RATE),
            samplerate=SAMPLE_RATE,
            channels=1,
            dtype="float32"
        )
        sd.wait()

        return audio.flatten()

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
    # MAIN AUTH METHOD (Single Attempt Only)
    # ---------------------------------------
    def authenticate_once(self):

        if not os.path.exists(USER_DB):
            self.log("No users enrolled.")
            return {"status": "no_users"}

        audio = self.record_audio()
        audio = self.normalize_audio(audio)

        transcript = self.transcribe(audio)
        self.log(f"Recognized text: {transcript}")

        # Activation phrase check
        if self.phrase_similarity(transcript, ACTIVATION_SENTENCE) < 0.75:
            self.log("Activation phrase mismatch.")
            return {"status": "activation_fail"}

        # Speaker embedding
        try:
            input_embedding = self.encoder.embed_utterance(audio)
        except Exception as e:
            self.log(f"Embedding error: {str(e)}")
            return {"status": "embedding_error"}

        # Load users
        try:
            with open(USER_DB, "r") as f:
                users = json.load(f)
        except Exception:
            self.log("User database error.")
            return {"status": "db_error"}

        scores = []

        for username, user_data in users.items():

            emb_path = os.path.join(EMB_DIR, user_data["voice_emb"])

            if not os.path.exists(emb_path):
                continue

            stored_embedding = np.load(emb_path)
            score = self.cosine_similarity(input_embedding, stored_embedding)
            scores.append((score, username, user_data))

        if not scores:
            self.log("No embeddings found.")
            return {"status": "no_embeddings"}

        scores.sort(key=lambda x: x[0], reverse=True)

        best_score, best_username, best_user = scores[0]
        second_score = scores[1][0] if len(scores) > 1 else 0.0

        self.log(f"Best match: {best_username} ({best_score:.3f})")

        if best_score < THRESHOLD:
            self.log("Speaker not recognized.")
            return {"status": "low_confidence"}

        if (best_score - second_score) < MARGIN:
            self.log("Speaker match ambiguous.")
            return {"status": "ambiguous"}

        # Authorization check
        if not best_user.get("authorized", False):
            self.log("User not authorized.")
            return {"status": "not_authorized"}

        self.log("Access granted.")

        return {
            "status": "auth_success",
            "user": {
                "name": best_username,
                "role": best_user.get("role"),
                "authorized": best_user.get("authorized")
            }
        }