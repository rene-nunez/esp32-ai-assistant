Import("env")
import os


env_path = os.path.join(env.subst("$PROJECT_DIR"), ".env")
print(f"extra_script: looking for .env at {env_path}")
print(f"extra_script: file exists = {os.path.isfile(env_path)}")

if os.path.isfile(env_path):
    for line in open(env_path, encoding="utf-8"):
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, _, val = line.partition("=")
        os.environ[key.strip()] = val.strip().strip('"').strip("'")


FLAGS_MAP = {
    "WIFI_SSID": "WIFI_SSID",
    "WIFI_PASSWORD": "WIFI_PASSWORD",
    "SERVER_IP": "SERVER_IP",
    "SERVER_WS_PORT": "WS_PORT",
}

for env_key, flag_name in FLAGS_MAP.items():
    val = os.environ.get(env_key)
    if val:
        flag = f"-D{flag_name}={val}" if val.isdigit() else f'-D{flag_name}="{val}"'
        print(f"extra_script: adding {flag}")
        env.Append(BUILD_FLAGS=[flag])
    else:
        print(f"extra_script: WARNING {env_key} not found in .env")