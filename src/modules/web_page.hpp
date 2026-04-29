// Implementation fragment included by src/main.cpp inside the HUD anonymous namespace.
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <title>IRON MAN HUD</title>
  <style>
    :root {
      --cyan: #35f6ff;
      --cyan-dim: rgba(53, 246, 255, 0.28);
      --panel-alpha: 0.5;
      --panel-strong-alpha: 0.5;
      --edge-gap: 2vh;
      --stack-gap: 2vh;
      --logs-height: 28vh;
      --jarvis-height: 34vh;
      --jarvis-extra: 24px;
      --atmos-height: 20vh;
      --time-height: 10.5vh;
      --grid: rgba(53, 246, 255, 0.12);
      --warn: #ffb347;
      --alert: #ff4f4f;
      --hud-brightness: 1;
      --hud-opacity: 1;
      --stream-invert: 0;
      --stream-gray: 0;
      --stream-contrast: 1.05;
      --stream-saturate: 1.2;
      --stream-brightness: 0.82;
      --stream-hue: 0deg;
      --stream-opacity: 1;
      --stream-glow-1: 0px;
      --stream-glow-2: 0px;
      --stream-glow-color-1: rgba(53, 246, 255, 0.55);
      --stream-glow-color-2: rgba(53, 246, 255, 0.35);
      --edge-opacity: 0;
      --edge-contrast: 2.2;
      --edge-brightness: 1.1;
      --edge-saturate: 4;
      --edge-hue: 190deg;
      --edge-blur: 0.5px;
      --edge-glow-1: 4px;
      --edge-glow-2: 12px;
      --slider-height: 120px;
      --slider-label-height: 24px;
      --edge-glow-color-1: rgba(120, 220, 255, 0.8);
      --edge-glow-color-2: rgba(50, 140, 255, 0.5);
      --helmet-opacity: 1;
      --helmet-brightness-comp: 1;
    }
    * { box-sizing: border-box; }
    html, body {
      margin: 0;
      height: 100%;
      background: #05090b;
      font-family: "Orbitron", "Share Tech Mono", "Consolas", "Lucida Console", monospace;
      color: var(--cyan);
      letter-spacing: 0.08em;
    }
    #app {
      position: fixed;
      inset: 0;
      overflow: hidden;
      background: #04080a;
    }
    #stream {
      position: absolute;
      inset: 0;
      width: 100%;
      height: 100%;
      object-fit: cover;
      opacity: var(--stream-opacity);
      transform: scaleX(-1);
      transform-origin: center;
      filter: invert(var(--stream-invert)) grayscale(var(--stream-gray)) contrast(var(--stream-contrast))
        saturate(var(--stream-saturate)) brightness(var(--stream-brightness)) hue-rotate(var(--stream-hue))
        drop-shadow(0 0 var(--stream-glow-1) var(--stream-glow-color-1))
        drop-shadow(0 0 var(--stream-glow-2) var(--stream-glow-color-2));
      z-index: 0;
    }
    #stream-edge {
      position: absolute;
      inset: 0;
      width: 100%;
      height: 100%;
      object-fit: cover;
      transform: scaleX(-1);
      transform-origin: center;
      opacity: 0;
      pointer-events: none;
      mix-blend-mode: screen;
      filter: grayscale(1) contrast(var(--edge-contrast)) brightness(var(--edge-brightness))
        sepia(1) hue-rotate(var(--edge-hue)) saturate(var(--edge-saturate)) blur(var(--edge-blur))
        drop-shadow(0 0 var(--edge-glow-1) var(--edge-glow-color-1))
        drop-shadow(0 0 var(--edge-glow-2) var(--edge-glow-color-2));
      z-index: 1;
    }
    #hud-layer {
      position: absolute;
      inset: 0;
      pointer-events: none;
      z-index: 2;
    }
    #hud-fade {
      position: absolute;
      inset: 0;
      pointer-events: none;
      opacity: var(--hud-opacity);
      transition: opacity 0.25s ease;
    }
    #jarvis-input-overlay {
      position: fixed;
      inset: 0;
      pointer-events: none;
      z-index: 3;
      filter: brightness(var(--hud-brightness));
      transition: filter 0.25s ease;
    }
    #jarvis-input-overlay .jarvis-input-row {
      position: fixed;
      pointer-events: auto;
      margin: 0;
      transform-origin: center;
      transition: transform 0.6s ease, left 0.6s ease, top 0.6s ease;
    }
    #holo-overlay {
      position: absolute;
      inset: 0;
      pointer-events: none;
      z-index: 1;
      opacity: 0;
      mix-blend-mode: screen;
      background:
        repeating-linear-gradient(180deg, rgba(80, 200, 255, 0.1) 0px, rgba(80, 200, 255, 0.1) 1px, transparent 1px, transparent 4px),
        repeating-linear-gradient(90deg, rgba(80, 200, 255, 0.06) 0px, rgba(80, 200, 255, 0.06) 1px, transparent 1px, transparent 8px),
        radial-gradient(circle at 22% 35%, rgba(120, 220, 255, 0.2), transparent 48%),
        radial-gradient(circle at 70% 68%, rgba(70, 170, 255, 0.16), transparent 55%);
      filter: blur(0.35px) contrast(1.1) saturate(1.2);
      transition: opacity 0.2s ease;
    }
    #holo-vignette {
      position: absolute;
      inset: 0;
      pointer-events: none;
      z-index: 1;
      opacity: 0;
      mix-blend-mode: multiply;
      background: radial-gradient(circle at 50% 45%, rgba(0, 0, 0, 0) 35%, rgba(0, 0, 0, 0.55) 100%);
      transition: opacity 0.2s ease;
    }
    .holo-on #holo-overlay {
      opacity: 0.78;
    }
    .holo-on #holo-vignette {
      opacity: 0.55;
    }
    .holo-on #stream-edge {
      opacity: var(--edge-opacity);
    }
    #ai-overlay {
      position: absolute;
      inset: 0;
      pointer-events: none;
      z-index: 1;
      opacity: 0;
      visibility: hidden;
      transition: opacity 0.2s ease;
    }
    #ai-overlay.active {
      opacity: 1;
      visibility: visible;
    }
    .ai-detection {
      position: absolute;
      border-radius: 50%;
      border: 2px solid rgba(90, 235, 255, 0.9);
      box-shadow:
        0 0 12px rgba(90, 235, 255, 0.6),
        0 0 22px rgba(50, 140, 255, 0.45);
      background: rgba(5, 25, 40, 0.15);
      mix-blend-mode: screen;
    }
    .ai-label {
      position: absolute;
      top: calc(100% + 6px);
      left: 50%;
      transform: translateX(-50%);
      font-size: 0.55rem;
      letter-spacing: 0.12em;
      text-transform: uppercase;
      color: var(--cyan);
      padding: 2px 6px;
      border-radius: 10px;
      border: 1px solid rgba(90, 235, 255, 0.5);
      background: rgba(0, 0, 0, 0.45);
      white-space: nowrap;
    }
    #hud {
      position: absolute;
      inset: 0;
      pointer-events: none;
      filter: brightness(var(--hud-brightness));
      transition: filter 0.25s ease;
      z-index: 2;
    }
    #grid {
      position: absolute;
      inset: 0;
      background-image:
        repeating-linear-gradient(0deg, transparent 0px, transparent 38px, var(--grid) 39px),
        repeating-linear-gradient(90deg, transparent 0px, transparent 38px, var(--grid) 39px);
      opacity: 0.5;
      mix-blend-mode: screen;
    }
    #snapshot-overlay {
      position: absolute;
      left: 50%;
      top: 50%;
      transform: translate(-50%, -50%);
      width: min(75vw, 85vh);
      max-width: calc(100% - 8vw);
      max-height: calc(100% - 6vh);
      padding: 18px 14px;
      border: 1px solid var(--cyan-dim);
      background: rgba(0, 0, 0, 0.45);
      box-shadow: 0 0 20px rgba(53, 246, 255, 0.25);
      display: none;
      align-items: center;
      justify-content: center;
      z-index: 2;
      pointer-events: auto;
    }
    #snapshot-overlay.active {
      display: flex;
    }
    #snapshot-overlay img {
      max-width: 100%;
      max-height: 72vh;
      width: auto;
      height: auto;
      aspect-ratio: 16 / 9;
      object-fit: contain;
      display: block;
      transform-origin: center;
      cursor: grab;
      user-select: none;
      touch-action: none;
    }
    #snapshot-overlay img.dragging {
      cursor: grabbing;
    }
    .panel {
      position: absolute;
      background: rgba(0, 0, 0, var(--panel-alpha));
      border: 1px solid var(--cyan-dim);
      box-shadow: 0 0 20px rgba(53, 246, 255, 0.15);
      padding: 12px 14px;
      text-transform: uppercase;
      backdrop-filter: blur(4px);
    }
    .panel strong {
      color: var(--cyan);
    }
    .header {
      font-size: 0.85rem;
      display: flex;
      align-items: center;
      gap: 0.5rem;
    }
    .header .pip {
      width: 6px;
      height: 22px;
      background: var(--cyan);
      box-shadow: 0 0 12px rgba(53, 246, 255, 0.8);
    }
    .muted {
      color: rgba(53, 246, 255, 0.65);
      font-size: 0.68rem;
    }
    .status-pill {
      display: inline-block;
      padding: 2px 6px;
      border-radius: 10px;
      border: 1px solid rgba(53, 246, 255, 0.25);
      font-size: 0.65rem;
      letter-spacing: 0.12em;
      text-transform: uppercase;
    }
    .status-good {
      color: #6cff9c;
      border-color: rgba(108, 255, 156, 0.6);
      box-shadow: 0 0 10px rgba(108, 255, 156, 0.55);
    }
    .status-ok {
      color: var(--warn);
      border-color: rgba(255, 179, 71, 0.6);
      box-shadow: 0 0 10px rgba(255, 179, 71, 0.55);
    }
    .status-weak,
    .status-search {
      color: #ff5e5e;
      border-color: rgba(255, 94, 94, 0.7);
      box-shadow: 0 0 10px rgba(255, 94, 94, 0.6);
    }
    .status-fix {
      color: #6cff9c;
      border-color: rgba(108, 255, 156, 0.6);
      box-shadow: 0 0 10px rgba(108, 255, 156, 0.55);
    }
    .big {
      font-size: 1.6rem;
    }
    .grid-row {
      display: flex;
      justify-content: space-between;
      gap: 1rem;
      font-size: 0.75rem;
      margin-top: 6px;
    }
    .bar {
      height: 6px;
      background: rgba(53, 246, 255, 0.1);
      border: 1px solid rgba(53, 246, 255, 0.2);
      margin-top: 6px;
      position: relative;
      overflow: hidden;
    }
    .bar .fill {
      position: absolute;
      inset: 0;
      width: 0%;
      background: linear-gradient(90deg, rgba(53, 246, 255, 0.2), rgba(53, 246, 255, 0.9));
      box-shadow: 0 0 10px rgba(53, 246, 255, 0.65);
      transition: width 0.6s ease;
    }
    #core {
      display: block;
      position: absolute;
      left: 50%;
      top: 52%;
      width: 36vmin;
      height: 36vmin;
      transform: translate(-50%, -50%);
      border-radius: 50%;
      border: 1px solid rgba(53, 246, 255, 0.3);
      box-shadow: 0 0 30px rgba(53, 246, 255, 0.2);
    }
    #core .ring {
      position: absolute;
      inset: -6vmin;
      border-radius: 50%;
      border: 1px dashed rgba(53, 246, 255, 0.3);
      animation: spin 24s linear infinite;
    }
    #core .ring2 {
      inset: -12vmin;
      border: 1px solid rgba(53, 246, 255, 0.15);
      animation: spinReverse 40s linear infinite;
    }
    #core .pulse {
      position: absolute;
      inset: 30%;
      border-radius: 50%;
      border: 1px solid rgba(53, 246, 255, 0.7);
      animation: pulse 2.8s ease-in-out infinite;
    }
    #core .center {
      position: absolute;
      inset: 34%;
      border-radius: 50%;
      border: 1px solid rgba(53, 246, 255, 0.3);
      background: transparent;
    }
    #core .center .value {
      font-size: 2.6vmin;
      letter-spacing: 0.12em;
    }
    #core .center .unit {
      font-size: 0.6rem;
      color: rgba(53, 246, 255, 0.65);
    }
    #right-top-stack {
      position: absolute;
      top: 2vh;
      right: 2vw;
      width: min(24vw, 320px);
      display: flex;
      flex-direction: column;
      gap: var(--stack-gap);
      pointer-events: auto;
    }
    #right-top-stack .panel {
      position: relative;
      width: 100%;
    }
    #top-left {
      width: 100%;
    }
    .mode-row {
      margin-top: 8px;
      display: flex;
      align-items: center;
      gap: 8px;
    }
    #top-controls {
      text-align: left;
      pointer-events: auto;
    }
    #left-bottom-stack {
      position: absolute;
      bottom: var(--edge-gap);
      left: 2vw;
      width: min(44vw, 560px);
      display: flex;
      flex-direction: column;
      gap: var(--stack-gap);
      pointer-events: auto;
    }
    #left-bottom-stack .panel {
      position: relative;
      width: 100%;
    }
    #left-bottom-stack #top-controls {
      width: calc((100% - var(--stack-gap)) / 2);
      align-self: flex-start;
    }
    #left-bottom-row {
      display: flex;
      gap: var(--stack-gap);
      align-items: flex-end;
    }
    #left-bottom-row .panel {
      flex: 1 1 0;
      width: auto;
    }
    #right-bottom-stack {
      position: absolute;
      bottom: var(--edge-gap);
      right: 2vw;
      width: min(24vw, 320px);
      display: flex;
      flex-direction: column;
      gap: var(--stack-gap);
      pointer-events: auto;
    }
    #right-bottom-stack .panel {
      position: relative;
      width: 100%;
    }
    #right-bottom-stack #time-panel {
      width: 50%;
    }
    #time-panel {
      position: relative;
      height: var(--time-height);
      text-align: right;
      border-radius: 0;
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: flex-end;
      padding: 8px 10px;
      width: 50%;
      align-self: flex-end;
    }
    #left-bars {
      width: 100%;
      height: auto;
    }
    #left-logs {
      width: 100%;
      height: var(--logs-height);
      overflow: hidden;
    }
    #right-atmos {
      width: 100%;
      height: auto;
    }
    #right-sensory {
      width: 100%;
      height: calc(var(--jarvis-height) + var(--jarvis-extra));
      display: flex;
      flex-direction: column;
      gap: 6px;
      overflow: hidden;
      pointer-events: auto;
    }
    .controls-buttons {
      display: grid;
      grid-template-columns: repeat(2, minmax(0, 1fr));
      gap: 6px;
      margin-top: 10px;
      font-size: 0.26rem;
      width: 100%;
    }
    .controls-buttons .chip.button {
      width: 100%;
      padding: 2px 6px;
      border-radius: 6px;
      line-height: 1.1;
      font-size: 0.6rem;
      letter-spacing: 0.06em;
    }
    .controls-buttons .chip.button:last-child {
      grid-column: 1 / -1;
      justify-self: center;
      width: 65%;
    }
    .chip {
      padding: 6px 12px;
      border: 1px solid var(--cyan-dim);
      background: rgba(0, 0, 0, var(--panel-strong-alpha));
      box-shadow: 0 0 10px rgba(53, 246, 255, 0.25);
      color: var(--cyan);
      text-transform: uppercase;
    }
    .chip.button {
      cursor: pointer;
    }
    .chip.button:active {
      transform: translateY(1px);
    }
    .chip.button.active {
      background: rgba(53, 246, 255, 0.15);
      box-shadow: 0 0 14px rgba(53, 246, 255, 0.45);
    }
    #net-mode {
      width: 100%;
      text-align: center;
      letter-spacing: 0.08em;
    }
    #camera-off {
      position: absolute;
      inset: 0;
      display: none;
      align-items: center;
      justify-content: center;
      background: rgba(0, 0, 0, 0.75);
      color: var(--cyan);
      font-size: 1.2rem;
      letter-spacing: 0.2em;
      text-transform: uppercase;
      z-index: 1;
    }
    .control-block {
      margin-top: 10px;
    }
    .control-block.slider-block {
      display: grid;
      grid-template-rows: var(--slider-label-height) var(--slider-height) var(--slider-label-height);
      justify-items: center;
      row-gap: 0;
      flex: 1 1 0;
      min-width: 0;
    }
    .slider-grid {
      display: flex;
      gap: 4px;
      justify-content: space-between;
      flex-wrap: nowrap;
      margin-top: 6px;
    }
    .slider-grid .control-block {
      margin-top: 0;
    }
    .slider-label {
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      gap: 2px;
      width: 100%;
      line-height: 1.05;
      min-height: var(--slider-label-height);
      text-align: center;
    }
    .slider-label .label-text {
      white-space: nowrap;
    }
    .slider-label.bottom {
      grid-row: 3;
    }
    .control-block.slider-block .slider-label:not(.bottom) {
      grid-row: 1;
    }
    .slider {
      width: 14px;
      height: var(--slider-height);
      margin-top: 0;
      grid-row: 2;
      appearance: none;
      writing-mode: bt-lr;
      -webkit-appearance: slider-vertical;
      appearance: slider-vertical;
      background: rgba(53, 246, 255, 0.15);
      border: 1px solid rgba(53, 246, 255, 0.25);
      box-shadow: 0 0 8px rgba(53, 246, 255, 0.2);
    }
    .slider::-webkit-slider-thumb {
      appearance: none;
      width: 14px;
      height: 14px;
      border-radius: 50%;
      background: var(--cyan);
      box-shadow: 0 0 10px rgba(53, 246, 255, 0.8);
      cursor: pointer;
    }
    .control-block.slider-block .slider {
      margin-top: 0;
    }
    .slider::-moz-range-thumb {
      width: 14px;
      height: 14px;
      border-radius: 50%;
      background: var(--cyan);
      box-shadow: 0 0 10px rgba(53, 246, 255, 0.8);
      cursor: pointer;
    }
    .log-list {
      margin: 4px 0 0;
      padding: 0;
      list-style: none;
      font-size: 0.7rem;
      color: rgba(53, 246, 255, 0.7);
    }
    .log-list li {
      margin-bottom: 4px;
      white-space: pre-wrap;
      overflow-wrap: normal;
      word-break: normal;
      hyphens: none;
    }
    .kpi-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
      margin-top: 10px;
      font-size: 0.75rem;
    }
    .kpi-grid span {
      display: block;
      margin-top: 4px;
      font-size: 1.1rem;
    }
    .coord-readout {
      display: flex;
      flex-direction: column;
      gap: 4px;
      margin-top: 4px;
      font-size: 1.1rem;
      line-height: 1.15;
    }
    .coord-line {
      display: flex;
      align-items: baseline;
      gap: 8px;
      white-space: nowrap;
    }
    .coord-label {
      min-width: 34px;
      color: rgba(53, 246, 255, 0.9);
    }
    .coord-value {
      font-variant-numeric: tabular-nums;
      letter-spacing: 0.08em;
    }
    .coord-meta {
      margin-top: 6px;
      font-size: 0.72rem;
      color: rgba(53, 246, 255, 0.7);
      letter-spacing: 0.08em;
    }
    .sensory {
      display: grid;
      grid-template-columns: repeat(7, 1fr);
      gap: 6px;
      align-items: end;
      height: 70px;
      margin-top: 6px;
    }
    .jarvis-section {
      display: flex;
      flex-direction: column;
      gap: 4px;
      min-height: 0;
    }
    #right-sensory > .jarvis-section:not(.jarvis-controls) {
      flex: 1 1 auto;
    }
    .jarvis-logs {
      flex: 1 1 auto;
      max-height: none;
      min-height: 0;
      overflow-y: scroll;
      overflow-x: auto;
      scrollbar-gutter: stable;
      padding-bottom: 6px;
    }
    .jarvis-log-header {
      display: flex;
      flex-direction: column;
      gap: 4px;
      margin-bottom: 6px;
    }
    .jarvis-logs li > img {
      width: 100%;
      height: 96px;
      aspect-ratio: 16 / 9;
      object-fit: cover;
      border: 1px solid rgba(53, 246, 255, 0.25);
      box-shadow: 0 0 12px rgba(53, 246, 255, 0.2);
      margin-top: 4px;
    }
    .jarvis-media-thumb {
      position: relative;
      width: 100%;
      height: 96px;
      aspect-ratio: 16 / 9;
      overflow: hidden;
      margin-top: 4px;
      border: 1px solid rgba(53, 246, 255, 0.25);
      background: rgba(0, 0, 0, 0.45);
      box-shadow: 0 0 12px rgba(53, 246, 255, 0.2);
      cursor: pointer;
    }
    .jarvis-media-thumb img {
      display: block;
      width: 100%;
      height: 100%;
      object-fit: cover;
      margin-top: 0;
      border: 0;
      box-shadow: none;
    }
    .jarvis-media-badge {
      position: absolute;
      right: 8px;
      bottom: 8px;
      padding: 3px 7px;
      border: 1px solid rgba(53, 246, 255, 0.45);
      background: rgba(0, 0, 0, 0.72);
      color: var(--cyan);
      letter-spacing: 0.18em;
      font-size: 0.65rem;
      box-shadow: 0 0 10px rgba(53, 246, 255, 0.25);
    }
    .jarvis-controls {
      flex: 0 0 auto;
      pointer-events: auto;
      padding-top: 2px;
      padding-bottom: 14px;
    }
    .jarvis-input-row {
      display: flex;
      align-items: center;
      gap: 6px;
      margin-top: 6px;
      margin-bottom: 6px;
    }
    .jarvis-mic-icon {
      width: 34px;
      height: 28px;
      padding: 0;
      position: relative;
      font-size: 0;
      letter-spacing: 0;
      background-image: linear-gradient(var(--cyan), var(--cyan));
      background-size: 2px 6px;
      background-position: center 17px;
      background-repeat: no-repeat;
    }
    .jarvis-mic-icon::before {
      content: "";
      position: absolute;
      top: 5px;
      left: 50%;
      width: 10px;
      height: 14px;
      border: 1.6px solid var(--cyan);
      border-radius: 6px;
      transform: translateX(-50%);
      box-shadow: 0 0 6px rgba(53, 246, 255, 0.6);
    }
    .jarvis-mic-icon::after {
      content: "";
      position: absolute;
      left: 50%;
      bottom: 6px;
      width: 16px;
      height: 8px;
      border: 1.6px solid var(--cyan);
      border-top: none;
      border-radius: 0 0 10px 10px;
      transform: translateX(-50%);
      box-shadow: 0 0 6px rgba(53, 246, 255, 0.5);
    }
    .chip.button.jarvis-mic-icon.active {
      background-image: linear-gradient(var(--cyan), var(--cyan));
    }
    .jarvis-mic-icon.listening {
      background-color: rgba(53, 246, 255, 0.12);
      box-shadow: 0 0 16px rgba(53, 246, 255, 0.55);
    }
    .jarvis-input {
      flex: 1;
      min-width: 0;
      background: rgba(0, 0, 0, 0.4);
      border: 1px solid var(--cyan-dim);
      color: var(--cyan);
      padding: 6px 8px;
      font-family: inherit;
      font-size: 0.7rem;
      letter-spacing: 0.08em;
      text-transform: uppercase;
      outline: none;
    }
    .jarvis-input::placeholder {
      color: rgba(53, 246, 255, 0.55);
    }
    .jarvis-input-placeholder {
      visibility: hidden;
      pointer-events: none;
    }
    .jarvis-status {
      margin-top: 4px;
      font-size: 0.62rem;
      color: rgba(53, 246, 255, 0.6);
    }
    .jarvis-status.detail {
      font-size: 0.52rem;
      letter-spacing: 0.16em;
      text-transform: uppercase;
      color: rgba(53, 246, 255, 0.55);
      margin-top: 2px;
    }
    .jarvis-logs::-webkit-scrollbar {
      width: 6px;
    }
    .jarvis-logs::-webkit-scrollbar-thumb {
      background: rgba(53, 246, 255, 0.35);
      border-radius: 6px;
    }
    .jarvis-logs::-webkit-scrollbar-track {
      background: rgba(0, 0, 0, 0.2);
    }
    .subheading {
      margin-top: 4px;
      font-size: 0.7rem;
      color: rgba(53, 246, 255, 0.65);
    }
    .helmet-wrap {
      width: 100%;
      height: 100%;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .helmet-video {
      width: 100%;
      height: 100%;
      object-fit: cover;
      opacity: var(--helmet-opacity);
      filter: brightness(var(--helmet-brightness-comp));
      transition: opacity 0.25s ease, filter 0.25s ease;
    }
    .helmet-svg {
      width: 100%;
      height: 100%;
    }
    .helmet-svg .outline {
      fill: none;
      stroke: rgba(53, 246, 255, 0.75);
      stroke-width: 1.6;
      stroke-linejoin: round;
      stroke-linecap: round;
      filter: drop-shadow(0 0 4px rgba(53, 246, 255, 0.35));
    }
    .helmet-svg .detail {
      fill: none;
      stroke: rgba(53, 246, 255, 0.5);
      stroke-width: 1;
      stroke-linejoin: round;
      stroke-linecap: round;
    }
    .sensory .bar-vert {
      width: 100%;
      background: rgba(53, 246, 255, 0.1);
      border: 1px solid rgba(53, 246, 255, 0.2);
      position: relative;
      overflow: hidden;
    }
    .sensory .bar-vert span {
      position: absolute;
      bottom: 0;
      width: 100%;
      height: 20%;
      background: linear-gradient(180deg, rgba(53, 246, 255, 0.95), rgba(53, 246, 255, 0.2));
      box-shadow: 0 0 10px rgba(53, 246, 255, 0.6);
      transition: height 0.6s ease;
    }
    @keyframes spin {
      from { transform: rotate(0deg); }
      to { transform: rotate(360deg); }
    }
    @keyframes spinReverse {
      from { transform: rotate(360deg); }
      to { transform: rotate(0deg); }
    }
    @keyframes pulse {
      0%, 100% { transform: scale(0.96); opacity: 0.6; }
      50% { transform: scale(1.04); opacity: 1; }
    }
  </style>
</head>
<body>
  <div id="app">
    <img id="stream" src="/stream" data-src="/stream" alt="camera stream"/>
    <img id="stream-edge" src="/stream" data-src="/stream" alt="" aria-hidden="true"/>
    <div id="hud-layer">
      <div id="hud-fade">
        <div id="holo-overlay" aria-hidden="true"></div>
        <div id="holo-vignette" aria-hidden="true"></div>
        <div id="ai-overlay" class="ai-overlay" aria-hidden="true"></div>
        <div id="camera-off">CAMERA OFF</div>
        <div id="hud">
          <div id="grid"></div>
          <div id="snapshot-overlay" aria-hidden="true">
            <img id="snapshot-expanded" alt="Snapshot"/>
          </div>

        <div id="right-top-stack">
          <section id="top-left" class="panel">
            <div class="header">
              <div class="pip"></div>
              <div>
                <div><strong>IRON MAN MARK I</strong> <span class="muted">CORE</span></div>
                <div class="muted">Sacha Singh Atwal</div>
              </div>
            </div>
            <div class="mode-row">
              <div id="net-mode" class="chip">MODE: --</div>
            </div>
          </section>
        </div>

      <div id="left-bottom-stack">
        <section id="top-controls" class="panel">
          <div class="muted">SYSTEM CONTROL</div>
          <div class="slider-grid">
            <div class="control-block slider-block">
              <div class="muted slider-label"><span class="label-text">Brightness</span><span id="brightness-val">100%</span></div>
              <input id="brightness" class="slider vertical" type="range" min="0" max="100" step="1" orient="vertical" value="100"/>
            </div>
            <div class="control-block slider-block">
              <input id="hud-opacity" class="slider vertical" type="range" min="0" max="100" step="1" orient="vertical" value="100"/>
              <div class="muted slider-label bottom"><span class="label-text">Opacity</span><span id="hud-opacity-val">100%</span></div>
            </div>
            <div class="control-block slider-block">
              <div class="muted slider-label"><span class="label-text">Transparency</span><span id="transparency-val">50%</span></div>
              <input id="transparency" class="slider vertical" type="range" min="0" max="100" step="1" orient="vertical" value="50"/>
            </div>
            <div class="control-block slider-block">
              <input id="volume" class="slider vertical" type="range" min="0" max="100" step="1" orient="vertical" value="100"/>
              <div class="muted slider-label bottom"><span class="label-text">Volume</span><span id="volume-val">100%</span></div>
            </div>
          </div>
          <div class="controls-buttons">
            <button id="power-toggle" class="chip button" type="button">CAMERA: ON</button>
            <button id="invert-toggle" class="chip button" type="button">VIDEO_INVERT: OFF</button>
            <button id="bw-toggle" class="chip button" type="button">BLACK_WHITE: OFF</button>
          </div>
        </section>
        <div id="left-bottom-row">
          <section id="left-logs" class="panel">
            <div class="muted">HELMET_VIEW</div>
            <div class="helmet-wrap">
              <video class="helmet-video" autoplay muted loop playsinline>
                <source src="/holo.mp4?v=2" type="video/mp4"/>
              </video>
            </div>
          </section>

          <section id="left-bars" class="panel">
            <div class="muted">SUBSYSTEM_OPTICS</div>
            <div class="grid-row">
              <div>GNSS</div><div id="gnss-status">SEARCHING</div>
            </div>
            <div class="grid-row">
              <div>SPEED_MPH</div><div id="mph-val">--</div>
            </div>
            <div class="bar"><div id="mph-bar" class="fill"></div></div>
            <div class="grid-row">
              <div>SPEED_MS</div><div id="ms-val">--</div>
            </div>
            <div class="bar"><div id="ms-bar" class="fill"></div></div>
          </section>
        </div>
      </div>

      <div id="right-bottom-stack">
        <section id="time-panel" class="panel">
          <div class="muted">LOCAL_TIME</div>
          <div id="time" class="big">--:--</div>
          <div class="muted">LINK_STATUS: <span id="link">--</span></div>
        </section>

        <section id="right-atmos" class="panel">
          <div class="muted">ATMOSPHERICS</div>
          <div class="kpi-grid">
            <div>LOCATION
              <div class="coord-readout">
                <div class="coord-line"><span class="coord-label">LAT</span><span id="lat-atm" class="coord-value">--.------</span></div>
                <div class="coord-line"><span class="coord-label">LON</span><span id="lon-atm" class="coord-value">--.------</span></div>
                <div id="location-meta" class="coord-meta">GNSS: SEARCHING</div>
              </div>
            </div>
            <div>CORE_TEMP<span id="temp">--</span></div>
            <div>CONNECTION<span id="conn-status">--</span></div>
            <div>POWER<span id="power-val">--</span></div>
          </div>
        </section>

        <section id="right-sensory" class="panel">
          <div class="muted">JARVIS</div>
          <div class="jarvis-section">
            <div id="jarvis-log-box" class="jarvis-logs">
              <div class="jarvis-log-header">
                <div class="subheading">LOGS:</div>
                <div id="jarvis-status" class="jarvis-status">MIMICLAW: NOT CONFIGURED</div>
                <div id="jarvis-mimic-detail" class="jarvis-status detail">WS: --</div>
                <div class="jarvis-status">MIC_LEVEL: <span id="mic-level">--</span></div>
                <div class="subheading">SENSORY:</div>
                <div class="sensory" id="sensory"></div>
              </div>
              <ul id="logs" class="log-list"></ul>
            </div>
          </div>
          <div class="jarvis-section jarvis-controls">
            <div class="subheading">COMMAND:</div>
            <div id="jarvis-input-anchor" class="jarvis-input-row jarvis-input-placeholder" aria-hidden="true">
              <input class="jarvis-input" type="text" tabindex="-1" aria-hidden="true" disabled/>
              <button class="chip button jarvis-mic-icon" type="button" tabindex="-1" aria-hidden="true"></button>
            </div>
          </div>
        </section>
      </div>

      <div id="core">
        <div class="ring ring2"></div>
        <div class="ring"></div>
        <div class="pulse"></div>
        <div class="center"></div>
      </div>
        </div>
      </div>
    </div>
  </div>
  <div id="jarvis-input-overlay">
    <div class="jarvis-input-row">
      <input id="jarvis-input" class="jarvis-input" type="text" placeholder="Say 'Jarvis' then a command"/>
      <button id="jarvis-mic" class="chip button jarvis-mic-icon" type="button" aria-label="Mic" title="Mic"></button>
    </div>
  </div>

  <script>
    const sensory = document.getElementById("sensory");
    const stream = document.getElementById("stream");
    const streamEdge = document.getElementById("stream-edge");
    const invertToggle = document.getElementById("invert-toggle");
    const bwToggle = document.getElementById("bw-toggle");
    const powerToggle = document.getElementById("power-toggle");
    const brightness = document.getElementById("brightness");
    const brightnessVal = document.getElementById("brightness-val");
    const hudOpacity = document.getElementById("hud-opacity");
    const hudOpacityVal = document.getElementById("hud-opacity-val");
    const transparency = document.getElementById("transparency");
    const transparencyVal = document.getElementById("transparency-val");
    const gnssStatus = document.getElementById("gnss-status");
    const connStatus = document.getElementById("conn-status");
    const powerStatus = document.getElementById("power-val");
    const netMode = document.getElementById("net-mode");
    const volume = document.getElementById("volume");
    const volumeVal = document.getElementById("volume-val");
    const cameraOff = document.getElementById("camera-off");
    const jarvisInput = document.getElementById("jarvis-input");
    const jarvisMic = document.getElementById("jarvis-mic");
    const jarvisInputAnchor = document.getElementById("jarvis-input-anchor");
    const jarvisInputOverlay = document.getElementById("jarvis-input-overlay");
    const jarvisInputRow = jarvisInputOverlay ? jarvisInputOverlay.querySelector(".jarvis-input-row") : null;
    const jarvisStatus = document.getElementById("jarvis-status");
    const jarvisMimicDetail = document.getElementById("jarvis-mimic-detail");
    const jarvisLogBox = document.getElementById("jarvis-log-box");
    const micLevel = document.getElementById("mic-level");
    const snapshotOverlay = document.getElementById("snapshot-overlay");
    const snapshotExpandedImg = document.getElementById("snapshot-expanded");
    const aiOverlay = document.getElementById("ai-overlay");
    const rootStyle = document.documentElement.style;
    let streamBase = "/stream";
    let streamFallbackTried = false;
    function applyStreamBase(base) {
      if (!stream) return;
      stream.crossOrigin = "anonymous";
      stream.dataset.src = base;
      stream.src = base;
      if (streamEdge) {
        streamEdge.crossOrigin = "anonymous";
        streamEdge.dataset.src = base;
        streamEdge.src = base + "?edge=1";
      }
    }
    function streamBaseForPort(port) {
      if (!port || !location.hostname) {
        return "/stream";
      }
      if (port === 81 || port === "81") {
        return location.protocol + "//" + location.hostname + ":81/stream";
      }
      return "/stream";
    }
    function updateStreamBaseFromStatus(data) {
      if (!data || !stream) return;
      const target = streamBaseForPort(data.stream_port);
      if (target && (stream.dataset.src || "") !== target) {
        streamFallbackTried = false;
        applyStreamBase(target);
      }
    }
    if (stream && location.hostname) {
      streamBase = location.protocol + "//" + location.hostname + ":81/stream";
    }
    applyStreamBase(streamBase);
    const streamDefaults = {
      contrast: "1.05",
      saturate: "1.2",
      brightness: "0.82",
      hue: "0deg",
      opacity: "1",
      glow1: "0px",
      glow2: "0px",
      gray: "0",
      glowColor1: "rgba(53, 246, 255, 0.55)",
      glowColor2: "rgba(53, 246, 255, 0.35)",
      edgeOpacity: "0",
      edgeContrast: "2.2",
      edgeBrightness: "1.1",
      edgeSaturate: "4",
      edgeHue: "190deg",
      edgeBlur: "0.5px",
      edgeGlow1: "4px",
      edgeGlow2: "12px",
      edgeGlowColor1: "rgba(120, 220, 255, 0.8)",
      edgeGlowColor2: "rgba(50, 140, 255, 0.5)"
    };
    const aiParams = new URLSearchParams(window.location.search);
    let AI_PROFILE = (aiParams.get("ai") || "person").toLowerCase();
    if (AI_PROFILE === "object") {
      AI_PROFILE = "person";
    }
    const AI_LABELS = {
      person: ["person"],
      gender: ["male", "female"],
      age: ["child", "teen", "adult", "senior"],
      emotion: ["neutral", "happy", "sad", "angry", "surprised", "fear", "disgust"]
    };
    let activeFilter = "none";
    let invertOn = false;
    let bwOn = false;
    let lastStatus = null;
    let lastVoiceSeq = 0;
    let ttsBusy = false;
    let micHoldoffUntil = 0;
    let volumeDragging = false;
    let volumeSendTimer = null;
    let lastVolumeSetAt = 0;
    let lastVolumeSetValue = null;
    let systemLogs = [];
    let lastSystemLogSnapshot = [];
    const chatLogs = [];
    let hostBatteryLevel = null;
    let hostBatteryCharging = null;
    let setCameraState = null;
    let snapshotBlobUrl = "";
    let snapshotDataUrl = "";
    let snapshotExpanded = false;
    let snapshotRotation = 0;
    let snapshotFlipX = 1;
    let snapshotFlipY = 1;
    let snapshotZoom = 1;
    let snapshotPanX = 0;
    let snapshotPanY = 0;
    let snapshotDragging = false;
    let snapshotDragStartX = 0;
    let snapshotDragStartY = 0;
    let snapshotDragOriginX = 0;
    let snapshotDragOriginY = 0;
    let expandedMediaType = "snapshot";
    let clipFrameBuffer = [];
    let clipTimer = null;
    let clipCanvas = null;
    let clipCanvasCtx = null;
    let clipFrameBusy = false;
    let clipUseSnapshotFallback = false;
    let clipStreamAbort = null;
    let clipStreamStarting = false;
    let lastClipFrameAt = 0;
    let lastClipSnapshotFallbackAt = 0;
    let latestClip = null;
    let clipSeq = 0;
    let expandedClipFrame = 0;
    let clipPlaybackTimer = null;
    let lastMediaIntent = "";
    let visionBlobUrl = "";
    let visionDataUrl = "";
    let visionCaptureBusy = false;
    let visionTimer = null;
    let jarvisAutoScroll = true;
    let currentMicLevel = "--";
    for (let i = 0; i < 7; i++) {
      const bar = document.createElement("div");
      bar.className = "bar-vert";
      const span = document.createElement("span");
      bar.appendChild(span);
      sensory.appendChild(bar);
    }

    function setText(id, value, fallback = "--") {
      const el = document.getElementById(id);
      if (!el) return;
      el.textContent = value !== undefined && value !== null && value !== "" ? value : fallback;
    }

    function formatCoordinate(value, axis) {
      const numeric = Number(value);
      if (!Number.isFinite(numeric)) {
        return "";
      }
      const abs = Math.abs(numeric).toFixed(6);
      const hemisphere = axis === "lat"
        ? (numeric >= 0 ? "N" : "S")
        : (numeric >= 0 ? "E" : "W");
      return abs + " " + hemisphere;
    }

    function renderLocation(data) {
      const rawLat = data && data.lat !== undefined && data.lat !== null && data.lat !== "" ? data.lat : "";
      const rawLon = data && data.lon !== undefined && data.lon !== null && data.lon !== "" ? data.lon : "";
      const hasLat = rawLat !== "" && Number.isFinite(Number(rawLat));
      const hasLon = rawLon !== "" && Number.isFinite(Number(rawLon));
      setText("lat-atm", hasLat ? formatCoordinate(rawLat, "lat") : "", "--.------");
      setText("lon-atm", hasLon ? formatCoordinate(rawLon, "lon") : "", "--.------");

      const meta = document.getElementById("location-meta");
      if (!meta) return;

      const parts = [];
      if (data && data.nav) {
        if (data.nav.fix && hasLat && hasLon) {
          parts.push("GNSS LOCK");
        } else if (hasLat && hasLon) {
          parts.push("LAST KNOWN");
        } else if (data.nav.serial && data.nav.signal === false) {
          parts.push("GNSS SERIAL");
        } else if (data.nav.signal === false) {
          parts.push("SEARCHING");
        } else {
          parts.push("NO COORDS");
        }

        if (Number.isFinite(Number(data.nav.satellites)) && Number(data.nav.satellites) > 0) {
          parts.push(String(Number(data.nav.satellites)) + " SAT");
        }
        if (Number.isFinite(Number(data.nav.hdop)) && Number(data.nav.hdop) >= 0) {
          parts.push("HDOP " + Number(data.nav.hdop).toFixed(1));
        }
      } else {
        parts.push(hasLat && hasLon ? "COORDS READY" : "NO COORDS");
      }

      meta.textContent = parts.join(" | ");
    }

    function setBar(id, percent) {
      const el = document.getElementById(id);
      if (!el) return;
      const clamped = Math.max(0, Math.min(100, percent || 0));
      el.style.width = clamped + "%";
    }

    function browserTimeString() {
      const now = new Date();
      const hh = String(now.getHours()).padStart(2, "0");
      const mm = String(now.getMinutes()).padStart(2, "0");
      return hh + ":" + mm;
    }

    function syncJarvisInputOverlay() {
      if (!jarvisInputAnchor || !jarvisInputRow) return;
      const rect = jarvisInputAnchor.getBoundingClientRect();
      const centerX = rect.left + rect.width / 2;
      const centerY = rect.top + rect.height / 2;
      const width = jarvisInputAnchor.offsetWidth || rect.width;
      const height = jarvisInputAnchor.offsetHeight || rect.height;
      jarvisInputRow.style.left = centerX + "px";
      jarvisInputRow.style.top = centerY + "px";
      jarvisInputRow.style.width = width + "px";
      jarvisInputRow.style.height = height + "px";
      jarvisInputRow.style.transform = "translate(-50%, -50%)";
    }

    const STATUS_CLASSES = ["status-good", "status-ok", "status-weak", "status-search", "status-fix"];
    function setStatusPill(el, text, cls) {
      if (!el) return;
      el.textContent = text || "--";
      el.classList.add("status-pill");
      STATUS_CLASSES.forEach((name) => el.classList.remove(name));
      if (cls) {
        el.classList.add(cls);
      }
    }

    function setSensory(levels) {
      const bars = sensory.querySelectorAll("span");
      bars.forEach((bar, idx) => {
        const level = levels && levels[idx] !== undefined ? levels[idx] : 0;
        bar.style.height = Math.round(Math.max(0.05, Math.min(1, level)) * 100) + "%";
      });
    }

    function renderLogs() {
      const list = document.getElementById("logs");
      if (!list) return;
      const wasAutoScroll = jarvisAutoScroll;
      const prevScrollTop = jarvisLogBox ? jarvisLogBox.scrollTop : 0;
      const prevScrollHeight = jarvisLogBox ? jarvisLogBox.scrollHeight : 0;
      list.innerHTML = "";
      const combined = [];
      (systemLogs || []).forEach((entry) => {
        combined.push({ type: "text", text: "SYS> " + entry });
      });
      chatLogs.forEach((entry) => {
        if (typeof entry === "string") {
          combined.push({ type: "text", text: entry });
        } else {
          combined.push(entry);
        }
      });
      combined.forEach((entry) => {
        const li = document.createElement("li");
        if (entry && entry.type === "image") {
          const wrap = document.createElement("div");
          wrap.className = "jarvis-media-thumb";
          wrap.title = "Expand snapshot";
          const img = document.createElement("img");
          img.src = entry.src || "";
          img.alt = entry.alt || "Snapshot";
          wrap.appendChild(img);
          wrap.addEventListener("click", () => {
            if (snapshotBlobUrl) {
              setSnapshotExpanded(true, "snapshot");
            }
          });
          li.appendChild(wrap);
        } else if (entry && entry.type === "clip") {
          const wrap = document.createElement("div");
          wrap.className = "jarvis-media-thumb";
          wrap.title = "Play clip";
          const img = document.createElement("img");
          img.src = entry.thumb || "";
          img.alt = entry.alt || "Clip";
          const badge = document.createElement("span");
          badge.className = "jarvis-media-badge";
          badge.textContent = "CLIP";
          wrap.appendChild(img);
          wrap.appendChild(badge);
          wrap.addEventListener("click", () => {
            if (entry.id && latestClip && latestClip.id === entry.id) {
              setSnapshotExpanded(true, "clip");
            }
          });
          li.appendChild(wrap);
        } else {
          li.textContent = entry && entry.text !== undefined ? entry.text : String(entry || "");
        }
        list.appendChild(li);
      });
      if (jarvisLogBox) {
        if (wasAutoScroll) {
          jarvisLogBox.scrollTop = jarvisLogBox.scrollHeight;
        } else {
          const newScrollHeight = jarvisLogBox.scrollHeight;
          const delta = newScrollHeight - prevScrollHeight;
          jarvisLogBox.scrollTop = prevScrollTop + (delta > 0 ? delta : 0);
        }
      }
    }

    function stopClipPlayback() {
      if (clipPlaybackTimer) {
        clearInterval(clipPlaybackTimer);
        clipPlaybackTimer = null;
      }
    }

    function playExpandedClip() {
      stopClipPlayback();
      if (!latestClip || !latestClip.frames || !latestClip.frames.length || !snapshotExpandedImg) return;
      expandedMediaType = "clip";
      expandedClipFrame = 0;
      snapshotExpandedImg.src = latestClip.frames[expandedClipFrame];
      const frameMs = latestClip.frameMs || 250;
      clipPlaybackTimer = setInterval(() => {
        if (!snapshotExpanded || expandedMediaType !== "clip" || !latestClip || !latestClip.frames.length) {
          stopClipPlayback();
          return;
        }
        expandedClipFrame = (expandedClipFrame + 1) % latestClip.frames.length;
        snapshotExpandedImg.src = latestClip.frames[expandedClipFrame];
      }, frameMs);
    }

    function setSnapshotExpanded(next, mediaType = "snapshot") {
      snapshotExpanded = !!next;
      if (!snapshotOverlay) return;
      snapshotOverlay.classList.toggle("active", snapshotExpanded);
      if (snapshotExpanded && snapshotExpandedImg) {
        expandedMediaType = mediaType;
        if (expandedMediaType === "clip" && latestClip && latestClip.frames && latestClip.frames.length) {
          resetSnapshotView();
          updateSnapshotTransform();
          playExpandedClip();
        } else if (snapshotBlobUrl) {
          expandedMediaType = "snapshot";
          stopClipPlayback();
          snapshotExpandedImg.src = snapshotBlobUrl;
          updateSnapshotTransform();
        } else {
          setSnapshotExpanded(false);
        }
      } else {
        stopClipPlayback();
        snapshotDragging = false;
        if (snapshotExpandedImg) {
          snapshotExpandedImg.classList.remove("dragging");
        }
      }
    }

    function resetSnapshotView() {
      snapshotRotation = 0;
      snapshotFlipX = 1;
      snapshotFlipY = 1;
      snapshotZoom = 1;
      snapshotPanX = 0;
      snapshotPanY = 0;
      snapshotDragging = false;
      if (snapshotExpandedImg) {
        snapshotExpandedImg.classList.remove("dragging");
      }
    }

    function updateSnapshotTransform() {
      if (!snapshotExpandedImg) return;
      snapshotExpandedImg.style.transform =
        "translate(" + snapshotPanX + "px, " + snapshotPanY + "px) " +
        "rotate(" + snapshotRotation + "deg) " +
        "scale(" + (snapshotFlipX * snapshotZoom) + "," + (snapshotFlipY * snapshotZoom) + ")";
    }

    function setSnapshotZoom(nextZoom) {
      snapshotZoom = Math.max(1, Math.min(8, nextZoom));
      if (snapshotZoom === 1) {
        snapshotPanX = 0;
        snapshotPanY = 0;
      }
      updateSnapshotTransform();
    }

    function expandedMediaAvailable() {
      if (!snapshotExpanded) return false;
      if (expandedMediaType === "clip") {
        return !!(latestClip && latestClip.frames && latestClip.frames.length);
      }
      return !!snapshotBlobUrl;
    }

    function expandedMediaLabel() {
      return expandedMediaType === "clip" ? "Clip" : "Snapshot";
    }

    async function sendMaskCommand(action, announce = true) {
      try {
        const res = await fetch("/mask?action=" + encodeURIComponent(action) + "&ts=" + Date.now(), { cache: "no-store" });
        if (!res.ok) throw new Error("mask_http_failed");
        const data = await res.json();
        if (!data || !data.ok) {
          if (announce && data && data.error === "pwm_driver_not_detected") {
            pushChat("jarvis", "Local mask command received, but the PCA9685 PWM driver is not detected.");
          } else if (announce) {
            pushChat("jarvis", "Local mask command received, but mask hardware is unavailable.");
          }
          return false;
        }
        if (announce) {
          if (data.state === "open") {
            pushChat("jarvis", "Mask off.");
          } else if (data.state === "closed") {
            pushChat("jarvis", "Mask on.");
          } else {
            pushChat("jarvis", "Mask moving.");
          }
        }
        return true;
      } catch (err) {
        if (announce) pushChat("jarvis", "Local mask endpoint unavailable.");
        return false;
      }
    }

    function executeHudAction(action, payload = {}, options = {}) {
      const announce = options && options.announce;
      const value = payload && payload.value !== undefined ? Number(payload.value) : NaN;

      if (action === "brightness" && Number.isFinite(value)) {
        setBrightness(value);
        if (brightness) brightness.value = Math.max(0, Math.min(100, value));
        if (announce) pushChat("jarvis", "Acknowledged. Brightness set to " + Math.round(value) + "%.");
        return true;
      }

      if (action === "hud_opacity" && Number.isFinite(value)) {
        setHudOpacity(value);
        if (hudOpacity) hudOpacity.value = Math.max(0, Math.min(100, value));
        if (announce) pushChat("jarvis", "Acknowledged. HUD opacity set to " + Math.round(value) + "%.");
        return true;
      }

      if (action === "widget_transparency" && Number.isFinite(value)) {
        setTransparency(value);
        if (transparency) transparency.value = Math.max(0, Math.min(100, value));
        if (announce) pushChat("jarvis", "Acknowledged. Widget transparency set to " + Math.round(value) + "%.");
        return true;
      }

      if (action === "volume" && Number.isFinite(value)) {
        setVolume(value, true);
        if (announce) pushChat("jarvis", "Acknowledged. Volume set to " + Math.round(value) + "%.");
        return true;
      }

      if (action === "mute") {
        setVolume(0, true);
        if (announce) pushChat("jarvis", "Acknowledged. Audio muted.");
        return true;
      }

      if (action === "snapshot_capture") {
        captureSnapshot({ announce: announce, showInChat: true });
        return true;
      }

      if (action === "clip_capture") {
        captureClip({ announce: announce, showInChat: true });
        return true;
      }

      if (action === "clip_play" || action === "clip_expand") {
        expandLastMedia(announce, true);
        return true;
      }

      if (action === "clip_minimize") {
        setSnapshotExpanded(false);
        if (announce) pushChat("jarvis", "Clip minimized.");
        return true;
      }

      if (action === "snapshot_analyze") {
        analyzeSnapshot();
        return true;
      }

      if (action === "snapshot_expand") {
        if (!snapshotBlobUrl) {
          if (announce) pushChat("jarvis", "No snapshot available.");
          return true;
        }
        setSnapshotExpanded(true);
        if (announce) pushChat("jarvis", "Snapshot expanded.");
        return true;
      }

      if (action === "snapshot_minimize") {
        setSnapshotExpanded(false);
        if (announce) pushChat("jarvis", "Media minimized.");
        return true;
      }

      if (action === "snapshot_rotate_left" || action === "snapshot_rotate_right") {
        if (!snapshotBlobUrl) {
          if (announce) pushChat("jarvis", "No snapshot available.");
          return true;
        }
        snapshotRotation = (snapshotRotation + (action === "snapshot_rotate_left" ? -90 : 90) + 360) % 360;
        updateSnapshotTransform();
        if (announce) pushChat("jarvis", "Snapshot rotated.");
        return true;
      }

      if (action === "snapshot_flip_horizontal" || action === "snapshot_flip_vertical") {
        if (!snapshotBlobUrl) {
          if (announce) pushChat("jarvis", "No snapshot available.");
          return true;
        }
        if (action === "snapshot_flip_vertical") {
          snapshotFlipY *= -1;
        } else {
          snapshotFlipX *= -1;
        }
        updateSnapshotTransform();
        if (announce) pushChat("jarvis", "Snapshot flipped.");
        return true;
      }

      if (action === "snapshot_reset") {
        if (!snapshotBlobUrl) {
          if (announce) pushChat("jarvis", "No snapshot available.");
          return true;
        }
        resetSnapshotView();
        updateSnapshotTransform();
        if (announce) pushChat("jarvis", "Snapshot reset.");
        return true;
      }

      if (action === "snapshot_zoom_in" || action === "snapshot_zoom_out") {
        if (!snapshotExpanded) {
          if (announce) pushChat("jarvis", "Expand the media before zooming.");
          return true;
        }
        if (!expandedMediaAvailable()) {
          if (announce) pushChat("jarvis", "No media available.");
          return true;
        }
        let nextZoom = action === "snapshot_zoom_in" ? snapshotZoom * 1.5 : snapshotZoom / 1.5;
        if (Number.isFinite(value) && value > 0) {
          nextZoom = value >= 10 ? value / 100 : value;
        }
        setSnapshotZoom(nextZoom);
        if (announce) pushChat("jarvis", action === "snapshot_zoom_in" ? expandedMediaLabel() + " zoomed in." : expandedMediaLabel() + " zoomed out.");
        return true;
      }

      if (action === "snapshot_zoom" && Number.isFinite(value) && value > 0) {
        if (!snapshotExpanded) {
          if (announce) pushChat("jarvis", "Expand the media before zooming.");
          return true;
        }
        if (!expandedMediaAvailable()) {
          if (announce) pushChat("jarvis", "No media available.");
          return true;
        }
        setSnapshotZoom(value >= 10 ? value / 100 : value);
        if (announce) pushChat("jarvis", expandedMediaLabel() + " zoom set.");
        return true;
      }

      if (action === "snapshot_delete_all") {
        const hadSnapshots = chatLogs.some((entry) => isMediaEntry(entry)) || !!snapshotBlobUrl || !!latestClip;
        clearSnapshotImages();
        setSnapshotExpanded(false);
        if (announce) pushChat("jarvis", hadSnapshots ? "All media deleted." : "No media to delete.");
        return true;
      }

      if (action === "snapshot_delete_last") {
        const removed = removeLastSnapshotImage();
        if (!removed) {
          if (announce) pushChat("jarvis", "No media to delete.");
          return true;
        }
        if (announce) pushChat("jarvis", "Last media deleted.");
        return true;
      }

      if (action === "camera_on" || action === "camera_off") {
        if (!setCameraState) {
          if (announce) pushChat("jarvis", "Camera control unavailable.");
          return true;
        }
        setCameraState(action === "camera_on", announce);
        return true;
      }

      if (action === "filter_invert_on") {
        setActiveFilter("invert");
        if (announce) pushChat("jarvis", "Inverted filter engaged.");
        return true;
      }

      if (action === "filter_invert_off" || action === "filter_clear") {
        setActiveFilter("none");
        if (announce) pushChat("jarvis", "Inverted filter disengaged.");
        return true;
      }

      if (action === "filter_bw_on") {
        setActiveFilter("bw");
        if (announce) pushChat("jarvis", "Monochrome filter engaged.");
        return true;
      }

      if (action === "filter_bw_off") {
        setActiveFilter("none");
        if (announce) pushChat("jarvis", "Monochrome filter disengaged.");
        return true;
      }

      if (action === "mask_on" || action === "mask_close" || action === "mask_down") {
        sendMaskCommand("close", announce);
        return true;
      }

      if (action === "mask_off" || action === "mask_open" || action === "mask_up") {
        sendMaskCommand("open", announce);
        return true;
      }

      if (action === "mask_toggle") {
        sendMaskCommand("toggle", announce);
        return true;
      }

      return false;
    }

    function applyHudActionsFromResponse(text) {
      const source = text || "";
      const commands = [];
      const cleaned = source.replace(/<hud>\s*(\{[\s\S]*?\})\s*<\/hud>/gi, (full, jsonText) => {
        try {
          const payload = JSON.parse(jsonText);
          commands.push(payload);
        } catch (err) {
          // Ignore malformed command tags and preserve visible text.
          return full;
        }
        return "";
      }).trim();

      let executed = false;
      commands.forEach((payload) => {
        if (payload && payload.action) {
          executed = executeHudAction(payload.action, payload, { announce: false }) || executed;
        }
      });
      return { text: cleaned, executed };
    }

    function blobToDataUrl(blob) {
      return new Promise((resolve) => {
        const reader = new FileReader();
        reader.onloadend = () => resolve(reader.result || "");
        reader.readAsDataURL(blob);
      });
    }

    async function captureSnapshot(options = {}) {
      const { announce = true, showInChat = true } = options;
      try {
        const res = await fetch("/snapshot?ts=" + Date.now());
        if (!res.ok) {
          throw new Error("snapshot_failed");
        }
        const blob = await res.blob();
        const dataUrl = await blobToDataUrl(blob);
        if (snapshotBlobUrl) {
          URL.revokeObjectURL(snapshotBlobUrl);
        }
        snapshotBlobUrl = URL.createObjectURL(blob);
        snapshotDataUrl = dataUrl;
        if (snapshotExpandedImg) {
          snapshotExpandedImg.src = snapshotBlobUrl;
        }
        resetSnapshotView();
        updateSnapshotTransform();
        if (announce) {
          pushChat("jarvis", "Snapshot captured.");
        }
        if (showInChat) {
          pushChatImage(snapshotBlobUrl);
        }
        return true;
      } catch (err) {
        pushChat("jarvis", "Snapshot failed.");
        return false;
      }
    }

    function getClipCanvas() {
      if (!clipCanvas) {
        clipCanvas = document.createElement("canvas");
        clipCanvasCtx = clipCanvas.getContext("2d", { willReadFrequently: false });
      }
      return clipCanvasCtx ? clipCanvas : null;
    }

    function waitMs(ms) {
      return new Promise((resolve) => setTimeout(resolve, ms));
    }

    const CLIP_BUFFER_MS = 5600;
    const CLIP_TARGET_MS = 5000;
    const CLIP_SAMPLE_MS = 250;
    const CLIP_MIN_DURATION_MS = 3800;

    function pruneClipFrames() {
      const cutoff = Date.now() - CLIP_BUFFER_MS;
      clipFrameBuffer = clipFrameBuffer.filter((frame) => frame.t >= cutoff);
    }

    function addClipFrame(src, capturedAt = Date.now()) {
      if (!src) return;
      clipFrameBuffer.push({ t: capturedAt, src: src });
      lastClipFrameAt = capturedAt;
      pruneClipFrames();
    }

    function recentClipFrames() {
      const now = Date.now();
      return clipFrameBuffer.filter((frame) => frame.t >= now - CLIP_TARGET_MS);
    }

    function clipDurationMs(frames) {
      if (!frames || frames.length < 2) return 0;
      return Math.max(0, frames[frames.length - 1].t - frames[0].t);
    }

    function appendBytes(a, b) {
      if (!a || !a.length) return b;
      const next = new Uint8Array(a.length + b.length);
      next.set(a, 0);
      next.set(b, a.length);
      return next;
    }

    function findJpegMarker(bytes, marker, start = 0) {
      for (let i = Math.max(0, start); i < bytes.length - 1; i++) {
        if (bytes[i] === 0xff && bytes[i + 1] === marker) return i;
      }
      return -1;
    }

    async function addClipJpegBytes(bytes, capturedAt) {
      try {
        const dataUrl = await blobToDataUrl(new Blob([bytes], { type: "image/jpeg" }));
        addClipFrame(dataUrl, capturedAt);
      } catch (err) {
        // A bad frame should not stop the rolling clip recorder.
      }
    }

    function clipStreamUrl() {
      const base = stream && (stream.dataset.src || stream.src) ? (stream.dataset.src || stream.src) : streamBase;
      if (!base) return "";
      return base + (base.includes("?") ? "&" : "?") + "clip_feed=" + Date.now();
    }

    async function startClipStreamRecorder() {
      if (clipStreamStarting) return;
      if (!stream || stream.style.display === "none") return;
      const url = clipStreamUrl();
      if (!url || !window.fetch || !window.ReadableStream) return;
      if (clipStreamAbort) {
        try {
          clipStreamAbort.abort();
        } catch (err) {
          // Ignore abort errors.
        }
      }
      clipStreamStarting = true;
      clipUseSnapshotFallback = false;
      const controller = new AbortController();
      clipStreamAbort = controller;
      try {
        const res = await fetch(url, { cache: "no-store", mode: "cors", signal: controller.signal });
        if (!res.ok || !res.body) {
          throw new Error("clip_stream_unavailable");
        }
        const reader = res.body.getReader();
        let buffer = new Uint8Array(0);
        while (!controller.signal.aborted) {
          const { value, done } = await reader.read();
          if (done) break;
          if (!value || !value.length) continue;
          buffer = appendBytes(buffer, value);
          while (buffer.length > 4) {
            const start = findJpegMarker(buffer, 0xd8, 0);
            if (start < 0) {
              buffer = buffer.slice(Math.max(0, buffer.length - 2));
              break;
            }
            if (start > 0) {
              buffer = buffer.slice(start);
            }
            const end = findJpegMarker(buffer, 0xd9, 2);
            if (end < 0) {
              if (buffer.length > 900000) {
                buffer = buffer.slice(Math.max(0, buffer.length - 2));
              }
              break;
            }
            const capturedAt = Date.now();
            const frame = buffer.slice(0, end + 2);
            buffer = buffer.slice(end + 2);
            if (capturedAt - lastClipFrameAt >= CLIP_SAMPLE_MS) {
              addClipJpegBytes(frame, capturedAt);
            }
          }
        }
      } catch (err) {
        clipUseSnapshotFallback = true;
      } finally {
        clipStreamStarting = false;
        if (clipStreamAbort === controller) {
          clipStreamAbort = null;
        }
      }
    }

    async function captureClipFrameFromSnapshot() {
      const now = Date.now();
      if (now - lastClipSnapshotFallbackAt < 900) return;
      lastClipSnapshotFallbackAt = now;
      try {
        const res = await fetch("/snapshot?clip=1&ts=" + Date.now(), { cache: "no-store" });
        if (!res.ok) {
          throw new Error("clip_snapshot_failed");
        }
        const blob = await res.blob();
        const dataUrl = await blobToDataUrl(blob);
        addClipFrame(dataUrl);
      } catch (err) {
        // Keep the rolling buffer quiet; the user-facing clip command reports availability.
      }
    }

    async function captureClipFrame() {
      if (document.hidden || !stream || stream.style.display === "none") return;
      if (clipFrameBusy) return;
      clipFrameBusy = true;
      try {
        if (!clipUseSnapshotFallback) {
          const sourceWidth = stream.naturalWidth || stream.videoWidth || 0;
          const sourceHeight = stream.naturalHeight || stream.videoHeight || 0;
          if (sourceWidth && sourceHeight) {
            const canvas = getClipCanvas();
            if (canvas && clipCanvasCtx) {
              const targetWidth = 480;
              const targetHeight = Math.max(1, Math.round(targetWidth * sourceHeight / sourceWidth));
              if (canvas.width !== targetWidth || canvas.height !== targetHeight) {
                canvas.width = targetWidth;
                canvas.height = targetHeight;
              }
              try {
                clipCanvasCtx.drawImage(stream, 0, 0, targetWidth, targetHeight);
                addClipFrame(canvas.toDataURL("image/jpeg", 0.72));
                return;
              } catch (err) {
                clipUseSnapshotFallback = true;
              }
            }
          }
        }
        await captureClipFrameFromSnapshot();
      } finally {
        clipFrameBusy = false;
      }
    }

    function startClipBuffer() {
      if (clipTimer) {
        clearInterval(clipTimer);
        clipTimer = null;
      }
      if (clipStreamAbort) {
        try {
          clipStreamAbort.abort();
        } catch (err) {
          // Ignore abort errors.
        }
        clipStreamAbort = null;
      }
      clipStreamStarting = false;
      clipFrameBuffer = [];
      lastClipFrameAt = 0;
    }

    async function fetchDeviceClip() {
      const res = await fetch("/clip?ts=" + Date.now(), { cache: "no-store" });
      if (!res.ok) {
        throw new Error("clip_fetch_failed");
      }
      const data = await res.json();
      if (!data || !data.ok || !Array.isArray(data.frames)) {
        return null;
      }
      const frames = data.frames.filter((frame) => typeof frame === "string" && frame.length > 24);
      if (frames.length < 3) {
        return null;
      }
      return {
        frames: frames,
        frameMs: Math.max(80, Number(data.frame_ms) || CLIP_SAMPLE_MS)
      };
    }

    async function warmClipBuffer(minDurationMs = CLIP_MIN_DURATION_MS) {
      const deadline = Date.now() + Math.min(4500, minDurationMs + 800);
      let frames = recentClipFrames();
      while ((frames.length < 4 || clipDurationMs(frames) < minDurationMs) && Date.now() < deadline) {
        await captureClipFrame();
        await waitMs(180);
        frames = recentClipFrames();
      }
      return frames;
    }

    async function captureClip(options = {}) {
      const { announce = true, showInChat = true } = options;
      lastMediaIntent = "clip";
      let deviceClip = null;
      try {
        deviceClip = await fetchDeviceClip();
      } catch (err) {
        deviceClip = null;
      }
      if (!deviceClip || !deviceClip.frames || deviceClip.frames.length < 3) {
        if (announce) {
          pushChat("jarvis", "Clip unavailable. Keep the camera on for a few seconds first.");
        }
        return false;
      }
      const durationMs = Math.max(0, (deviceClip.frames.length - 1) * deviceClip.frameMs);
      latestClip = {
        id: "clip-" + (++clipSeq),
        frames: deviceClip.frames,
        thumb: deviceClip.frames[deviceClip.frames.length - 1],
        frameMs: deviceClip.frameMs,
        durationMs: durationMs
      };
      if (announce) {
        pushChat("jarvis", "Five second clip captured.");
      }
      if (showInChat) {
        pushChatClip(latestClip);
      }
      return true;
    }

    async function captureVisionFrame() {
      if (visionCaptureBusy) return;
      visionCaptureBusy = true;
      try {
        const res = await fetch("/snapshot?ts=" + Date.now());
        if (!res.ok) {
          throw new Error("vision_snapshot_failed");
        }
        const blob = await res.blob();
        const dataUrl = await blobToDataUrl(blob);
        if (visionBlobUrl) {
          URL.revokeObjectURL(visionBlobUrl);
        }
        visionBlobUrl = URL.createObjectURL(blob);
        visionDataUrl = dataUrl;
      } catch (err) {
        // Ignore capture errors to keep UI responsive.
      } finally {
        visionCaptureBusy = false;
      }
    }

    function startVisionTicker() {
      if (visionTimer) {
        clearInterval(visionTimer);
      }
      captureVisionFrame();
      visionTimer = setInterval(() => {
        if (document.hidden) return;
        if (stream && stream.style.display === "none") return;
        captureVisionFrame();
      }, 5000);
    }

    async function analyzeSnapshot() {
      if (snapshotExpanded && expandedMediaType === "clip" && latestClip && latestClip.frames && latestClip.frames.length) {
        const frame = latestClip.frames[Math.min(expandedClipFrame, latestClip.frames.length - 1)];
        if (frame) {
          const sent = sendMimiclawPayload({
            type: "vision",
            content: "Describe this frame from the enlarged video clip.",
            image: frame,
            chat_id: "hud"
          });
          if (sent) {
            pushChat("jarvis", "Analyzing clip frame.");
            return;
          }
          pushChat("jarvis", "Clip analysis unavailable.");
          return;
        }
      }

      if (snapshotExpanded && snapshotBlobUrl) {
        if (!snapshotDataUrl) {
          try {
            const blob = await fetch(snapshotBlobUrl).then((r) => r.blob());
            snapshotDataUrl = await blobToDataUrl(blob);
          } catch (err) {
            snapshotDataUrl = "";
          }
        }
        if (snapshotDataUrl) {
          const sent = sendMimiclawPayload({
            type: "vision",
            content: "Describe this snapshot image.",
            image: snapshotDataUrl,
            chat_id: "hud"
          });
          if (sent) {
            pushChat("jarvis", "Analyzing snapshot.");
            return;
          }
          pushChat("jarvis", "Snapshot analysis unavailable.");
          return;
        }
      }

      await captureVisionFrame();
      if (visionDataUrl) {
        const sent = sendMimiclawPayload({
          type: "vision",
          content: "Describe what is currently visible in the camera view.",
          image: visionDataUrl,
          chat_id: "hud"
        });
        if (sent) {
          pushChat("jarvis", "Analyzing current view.");
          return;
        }
      }
      try {
        const res = await fetch("/ai");
        if (!res.ok) {
          throw new Error("ai_unavailable");
        }
        const data = await res.json();
        if (!data || !data.ok) {
          pushChat("jarvis", "Analysis unavailable.");
          return;
        }
        const labels = [];
        if (Array.isArray(data.classes) && data.classes.length) {
          data.classes.forEach((cls) => {
            labels.push(labelForClass(cls.target, cls.score));
          });
        } else if (Array.isArray(data.boxes) && data.boxes.length) {
          labels.push("Targets detected.");
        }
        if (labels.length) {
          pushChat("jarvis", "Analysis: " + labels.join(", ") + ".");
        } else {
          pushChat("jarvis", "Analysis: no notable targets detected.");
        }
      } catch (err) {
        pushChat("jarvis", "Analysis unavailable.");
      }
    }

    syncJarvisInputOverlay();
    window.addEventListener("resize", () => {
      syncJarvisInputOverlay();
    });

    const jarvisSpeechQueue = [];

    function delayMs(ms) {
      return new Promise((resolve) => setTimeout(resolve, ms));
    }

    async function speakJarvis(text) {
      if (!text) return false;
      for (let attempt = 0; attempt < 24; attempt++) {
        try {
          const res = await fetch("/speak", {
            method: "POST",
            headers: { "Content-Type": "text/plain" },
            body: text
          });
          if (res.ok) {
            micHoldoffUntil = Date.now() + 800;
            return true;
          }
          let payload = null;
          try {
            payload = await res.json();
          } catch (parseErr) {
            payload = null;
          }
          if (!payload || (payload.error !== "voice_busy" && payload.error !== "tts_queue_full")) {
            return false;
          }
        } catch (err) {
          return false;
        }
        await delayMs(180);
      }
      return false;
    }

    async function flushJarvisSpeechQueue() {
      if (ttsBusy || !jarvisSpeechQueue.length) return;
      ttsBusy = true;
      try {
        while (jarvisSpeechQueue.length) {
          const next = jarvisSpeechQueue.shift();
          if (!next) continue;
          await speakJarvis(next);
        }
      } finally {
        ttsBusy = false;
        if (jarvisSpeechQueue.length) {
          void flushJarvisSpeechQueue();
        }
      }
    }

    function queueJarvisSpeech(text) {
      if (!text) return;
      jarvisSpeechQueue.push(text);
      void flushJarvisSpeechQueue();
    }

    function pushChat(role, text) {
      if (!text) return;
      const label = role === "user" ? "YOU> " : "JARVIS> ";
      chatLogs.push({ type: "text", text: label + text });
      jarvisAutoScroll = true;
      renderLogs();
      if (role === "jarvis") {
        queueJarvisSpeech(text);
      }
    }

    function pushChatImage(src, altText = "Snapshot") {
      if (!src) return;
      lastMediaIntent = "snapshot";
      chatLogs.push({ type: "image", src, alt: altText });
      jarvisAutoScroll = true;
      renderLogs();
    }

    function pushChatClip(clip) {
      if (!clip || !clip.thumb || !clip.id) return;
      lastMediaIntent = "clip";
      chatLogs.push({ type: "clip", id: clip.id, thumb: clip.thumb, alt: "Five second clip" });
      jarvisAutoScroll = true;
      renderLogs();
    }

    function isMediaEntry(entry) {
      return !!(entry && (entry.type === "image" || entry.type === "clip"));
    }

    function clearCurrentSnapshot() {
      if (snapshotBlobUrl) {
        URL.revokeObjectURL(snapshotBlobUrl);
        snapshotBlobUrl = "";
      }
      snapshotDataUrl = "";
      resetSnapshotView();
      if (expandedMediaType === "snapshot") {
        setSnapshotExpanded(false);
        if (snapshotExpandedImg) {
          snapshotExpandedImg.src = "";
        }
      }
    }

    function clearCurrentClip() {
      latestClip = null;
      expandedClipFrame = 0;
      if (expandedMediaType === "clip") {
        setSnapshotExpanded(false);
      }
    }

    function clearSnapshotImages() {
      for (let i = chatLogs.length - 1; i >= 0; i--) {
        if (isMediaEntry(chatLogs[i])) {
          chatLogs.splice(i, 1);
        }
      }
      clearCurrentSnapshot();
      clearCurrentClip();
      renderLogs();
    }

    function removeLastSnapshotImage() {
      for (let i = chatLogs.length - 1; i >= 0; i--) {
        if (isMediaEntry(chatLogs[i])) {
          const removed = chatLogs.splice(i, 1)[0];
          if (removed.type === "clip") {
            clearCurrentClip();
          } else {
            clearCurrentSnapshot();
          }
          renderLogs();
          return true;
        }
      }
      return false;
    }

    function getLastMediaType() {
      for (let i = chatLogs.length - 1; i >= 0; i--) {
        if (isMediaEntry(chatLogs[i])) {
          return chatLogs[i].type;
        }
      }
      if (latestClip) return "clip";
      if (snapshotBlobUrl) return "image";
      return "";
    }

    function expandLastMedia(announce = true, preferClip = false) {
      if (preferClip && !latestClip) {
        if (announce) pushChat("jarvis", "No clip available.");
        return true;
      }
      const mediaType = preferClip && latestClip ? "clip" : getLastMediaType();
      if (mediaType === "clip" && latestClip) {
        setSnapshotExpanded(true, "clip");
        if (announce) pushChat("jarvis", "Clip playing.");
        return true;
      }
      if (snapshotBlobUrl) {
        setSnapshotExpanded(true, "snapshot");
        if (announce) pushChat("jarvis", "Snapshot expanded.");
        return true;
      }
      if (announce) pushChat("jarvis", "No media available.");
      return true;
    }

    function logWindowsEqual(a, b) {
      if (a.length !== b.length) return false;
      for (let i = 0; i < a.length; i++) {
        if (a[i] !== b[i]) return false;
      }
      return true;
    }

    function findLogWindowOverlap(previous, next) {
      const max = Math.min(previous.length, next.length);
      for (let len = max; len > 0; len--) {
        let match = true;
        for (let i = 0; i < len; i++) {
          if (previous[previous.length - len + i] !== next[i]) {
            match = false;
            break;
          }
        }
        if (match) return len;
      }
      return 0;
    }

    function setLogs(items) {
      const incoming = Array.isArray(items)
        ? items.filter((entry) => entry !== undefined && entry !== null && entry !== "")
        : [];
      if (!incoming.length) {
        lastSystemLogSnapshot = [];
        return;
      }
      if (logWindowsEqual(lastSystemLogSnapshot, incoming)) {
        return;
      }
      const overlap = findLogWindowOverlap(lastSystemLogSnapshot, incoming);
      const additions = incoming.slice(overlap);
      lastSystemLogSnapshot = incoming.slice();
      if (!additions.length) {
        return;
      }
      systemLogs.push(...additions);
      renderLogs();
    }

    function initHostBattery() {
      if (!navigator.getBattery) return;
      navigator.getBattery()
        .then((battery) => {
          const updateBattery = () => {
            hostBatteryLevel = Number.isFinite(battery.level) ? battery.level : null;
            hostBatteryCharging = typeof battery.charging === "boolean" ? battery.charging : null;
          };
          updateBattery();
          battery.addEventListener("levelchange", updateBattery);
          battery.addEventListener("chargingchange", updateBattery);
        })
        .catch(() => {});
    }

    initHostBattery();

    function getPowerStatus(data) {
      const minPct = 20;
      const devicePct = data && Number.isFinite(Number(data.power_pct)) ? Number(data.power_pct) : -1;
      if (devicePct >= 0) {
        return devicePct >= minPct
          ? { label: "SUFFICIENT", cls: "status-good" }
          : { label: "INSUFFICIENT", cls: "status-weak" };
      }
      if (hostBatteryCharging === true) {
        return { label: "SUFFICIENT", cls: "status-good" };
      }
      if (Number.isFinite(hostBatteryLevel)) {
        return hostBatteryLevel * 100 >= minPct
          ? { label: "SUFFICIENT", cls: "status-good" }
          : { label: "INSUFFICIENT", cls: "status-weak" };
      }
      return { label: "SUFFICIENT", cls: "status-good" };
    }

    function getConnectionStatus(data) {
      if (!data) {
        return { label: "--", cls: "" };
      }
      if (data.link === "AP_MODE") {
        const clients = data.ap_clients !== undefined ? data.ap_clients : 0;
        if (clients > 0) {
          return { label: "VIABLE", cls: "status-ok" };
        }
        return { label: "UNSTABLE", cls: "status-weak" };
      }
      const quality = data.wifi_quality !== undefined ? data.wifi_quality : 0;
      if (quality >= 70) {
        return { label: "STABLE", cls: "status-good" };
      }
      if (quality >= 40) {
        return { label: "VIABLE", cls: "status-ok" };
      }
      return { label: "UNSTABLE", cls: "status-weak" };
    }

    function labelFromList(list, target) {
      if (!Array.isArray(list)) return "--";
      if (target === undefined || target === null) return "--";
      if (target < 0 || target >= list.length) return "--";
      return list[target] || "--";
    }

    function withScore(text, score) {
      if (score !== undefined && score !== null) {
        return text + " " + Math.round(score * 100) + "%";
      }
      return text;
    }

    function labelForBox(target, score) {
      if (AI_PROFILE === "face") return withScore("FACE", score);
      if (AI_PROFILE === "hand") return withScore("HAND", score);
      if (AI_PROFILE === "person") return withScore("PERSON", score);
      if (AI_PROFILE === "gender") return withScore("GENDER: " + labelFromList(AI_LABELS.gender, target), score);
      if (AI_PROFILE === "age") return withScore("AGE: " + labelFromList(AI_LABELS.age, target), score);
      if (AI_PROFILE === "emotion") return withScore("EMOTION: " + labelFromList(AI_LABELS.emotion, target), score);
      return withScore("PERSON", score);
    }

    function labelForClass(target, score) {
      if (AI_PROFILE === "gender") return withScore("GENDER: " + labelFromList(AI_LABELS.gender, target), score);
      if (AI_PROFILE === "age") return withScore("AGE: " + labelFromList(AI_LABELS.age, target), score);
      if (AI_PROFILE === "emotion") return withScore("EMOTION: " + labelFromList(AI_LABELS.emotion, target), score);
      if (AI_PROFILE === "face") return withScore("FACE", score);
      if (AI_PROFILE === "hand") return withScore("HAND", score);
      if (AI_PROFILE === "person") return withScore("PERSON", score);
      return withScore("PERSON", score);
    }

    function labelForPoint(target, score) {
      if (AI_PROFILE === "hand") return withScore("HAND", score);
      if (AI_PROFILE === "face") return withScore("FACE", score);
      if (AI_PROFILE === "person") return withScore("PERSON", score);
      return withScore("POINT", score);
    }

    function renderAiOverlay(data) {
      if (!aiOverlay) return;
      aiOverlay.innerHTML = "";
      if (!data || !data.ok) {
        aiOverlay.classList.remove("active");
        return;
      }
      const rect = aiOverlay.getBoundingClientRect();
      const width = rect.width || 1;
      const height = rect.height || 1;
      const frameW = data.frame_w || width;
      const frameH = data.frame_h || height;
      const dets = [];

      if (Array.isArray(data.boxes)) {
        data.boxes.forEach((b) => {
          const norm = Math.abs(b.x) <= 1.2 && Math.abs(b.y) <= 1.2 && b.w <= 1.5 && b.h <= 1.5;
          const cx = norm ? (b.x + b.w / 2) * width : (b.x + b.w / 2) * (width / frameW);
          const cy = norm ? (b.y + b.h / 2) * height : (b.y + b.h / 2) * (height / frameH);
          const r = norm ? Math.max(b.w, b.h) * 0.5 * Math.min(width, height) :
            Math.max(b.w, b.h) * 0.5 * (Math.min(width, height) / Math.min(frameW, frameH));
          dets.push({ x: cx, y: cy, r, label: labelForBox(b.target, b.score) });
        });
      }

      if (Array.isArray(data.points)) {
        data.points.forEach((p) => {
          const norm = Math.abs(p.x) <= 1.2 && Math.abs(p.y) <= 1.2;
          const cx = norm ? p.x * width : p.x * (width / frameW);
          const cy = norm ? p.y * height : p.y * (height / frameH);
          const r = Math.max(10, Math.min(width, height) * 0.02);
          dets.push({ x: cx, y: cy, r, label: labelForPoint(p.target, p.score) });
        });
      }

      if (Array.isArray(data.keypoints)) {
        data.keypoints.forEach((kp) => {
          if (kp.box) {
            const b = kp.box;
            const norm = Math.abs(b.x) <= 1.2 && Math.abs(b.y) <= 1.2 && b.w <= 1.5 && b.h <= 1.5;
            const cx = norm ? (b.x + b.w / 2) * width : (b.x + b.w / 2) * (width / frameW);
            const cy = norm ? (b.y + b.h / 2) * height : (b.y + b.h / 2) * (height / frameH);
            const r = norm ? Math.max(b.w, b.h) * 0.5 * Math.min(width, height) :
              Math.max(b.w, b.h) * 0.5 * (Math.min(width, height) / Math.min(frameW, frameH));
            dets.push({ x: cx, y: cy, r, label: labelForBox(b.target, b.score) });
          }
          if (Array.isArray(kp.points)) {
            kp.points.forEach((p) => {
              const norm = Math.abs(p.x) <= 1.2 && Math.abs(p.y) <= 1.2;
              const cx = norm ? p.x * width : p.x * (width / frameW);
              const cy = norm ? p.y * height : p.y * (height / frameH);
              const r = Math.max(8, Math.min(width, height) * 0.015);
              dets.push({ x: cx, y: cy, r, label: labelForPoint(p.target, p.score) });
            });
          }
        });
      }

      if (Array.isArray(data.classes)) {
        data.classes.forEach((c, idx) => {
          const cx = width * 0.08;
          const cy = height * (0.12 + idx * 0.09);
          const r = Math.max(14, Math.min(width, height) * 0.035);
          dets.push({ x: cx, y: cy, r, label: labelForClass(c.target, c.score) });
        });
      }

      if (dets.length === 0) {
        aiOverlay.classList.remove("active");
        return;
      }

      aiOverlay.classList.add("active");
      dets.forEach((d) => {
        const size = Math.max(18, d.r * 2);
        const el = document.createElement("div");
        el.className = "ai-detection";
        el.style.width = size + "px";
        el.style.height = size + "px";
        el.style.left = (d.x - size / 2) + "px";
        el.style.top = (d.y - size / 2) + "px";
        const label = document.createElement("div");
        label.className = "ai-label";
        label.textContent = d.label;
        el.appendChild(label);
        aiOverlay.appendChild(el);
      });
    }

    let lastAiTs = 0;
    async function fetchAi() {
      try {
        const res = await fetch("/ai");
        if (!res.ok) return;
        const data = await res.json();
        if (data && data.ts !== undefined && data.ts === lastAiTs) {
          return;
        }
        if (data && data.ts !== undefined) {
          lastAiTs = data.ts;
        }
        renderAiOverlay(data);
      } catch (err) {
        // Ignore transient AI overlay errors
      }
    }

    async function fetchStatus() {
      try {
        const res = await fetch("/status");
        if (!res.ok) return;
        const data = await res.json();
        lastStatus = data;
        updateStreamBaseFromStatus(data);
        if (!mimicUrl && data.voice_host) {
          const voicePort = data.voice_port || 8000;
          mimicUrl = "ws://" + data.voice_host + ":" + voicePort + "/ws";
          setStatusLine("MIMICLAW: DISCOVERED");
          connectMimiclaw();
        }

        setText("time", data.time && data.time !== "--:--" ? data.time : browserTimeString());
        setText("link", data.link);
        if (netMode) {
          const modeLabel = data.link === "AP_MODE" ? "AP MODE" : "WIFI";
          netMode.textContent = "MODE: " + modeLabel;
          netMode.classList.toggle("active", data.link === "AP_MODE");
        }

        renderLocation(data);
        setText("mph-val", data.nav && data.nav.speed_mph >= 0 ? data.nav.speed_mph.toFixed(1) + " MPH" : "--");
        setText("ms-val", data.nav && data.nav.speed_ms >= 0 ? data.nav.speed_ms.toFixed(2) + " M/S" : "--");
        setBar("mph-bar", data.nav && data.nav.speed_mph >= 0 ? Math.min(100, data.nav.speed_mph) : 0);
        setBar("ms-bar", data.nav && data.nav.speed_ms >= 0 ? Math.min(100, data.nav.speed_ms) : 0);
        if (gnssStatus) {
          if (data.nav && data.nav.fix !== undefined) {
            if (data.nav.fix) {
              setStatusPill(gnssStatus, "LOCKED", "status-fix");
            } else if (data.nav.serial && data.nav.signal === false) {
              setStatusPill(gnssStatus, "CHECK GNSS", "status-search");
            } else if (data.nav.signal === false) {
              setStatusPill(gnssStatus, "NO DATA", "status-search");
            } else {
              setStatusPill(gnssStatus, "NO FIX", "status-search");
            }
          } else {
            setStatusPill(gnssStatus, "--", "");
          }
        }
        setText("temp", data.atmos && data.atmos.temp_c !== -1 ? data.atmos.temp_c.toFixed(1) + "C" : "--");
        const power = getPowerStatus(data);
        setStatusPill(powerStatus, power.label, power.cls);
        const conn = getConnectionStatus(data);
        setStatusPill(connStatus, conn.label, conn.cls);

        setSensory(data.levels);
        setLogs(data.logs);
        if (micLevel) {
          const level = data.mic_level !== undefined ? Math.round(data.mic_level * 100) : null;
          currentMicLevel = level !== null ? level + "%" : "--";
          micLevel.textContent = currentMicLevel;
        }
        if (data.volume !== undefined && !volumeDragging) {
          const now = Date.now();
          const recentSet = lastVolumeSetAt && (now - lastVolumeSetAt) < 4000;
          if (!recentSet || lastVolumeSetValue === null || Math.abs(data.volume - lastVolumeSetValue) <= 1) {
            setVolume(data.volume, false);
          }
        }
        if (data.voice_seq !== undefined && data.voice_seq !== lastVoiceSeq) {
          lastVoiceSeq = data.voice_seq;
          if (data.voice_text) {
            handleMicTranscript(data.voice_text);
          }
        }
        syncJarvisInputOverlay();
      } catch (err) {
        // Ignore transient errors
      }
    }

    fetchStatus();
    setInterval(fetchStatus, 1000);
    setInterval(() => {
      if (!lastStatus || !lastStatus.time || lastStatus.time === "--:--") {
        setText("time", browserTimeString());
      }
    }, 1000);
    fetchAi();
    setInterval(fetchAi, 350);

    function applyStreamFilters() {
      invertOn = activeFilter === "invert";
      bwOn = activeFilter === "bw";
      rootStyle.setProperty("--stream-invert", invertOn ? "1" : "0");
      const preset = streamDefaults;
      const gray = bwOn ? "1" : "0";
      rootStyle.setProperty("--stream-gray", gray);
      rootStyle.setProperty("--stream-contrast", preset.contrast);
      rootStyle.setProperty("--stream-saturate", preset.saturate);
      rootStyle.setProperty("--stream-brightness", preset.brightness);
      rootStyle.setProperty("--stream-hue", preset.hue);
      rootStyle.setProperty("--stream-opacity", preset.opacity || "1");
      rootStyle.setProperty("--stream-glow-1", preset.glow1);
      rootStyle.setProperty("--stream-glow-2", preset.glow2);
      rootStyle.setProperty("--stream-glow-color-1", preset.glowColor1);
      rootStyle.setProperty("--stream-glow-color-2", preset.glowColor2);
      rootStyle.setProperty("--edge-opacity", preset.edgeOpacity || "0");
      rootStyle.setProperty("--edge-contrast", preset.edgeContrast || "2.2");
      rootStyle.setProperty("--edge-brightness", preset.edgeBrightness || "1.1");
      rootStyle.setProperty("--edge-saturate", preset.edgeSaturate || "4");
      rootStyle.setProperty("--edge-hue", preset.edgeHue || "190deg");
      rootStyle.setProperty("--edge-blur", preset.edgeBlur || "0.5px");
      rootStyle.setProperty("--edge-glow-1", preset.edgeGlow1 || "4px");
      rootStyle.setProperty("--edge-glow-2", preset.edgeGlow2 || "12px");
      rootStyle.setProperty("--edge-glow-color-1", preset.edgeGlowColor1 || "rgba(120, 220, 255, 0.8)");
      rootStyle.setProperty("--edge-glow-color-2", preset.edgeGlowColor2 || "rgba(50, 140, 255, 0.5)");
      document.body.classList.remove("holo-on");
    }

    function setToggleState(button, on, label) {
      if (!button) return;
      button.classList.toggle("active", on);
      button.textContent = label + (on ? ": ON" : ": OFF");
    }

    function setActiveFilter(next) {
      activeFilter = next;
      applyStreamFilters();
      setToggleState(invertToggle, activeFilter === "invert", "VIDEO_INVERT");
      setToggleState(bwToggle, activeFilter === "bw", "BLACK_WHITE");
    }

    function toggleFilter(name) {
      setActiveFilter(activeFilter === name ? "none" : name);
    }

    setActiveFilter("none");

    function normalizeText(text) {
      return (text || "").trim().toLowerCase();
    }

    function getCurrentHudTime() {
      const statusTime = lastStatus && typeof lastStatus.time === "string" ? lastStatus.time.trim() : "";
      if (statusTime && statusTime !== "--:--") return statusTime;
      const timeEl = document.getElementById("time");
      const hudTime = timeEl && timeEl.textContent ? timeEl.textContent.trim() : "";
      if (hudTime && hudTime !== "--:--") return hudTime;
      try {
        return new Date().toLocaleTimeString("en-GB", { hour: "2-digit", minute: "2-digit", hour12: false });
      } catch (err) {
        return "";
      }
    }

    function timeCommandRequested(msg) {
      return msg === "time" ||
        msg === "jarvis time" ||
        /\b(what|current|local|tell|give|say)\b.*\btime\b/.test(msg) ||
        /\btime\b.*\b(is it|now|please)\b/.test(msg);
    }

    function currentTimeResponse() {
      const time = getCurrentHudTime();
      return time ? "The current time is " + time + "." : "Time is currently unavailable.";
    }

    function parsePercent(text) {
      const match = /(\d{1,3})\s*%?/.exec(text);
      if (!match) return null;
      const value = Math.max(0, Math.min(100, parseInt(match[1], 10)));
      return isNaN(value) ? null : value;
    }

    function parseZoomRequest(text) {
      const msg = normalizeText(text);
      if (!msg.includes("zoom")) return null;

      const percentMatch = /(\d{1,4})\s*%?/.exec(msg);
      const percent = percentMatch ? parseInt(percentMatch[1], 10) : null;
      const isOut = /(zoom\s*out|unzoom|farther|further away|back out)/.test(msg);
      const relative = /\bby\b/.test(msg);

      if (percent !== null && !isNaN(percent)) {
        if (relative) {
          const delta = Math.max(1, percent) / 100;
          return isOut ? snapshotZoom * Math.max(0.05, 1 - delta) : snapshotZoom * (1 + delta);
        }
        return Math.max(1, percent) / 100;
      }

      return isOut ? snapshotZoom / 1.5 : snapshotZoom * 1.5;
    }

    function respondCoreTemp() {
      const temp = lastStatus && lastStatus.atmos ? lastStatus.atmos.temp_c : undefined;
      if (temp !== undefined && temp !== null && temp !== -1) {
        pushChat("jarvis", "Core temperature: " + temp.toFixed(1) + "C.");
      } else {
        pushChat("jarvis", "Core temperature unavailable.");
      }
    }

    function handleLocalCommand(text) {
      const msg = normalizeText(text);
      if (!msg) return false;

      if (timeCommandRequested(msg)) {
        pushChat("jarvis", currentTimeResponse());
        return true;
      }

      if (/\b(mask|faceplate|face plate|visor)\b/.test(msg)) {
        if (/\b(off|open|up|raise|lift)\b/.test(msg)) {
          sendMaskCommand("open", true);
        } else if (/\b(on|close|closed|down|lower|shut)\b/.test(msg)) {
          sendMaskCommand("close", true);
        } else if (/\b(toggle|cycle)\b/.test(msg)) {
          sendMaskCommand("toggle", true);
        } else {
          pushChat("jarvis", "Say mask on to close, or mask off to open.");
        }
        return true;
      }

      if (msg.includes("core temperature") || msg.includes("core temp")) {
        respondCoreTemp();
        return true;
      }

      if (msg.includes("brightness")) {
        const value = parsePercent(msg);
        if (value !== null) {
          setBrightness(value);
          if (brightness) brightness.value = value;
          pushChat("jarvis", "Acknowledged. Brightness set to " + value + "%.");
          return true;
        }
      }

      if (msg.includes("opacity") || (msg.includes("hud") && msg.includes("transparency"))) {
        const value = parsePercent(msg);
        if (value !== null) {
          setHudOpacity(value);
          if (hudOpacity) hudOpacity.value = value;
          pushChat("jarvis", "Acknowledged. HUD opacity set to " + value + "%.");
          return true;
        }
      }

      if (msg.includes("transparency")) {
        const value = parsePercent(msg);
        if (value !== null) {
          setTransparency(value);
          if (transparency) transparency.value = value;
          pushChat("jarvis", "Acknowledged. Widget transparency set to " + value + "%.");
          return true;
        }
      }

      if (msg.includes("volume") || msg.includes("speaker")) {
        const value = parsePercent(msg);
        if (value !== null) {
          setVolume(value, true);
          pushChat("jarvis", "Acknowledged. Volume set to " + value + "%.");
          return true;
        }
      }

      if (/(minimise|minimize|shrink|close)/.test(msg)) {
        setSnapshotExpanded(false);
        pushChat("jarvis", "Media minimized.");
        return true;
      }

      const mentionsClip =
        !/(delete|clear|remove|analyse|analyze|analysis|minimise|minimize|shrink|close)/.test(msg) &&
        (msg.includes("clip") || msg.includes("replay") || msg.includes("record") || (msg.includes("video") && !msg.includes("camera") && !msg.includes("stream")));
      if (mentionsClip) {
        lastMediaIntent = "clip";
        if (/(play|enlarge|expand|maximi[sz]e|open|show)/.test(msg)) {
          expandLastMedia(true, true);
        } else {
          captureClip({ announce: true, showInChat: true });
        }
        return true;
      }

      if (/(snapshot|photo|picture|capture)/.test(msg)) {
        lastMediaIntent = "snapshot";
        captureSnapshot({ announce: true, showInChat: true });
        return true;
      }

      if (/(analyse|analyze|analysis|what.*see|what.*visible|describe.*view|what.*happening)/.test(msg)) {
        analyzeSnapshot();
        return true;
      }

      if (msg.includes("rotate") || msg.includes("turn")) {
        if (!snapshotBlobUrl) {
          pushChat("jarvis", "No snapshot available.");
          return true;
        }
        const delta = msg.includes("left") || msg.includes("counter") ? -90 : 90;
        snapshotRotation = (snapshotRotation + delta + 360) % 360;
        updateSnapshotTransform();
        pushChat("jarvis", "Snapshot rotated.");
        return true;
      }

      if (msg.includes("flip") || msg.includes("mirror")) {
        if (!snapshotBlobUrl) {
          pushChat("jarvis", "No snapshot available.");
          return true;
        }
        if (msg.includes("vertical") || msg.includes("up") || msg.includes("down")) {
          snapshotFlipY = snapshotFlipY * -1;
        } else {
          snapshotFlipX = snapshotFlipX * -1;
        }
        updateSnapshotTransform();
        pushChat("jarvis", "Snapshot flipped.");
        return true;
      }

      if (msg.includes("reset")) {
        if (!snapshotBlobUrl) {
          pushChat("jarvis", "No snapshot available.");
          return true;
        }
        if (msg.length <= 6 || msg.includes("snapshot") || msg.includes("photo") || msg.includes("image")) {
          resetSnapshotView();
          updateSnapshotTransform();
          pushChat("jarvis", "Snapshot reset.");
          return true;
        }
      }

      if (msg.includes("delete") || msg.includes("clear") || msg.includes("remove")) {
        const mentionsSnapshot = msg.includes("snapshot") || msg.includes("photo") || msg.includes("image") || msg.includes("clip") || msg.includes("video") || msg.includes("media");
        const wantsAll = msg.includes("all");
        if (mentionsSnapshot || msg === "delete" || msg === "delete all" || msg === "clear" || msg === "clear all" || msg === "remove" || msg === "remove all") {
          if (wantsAll) {
            const hadSnapshots = chatLogs.some((entry) => isMediaEntry(entry)) || !!snapshotBlobUrl || !!latestClip;
            clearSnapshotImages();
            setSnapshotExpanded(false);
            pushChat("jarvis", hadSnapshots ? "All media deleted." : "No media to delete.");
            return true;
          }
          const removed = removeLastSnapshotImage();
          if (!removed) {
            pushChat("jarvis", "No media to delete.");
            return true;
          }
          pushChat("jarvis", "Last media deleted.");
          return true;
        }
      }

      if (/(expand|enlarge|maximi[sz]e)/.test(msg)) {
        expandLastMedia(true, msg.includes("clip") || msg.includes("video") || lastMediaIntent === "clip");
        return true;
      }

      if (msg === "play" || msg.includes("play it") || msg.includes("play media")) {
        expandLastMedia(true, true);
        return true;
      }

      if (msg.includes("zoom")) {
        if (!snapshotExpanded) {
          pushChat("jarvis", "Expand the media before zooming.");
          return true;
        }
        if (!expandedMediaAvailable()) {
          pushChat("jarvis", "No media available.");
          return true;
        }
        const nextZoom = parseZoomRequest(msg);
        if (nextZoom === null) {
          pushChat("jarvis", "Specify a zoom percentage, for example 'zoom to 200%' or 'zoom out by 25%'.");
          return true;
        }
        setSnapshotZoom(nextZoom);
        pushChat("jarvis", expandedMediaLabel() + " zoom set to " + Math.round(snapshotZoom * 100) + "%.");
        return true;
      }

      if (msg.includes("camera") || msg.includes("video") || msg.includes("stream")) {
        if (setCameraState) {
          if (/(off|disable|stop)/.test(msg)) {
            setCameraState(false, true);
          } else if (/(on|enable|start)/.test(msg)) {
            setCameraState(true, true);
          } else {
            const isOn = stream && stream.style.display !== "none";
            setCameraState(!isOn, true);
          }
          return true;
        }
        pushChat("jarvis", "Camera control unavailable.");
        return true;
      }

      if (msg.includes("mute")) {
        setVolume(0, true);
        pushChat("jarvis", "Acknowledged. Audio muted.");
        return true;
      }

      if (msg.includes("invert")) {
        if (/(off|disable|normal|clear)/.test(msg)) {
          setActiveFilter("none");
          pushChat("jarvis", "Inverted filter disengaged.");
        } else {
          setActiveFilter("invert");
          pushChat("jarvis", "Inverted filter engaged.");
        }
        return true;
      }

      if (msg.includes("black") || msg.includes("white") || msg.includes("grayscale")) {
        if (/(off|disable|normal|clear)/.test(msg)) {
          setActiveFilter("none");
          pushChat("jarvis", "Monochrome filter disengaged.");
        } else {
          setActiveFilter("bw");
          pushChat("jarvis", "Monochrome filter engaged.");
        }
        return true;
      }

      if (msg.includes("depth") || msg.includes("holo") || msg.includes("holographic")) {
        pushChat("jarvis", "Depth holo is unavailable.");
        return true;
      }

      return false;
    }

    function handleSmallTalk(text) {
      const msg = normalizeText(text);
      if (!msg) return "";

      if (/(^|\b)(hi|hello|hey|yo|jarvis)\b/.test(msg)) {
        return "Hello. Standing by.";
      }
      if (msg.includes("how are you")) {
        return "All systems nominal.";
      }
      if (msg.includes("what can you do") || msg.includes("capabilities")) {
        return "I can manage filters, brightness, HUD opacity, widget transparency, volume, and report telemetry.";
      }
      if (msg.includes("good morning") || msg.includes("good afternoon") || msg.includes("good evening")) {
        return "Greetings. Ready when you are.";
      }
      if (msg.includes("your name") || msg.includes("who are you")) {
        return "I am Jarvis, your onboard assistant.";
      }
      if (msg.includes("thank")) {
        return "Anytime.";
      }
      if (timeCommandRequested(msg)) {
        return currentTimeResponse();
      }
      if (msg.includes("location") || msg.includes("where are we")) {
        if (lastStatus && lastStatus.nav && lastStatus.nav.fix && lastStatus.lat && lastStatus.lon) {
          return "Current location: " + lastStatus.lat + ", " + lastStatus.lon + ".";
        }
        if (lastStatus && lastStatus.lat && lastStatus.lon) {
          return "Last known location: " + lastStatus.lat + ", " + lastStatus.lon + ". Awaiting GNSS lock.";
        }
        if (lastStatus && lastStatus.nav && lastStatus.nav.serial && !lastStatus.nav.signal) {
          return "GNSS serial is present, but valid location sentences are not being decoded yet. Check baud rate or fix conditions.";
        }
        if (lastStatus && lastStatus.nav && lastStatus.nav.signal) {
          const sats = lastStatus.nav.satellites || 0;
          return sats > 0
            ? "GNSS is receiving satellites, but it does not have a position fix yet."
            : "GNSS is active and still searching for satellites.";
        }
        return "Location data is unavailable.";
      }
      if (msg.includes("speed")) {
        if (lastStatus && lastStatus.nav && lastStatus.nav.speed_mph >= 0) {
          return "Current speed: " + lastStatus.nav.speed_mph.toFixed(1) + " miles per hour.";
        }
        return "Speed data is unavailable.";
      }
      return "";
    }

    function updateJarvisStatus(text) {
      if (jarvisStatus) {
        jarvisStatus.textContent = text;
      }
      statusLine = text;
      renderLogs();
    }

    let statusLine = "MIMICLAW: NOT CONFIGURED";
    function setStatusLine(text) {
      statusLine = text;
      updateJarvisStatus(text);
    }
    function restoreStatusLine() {
      updateJarvisStatus(statusLine);
    }

    const queryParams = new URLSearchParams(window.location.search);
    let mimicUrl = queryParams.get("mimi") || "";
    if (mimicUrl && !/^wss?:\/\//.test(mimicUrl)) {
      mimicUrl = "ws://" + mimicUrl;
    }
    let mimicWs = null;

    function isMimiclawOnline() {
      return !!(mimicWs && mimicWs.readyState === WebSocket.OPEN);
    }

    function formatMimicTarget(url) {
      if (!url) return "--";
      try {
        const parsed = new URL(url);
        return parsed.host;
      } catch (err) {
        return url.replace(/^wss?:\/\//, "").replace(/\/.*$/, "") || url;
      }
    }

    function setMimicDetail(url) {
      if (!jarvisMimicDetail) return;
      jarvisMimicDetail.textContent = "WS: " + formatMimicTarget(url);
    }

    setMimicDetail(mimicUrl);

    function connectMimiclaw() {
      if (!mimicUrl) {
        setStatusLine("MIMICLAW: NOT CONFIGURED");
        setMimicDetail("");
        return false;
      }
      if (mimicWs && (mimicWs.readyState === WebSocket.OPEN || mimicWs.readyState === WebSocket.CONNECTING)) {
        return true;
      }
      try {
        mimicWs = new WebSocket(mimicUrl);
      } catch (err) {
        setStatusLine("MIMICLAW: INVALID URL");
        return false;
      }
      setMimicDetail(mimicUrl);
      setStatusLine("MIMICLAW: CONNECTING");
      mimicWs.onopen = () => setStatusLine("MIMICLAW: ONLINE");
      mimicWs.onclose = () => setStatusLine("MIMICLAW: OFFLINE");
      mimicWs.onerror = () => setStatusLine("MIMICLAW: ERROR");
      mimicWs.onmessage = (evt) => {
        let payload = null;
        try {
          payload = JSON.parse(evt.data);
        } catch (err) {
          payload = null;
        }
        if (payload && payload.type === "response" && payload.content) {
          const parsed = applyHudActionsFromResponse(payload.content);
          if (parsed.text) {
            pushChat("jarvis", parsed.text);
          } else if (parsed.executed) {
            pushChat("jarvis", "Acknowledged.");
          }
        }
      };
      return true;
    }

    function sendToMimiclaw(text) {
      return sendMimiclawPayload({ type: "message", content: text, chat_id: "hud" });
    }

    function sendMimiclawPayload(payload) {
      if (!connectMimiclaw()) {
        return false;
      }
      if (!mimicWs || mimicWs.readyState !== WebSocket.OPEN) {
        setStatusLine("MIMICLAW: OFFLINE");
        return false;
      }
      try {
        mimicWs.send(JSON.stringify(payload));
        return true;
      } catch (err) {
        setStatusLine("MIMICLAW: ERROR");
        return false;
      }
    }

    function processCommand(text, options = {}) {
      const cleaned = (text || "").trim();
      if (!cleaned) return;
      const skipUser = options && options.skipUser;
      if (!skipUser) {
        pushChat("user", cleaned);
      }
      if (handleLocalCommand(cleaned)) {
        return;
      }
      const smallTalk = handleSmallTalk(cleaned);
      if (smallTalk) {
        pushChat("jarvis", smallTalk);
        return;
      }
      if (sendToMimiclaw(cleaned)) {
        return;
      }
      if (mimicUrl || isMimiclawOnline()) {
        pushChat("jarvis", "Mimiclaw offline. Local controls remain available.");
      } else {
        pushChat("jarvis", "No matching local command available.");
      }
    }

    function handleUserInput(text) {
      processCommand(text);
    }

    let wakeArmedUntil = 0;
    function handleMicTranscript(text, options = {}) {
      const cleaned = (text || "").trim();
      if (!cleaned) {
        updateJarvisStatus("MIC: NO VOICE");
        setTimeout(restoreStatusLine, 800);
        return;
      }
      const force = options && options.force;
      pushChat("user", cleaned);
      if (force) {
        processCommand(cleaned, { skipUser: true });
        return;
      }
      const lower = cleaned.toLowerCase();
      const key = "jarvis";
      if (lower.includes(key)) {
        const idx = lower.indexOf(key);
        const after = cleaned.slice(idx + key.length).trim();
        if (after) {
          processCommand(after, { skipUser: true });
        } else {
          wakeArmedUntil = Date.now() + 6000;
          updateJarvisStatus("JARVIS: AWAITING COMMAND");
        }
        return;
      }
      if (wakeArmedUntil && Date.now() < wakeArmedUntil) {
        wakeArmedUntil = 0;
        restoreStatusLine();
        processCommand(cleaned, { skipUser: true });
        return;
      }
      updateJarvisStatus("SAY 'JARVIS' TO WAKE");
      setTimeout(restoreStatusLine, 1000);
    }

    if (invertToggle) {
      invertToggle.addEventListener("click", () => toggleFilter("invert"));
    }

    if (bwToggle) {
      bwToggle.addEventListener("click", () => toggleFilter("bw"));
    }

    if (jarvisInput) {
      jarvisInput.addEventListener("keydown", (evt) => {
        if (evt.key === "Enter") {
          handleUserInput(jarvisInput.value);
          jarvisInput.value = "";
        }
      });
    }

    const MIC_SILENCE_MS = 1000;
    const MIC_POLL_MS = 320;
    const MIC_LEVEL_ACTIVE = 0.08;
    const MIC_EMPTY_REQUIRED = 2;
    let micStreaming = false;
    let micRequestInFlight = false;
    let micBuffer = "";
    let micLastSpeechAt = 0;
    let micEmptyCount = 0;
    let micLoopHandle = null;

    function setMicUi(on) {
      if (!jarvisMic) return;
      jarvisMic.classList.toggle("listening", on);
    }

    function stopMicStream() {
      micStreaming = false;
      micBuffer = "";
      micLastSpeechAt = 0;
      micEmptyCount = 0;
      if (micLoopHandle) {
        clearTimeout(micLoopHandle);
        micLoopHandle = null;
      }
      if (jarvisMic) {
        jarvisMic.classList.remove("active");
      }
      setMicUi(false);
      updateJarvisStatus("MIC: OFF");
      restoreStatusLine();
    }

    function startMicStream() {
      if (!jarvisMic) return;
      micStreaming = true;
      micBuffer = "";
      micLastSpeechAt = 0;
      micEmptyCount = 0;
      setMicUi(true);
      updateJarvisStatus("MIC: LISTENING");
      runMicLoop();
    }

    function toggleMicStream() {
      if (micStreaming) {
        stopMicStream();
      } else {
        startMicStream();
      }
    }

    async function runMicLoop() {
      if (!micStreaming || micRequestInFlight) return;
      if (ttsBusy || Date.now() < micHoldoffUntil) {
        micLoopHandle = setTimeout(runMicLoop, MIC_POLL_MS);
        return;
      }
      micRequestInFlight = true;
      jarvisMic.classList.add("active");
      try {
        const res = await fetch("/voice", { method: "POST" });
        if (!res.ok) {
          throw new Error("voice_failed");
        }
        const data = await res.json();
        if (!micStreaming) {
          return;
        }
        const chunk = data && data.transcript ? data.transcript.trim() : "";
        if (chunk) {
          micBuffer = micBuffer ? micBuffer + " " + chunk : chunk;
          micLastSpeechAt = Date.now();
          micEmptyCount = 0;
        } else if (data && data.error === "stt_not_configured") {
          pushChat("jarvis", "Speech-to-text not configured.");
          stopMicStream();
          return;
        } else if (data && data.error === "stt_failed") {
          pushChat("jarvis", "Mic error: stt_failed.");
          micBuffer = "";
          micLastSpeechAt = 0;
          micEmptyCount = 0;
        } else if (data && data.error && data.error !== "voice_busy") {
          pushChat("jarvis", "Mic error: " + data.error + ".");
        } else {
          micEmptyCount++;
        }

        if (
          micBuffer &&
          micLastSpeechAt &&
          (Date.now() - micLastSpeechAt) >= MIC_SILENCE_MS &&
          micEmptyCount >= MIC_EMPTY_REQUIRED
        ) {
          const utterance = micBuffer;
          micBuffer = "";
          micLastSpeechAt = 0;
          micEmptyCount = 0;
          pushChat("user", utterance);
          processCommand(utterance, { skipUser: true });
        }
      } catch (err) {
        pushChat("jarvis", "Mic request failed.");
      } finally {
        jarvisMic.classList.remove("active");
        micRequestInFlight = false;
        if (micStreaming) {
          micLoopHandle = setTimeout(runMicLoop, MIC_POLL_MS);
        }
      }
    }

    if (jarvisMic) {
      jarvisMic.addEventListener("click", toggleMicStream);
    }

    if (mimicUrl) {
      setStatusLine("MIMICLAW: OFFLINE");
      connectMimiclaw();
    } else {
      setStatusLine("MIMICLAW: NOT CONFIGURED");
    }

    if (powerToggle && stream && cameraOff) {
      const refreshStream = () => {
        const src = stream.dataset.src || stream.src;
        if (stream.style.display === "none") return;
        const bust = "?t=" + Date.now();
        stream.src = src + bust;
        if (streamEdge) {
          streamEdge.src = src + bust + "&edge=1";
        }
      };
      const fallbackToLocal = () => {
        if (streamFallbackTried) return false;
        const current = stream.dataset.src || "";
        if (current.includes(":81/")) {
          streamFallbackTried = true;
          applyStreamBase("/stream");
          return true;
        }
        return false;
      };
      const setCamera = (on, notify = false) => {
        if (on) {
          stream.style.display = "block";
          if (streamEdge) {
            streamEdge.style.display = "block";
          }
          refreshStream();
          setTimeout(() => {
            if (!stream || stream.style.display === "none") return;
            if (stream.naturalWidth === 0 && fallbackToLocal()) {
              refreshStream();
            }
          }, 1500);
          cameraOff.style.display = "none";
          powerToggle.textContent = "CAMERA: ON";
          if (notify) {
            pushChat("jarvis", "Camera engaged.");
          }
        } else {
          stream.src = "";
          stream.style.display = "none";
          if (streamEdge) {
            streamEdge.src = "";
            streamEdge.style.display = "none";
          }
          cameraOff.style.display = "flex";
          powerToggle.textContent = "CAMERA: OFF";
          if (notify) {
            pushChat("jarvis", "Camera disengaged.");
          }
        }
      };
      setCameraState = setCamera;
      setCamera(true);
      stream.addEventListener("error", () => {
        if (fallbackToLocal()) return;
        setTimeout(refreshStream, 600);
      });
      if (streamEdge) {
        streamEdge.addEventListener("error", () => {
          if (fallbackToLocal()) return;
          setTimeout(refreshStream, 600);
        });
      }
      powerToggle.addEventListener("click", () => {
        const isOn = stream.style.display !== "none";
        setCamera(!isOn, true);
      });
    }

    const setBrightness = (value) => {
      const pct = Math.max(0, Math.min(100, value));
      // Keep UI range 0-100%, but make 0% still 10% brighter than previous baseline.
      const level = 0.1 + pct / 100;
      document.documentElement.style.setProperty("--hud-brightness", level.toFixed(2));
      const helmetOpacity = 0.25 + (pct / 100) * 0.75;
      const helmetComp = 1 / level;
      document.documentElement.style.setProperty("--helmet-opacity", helmetOpacity.toFixed(2));
      document.documentElement.style.setProperty("--helmet-brightness-comp", helmetComp.toFixed(2));
      if (brightnessVal) brightnessVal.textContent = pct.toFixed(0) + "%";
    };

    const setTransparency = (value) => {
      const pct = Math.max(0, Math.min(100, value));
      const factor = pct / 100;
      document.documentElement.style.setProperty("--panel-alpha", factor.toFixed(2));
      document.documentElement.style.setProperty("--panel-strong-alpha", factor.toFixed(2));
      if (transparencyVal) transparencyVal.textContent = pct.toFixed(0) + "%";
    };

    const setHudOpacity = (value) => {
      const pct = Math.max(0, Math.min(100, value));
      const factor = pct / 100;
      document.documentElement.style.setProperty("--hud-opacity", factor.toFixed(2));
      if (hudOpacityVal) hudOpacityVal.textContent = pct.toFixed(0) + "%";
    };

    function sendVolume(pct) {
      clearTimeout(volumeSendTimer);
      volumeSendTimer = setTimeout(() => {
        fetch("/volume", {
          method: "POST",
          headers: { "Content-Type": "text/plain" },
          body: String(pct)
        })
          .then((res) => res.ok ? res.json() : null)
          .then((data) => {
            if (data && data.volume !== undefined) {
              lastVolumeSetAt = Date.now();
              lastVolumeSetValue = data.volume;
              setVolume(data.volume, false);
            }
          })
          .catch(() => {});
      }, 160);
    }

    function setVolume(value, send = true) {
      const pct = Math.max(0, Math.min(100, value));
      if (volumeVal) volumeVal.textContent = pct.toFixed(0) + "%";
      if (volume) volume.value = pct;
      if (send) {
        lastVolumeSetAt = Date.now();
        lastVolumeSetValue = pct;
        sendVolume(pct);
      }
    }

    if (brightness) {
      setBrightness(brightness.value);
      brightness.addEventListener("input", () => setBrightness(brightness.value));
    }

    if (hudOpacity) {
      setHudOpacity(hudOpacity.value);
      hudOpacity.addEventListener("input", () => setHudOpacity(hudOpacity.value));
    }

    if (transparency) {
      setTransparency(transparency.value);
      transparency.addEventListener("input", () => setTransparency(transparency.value));
    }

    if (volume) {
      setVolume(volume.value, true);
      volume.addEventListener("input", () => {
        volumeDragging = true;
        setVolume(volume.value, true);
      });
      volume.addEventListener("change", () => {
        volumeDragging = false;
      });
      window.addEventListener("pointerup", () => {
        volumeDragging = false;
      });
    }

    if (snapshotOverlay) {
      snapshotOverlay.addEventListener("click", (evt) => {
        if (evt.target === snapshotOverlay) {
          setSnapshotExpanded(false);
        }
      });
    }

    if (snapshotExpandedImg) {
      snapshotExpandedImg.addEventListener("click", (evt) => {
        evt.stopPropagation();
      });
      snapshotExpandedImg.addEventListener("pointerdown", (evt) => {
        if (!snapshotExpanded || snapshotZoom <= 1) return;
        snapshotDragging = true;
        snapshotDragStartX = evt.clientX;
        snapshotDragStartY = evt.clientY;
        snapshotDragOriginX = snapshotPanX;
        snapshotDragOriginY = snapshotPanY;
        snapshotExpandedImg.classList.add("dragging");
        if (snapshotExpandedImg.setPointerCapture) {
          snapshotExpandedImg.setPointerCapture(evt.pointerId);
        }
        evt.preventDefault();
        evt.stopPropagation();
      });
      snapshotExpandedImg.addEventListener("pointermove", (evt) => {
        if (!snapshotDragging) return;
        snapshotPanX = snapshotDragOriginX + (evt.clientX - snapshotDragStartX);
        snapshotPanY = snapshotDragOriginY + (evt.clientY - snapshotDragStartY);
        updateSnapshotTransform();
      });
      const stopSnapshotDrag = (evt) => {
        if (!snapshotDragging) return;
        snapshotDragging = false;
        snapshotExpandedImg.classList.remove("dragging");
        if (
          evt &&
          snapshotExpandedImg.releasePointerCapture &&
          snapshotExpandedImg.hasPointerCapture &&
          snapshotExpandedImg.hasPointerCapture(evt.pointerId)
        ) {
          snapshotExpandedImg.releasePointerCapture(evt.pointerId);
        }
      };
      snapshotExpandedImg.addEventListener("pointerup", stopSnapshotDrag);
      snapshotExpandedImg.addEventListener("pointercancel", stopSnapshotDrag);
    }

    if (jarvisLogBox) {
      jarvisLogBox.addEventListener("scroll", () => {
        const gap = jarvisLogBox.scrollHeight - jarvisLogBox.scrollTop - jarvisLogBox.clientHeight;
        jarvisAutoScroll = gap < 12;
      });
    }

    const scheduleJarvisOverlay = () => {
      if (!jarvisInputRow || !jarvisInputAnchor) return;
      requestAnimationFrame(syncJarvisInputOverlay);
    };
    window.addEventListener("load", scheduleJarvisOverlay);
    window.addEventListener("resize", scheduleJarvisOverlay);
    window.addEventListener("orientationchange", scheduleJarvisOverlay);
    setTimeout(scheduleJarvisOverlay, 0);
    startVisionTicker();
    startClipBuffer();
  </script>
</body>
</html>
)rawliteral";
