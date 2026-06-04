import time
import logging

from groq import Groq

from server import config

log = logging.getLogger(__name__)

_client = Groq(api_key=config.GROQ_API_KEY)


def ask(text: str) -> str:
    log.info("Asking the LLM...")
    start = time.time()

    chat_completion = _client.chat.completions.create(
        messages=[config.SYSTEM_PROMPT, {"role": "user", "content": text}],
        model=config.LLM_MODEL,
        temperature=config.LLM_TEMPERATURE,
        max_tokens=config.LLM_MAX_TOKENS,
    )

    response = chat_completion.choices[0].message.content.strip()
    log.info("AI: %s", response)
    log.info("LLM latency: %.2fs", time.time() - start)

    return response
