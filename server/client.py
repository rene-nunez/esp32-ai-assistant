from groq import Groq
from server import config

groq = Groq(api_key=config.GROQ_API_KEY)