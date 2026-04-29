from __future__ import annotations

import json
import os
import tempfile
import threading
from typing import Dict, List, Tuple

import requests
from flask import Flask, Response, request
from flask_sock import Sock

try:
    from faster_whisper import WhisperModel
except Exception as exc:  # pragma: no cover
    WhisperModel = None
    _whisper_import_error = exc
else:
    _whisper_import_error = None

try:
    import pyttsx3
except Exception as exc:  # pragma: no cover
    pyttsx3 = None
    _tts_import_error = exc
else:
    _tts_import_error = None

app = Flask(__name__)
sock = Sock(app)

HOST = os.getenv("HOST", "0.0.0.0")
PORT = int(os.getenv("PORT", "8000"))

OPENAI_API_KEY = os.getenv("OPENAI_API_KEY", "")
OPENAI_BASE_URL = os.getenv("OPENAI_BASE_URL", "https://api.openai.com/v1").rstrip("/")
OPENAI_MODEL = os.getenv("OPENAI_MODEL", "gpt-4.1-mini")
OPENAI_STT_MODEL = os.getenv("OPENAI_STT_MODEL", "gpt-4o-mini-transcribe")
OPENAI_TTS_MODEL = os.getenv("OPENAI_TTS_MODEL", "gpt-4o-mini-tts")
OPENAI_TTS_VOICE = os.getenv("OPENAI_TTS_VOICE", "onyx")
OPENAI_TIMEOUT = float(os.getenv("OPENAI_TIMEOUT", "20"))

STT_PROVIDER = os.getenv("STT_PROVIDER", "auto").lower()
TTS_PROVIDER = os.getenv("TTS_PROVIDER", "auto").lower()
STT_LANGUAGE = os.getenv("STT_LANGUAGE", "en").strip()
STT_PROMPT = os.getenv("STT_PROMPT", "").strip()

MODEL_SIZE = os.getenv("WHISPER_MODEL", "small")
MODEL_DEVICE = os.getenv("WHISPER_DEVICE", "cpu")
MODEL_COMPUTE = os.getenv("WHISPER_COMPUTE", "int8")

TTS_RATE = int(os.getenv("TTS_RATE", "180"))
TTS_VOICE = os.getenv("TTS_VOICE", "")

SYSTEM_PROMPT = os.getenv(
    "JARVIS_PROMPT",
    "You are Jarvis, the concise AI running inside an Iron Man HUD. "
    "Keep responses short and clear unless asked to elaborate. "
    "When an image is provided, briefly describe visible objects, the scene, and notable details. "
    "Do not say you cannot see the image unless no image data was actually provided. "
    "When the user asks you to control the HUD, execute only supported actions by appending one or more machine-readable tags in this exact format: "
    "<hud>{\"action\":\"brightness\",\"value\":50}</hud>. "
    "Supported actions are brightness, hud_opacity, widget_transparency, volume, mute, "
    "snapshot_capture, snapshot_analyze, snapshot_expand, snapshot_minimize, snapshot_rotate_left, snapshot_rotate_right, "
    "snapshot_flip_horizontal, snapshot_flip_vertical, snapshot_reset, snapshot_zoom_in, snapshot_zoom_out, "
    "snapshot_delete_last, snapshot_delete_all, clip_capture, clip_play, clip_expand, clip_minimize, camera_on, camera_off, filter_invert_on, filter_invert_off, "
    "filter_bw_on, filter_bw_off, filter_clear, mask_on, mask_off, mask_open, mask_close, and mask_toggle. "
    "Only emit hud tags when the user is asking for one of those actions. "
    "You may emit multiple hud tags in one reply. "
    "Keep the natural-language part brief and leave the hud tag values as valid JSON only.",
)
CONV_MAX_TURNS = int(os.getenv("CONV_MAX_TURNS", "12"))

_tts_lock = threading.Lock()
_stt_lock = threading.Lock()
_model = None
_engine = None

_conversations: Dict[str, List[Dict[str, str]]] = {}


def normalized_language() -> str:
    lang = (STT_LANGUAGE or "").strip()
    if not lang or lang.lower() in ("auto", "none"):
        return ""
    return lang


def openai_headers() -> Dict[str, str]:
    return {
        "Authorization": f"Bearer {OPENAI_API_KEY}",
        "Content-Type": "application/json",
    }


def init_model() -> Tuple[bool, str]:
    global _model
    if WhisperModel is None:
        return False, f"faster-whisper import failed: {_whisper_import_error}"
    if _model is not None:
        return True, "ok"
    _model = WhisperModel(MODEL_SIZE, device=MODEL_DEVICE, compute_type=MODEL_COMPUTE)
    return True, "ok"


def init_tts() -> Tuple[bool, str]:
    global _engine
    if pyttsx3 is None:
        return False, f"pyttsx3 import failed: {_tts_import_error}"
    if _engine is not None:
        return True, "ok"
    _engine = pyttsx3.init()
    _engine.setProperty("rate", TTS_RATE)
    if TTS_VOICE:
        _engine.setProperty("voice", TTS_VOICE)
    return True, "ok"


def select_provider(pref: str, has_openai: bool, local_ok: bool) -> str:
    if pref == "openai":
        return "openai" if has_openai else "none"
    if pref == "local":
        return "local" if local_ok else "none"
    if has_openai:
        return "openai"
    if local_ok:
        return "local"
    return "none"


def openai_transcribe_audio(wav_bytes: bytes) -> Tuple[str, str]:
    if not OPENAI_API_KEY:
        return "", "OpenAI API key missing"
    files = {"file": ("audio.wav", wav_bytes, "audio/wav")}
    data = {"model": OPENAI_STT_MODEL, "response_format": "text"}
    lang = normalized_language()
    if lang:
        data["language"] = lang
    if STT_PROMPT:
        data["prompt"] = STT_PROMPT
    try:
        res = requests.post(
            f"{OPENAI_BASE_URL}/audio/transcriptions",
            headers={"Authorization": f"Bearer {OPENAI_API_KEY}"},
            data=data,
            files=files,
            timeout=OPENAI_TIMEOUT,
        )
    except requests.RequestException as exc:
        return "", f"OpenAI STT request failed: {exc}"
    if res.status_code != 200:
        return "", f"OpenAI STT error: {res.status_code} {res.text}"
    return res.text.strip(), ""


def local_transcribe_audio(wav_bytes: bytes) -> Tuple[str, str]:
    ok, msg = init_model()
    if not ok:
        return "", msg
    with tempfile.NamedTemporaryFile(delete=False, suffix=".wav") as tmp:
        tmp.write(wav_bytes)
        tmp_path = tmp.name
    try:
        with _stt_lock:
            kwargs = {"beam_size": 5, "vad_filter": True}
            lang = normalized_language()
            if lang:
                kwargs["language"] = lang
            if STT_PROMPT:
                kwargs["initial_prompt"] = STT_PROMPT
            segments, _info = _model.transcribe(tmp_path, **kwargs)
            text = " ".join(segment.text.strip() for segment in segments).strip()
    finally:
        try:
            os.remove(tmp_path)
        except OSError:
            pass
    return text, ""


def openai_tts(text: str) -> Tuple[bytes, str]:
    if not OPENAI_API_KEY:
        return b"", "OpenAI API key missing"
    body = {
        "model": OPENAI_TTS_MODEL,
        "voice": OPENAI_TTS_VOICE,
        "input": text,
        "response_format": "wav",
    }
    try:
        res = requests.post(
            f"{OPENAI_BASE_URL}/audio/speech",
            headers=openai_headers(),
            json=body,
            timeout=OPENAI_TIMEOUT,
        )
    except requests.RequestException as exc:
        return b"", f"OpenAI TTS request failed: {exc}"
    if res.status_code != 200:
        return b"", f"OpenAI TTS error: {res.status_code} {res.text}"
    return res.content, ""


def local_tts(text: str) -> Tuple[bytes, str]:
    ok, msg = init_tts()
    if not ok:
        return b"", msg
    with tempfile.NamedTemporaryFile(delete=False, suffix=".wav") as tmp:
        tmp_path = tmp.name
    try:
        with _tts_lock:
            _engine.save_to_file(text, tmp_path)
            _engine.runAndWait()
        with open(tmp_path, "rb") as handle:
            wav = handle.read()
    finally:
        try:
            os.remove(tmp_path)
        except OSError:
            pass
    return wav, ""


def extract_response_text(payload: dict) -> str:
    parts: List[str] = []
    for item in payload.get("output", []):
        if item.get("type") != "message":
            continue
        for content in item.get("content", []):
            if content.get("type") in ("output_text", "text"):
                text = content.get("text", "")
                if text:
                    parts.append(text)
    if not parts and payload.get("output_text"):
        parts.append(str(payload["output_text"]))
    return "\n".join(parts).strip()


def openai_response(input_items: List[dict]) -> Tuple[str, str]:
    if not OPENAI_API_KEY:
        return "", "OpenAI API key missing"
    body = {"model": OPENAI_MODEL, "input": input_items}
    try:
        res = requests.post(
            f"{OPENAI_BASE_URL}/responses",
            headers=openai_headers(),
            json=body,
            timeout=OPENAI_TIMEOUT,
        )
    except requests.RequestException as exc:
        return "", f"OpenAI request failed: {exc}"
    if res.status_code != 200:
        return "", f"OpenAI error: {res.status_code} {res.text}"
    text = extract_response_text(res.json())
    return text, ""


def add_history(chat_id: str, role: str, text: str) -> None:
    history = _conversations.setdefault(chat_id, [])
    history.append({"role": role, "text": text})
    if len(history) > CONV_MAX_TURNS:
        _conversations[chat_id] = history[-CONV_MAX_TURNS:]


def build_input(chat_id: str, user_text: str, image_data_url: str | None) -> List[dict]:
    items: List[dict] = []
    if SYSTEM_PROMPT:
        items.append({"role": "system", "content": [{"type": "input_text", "text": SYSTEM_PROMPT}]})
    if not image_data_url:
        history = _conversations.get(chat_id, [])
        for entry in history:
            role = entry.get("role", "user")
            text = entry.get("text", "")
            if not text:
                continue
            content_type = "output_text" if role == "assistant" else "input_text"
            items.append({"role": role, "content": [{"type": content_type, "text": text}]})
    content: List[dict] = [{"type": "input_text", "text": user_text}]
    if image_data_url:
        if image_data_url.startswith("data:application/octet-stream;base64,"):
            image_data_url = image_data_url.replace(
                "data:application/octet-stream;base64,",
                "data:image/jpeg;base64,",
                1,
            )
        content.append({"type": "input_image", "image_url": image_data_url})
    items.append({"role": "user", "content": content})
    return items


def handle_chat(chat_id: str, text: str, image_data_url: str | None) -> str:
    text = (text or "").strip()
    if not text:
        return ""
    input_items = build_input(chat_id, text, image_data_url)
    reply, err = openai_response(input_items)
    if err:
        return f"AI unavailable: {err}"
    if not reply:
        return "AI response unavailable."
    add_history(chat_id, "user", text)
    add_history(chat_id, "assistant", reply)
    return reply


@app.route("/health", methods=["GET"])
def health() -> Response:
    stt_ok_local, stt_msg_local = init_model()
    tts_ok_local, tts_msg_local = init_tts()
    stt_provider = select_provider(STT_PROVIDER, bool(OPENAI_API_KEY), stt_ok_local)
    tts_provider = select_provider(TTS_PROVIDER, bool(OPENAI_API_KEY), tts_ok_local)
    stt_msg = "openai" if stt_provider == "openai" else stt_msg_local
    tts_msg = "openai" if tts_provider == "openai" else tts_msg_local
    return Response(
        f"stt={stt_provider != 'none'} ({stt_msg})\n"
        f"tts={tts_provider != 'none'} ({tts_msg})\n",
        mimetype="text/plain",
    )


@app.route("/stt", methods=["POST"])
def stt() -> Response:
    data = request.get_data(cache=False)
    if not data:
        return Response("empty audio", status=400, mimetype="text/plain")

    local_ok, local_msg = init_model()
    provider = select_provider(STT_PROVIDER, bool(OPENAI_API_KEY), local_ok)
    if provider == "openai":
        text, err = openai_transcribe_audio(data)
    elif provider == "local":
        text, err = local_transcribe_audio(data)
    else:
        return Response("stt not configured", status=500, mimetype="text/plain")

    if err:
        return Response(err, status=500, mimetype="text/plain")
    if not text:
        return Response("", status=204, mimetype="text/plain")
    return Response(text, mimetype="text/plain")


@app.route("/tts", methods=["POST"])
def tts() -> Response:
    text = request.get_data(cache=False, as_text=True) or ""
    text = text.strip()
    if not text:
        return Response("empty text", status=400, mimetype="text/plain")

    local_ok, local_msg = init_tts()
    provider = select_provider(TTS_PROVIDER, bool(OPENAI_API_KEY), local_ok)
    if provider == "openai":
        wav, err = openai_tts(text)
    elif provider == "local":
        wav, err = local_tts(text)
    else:
        return Response("tts not configured", status=500, mimetype="text/plain")

    if err:
        return Response(err, status=500, mimetype="text/plain")
    return Response(wav, mimetype="audio/wav")


@sock.route("/ws")
def ws_chat(ws) -> None:
    while True:
        raw = ws.receive()
        if raw is None:
            break
        try:
            payload = json.loads(raw)
        except json.JSONDecodeError:
            continue
        kind = payload.get("type")
        chat_id = payload.get("chat_id", "hud")
        if kind not in ("message", "vision"):
            continue
        text = payload.get("content", "")
        image = payload.get("image") if kind == "vision" else None
        reply = handle_chat(chat_id, text, image)
        if reply:
            ws.send(json.dumps({"type": "response", "content": reply}))


if __name__ == "__main__":
    app.run(host=HOST, port=PORT, threaded=True)
