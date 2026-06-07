import logging

import numpy as np
from numpy.typing import NDArray
from faster_whisper import WhisperModel

from server import config

log = logging.getLogger(__name__)

class Transcriber:
    def __init__(self) -> None:
        self._model: WhisperModel | None = None

    def ensure_loaded(self) -> None:
        if self._model is not None:
            return
        log.info("Loading Whisper model...")
        self._model = WhisperModel(
            config.WHISPER_MODEL,
            device=config.WHISPER_DEVICE,
            compute_type=config.WHISPER_COMPUTE_TYPE,
        )

    def transcribe(self, audio: NDArray[np.float32]) -> str:
        self.ensure_loaded()

        volume = float(np.max(np.abs(audio)))
        if volume < config.VOLUME_MIN_THRESHOLD:
            return ""

        audio_norm = audio / volume if volume > 0 else audio

        segments, _ = self._model.transcribe(
            audio_norm,
            beam_size=5,
            language="en",
            vad_filter=True,
            vad_parameters=dict(
                min_silence_duration_ms=config.VAD_MIN_SILENCE_MS,
                threshold=config.VAD_THRESHOLD,
                min_speech_duration_ms=config.VAD_MIN_SPEECH_MS,
            ),
            no_speech_threshold=config.NO_SPEECH_THRESHOLD,
            condition_on_previous_text=False,
        )

        text = ""
        for segment in segments:
            if segment.no_speech_prob < config.NO_SPEECH_PROB_THRESHOLD and segment.text.strip():
                text += segment.text + " "

        return text.strip()