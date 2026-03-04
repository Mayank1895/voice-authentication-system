import sounddevice as sd
import numpy as np
import os
from resemblyzer import VoiceEncoder

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(BASE_DIR, "data")
EMB_DIR = os.path.join(DATA_DIR, "embeddings")

SAMPLE_RATE = 16000
DURATION = 5

ENROLLMENT_PHRASES = [
    "start voice assistant",
    "master off",
    "shutdown",
    "insert faraday cup"
]


class EnrollmentEngine:

    def __init__(self, message_callback=None):

        self.message_callback = message_callback

        os.makedirs(DATA_DIR, exist_ok=True)
        os.makedirs(EMB_DIR, exist_ok=True)

        self.encoder = VoiceEncoder("cpu")

        self.log(f"Embedding directory: {EMB_DIR}")

    def log(self, message):
        if self.message_callback:
            self.message_callback(message)

    def normalize_audio(self, audio):
        max_val = np.max(np.abs(audio)) + 1e-6
        return audio / max_val

    def enroll(self, name):

        name = name.strip().lower()

        if not name:
            return {"status": "invalid_name"}

        embeddings = []

        try:

            for phrase in ENROLLMENT_PHRASES:

                self.log(f"Recording phrase: {phrase}")
                self.log(f"Recording for {DURATION} seconds")

                audio = sd.rec(
                    int(DURATION * SAMPLE_RATE),
                    samplerate=SAMPLE_RATE,
                    channels=1,
                    dtype="float32"
                )

                sd.wait()

                audio = audio.flatten()
                audio = self.normalize_audio(audio)

                self.log("Generating embedding")

                emb = self.encoder.embed_utterance(audio)

                embeddings.append(emb)

        except Exception as e:

            self.log(f"Recording error: {str(e)}")

            return {"status": "recording_failed"}

        if not embeddings:
            return {"status": "no_embeddings"}

        try:

            avg_embedding = np.mean(embeddings, axis=0)

            filename = f"{name}.npy"
            save_path = os.path.join(EMB_DIR, filename)

            self.log(f"Saving embedding to {save_path}")

            np.save(save_path, avg_embedding)

        except Exception as e:

            self.log(f"Embedding save error: {str(e)}")

            return {"status": "embedding_save_failed"}

        return {
            "status": "embedding_created",
            "embedding_file": filename
        }