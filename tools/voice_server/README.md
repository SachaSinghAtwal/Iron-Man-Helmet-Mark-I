# Local Jarvis Voice + Mimiclaw Bridge

This server gives your ESP32 the endpoints it expects **and** the Mimiclaw-style WebSocket
so your HUD can talk to the AI from a laptop on the same network.

## Endpoints
- `POST /stt` receives WAV audio and returns plain text
- `POST /tts` receives plain text and returns a WAV stream
- `GET /health` reports STT/TTS availability
- `WS /ws` Mimiclaw-style chat + vision responses

## Quick Start (Windows)
1. Install Python 3.10+.
2. Open a terminal in this folder.
3. Install dependencies:
   - `pip install -r requirements.txt`
4. Start the server:
   - `python server.py`

Default host/port is `0.0.0.0:8000`.

## Connect the HUD
The ESP32 will auto-discover this server on port `8000`. Once found, the HUD will
auto-connect to `ws://<PC-IP>:8000/ws`.

If you want to hardcode it instead, set these in `include/secrets.h`:
- `LOCAL_STT_URL "http://<PC-IP>:8000/stt"`
- `LOCAL_TTS_URL "http://<PC-IP>:8000/tts"`

You can also open the HUD with a query string:
- `http://<ESP32-IP>/?mimi=ws://<PC-IP>:8000/ws`

Replace `<PC-IP>` with the IP address of the computer running the server.

## AI Mode (OpenAI)
Set `OPENAI_API_KEY` in your environment to enable chat + vision and optional STT/TTS.

Optional environment variables:
- `OPENAI_MODEL` (default `gpt-4o-mini`)
- `OPENAI_STT_MODEL` (default `gpt-4o-mini-transcribe`)
- `OPENAI_TTS_MODEL` (default `gpt-4o-mini-tts`)
- `OPENAI_TTS_VOICE` (default `onyx`)
- `OPENAI_BASE_URL` (default `https://api.openai.com/v1`)
- `OPENAI_TIMEOUT` (seconds)
- `STT_LANGUAGE` (default `en`, use `auto` to enable auto-detect)
- `STT_PROMPT` (optional hint to bias recognition)
- `JARVIS_PROMPT` (override system prompt)
- `CONV_MAX_TURNS` (default `12`)

## Offline Mode
If `OPENAI_API_KEY` is not set, the server falls back to offline STT/TTS:
- `faster-whisper` for speech-to-text
- `pyttsx3` for text-to-speech

Optional tuning:
- `WHISPER_MODEL` (default `small`)
- `WHISPER_DEVICE` (default `cpu`)
- `WHISPER_COMPUTE` (default `int8`)
- `STT_LANGUAGE` (default `en`, use `auto` to enable auto-detect)
- `STT_PROMPT` (optional hint to bias recognition)
- `TTS_RATE` (default `180`)
- `TTS_VOICE` (default empty, uses system default)
- `STT_PROVIDER` / `TTS_PROVIDER` (`auto`, `openai`, or `local`)
- `HOST` and `PORT`

## Health check
- Visit `http://<PC-IP>:8000/health`
