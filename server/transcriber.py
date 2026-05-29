import numpy as np
from faster_whisper import WhisperModel

from server import config


class Transcriber:
    def __init__(self):
        self._model: WhisperModel | None = None

    def ensure_loaded(self):
        if self._model is not None:
            return
        import logging
        log = logging.getLogger(__name__)
        log.info("Cargando modelo Whisper...")
        self._model = WhisperModel(
            config.WHISPER_MODEL,
            device=config.WHISPER_DEVICE,
            compute_type=config.WHISPER_COMPUTE_TYPE,
        )

    def transcribe(self, audio: np.ndarray) -> str:
        self.ensure_loaded()
        import logging
        log = logging.getLogger(__name__)

        volumen = np.max(np.abs(audio))
        if volumen < config.VOLUME_MIN_THRESHOLD:
            return ""

        audio_norm = audio / volumen if volumen > 0 else audio

        segments, _ = self._model.transcribe(
            audio_norm,
            beam_size=5,
            language="es",
            vad_filter=True,
            vad_parameters=dict(
                min_silence_duration_ms=config.VAD_MIN_SILENCE_MS,
                threshold=config.VAD_THRESHOLD,
                min_speech_duration_ms=config.VAD_MIN_SPEECH_MS,
            ),
            no_speech_threshold=config.NO_SPEECH_THRESHOLD,
            condition_on_previous_text=False,
        )

        texto = ""
        for segment in segments:
            if segment.no_speech_prob < 0.5 and segment.text.strip():
                texto += segment.text + " "

        return texto.strip()
