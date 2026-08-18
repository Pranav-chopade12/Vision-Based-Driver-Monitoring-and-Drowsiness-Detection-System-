import cv2
import mediapipe as mp
import numpy as np
import serial
import time
import csv
import os
import sys
import platform
import threading
import queue
from collections import deque
from datetime import datetime

import tkinter as tk
from tkinter import ttk
from PIL import Image, ImageTk

# ═══════════════════════════════════════════════════════════
#  CONFIGURATION
# ═══════════════════════════════════════════════════════════
SERIAL_PORT           = 'COM5'
BAUD_RATE             = 115200
CAMERA_INDEX          = 0

EAR_THRESHOLD_DEFAULT = 0.25
MAR_THRESHOLD         = 0.60
DROWSY_FRAME_COUNT    = 20
RECOVERY_FRAME_COUNT  = 10
YAWN_FRAME_COUNT      = 10
CALIBRATION_SECONDS   = 5
GRAPH_HISTORY_LEN     = 100
LOG_FOLDER            = "drowsiness_logs"

VIDEO_W, VIDEO_H = 640, 480
SIDEBAR_W        = 390

# Voice: only CRITICAL, cooldown 5s
VOICE_COOLDOWN = {'critical': 8.0}
VOICE_MESSAGES = {
    'critical': [
        "Drowsy alert! Pull over safely and take a break!",
        "Stop the vehicle now!",
    
    ],
}

# ═══════════════════════════════════════════════════════════
#  LANDMARK INDICES
# ═══════════════════════════════════════════════════════════
LEFT_EYE  = [33,  160, 158, 133, 153, 144]
RIGHT_EYE = [362, 385, 387, 263, 373, 380]
MOUTH     = {"top": 13, "bottom": 14, "left": 78,
             "right": 308, "top2": 312, "bottom2": 317}

# ═══════════════════════════════════════════════════════════
#  COLOUR PALETTE
# ═══════════════════════════════════════════════════════════
BG        = '#0c0c0c'
HDR       = '#111111'
CARD      = '#161616'
CARD2     = '#1c1c1c'
BORDER    = '#252525'
TEXT      = '#e0e0e0'
MUTED     = '#585858'
BLUE      = '#4fc3f7'
GREEN     = '#4caf50'
YELLOW    = '#fdd835'
ORANGE    = '#ff9800'
RED       = '#f44336'
GRAPH_BG  = '#0a0a0a'
GRAPH_GRD = '#1e1e1e'
PURPLE    = '#ce93d8'

STATES = {
    'calibrate': ('#001a30', '#4fc3f7', 'CALIBRATING...'),
    'noface':    ('#0d0d0d', '#3a3a3a', 'NO FACE DETECTED'),
    'normal':    ('#041504', '#4caf50', 'AWAKE \u2014 NORMAL'),
    'warning':   ('#1a1000', '#ff9800', 'WARNING \u2014 EYES CLOSING'),
    'critical':  ('#1a0404', '#f44336', 'DROWSY \u2014 CRITICAL !'),
    'yawn':      ('#130d00', '#ff9800', 'YAWNING DETECTED'),
}
SUB_MSG = {
    'calibrate': 'Keep your eyes open normally...',
    'noface':    'Position your face in the camera frame',
    'normal':    'Stay alert and focused on the road',
    'warning':   'Eyes closing detected \u2014 stay awake!',
    'critical':  'Pull over safely and take a break!',
    'yawn':      'Fatigue detected \u2014 consider stopping',
}


# ═══════════════════════════════════════════════════════════
#  BLUETOOTH / TTS VOICE ALERT ENGINE
# ═══════════════════════════════════════════════════════════
class BluetoothVoiceAlert:
    """
    pyttsx3 TTS alerts through default audio output (Bluetooth speaker).
    Fresh engine created per speech call — fixes one-time-only bug.
    Runs in dedicated daemon thread — zero GUI blocking.
    """
    def __init__(self):
        self._queue            = queue.Queue(maxsize=2)
        self._last             = {}
        self._ready            = False
        self._enabled          = True
        self._msg_idx          = {}
        self._device_name      = "Default Audio"
        self._preferred_voice  = None

        threading.Thread(target=self._worker, daemon=True, name="TTS").start()

    def speak(self, state: str):
        if not self._enabled or not self._ready:
            return
        if state not in VOICE_MESSAGES:
            return
        now = time.time()
        if now - self._last.get(state, 0.0) < VOICE_COOLDOWN.get(state, 5.0):
            return
        self._last[state] = now
        idx  = self._msg_idx.get(state, 0)
        text = VOICE_MESSAGES[state][idx % len(VOICE_MESSAGES[state])]
        self._msg_idx[state] = idx + 1
        try:
            self._queue.put_nowait(text)
        except queue.Full:
            pass

    def set_enabled(self, v):  self._enabled = v
    def is_ready(self):        return self._ready
    def device_name(self):     return self._device_name

    def _worker(self):
        try:
            import pyttsx3
            eng = pyttsx3.init()
            voices = eng.getProperty('voices')
            for v in voices:
                if 'zira' in v.name.lower() or 'female' in v.name.lower():
                    self._preferred_voice = v.id
                    break
            eng.stop()
            del eng

            try:
                import subprocess
                cmd = ('powershell -NoProfile -Command "'
                       '(Get-WmiObject -Class Win32_SoundDevice | '
                       'Where-Object {$_.Status -eq \\"OK\\"} | '
                       'Select-Object -First 1 -ExpandProperty Name)"')
                r = subprocess.run(cmd, shell=True, capture_output=True,
                                   text=True, timeout=4)
                n = r.stdout.strip()
                self._device_name = n if n else "Default Audio"
            except Exception:
                pass

            self._ready = True
            print(f"[VOICE] Ready — Output: {self._device_name}")
            print("[VOICE] Alert: CRITICAL only (warning/yawn = silent)")

        except ImportError:
            print("[VOICE] pyttsx3 not found. Run: pip install pyttsx3")
            return
        except Exception as e:
            print(f"[VOICE] Init error: {e}")
            return

        while True:
            try:
                text = self._queue.get(timeout=0.5)
                try:
                    import pyttsx3
                    e = pyttsx3.init()
                    e.setProperty('rate', 160)
                    e.setProperty('volume', 1.0)
                    if self._preferred_voice:
                        e.setProperty('voice', self._preferred_voice)
                    e.say(text)
                    e.runAndWait()
                    e.stop()
                    del e
                    print(f"[VOICE] Spoke: {text}")
                except Exception as ex:
                    print(f"[VOICE] Speak error: {ex}")
                self._queue.task_done()
            except queue.Empty:
                continue
            except Exception:
                continue


# ═══════════════════════════════════════════════════════════
#  BEEP  (fallback hardware alert)
# ═══════════════════════════════════════════════════════════
class AudioAlert:
    def __init__(self):
        self._sys  = platform.system()
        self._ws   = None
        self._last = 0.0
        if self._sys == "Windows":
            try:
                import winsound
                self._ws = winsound
            except ImportError:
                pass

    def beep(self, critical=False):
        now = time.time()
        if now - self._last < (1.5 if critical else 3.0):
            return
        self._last = now
        try:
            if self._sys == "Windows" and self._ws:
                freq, dur = (2500, 300) if critical else (1200, 150)
                threading.Thread(target=self._ws.Beep,
                                 args=(freq, dur), daemon=True).start()
            else:
                sys.stdout.write("\a"); sys.stdout.flush()
        except Exception:
            pass


# ═══════════════════════════════════════════════════════════
#  GEOMETRY
# ═══════════════════════════════════════════════════════════
def _d(p, q):
    return np.hypot(p[0]-q[0], p[1]-q[1])

def get_ear(lms, idx, w, h):
    p = [(lms[i].x*w, lms[i].y*h) for i in idx]
    return (_d(p[1],p[5]) + _d(p[2],p[4])) / (2*_d(p[0],p[3]) + 1e-6)

def get_mar(lms, w, h):
    def pt(k):
        lm = lms[MOUTH[k]]; return lm.x*w, lm.y*h
    return (_d(pt("top"),pt("bottom")) + _d(pt("top2"),pt("bottom2"))) / (2*_d(pt("left"),pt("right")) + 1e-6)


# ═══════════════════════════════════════════════════════════
#  SERIAL
# ═══════════════════════════════════════════════════════════
class STM32Comm:
    def __init__(self, port, baud):
        self.ser, self.last, self.ok = None, None, False
        try:
            self.ser = serial.Serial(port, baud, timeout=1)
            time.sleep(2); self.ok = True
            print(f"[SERIAL] Connected: {port} @ {baud}")
        except Exception as e:
            print(f"[SERIAL] {e}  -> display-only mode")

    def send(self, cmd):
        if cmd == self.last: return
        if self.ser and self.ser.is_open:
            self.ser.write(cmd.encode()); self.last = cmd

    def close(self):
        if self.ser and self.ser.is_open: self.ser.close()


# ═══════════════════════════════════════════════════════════
#  CSV LOGGER
# ═══════════════════════════════════════════════════════════
class Logger:
    def __init__(self):
        os.makedirs(LOG_FOLDER, exist_ok=True)
        fn = datetime.now().strftime("session_%Y%m%d_%H%M%S.csv")
        self.path = os.path.join(LOG_FOLDER, fn)
        self.f = open(self.path, "w", newline="")
        self.w = csv.writer(self.f)
        self.w.writerow(["timestamp","ear","mar","alert","yawn"])
        print(f"[LOG] -> {self.path}")

    def log(self, e, m, a, y):
        self.w.writerow([datetime.now().strftime("%H:%M:%S.%f")[:-3],
                         f"{e:.4f}", f"{m:.4f}", a, int(y)])

    def close(self):
        self.f.flush(); self.f.close()


# ═══════════════════════════════════════════════════════════
#  SCROLLABLE FRAME  (sidebar helper)
# ═══════════════════════════════════════════════════════════
class ScrollableFrame(tk.Frame):
    """A vertically scrollable frame for the sidebar."""
    def __init__(self, parent, width, **kwargs):
        super().__init__(parent, bg=BG, **kwargs)
        self._canvas = tk.Canvas(self, bg=BG, width=width,
                                 highlightthickness=0, bd=0)
        self._sb = tk.Scrollbar(self, orient='vertical',
                                command=self._canvas.yview)
        self.inner = tk.Frame(self._canvas, bg=BG)

        self._canvas.pack(side='left', fill='both', expand=True)
        # scrollbar only shown if needed — bound to mousewheel
        self._win_id = self._canvas.create_window(
            (0, 0), window=self.inner, anchor='nw')

        self.inner.bind('<Configure>', self._on_inner_configure)
        self._canvas.bind('<Configure>', self._on_canvas_configure)

        # Mousewheel scroll
        self._canvas.bind('<Enter>', self._bind_wheel)
        self._canvas.bind('<Leave>', self._unbind_wheel)

    def _on_inner_configure(self, _=None):
        self._canvas.configure(scrollregion=self._canvas.bbox('all'))

    def _on_canvas_configure(self, event):
        self._canvas.itemconfig(self._win_id, width=event.width)

    def _bind_wheel(self, _=None):
        self._canvas.bind_all('<MouseWheel>', self._on_wheel)

    def _unbind_wheel(self, _=None):
        self._canvas.unbind_all('<MouseWheel>')

    def _on_wheel(self, event):
        self._canvas.yview_scroll(-1*(event.delta//120), 'units')


# ═══════════════════════════════════════════════════════════
#  MAIN APPLICATION
# ═══════════════════════════════════════════════════════════
class DrowsinessApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Drowsiness Detection System  v3.2  [BT Voice]")
        self.root.configure(bg=BG)
        self.root.resizable(True, True)
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

        # ── Backend ───────────────────────────────────
        self.stm32  = STM32Comm(SERIAL_PORT, BAUD_RATE)
        self.audio  = AudioAlert()
        self.voice  = BluetoothVoiceAlert()
        self.logger = Logger()

        mp_mesh = mp.solutions.face_mesh
        self.fm = mp_mesh.FaceMesh(
            max_num_faces=1, refine_landmarks=True,
            min_detection_confidence=0.5, min_tracking_confidence=0.5)

        self.cap = cv2.VideoCapture(CAMERA_INDEX)
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH,  VIDEO_W)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, VIDEO_H)
        self.cap.set(cv2.CAP_PROP_FPS, 30)
        if not self.cap.isOpened():
            print("[ERROR] Camera not found!"); sys.exit(1)

        # ── Shared state (written by capture thread, read by GUI) ──
        self._lock         = threading.Lock()
        self._disp_frame   = None   # BGR frame for display
        self._ear          = 0.0
        self._mar          = 0.0
        self._face         = False

        # ── Logic state (only touched by capture thread) ──────────
        self.thr       = EAR_THRESHOLD_DEFAULT
        self.dcnt      = 0
        self.rcnt      = 0
        self.ycnt      = 0
        self.yawning   = False
        self.alert     = 0
        self._palert   = 0
        self._pyawn    = False
        self.t0        = time.time()
        self.devents   = 0
        self.yevents   = 0
        self.esum      = 0.0
        self.en        = 0
        self.ear_hist  = deque(maxlen=GRAPH_HISTORY_LEN)

        self.calibrating = False
        self.csamples    = []
        self.cstart      = None

        self._fps_t    = time.time()
        self._fps_n    = 0
        self._fps      = 0.0
        self._blink_on = True
        self._voice_on = True
        self._running  = True

        self._GW = SIDEBAR_W - 30
        self._GH = 88

        # ── Build UI ──────────────────────────────────
        self._build_ui()

        # ── Start capture thread ──────────────────────
        threading.Thread(target=self._capture_loop,
                         daemon=True, name="CaptureMP").start()

        # ── Start calibration + GUI loop ──────────────
        self.start_calibration()
        self._gui_loop()
        self._tick_blink()
        self._tick_voice_status()

    # ══════════════════════════════════════════════════
    #  BACKGROUND THREAD — Camera + MediaPipe + Logic
    # ══════════════════════════════════════════════════
    def _capture_loop(self):
        while self._running:
            ret, frame = self.cap.read()
            if not ret:
                time.sleep(0.01)
                continue

            h, w = frame.shape[:2]
            rgb  = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            res  = self.fm.process(rgb)

            ear_v = mar_v = 0.0
            face  = False

            if res.multi_face_landmarks:
                face = True
                lms  = res.multi_face_landmarks[0].landmark
                ear_v = (get_ear(lms, LEFT_EYE, w, h) +
                         get_ear(lms, RIGHT_EYE, w, h)) / 2.0
                mar_v = get_mar(lms, w, h)

                ec = (0, 0, 220) if ear_v < self.thr else (50, 220, 50)
                mc = (0, 165, 255) if self.yawning else (0, 220, 220)
                for idx in LEFT_EYE + RIGHT_EYE:
                    lm = lms[idx]
                    cv2.circle(frame,
                               (int(lm.x*w), int(lm.y*h)), 2, ec, -1)
                for k in MOUTH:
                    lm = lms[MOUTH[k]]
                    cv2.circle(frame,
                               (int(lm.x*w), int(lm.y*h)), 2, mc, -1)

                if self.calibrating:
                    self.csamples.append(ear_v)
                else:
                    self.ear_hist.append(ear_v)
                    self.esum += ear_v
                    self.en   += 1
                    self._run_logic(ear_v, mar_v)
                    self.logger.log(ear_v, mar_v, self.alert, self.yawning)

            # FPS count
            self._fps_n += 1
            dt = time.time() - self._fps_t
            if dt >= 1.0:
                self._fps   = self._fps_n / dt
                self._fps_n = 0
                self._fps_t = time.time()

            # Push frame + metrics to shared state
            with self._lock:
                self._disp_frame = frame.copy()
                self._ear  = ear_v
                self._mar  = mar_v
                self._face = face

    # ══════════════════════════════════════════════════
    #  GUI LOOP  (runs in main thread via after())
    # ══════════════════════════════════════════════════
    def _gui_loop(self):
        # Read shared state
        with self._lock:
            frame = self._disp_frame
            ear_v = self._ear
            mar_v = self._mar
            face  = self._face

        # Calibration countdown (GUI-safe, just reads cstart)
        if self.calibrating:
            rem = CALIBRATION_SECONDS - (time.time() - self.cstart)
            if rem <= 0:
                self.finish_calibration()
            else:
                pct = (1.0 - rem / CALIBRATION_SECONDS) * 100
                self._cal_bar_var.set(pct)
                self._cal_text.set(
                    f"Calibrating — keep eyes open  ({rem:.1f}s)")

        # Display camera frame
        if frame is not None:
            rgb_d = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            img   = Image.fromarray(rgb_d)
            imgtk = ImageTk.PhotoImage(image=img)
            self.cam_lbl.imgtk = imgtk
            self.cam_lbl.configure(image=imgtk)

        # Update FPS label
        self._v_fps.set(f'{self._fps:.0f} fps')

        # Update all dashboard widgets
        self._update_dash(ear_v, mar_v, face)

        self.root.after(20, self._gui_loop)   # ~50 fps GUI refresh

    # ══════════════════════════════════════════════════
    #  DROWSINESS + YAWN LOGIC  (called from capture thread)
    # ══════════════════════════════════════════════════
    def _run_logic(self, ear_v, mar_v):
        # EAR-based drowsiness
        if ear_v < self.thr:
            self.dcnt += 1; self.rcnt = 0
            if self.dcnt >= DROWSY_FRAME_COUNT // 2: self.alert = 1
            if self.dcnt >= DROWSY_FRAME_COUNT:      self.alert = 2
        else:
            self.rcnt += 1
            self.dcnt  = max(0, self.dcnt - 1)
            if self.rcnt >= RECOVERY_FRAME_COUNT:    self.alert = 0

        # MAR-based yawn
        if mar_v > MAR_THRESHOLD:
            self.ycnt += 1
            if self.ycnt >= YAWN_FRAME_COUNT: self.yawning = True
        else:
            self.ycnt = max(0, self.ycnt - 1)
            if self.ycnt == 0: self.yawning = False

        # Rising-edge event counting
        if self.alert == 2 and self._palert != 2: self.devents += 1
        if self.yawning and not self._pyawn:       self.yevents += 1
        self._palert = self.alert
        self._pyawn  = self.yawning

        # Serial
        if self.alert == 2:   self.stm32.send('A')
        elif self.alert == 1: self.stm32.send('D')
        elif self.yawning:    self.stm32.send('Y')
        else:                 self.stm32.send('W')

        # Beep
        if self.alert == 2:   self.audio.beep(critical=True)
        elif self.alert == 1: self.audio.beep(critical=False)

        # Voice — CRITICAL ONLY
        if self.alert == 2:
            self.voice.speak('critical')

    # ══════════════════════════════════════════════════
    #  UI CONSTRUCTION
    # ══════════════════════════════════════════════════
    def _build_ui(self):
        sty = ttk.Style()
        sty.theme_use('clam')
        sty.configure('Drowsy.Horizontal.TProgressbar',
                       background=RED, troughcolor=CARD2,
                       bordercolor=BORDER, lightcolor=RED, darkcolor='#b71c1c')
        sty.configure('Calib.Horizontal.TProgressbar',
                       background=BLUE, troughcolor=CARD2,
                       bordercolor=BORDER, lightcolor=BLUE, darkcolor='#0288d1')

        # ── Header ────────────────────────────────────
        hdr = tk.Frame(self.root, bg=HDR, height=44)
        hdr.pack(fill='x'); hdr.pack_propagate(False)

        tk.Label(hdr, text="  Drowsiness Detection System",
                 font=('Segoe UI', 13, 'bold'), fg=BLUE, bg=HDR,
                 ).pack(side='left', padx=12, pady=6)
        tk.Label(hdr, text="v3.2",
                 font=('Segoe UI', 8), fg=MUTED, bg=HDR,
                 ).pack(side='left', pady=12)

        rc = tk.Frame(hdr, bg=HDR); rc.pack(side='right', padx=10)
        self._v_voice_hdr = tk.StringVar(value='Voice: init...')
        self._lbl_vhdr = tk.Label(rc, textvariable=self._v_voice_hdr,
                                   font=('Segoe UI', 8), fg=PURPLE, bg=HDR)
        self._lbl_vhdr.pack(side='left', padx=(0, 12))
        self._v_fps = tk.StringVar(value='-- fps')
        tk.Label(rc, textvariable=self._v_fps,
                 font=('Consolas', 9), fg=MUTED, bg=HDR,
                 ).pack(side='left', padx=(0, 14))
        sc = GREEN if self.stm32.ok else MUTED
        st = f"Serial: {SERIAL_PORT}" if self.stm32.ok else "No serial"
        tk.Label(rc, text=st, font=('Segoe UI', 8), fg=sc, bg=HDR,
                 ).pack(side='left')

        # ── Body ──────────────────────────────────────
        body = tk.Frame(self.root, bg=BG)
        body.pack(fill='both', expand=True, padx=8, pady=6)

        # Left: camera panel
        left = tk.Frame(body, bg=BG)
        left.pack(side='left', fill='y')

        cam_border = tk.Frame(left, bg=BORDER, bd=1)
        cam_border.pack()
        self.cam_lbl = tk.Label(cam_border, bg='#050505')
        self.cam_lbl.pack()

        # Calibration bar (below camera, shown only during calibration)
        self.cal_frame = tk.Frame(left, bg=BG)
        self._cal_bar_var = tk.DoubleVar(value=0.0)
        ttk.Progressbar(self.cal_frame, variable=self._cal_bar_var,
                        maximum=100, length=VIDEO_W,
                        style='Calib.Horizontal.TProgressbar').pack(fill='x')
        self._cal_text = tk.StringVar(value='')
        tk.Label(self.cal_frame, textvariable=self._cal_text,
                 font=('Segoe UI', 9, 'bold'), fg=BLUE, bg=CARD2,
                 anchor='center', pady=3).pack(fill='x')

        # ── Session stats + buttons BELOW camera (left panel) ────────
        self._build_stats_below_camera(left)

        # Right: SCROLLABLE sidebar (5 cards — all fit now)
        self._sb_scroll = ScrollableFrame(body, width=SIDEBAR_W)
        self._sb_scroll.pack(side='left', fill='both', expand=True, padx=(8, 0))
        sb = self._sb_scroll.inner   # all cards go here

        self._build_alert_card(sb)
        self._build_metrics_card(sb)
        self._build_graph_card(sb)
        self._build_meter_card(sb)
        self._build_voice_card(sb)

    # ── Card helpers ──────────────────────────────────
    def _card(self, parent, pady_bottom=5):
        outer = tk.Frame(parent, bg=BORDER)
        outer.pack(fill='x', pady=(0, pady_bottom))
        inner = tk.Frame(outer, bg=CARD, padx=10, pady=7)
        inner.pack(fill='x', padx=1, pady=1)
        return outer, inner

    def _sec_title(self, parent, text):
        tk.Label(parent, text=text, font=('Segoe UI', 7, 'bold'),
                 fg=MUTED, bg=CARD).pack(anchor='w', pady=(0, 5))

    # ── Alert card ────────────────────────────────────
    def _build_alert_card(self, sb):
        self._al_outer = tk.Frame(sb, bg=BORDER)
        self._al_outer.pack(fill='x', pady=(0, 5))
        self._al_inner = tk.Frame(self._al_outer, bg='#041504', padx=10, pady=9)
        self._al_inner.pack(fill='x', padx=1, pady=1)

        self._al_dot = tk.Label(self._al_inner, text='●',
                                 font=('Arial', 20), fg=GREEN, bg='#041504')
        self._al_dot.pack(side='left')

        txt = tk.Frame(self._al_inner, bg='#041504')
        txt.pack(side='left', padx=(8, 0), fill='x', expand=True)
        self._al_main = tk.Label(txt, text='AWAKE \u2014 NORMAL',
                                  font=('Segoe UI', 12, 'bold'),
                                  fg=GREEN, bg='#041504', anchor='w')
        self._al_main.pack(fill='x')
        self._al_sub = tk.Label(txt, text='Stay alert and focused on the road',
                                 font=('Segoe UI', 8),
                                 fg='#1e5a1e', bg='#041504', anchor='w')
        self._al_sub.pack(fill='x')

    # ── Metrics card ──────────────────────────────────
    def _build_metrics_card(self, sb):
        _, card = self._card(sb)
        self._sec_title(card, "LIVE METRICS")
        top = tk.Frame(card, bg=CARD); top.pack(fill='x')

        ef = tk.Frame(top, bg=CARD); ef.pack(side='left', expand=True)
        tk.Label(ef, text='EAR', font=('Segoe UI', 8), fg=MUTED, bg=CARD).pack(anchor='w')
        self._v_ear = tk.StringVar(value='---')
        self._lbl_ear = tk.Label(ef, textvariable=self._v_ear,
                                  font=('Consolas', 24, 'bold'), fg=GREEN, bg=CARD)
        self._lbl_ear.pack(anchor='w')

        mf = tk.Frame(top, bg=CARD); mf.pack(side='left', expand=True)
        tk.Label(mf, text='MAR', font=('Segoe UI', 8), fg=MUTED, bg=CARD).pack(anchor='w')
        self._v_mar = tk.StringVar(value='---')
        self._lbl_mar = tk.Label(mf, textvariable=self._v_mar,
                                  font=('Consolas', 24, 'bold'), fg=BLUE, bg=CARD)
        self._lbl_mar.pack(anchor='w')

        bot = tk.Frame(card, bg=CARD); bot.pack(fill='x', pady=(6, 0))
        tk.Label(bot, text='Threshold:', font=('Segoe UI', 8), fg=MUTED, bg=CARD).pack(side='left')
        self._v_thr = tk.StringVar(value=f'{EAR_THRESHOLD_DEFAULT:.3f}')
        tk.Label(bot, textvariable=self._v_thr,
                 font=('Consolas', 9, 'bold'), fg=RED, bg=CARD).pack(side='left', padx=(4, 16))
        tk.Label(bot, text='Face:', font=('Segoe UI', 8), fg=MUTED, bg=CARD).pack(side='left')
        self._v_face = tk.StringVar(value='---')
        self._lbl_face = tk.Label(bot, textvariable=self._v_face,
                                   font=('Segoe UI', 9, 'bold'), fg=MUTED, bg=CARD)
        self._lbl_face.pack(side='left', padx=(4, 0))

    # ── Graph card ────────────────────────────────────
    def _build_graph_card(self, sb):
        _, card = self._card(sb)
        self._sec_title(card, "EAR TREND  (last 100 frames)")
        self.gcanvas = tk.Canvas(card, width=self._GW, height=self._GH,
                                  bg=GRAPH_BG, highlightthickness=1,
                                  highlightbackground=BORDER)
        self.gcanvas.pack(fill='x')

    # ── Meter card ────────────────────────────────────
    def _build_meter_card(self, sb):
        _, card = self._card(sb)
        self._sec_title(card, "DROWSINESS BUILDUP")
        self._v_meter = tk.DoubleVar(value=0)
        ttk.Progressbar(card, variable=self._v_meter,
                        maximum=DROWSY_FRAME_COUNT,
                        style='Drowsy.Horizontal.TProgressbar',
                        length=self._GW).pack(fill='x')
        self._v_meter_txt = tk.StringVar(value=f'0 / {DROWSY_FRAME_COUNT} frames')
        tk.Label(card, textvariable=self._v_meter_txt,
                 font=('Segoe UI', 7), fg=MUTED, bg=CARD, anchor='e').pack(fill='x', pady=(2,0))

    # ── Bluetooth Voice card ──────────────────────────
    def _build_voice_card(self, sb):
        _, card = self._card(sb)
        self._sec_title(card, "BLUETOOTH VOICE  (Critical only)")

        row1 = tk.Frame(card, bg=CARD); row1.pack(fill='x')
        self._voice_dot = tk.Label(row1, text='●', font=('Arial', 12), fg=MUTED, bg=CARD)
        self._voice_dot.pack(side='left')
        vcol = tk.Frame(row1, bg=CARD); vcol.pack(side='left', padx=(6,0), fill='x', expand=True)
        self._v_voice_status = tk.StringVar(value='Initialising...')
        tk.Label(vcol, textvariable=self._v_voice_status,
                 font=('Segoe UI', 9, 'bold'), fg=PURPLE, bg=CARD, anchor='w').pack(fill='x')
        self._v_voice_dev = tk.StringVar(value='Output: detecting...')
        tk.Label(vcol, textvariable=self._v_voice_dev,
                 font=('Segoe UI', 7), fg=MUTED, bg=CARD, anchor='w').pack(fill='x')

        self._voice_btn_var = tk.StringVar(value='Voice  ON')
        self._voice_btn = tk.Button(card, textvariable=self._voice_btn_var,
                                     command=self._toggle_voice,
                                     font=('Segoe UI', 9, 'bold'), fg=TEXT,
                                     bg='#1a2540', activebackground='#253050',
                                     relief='flat', pady=5, cursor='hand2', bd=0)
        self._voice_btn.pack(fill='x', pady=(6,0))

        row2 = tk.Frame(card, bg=CARD); row2.pack(fill='x', pady=(5,0))
        tk.Label(row2, text='Last:', font=('Segoe UI', 7), fg=MUTED, bg=CARD).pack(side='left')
        self._v_last_spoken = tk.StringVar(value='None yet')
        tk.Label(row2, textvariable=self._v_last_spoken,
                 font=('Segoe UI', 7), fg=PURPLE, bg=CARD,
                 wraplength=210, anchor='w').pack(side='left', padx=(4,0))

    # ── Stats + Buttons below camera (left panel) ─────
    def _build_stats_below_camera(self, left):
        """
        Session stats in a 2x2 horizontal grid + action buttons,
        placed below the camera feed using the empty space there.
        Total width matches VIDEO_W (640px).
        """
        container = tk.Frame(left, bg=BORDER)
        container.pack(fill='x', pady=(6, 0))
        inner = tk.Frame(container, bg=CARD2, padx=10, pady=8)
        inner.pack(fill='x', padx=1, pady=1)

        # Title row
        title_row = tk.Frame(inner, bg=CARD2)
        title_row.pack(fill='x', pady=(0, 6))
        tk.Label(title_row, text='SESSION STATISTICS',
                 font=('Segoe UI', 7, 'bold'), fg=MUTED, bg=CARD2,
                 ).pack(side='left')

        # Stats in 2 columns × 2 rows (horizontal, compact)
        stats_grid = tk.Frame(inner, bg=CARD2)
        stats_grid.pack(fill='x')

        rows = [
            ("Duration",      YELLOW, 0, 0),
            ("Drowsy Events", RED,    0, 1),
            ("Yawn Events",   ORANGE, 1, 0),
            ("Avg EAR",       TEXT,   1, 1),
        ]
        self._sv = []
        for label, color, row, col in rows:
            cell = tk.Frame(stats_grid, bg=CARD2)
            cell.grid(row=row, column=col, sticky='w',
                      padx=(0, 30), pady=2)
            tk.Label(cell, text=label,
                     font=('Segoe UI', 8), fg=MUTED, bg=CARD2,
                     ).pack(side='left')
            v = tk.StringVar(value='--')
            tk.Label(cell, textvariable=v,
                     font=('Consolas', 11, 'bold'), fg=color, bg=CARD2,
                     ).pack(side='left', padx=(8, 0))
            self._sv.append(v)

        # Buttons row below stats
        btn_row = tk.Frame(inner, bg=CARD2)
        btn_row.pack(fill='x', pady=(10, 0))

        tk.Button(btn_row, text='\u27f3  Re-Calibrate',
                  command=self.start_calibration,
                  font=('Segoe UI', 10), fg=TEXT,
                  bg='#1a3520', activebackground='#244a2c',
                  relief='flat', pady=7, cursor='hand2', bd=0,
                  ).pack(side='left', fill='x', expand=True, padx=(0, 6))

        tk.Button(btn_row, text='\u2715  Quit',
                  command=self.on_close,
                  font=('Segoe UI', 10), fg=TEXT,
                  bg='#3a1414', activebackground='#4a2020',
                  relief='flat', pady=7, cursor='hand2', bd=0,
                  ).pack(side='left', fill='x', expand=True)



    # ══════════════════════════════════════════════════
    #  CALIBRATION
    # ══════════════════════════════════════════════════
    def start_calibration(self):
        self.calibrating = True
        self.csamples    = []
        self.cstart      = time.time()
        self.dcnt = self.rcnt = self.alert = 0
        self.cal_frame.pack(fill='x', pady=(5, 0))
        print("[CALIB] Started — keep eyes open normally...")

    def finish_calibration(self):
        self.calibrating = False
        self.cal_frame.pack_forget()
        if len(self.csamples) >= 10:
            base = float(np.mean(self.csamples))
            self.thr = base * 0.80
            print(f"[CALIB] Baseline={base:.3f}  Threshold={self.thr:.3f}")
        else:
            self.thr = EAR_THRESHOLD_DEFAULT
            print(f"[CALIB] Default threshold={self.thr:.3f}")
        self._v_thr.set(f'{self.thr:.3f}')

    # ══════════════════════════════════════════════════
    #  EAR Graph
    # ══════════════════════════════════════════════════
    def draw_graph(self):
        c = self.gcanvas
        c.delete('all')
        W, H = self._GW, self._GH
        PL, PR, PT, PB = 28, 6, 4, 14
        DW, DH = W-PL-PR, H-PT-PB
        EMIN, EMAX = 0.05, 0.50

        def tx(i): return PL + (i/(GRAPH_HISTORY_LEN-1))*DW
        def ty(v):
            v = max(EMIN, min(EMAX, v))
            return PT + (1-(v-EMIN)/(EMAX-EMIN))*DH

        c.create_rectangle(PL, PT, PL+DW, PT+DH, fill=GRAPH_BG, outline=BORDER)
        for yv in [0.10, 0.20, 0.30, 0.40]:
            gy = ty(yv)
            c.create_line(PL, gy, PL+DW, gy, fill=GRAPH_GRD, dash=(3,3))
            c.create_text(PL-3, gy, text=f'{yv:.1f}', anchor='e',
                          fill='#404040', font=('Consolas', 6))

        thr_y = ty(self.thr)
        if thr_y < PT+DH:
            c.create_rectangle(PL, thr_y, PL+DW, PT+DH, fill='#1a0505', outline='')
        c.create_line(PL, thr_y, PL+DW, thr_y, fill=RED, dash=(5,3), width=1.5)
        c.create_text(PL+DW+2, thr_y, text='T', anchor='w', fill=RED, font=('Consolas',7))

        pts = list(self.ear_hist)
        n = len(pts)
        if n < 2: return

        poly = []
        for i, v in enumerate(pts): poly += [tx(i), ty(v)]
        poly += [tx(n-1), PT+DH, PL, PT+DH]
        if len(poly) >= 6:
            c.create_polygon(poly, fill='#0d2010', outline='')
        for i in range(1, n):
            c.create_line(tx(i-1), ty(pts[i-1]), tx(i), ty(pts[i]),
                          fill=(RED if pts[i] < self.thr else GREEN), width=1.5)
        c.create_text(PL+DW//2, H-2, text='<-- frames -->',
                      fill='#2a2a2a', font=('Consolas',6))

    # ══════════════════════════════════════════════════
    #  DASHBOARD UPDATE  (called from GUI loop)
    # ══════════════════════════════════════════════════
    def _update_dash(self, ear_v, mar_v, face):
        if self.calibrating:    state = 'calibrate'
        elif not face:          state = 'noface'
        elif self.alert == 2:   state = 'critical'
        elif self.alert == 1:   state = 'warning'
        elif self.yawning:      state = 'yawn'
        else:                   state = 'normal'

        cbg, dot, headline = STATES[state]
        sub = SUB_MSG[state]

        self._al_outer.config(bg=dot if state=='critical' else BORDER)
        self._al_inner.config(bg=cbg)
        self._al_dot.config(fg=dot, bg=cbg)
        self._al_main.config(text=headline, fg=dot, bg=cbg)
        self._al_sub.config(text=sub, fg=self._dim(dot), bg=cbg)

        if face:
            self._v_ear.set(f'{ear_v:.3f}')
            self._v_mar.set(f'{mar_v:.3f}')
            self._lbl_ear.config(fg=RED if ear_v < self.thr else GREEN)
            self._lbl_mar.config(fg=ORANGE if self.yawning else BLUE)
            self._v_face.set('Detected  v'); self._lbl_face.config(fg=GREEN)
        else:
            self._v_ear.set('---'); self._v_mar.set('---')
            self._v_face.set('Not found  x'); self._lbl_face.config(fg=MUTED)
        self._v_thr.set(f'{self.thr:.3f}')

        self._v_meter.set(min(self.dcnt, DROWSY_FRAME_COUNT))
        self._v_meter_txt.set(f'{self.dcnt} / {DROWSY_FRAME_COUNT} frames')

        # Last spoken message
        if self.alert == 2 and self.voice.is_ready():
            msgs = VOICE_MESSAGES['critical']
            idx  = (self.voice._msg_idx.get('critical', 1)-1) % len(msgs)
            self._v_last_spoken.set(msgs[idx])

        self.draw_graph()

        elapsed = time.time() - self.t0
        mm, ss  = divmod(int(elapsed), 60)
        hh, mm  = divmod(mm, 60)
        ts  = f'{hh:02d}:{mm:02d}:{ss:02d}' if hh else f'{mm:02d}:{ss:02d}'
        avg = self.esum / self.en if self.en else 0.0
        for sv, val in zip(self._sv,
                           [ts, str(self.devents), str(self.yevents), f'{avg:.3f}']):
            sv.set(val)

    @staticmethod
    def _dim(hex_color):
        try:
            r = int(hex_color[1:3], 16)//3
            g = int(hex_color[3:5], 16)//3
            b = int(hex_color[5:7], 16)//3
            return f'#{r:02x}{g:02x}{b:02x}'
        except Exception:
            return '#444444'

    # ══════════════════════════════════════════════════
    #  VOICE TOGGLE + STATUS TICKER
    # ══════════════════════════════════════════════════
    def _toggle_voice(self):
        self._voice_on = not self._voice_on
        self.voice.set_enabled(self._voice_on)
        if self._voice_on:
            self._voice_btn_var.set('Voice  ON')
            self._voice_btn.config(bg='#1a2540', activebackground='#253050')
        else:
            self._voice_btn_var.set('Voice  OFF (muted)')
            self._voice_btn.config(bg='#252525', activebackground='#303030')

    def _tick_voice_status(self):
        if self.voice.is_ready():
            dev = self.voice.device_name()
            self._v_voice_status.set('Ready  (ON)' if self._voice_on else 'Muted')
            self._v_voice_dev.set(f'Output: {dev}')
            self._voice_dot.config(fg=PURPLE if self._voice_on else MUTED)
            self._v_voice_hdr.set(dev[:24] if self._voice_on else 'Voice muted')
            self._lbl_vhdr.config(fg=PURPLE if self._voice_on else MUTED)
        else:
            self._v_voice_status.set('Initialising...')
            self._voice_dot.config(fg=MUTED)
            self._v_voice_hdr.set('Voice: init...')
        self.root.after(1000, self._tick_voice_status)

    # ══════════════════════════════════════════════════
    #  BLINK ANIMATION
    # ══════════════════════════════════════════════════
    def _tick_blink(self):
        if self.alert == 2:
            self._blink_on = not self._blink_on
            try:
                self._al_dot.config(fg=RED if self._blink_on else '#8b0000')
            except Exception:
                pass
        self.root.after(500, self._tick_blink)

    # ══════════════════════════════════════════════════
    #  CLEANUP
    # ══════════════════════════════════════════════════
    def on_close(self):
        print("[INFO] Shutting down...")
        self._running = False
        try: self.stm32.send('W'); self.stm32.close()
        except Exception: pass
        try: self.logger.close()
        except Exception: pass
        try: self.cap.release()
        except Exception: pass

        elapsed = time.time() - self.t0
        mm, ss  = divmod(int(elapsed), 60)
        avg     = self.esum / self.en if self.en else 0.0
        print("─" * 52)
        print("   Session Summary")
        print(f"   Duration      : {mm:02d}:{ss:02d}")
        print(f"   Drowsy events : {self.devents}")
        print(f"   Yawn events   : {self.yevents}")
        print(f"   Average EAR   : {avg:.3f}")
        print(f"   Threshold     : {self.thr:.3f}")
        print(f"   Log file      : {self.logger.path}")
        print("─" * 52)
        self.root.destroy()


# ═══════════════════════════════════════════════════════════
#  ENTRY POINT
# ═══════════════════════════════════════════════════════════
def main():
    print("=" * 60)
    print("   Drowsiness Detection System  v3.2  — BT Voice Edition")
    print("=" * 60)
    print("  BLUETOOTH: Set speaker as default output in Sound Settings")
    print("  SCROLL:    Mouse wheel on sidebar to see all cards")
    print()
    root = tk.Tk()
    DrowsinessApp(root)
    root.mainloop()

if __name__ == "__main__":
    main()
