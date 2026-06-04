import time
import logging

from server import config
from server.client import groq

log = logging.getLogger(__name__)


def ask(text: str) -> str:
    log.info("Asking the LLM...")
    start = time.time()

    chat_completion = groq.chat.completions.create(
        messages=[config.SYSTEM_PROMPT, {"role": "user", "content": text}],
        model=config.LLM_MODEL,
        temperature=config.LLM_TEMPERATURE,
        max_tokens=config.LLM_MAX_TOKENS,
    )

    response = chat_completion.choices[0].message.content.strip()
    log.info("AI: %s", response)
    log.info("LLM latency: %.2fs", time.time() - start)

    return response
