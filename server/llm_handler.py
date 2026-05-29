import time
import logging

from groq import Groq

from server import config


log = logging.getLogger(__name__)


def ask(text: str) -> str:
    log.info("Preguntándole al modelo...")
    inicio = time.time()

    client = Groq(api_key=config.GROQ_API_KEY)
    chat_completion = client.chat.completions.create(
        messages=[config.SYSTEM_PROMPT, {"role": "user", "content": text}],
        model=config.LLM_MODEL,
        temperature=config.LLM_TEMPERATURE,
        max_tokens=config.LLM_MAX_TOKENS,
    )

    respuesta = chat_completion.choices[0].message.content.strip()
    log.info("IA: %s", respuesta)
    log.info("Latencia LLM: %.2fs", time.time() - inicio)

    return respuesta
