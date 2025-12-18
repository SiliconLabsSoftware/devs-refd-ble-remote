import json
import os

_REMOTE_CHARACTERISTICS = {
    "AudioData": "00ce7a72-ec08-473d-943e-81ec27fdc5f2",
    "SampleRate": "00ce7a72-ec08-473d-943e-81ec27fdc601",
    "FilterEnable": "00ce7a72-ec08-473d-943e-81ec27fdc602",
    "EncodingEnable": "00ce7a72-ec08-473d-943e-81ec27fdc603",
    "TransferStatus": "00ce7a72-ec08-473d-943e-81ec27fdc604",
    "AudioChannels": "00ce7a72-ec08-473d-943e-81ec27fdc605",
    "KeyStatus": "faf57611-a4ab-463c-bea5-afe486466760",
    "KeyConfig": "438395b9-c5da-43b5-b886-2911de6e9a93",
    "Battery": "5ae3ff03-96e6-4ccf-a8c6-499a7cccfa7b",
    "BatteryMeasurementTime": "7e4f9ee9-5f89-4290-a9b4-3f6f28c30fab",
    "VoiceRecord": "47c24ddf-e65d-4406-942b-186111e46c35",
    "UseLeftMic": "4da595b4-ef2b-452f-87b8-26ec86f6a38a",
    "MicPdmDelay": "cc882a6f-fd2c-4b4b-a2ab-4be46c79db2c",
    "LedTx": "08e58602-d9e1-425b-be87-5949e753a3a3",
    "LedBacklight": "b10700cf-9f63-4db3-9d22-a767b9ec3831",
    "ForceKey": "a788971a-0ef3-449e-a10b-e58f9f4eae72",
    "Accelerometer": "9a964383-63e8-47bb-a32c-8590d1685ae4",
    "AccelerometerThreshold": "e8dc605b-6518-4cf7-a1d0-60f0e54f6e6a"
}

_AUDIO_FILE_PATH = "recording.wav"

_AI_PROMPT = """You are part of a toolchain.
1. Your main input is the attached audio file.
2. First, transcribe the audio.
- If transcription fails, output exactly: Transcription failed try again please.
3. If transcription succeeds, treat the transcribed text as the actual prompt and respond to it.
4. Output must be in the following format:

Transcribed command:
<the transcription here>

Response:
<the response here>

5. Do not include any additional text, notes, or formatting outside this structure.
6. The output must be plain text, ready for direct insertion into a Python Tkinter popup window."""

class Config:
    """Manages application configuration loaded from a JSON file. Supports nested keys."""

    def __init__(self, config_file="config.json"):
        self.config_file = os.path.join(os.path.dirname(__file__), config_file)
        self._config = self._load_config()

    def _load_config(self):
        try:
            with open(self.config_file, 'r') as file:
                config = json.load(file)
        except FileNotFoundError:
            raise FileNotFoundError(f"Configuration file '{self.config_file}' not found.")
        except json.JSONDecodeError as e:
            raise ValueError(f"Error decoding JSON from '{self.config_file}': {e}")

        config['remote_characteristics'] = _REMOTE_CHARACTERISTICS
        config['audio_file_path'] = _AUDIO_FILE_PATH
        config['ai_prompt'] = _AI_PROMPT
        return config

    def get(self, key, default=None):
        return self._config.get(key, default)
