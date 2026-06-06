Import("env")
import os.path


def load_env():
    env_path = os.path.join(env.subst("$PROJECT_DIR"), ".env")
    if not os.path.isfile(env_path):
        return
    for line in open(env_path):
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, _, val = line.partition("=")
        os.environ[key.strip()] = val.strip().strip('"').strip("'")


load_env()

FLAGS_MAP = {
    "WIFI_SSID": "WIFI_SSID",
    "WIFI_PASSWORD": "WIFI_PASSWORD",
    "SERVER_IP": "SERVER_IP",
    "SERVER_WS_PORT": "WS_PORT",
}

for env_key, flag_name in FLAGS_MAP.items():
    val = os.environ.get(env_key)
    if val:
        if val.isdigit():
            env.Append(BUILD_FLAGS=[f"-D{flag_name}={val}"])
        else:
            env.Append(BUILD_FLAGS=[f'-D{flag_name}="{val}"'])
