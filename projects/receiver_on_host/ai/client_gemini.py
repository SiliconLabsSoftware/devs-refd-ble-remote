import os
from google import genai
from .client import AiClient
from config import Config

DEFAULT_MODEL='gemini-2.5-flash'

class GeminiClient(AiClient):
    def __init__(self, api_key: str = None, model: str = None, config: Config = None):
        if config is None:
            try:
                config = Config()
            except Exception:
                config = None

        if api_key is None and config:
            config_api_key = config.get('ai_api_key')
            if config_api_key and config_api_key.strip():
                api_key = config_api_key

        if api_key is None:
            api_key = os.getenv('GEMINI_API_KEY')

        if model is None and config:
            config_model = config.get('ai_model_name')
            if config_model and config_model.strip():
                model = config_model

        if model is None:
            model = DEFAULT_MODEL

        if api_key is None:
            raise ValueError("No API key found. Please provide via config file, environment variable (GEMINI_API_KEY), or parameter.")

        self._client = genai.Client(api_key=api_key)
        super().__init__(api_key, model)

    def send_text(self, prompt: str) -> str:
        return self._client.models.generate_content(model=self._model, contents=[prompt]).text

    def send_file(self, prompt: str, file_path: str) -> str:
        file = self._client.files.upload(file=file_path)
        return self._client.models.generate_content(model=self._model, contents=[prompt, file]).text
