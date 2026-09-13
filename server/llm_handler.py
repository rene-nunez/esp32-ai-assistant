import time
import logging

from server import config
from server.client import groq

log = logging.getLogger("llm")

_LLM_ERROR_MSG = {
    "en": "I'm sorry, please try again.",
    "es": "Lo siento, inténtalo de nuevo.",
}

def ask(text: str) -> str:
    log.debug("asking %s...", config.LLM_MODEL)
    start = time.time()

    try:
        chat_completion = groq.chat.completions.create(
            messages=[config.SYSTEM_PROMPT, {"role": "user", "content": text}],
            model=config.LLM_MODEL,
            temperature=config.LLM_TEMPERATURE,
            max_tokens=config.LLM_MAX_TOKENS,
        )
        response = chat_completion.choices[0].message.content.strip()
    except Exception as e:
        log.error("llm api error: %s", e)
        response = _LLM_ERROR_MSG[config.LANGUAGE]

    log.debug("llm done (%.2fs)", time.time() - start)

    return response