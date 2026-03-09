import streamlit as st
import json
import os
import time
from datetime import datetime

STATE_FILE = os.path.join(os.path.dirname(__file__), "..", "state.json")

st.set_page_config(
    page_title="🏀 Basketball Jury Panel",
    page_icon="🏀",
    layout="wide",
    initial_sidebar_state="collapsed"
)

# ─────────────────────────────────────────────
# CSS
# ─────────────────────────────────────────────
st.markdown("""
<style>
@import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700;900&family=Orbitron:wght@700;900&display=swap');

html, body, [class*="css"] { font-family: 'Inter', sans-serif; }

/* Hide Streamlit default header/toolbar */
header[data-testid="stHeader"] { display: none !important; }
#MainMenu { display: none !important; }
footer { display: none !important; }

.main { background: #0d0d0d; }
.block-container { padding: 0.5rem 2rem 1rem 2rem !important; max-width: 100% !important; }

/* Scoreboard */
.scoreboard {
    background: linear-gradient(135deg, #0a0a1a 0%, #1a1a2e 100%);
    border: 2px solid #f5a623;
    border-radius: 16px;
    padding: 1.5rem;
    text-align: center;
    margin-bottom: 1rem;
    box-shadow: 0 0 30px rgba(245,166,35,0.2);
}
.score-display {
    font-family: 'Orbitron', monospace;
    font-size: 4rem;
    font-weight: 900;
    color: #f5a623;
    letter-spacing: 0.1em;
    text-shadow: 0 0 20px rgba(245,166,35,0.5);
}
.team-name {
    font-size: 1.4rem;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 0.15em;
}
.clock-display {
    font-family: 'Orbitron', monospace;
    font-size: 3rem;
    font-weight: 900;
    color: #00ff88;
    text-shadow: 0 0 15px rgba(0,255,136,0.4);
}
.shot-clock {
    font-family: 'Orbitron', monospace;
    font-size: 1.8rem;
    font-weight: 700;
    color: #ff4444;
    text-shadow: 0 0 10px rgba(255,68,68,0.4);
}
.quarter-badge {
    background: #f5a623;
    color: #000;
    font-weight: 900;
    font-size: 1rem;
    padding: 4px 16px;
    border-radius: 20px;
    display: inline-block;
    text-transform: uppercase;
    letter-spacing: 0.1em;
}

/* Section headers */
.section-header {
    background: linear-gradient(90deg, #1e3a5f, #0d0d0d);
    border-left: 4px solid #f5a623;
    color: #fff;
    font-weight: 700;
    font-size: 0.9rem;
    letter-spacing: 0.12em;
    text-transform: uppercase;
    padding: 8px 16px;
    border-radius: 0 8px 8px 0;
    margin: 0.8rem 0 0.5rem 0;
}

/* Buttons */
.stButton > button {
    border-radius: 8px !important;
    font-weight: 700 !important;
    letter-spacing: 0.05em !important;
    transition: all 0.15s ease !important;
    border: none !important;
}

/* Score buttons */
div[data-testid="column"] .stButton > button[kind="secondary"] {
    background: #1e3a5f !important;
    color: #fff !important;
}

.btn-score-1 > button { background: #2d5a27 !important; color: #fff !important; font-size: 1rem !important; }
.btn-score-2 > button { background: #1a5c8a !important; color: #fff !important; font-size: 1.1rem !important; }
.btn-score-3 > button { background: #7a3a0a !important; color: #fff !important; font-size: 1.2rem !important; }

.btn-foul > button { background: #8b1a1a !important; color: #fff !important; }
.btn-timeout > button { background: #4a3a00 !important; color: #ffd700 !important; }
.btn-undo > button { background: #333 !important; color: #aaa !important; }
.btn-danger > button { background: #6b0000 !important; color: #ff8888 !important; }
.btn-success > button { background: #1a5c1a !important; color: #88ff88 !important; }
.btn-clock > button { background: #1a3a1a !important; color: #00ff88 !important; font-size: 1.3rem !important; font-family: monospace !important; }

/* Status indicators */
.status-pill {
    display: inline-block;
    padding: 4px 14px;
    border-radius: 20px;
    font-size: 0.78rem;
    font-weight: 700;
    letter-spacing: 0.08em;
}
.status-running { background: #1a5c1a; color: #00ff88; }
.status-stopped { background: #5c1a1a; color: #ff8888; }
.status-possession { background: #1a3a5f; color: #88aaff; }

/* Foul dots */
.foul-dot {
    display: inline-block;
    width: 14px; height: 14px;
    border-radius: 50%;
    margin: 2px;
}
.foul-active { background: #ff4444; box-shadow: 0 0 6px #ff4444; }
.foul-inactive { background: #333; }

/* Event log */
.event-log {
    background: #0a0a0a;
    border: 1px solid #222;
    border-radius: 8px;
    padding: 0.5rem;
    height: 220px;
    overflow-y: auto;
    font-size: 0.8rem;
    font-family: monospace;
}
.event-item { padding: 3px 6px; border-bottom: 1px solid #111; color: #ccc; }
.event-score { color: #f5a623; }
.event-foul { color: #ff8888; }
.event-timeout { color: #ffd700; }
.event-clock { color: #88aaff; }
.event-quarter { color: #00ff88; font-weight: bold; }

/* Stat box */
.stat-box {
    background: #111;
    border: 1px solid #333;
    border-radius: 8px;
    padding: 0.6rem;
    text-align: center;
}
.stat-val { font-size: 1.6rem; font-weight: 900; color: #f5a623; font-family: 'Orbitron', monospace; }
.stat-lbl { font-size: 0.7rem; color: #888; text-transform: uppercase; letter-spacing: 0.1em; }

/* Dark inputs */
.stNumberInput input, .stTextInput input, .stSelectbox select {
    background: #111 !important;
    color: #fff !important;
    border-color: #333 !important;
}
label { color: #ccc !important; font-size: 0.85rem !important; }

/* Sidebar */
section[data-testid="stSidebar"] { background: #0a0a1a !important; }

/* Violations panel */
.violation-btn > button {
    background: #3a1a00 !important;
    color: #ffa040 !important;
    font-size: 0.8rem !important;
    padding: 6px !important;
}
</style>
""", unsafe_allow_html=True)

# ─────────────────────────────────────────────
# State helpers
# ─────────────────────────────────────────────
DEFAULT_STATE = {
    "team_a": {"name": "Team A", "score": 0, "fouls": 0, "timeouts": 3, "color": "#1a3a6b"},
    "team_b": {"name": "Team B", "score": 0, "fouls": 0, "timeouts": 3, "color": "#8b1a1a"},
    "quarter": 1, "game_clock": "10:00", "shot_clock": 24,
    "period_minutes": 10, "clock_running": False, "game_started": False,
    "game_over": False, "overtime": False, "possession": "A",
    "last_action": "", "events": [], "players_a": [], "players_b": [],
    "fouls_limit": 5, "team_fouls_limit": 10, "timeouts_per_half": 3,
    "shot_clock_reset": 24, "last_updated": 0
}

def load_state():
    try:
        with open(STATE_FILE, "r") as f:
            data = json.load(f)
        # Ensure required keys exist (merge with defaults)
        state = dict(DEFAULT_STATE)
        state.update(data)
        if "team_a" not in data:
            state["team_a"] = dict(DEFAULT_STATE["team_a"])
        if "team_b" not in data:
            state["team_b"] = dict(DEFAULT_STATE["team_b"])
        return state
    except:
        return dict(DEFAULT_STATE)

def save_state(state):
    state["last_updated"] = time.time()
    with open(STATE_FILE, "w") as f:
        json.dump(state, f, indent=2)

def log_event(state, category, msg):
    q = state.get("quarter", 1)
    clock = state.get("game_clock", "--:--")
    label = f"Q{q}" if q <= 4 else f"OT{q-4}"
    entry = {"time": clock, "period": label, "msg": msg, "cat": category, "ts": time.time()}
    events = state.get("events", [])
    events.insert(0, entry)
    state["events"] = events[:100]

def parse_clock(s):
    try:
        s = str(s)
        if ":" in s:
            parts = s.split(":")
            return int(parts[0]) * 60 + float(parts[1])
        else:
            return float(s)
    except:
        return 0.0

def format_clock(seconds):
    seconds = max(0.0, float(seconds))
    tenths = int((seconds % 1) * 10)
    if seconds < 60.0:
        # Last minute: SS.t
        return f"{int(seconds):02d}.{tenths}"
    # Normal play: MM:SS.t
    m = int(seconds) // 60
    s = int(seconds) % 60
    return f"{m:02d}:{s:02d}.{tenths}"

# ─────────────────────────────────────────────
# Header
# ─────────────────────────────────────────────
st.markdown("""
<div style='text-align:center; padding: 0.5rem 0 0.2rem 0;'>
  <span style='font-family:Orbitron,monospace; font-size:1.8rem; font-weight:900; color:#f5a623;
    letter-spacing:0.15em; text-shadow: 0 0 20px rgba(245,166,35,0.4);'>
    🏀 BASKETBALL JURY CONTROL PANEL
  </span><br>
  <span style='color:#666; font-size:0.78rem; letter-spacing:0.2em; text-transform:uppercase;'>
    Official Scorekeeping &amp; Game Management System
  </span>
</div>
""", unsafe_allow_html=True)

state = load_state()

# ─────────────────────────────────────────────
# TICK CLOCK (time-based, runs on every page load)
# ─────────────────────────────────────────────
if state.get("clock_running") and not state.get("game_over"):
    elapsed = time.time() - state.get("last_updated", time.time())
    if elapsed > 0:
        game_secs = parse_clock(state.get("game_clock", "00:00"))
        shot_secs = float(state.get("shot_clock", 24))
        game_secs = max(0.0, game_secs - elapsed)
        shot_secs = max(0.0, shot_secs - elapsed)
        state["game_clock"] = format_clock(game_secs)
        state["shot_clock"] = round(shot_secs, 1)
        if game_secs == 0.0:
            _q = state.get("quarter", 1)
            _lbl = f"QUARTER {_q}" if _q <= 4 else f"OVERTIME {_q-4}"
            state["clock_running"] = False
            log_event(state, "clock", f"End of {_lbl}")
        save_state(state)

# ─────────────────────────────────────────────
# TOP SCOREBOARD
# ─────────────────────────────────────────────
q = state.get("quarter", 1)
period_label = f"QUARTER {q}" if q <= 4 else f"OVERTIME {q-4}"
running_html = '<span class="status-pill status-running">⏱ CLOCK RUNNING</span>' if state.get("clock_running") else '<span class="status-pill status-stopped">⏹ CLOCK STOPPED</span>'
poss = state.get("possession", "A")
poss_name = state["team_a"]["name"] if poss == "A" else state["team_b"]["name"]

col_left, col_mid, col_right = st.columns([2, 1.4, 2])

with col_left:
    st.markdown(f"""
    <div class="scoreboard" style="border-color:#1a3a6b;">
      <div class="team-name" style="color:#6699ff;">{state['team_a']['name']}</div>
      <div class="score-display">{state['team_a']['score']}</div>
      <div style="margin-top:8px; font-size:0.8rem; color:#888;">
        {'🟡 ' * state['team_a']['fouls']}<span style="color:#888;">Fouls: {state['team_a']['fouls']}</span>
        &nbsp;|&nbsp;
        <span style="color:#ffd700;">⏸ TOs: {state['team_a']['timeouts']}</span>
      </div>
    </div>
    """, unsafe_allow_html=True)

with col_mid:
    st.markdown(f"""
    <div class="scoreboard" style="padding:1rem;">
      <div><span class="quarter-badge">{period_label}</span></div>
      <div class="clock-display" style="margin: 0.5rem 0;">{state.get('game_clock','10:00')}</div>
      <div class="shot-clock">SHOT: {int(state.get('shot_clock',24))}s</div>
      <div style="margin-top:8px;">{running_html}</div>
      <div style="margin-top:6px;"><span class="status-pill status-possession">🏀 {poss_name}</span></div>
    </div>
    """, unsafe_allow_html=True)

with col_right:
    st.markdown(f"""
    <div class="scoreboard" style="border-color:#8b1a1a;">
      <div class="team-name" style="color:#ff8888;">{state['team_b']['name']}</div>
      <div class="score-display">{state['team_b']['score']}</div>
      <div style="margin-top:8px; font-size:0.8rem; color:#888;">
        {'🟡 ' * state['team_b']['fouls']}<span style="color:#888;">Fouls: {state['team_b']['fouls']}</span>
        &nbsp;|&nbsp;
        <span style="color:#ffd700;">⏸ TOs: {state['team_b']['timeouts']}</span>
      </div>
    </div>
    """, unsafe_allow_html=True)

st.divider()

# ─────────────────────────────────────────────
# MAIN CONTROL AREA
# ─────────────────────────────────────────────
main_col, right_panel = st.columns([3, 1.2])

with main_col:

    # ── CLOCK CONTROLS ──
    st.markdown('<div class="section-header">⏱ GAME CLOCK & PERIOD CONTROLS</div>', unsafe_allow_html=True)
    cc1, cc2, cc3, cc4, cc5 = st.columns(5)

    with cc1:
        if state.get("clock_running"):
            if st.button("⏸ PAUSE", use_container_width=True, key="pause"):
                state["clock_running"] = False
                log_event(state, "clock", f"Clock paused at {state['game_clock']}")
                save_state(state)
                st.rerun()
        else:
            if st.button("▶ START", use_container_width=True, key="start"):
                state["clock_running"] = True
                state["game_started"] = True
                log_event(state, "clock", f"Clock started at {state['game_clock']}")
                save_state(state)
                st.rerun()

    with cc2:
        if st.button("⏭ NEXT PERIOD", use_container_width=True, key="next_q"):
            q = state.get("quarter", 1)
            if q < 4:
                state["quarter"] = q + 1
                state["game_clock"] = format_clock(state.get("period_minutes", 10) * 60)
                state["shot_clock"] = 24
                state["clock_running"] = False
                state["team_a"]["timeouts"] = state.get("timeouts_per_half", 3) if q == 2 else state["team_a"]["timeouts"]
                state["team_b"]["timeouts"] = state.get("timeouts_per_half", 3) if q == 2 else state["team_b"]["timeouts"]
                log_event(state, "quarter", f"Q{q+1} started")
            elif q == 4:
                # Check if tied → overtime
                if state["team_a"]["score"] == state["team_b"]["score"]:
                    state["quarter"] = 5
                    state["game_clock"] = "05:00"
                    state["clock_running"] = False
                    log_event(state, "quarter", "OVERTIME begins!")
                else:
                    state["game_over"] = True
                    log_event(state, "quarter", "GAME OVER!")
            else:
                state["game_over"] = True
                log_event(state, "quarter", "GAME OVER!")
            save_state(state)
            st.rerun()

    with cc3:
        if st.button("🔄 RESET SHOT CLK", use_container_width=True, key="reset_shot"):
            state["shot_clock"] = 24
            log_event(state, "clock", "Shot clock reset to 24s")
            save_state(state)
            st.rerun()

    with cc4:
        if st.button("↩ RESET PERIOD", use_container_width=True, key="reset_period"):
            mins = state.get("period_minutes", 10)
            state["game_clock"] = format_clock(mins * 60)
            state["clock_running"] = False
            save_state(state)
            st.rerun()

    with cc5:
        if st.button("⚠️ RESET GAME", use_container_width=True, key="reset_game"):
            if st.session_state.get("confirm_reset"):
                mins = state.get("period_minutes", 10)
                ta_name = state["team_a"]["name"]
                tb_name = state["team_b"]["name"]
                state["team_a"] = {"name": ta_name, "score": 0, "fouls": 0, "timeouts": 3, "color": "#1a3a6b"}
                state["team_b"] = {"name": tb_name, "score": 0, "fouls": 0, "timeouts": 3, "color": "#8b1a1a"}
                state["quarter"] = 1
                state["game_clock"] = format_clock(mins * 60)
                state["shot_clock"] = 24
                state["clock_running"] = False
                state["game_started"] = False
                state["game_over"] = False
                state["events"] = []
                state["possession"] = "A"
                log_event(state, "quarter", "Game RESET by jury")
                st.session_state["confirm_reset"] = False
                save_state(state)
                st.rerun()
            else:
                st.session_state["confirm_reset"] = True
                st.warning("Click again to confirm game reset!")

    # Manual clock adjustment
    with st.expander("🕐 Manual Clock Adjustment", expanded=False):
        adj1, adj2, adj3 = st.columns(3)
        with adj1:
            new_clock = st.text_input("Set Game Clock (MM:SS)", value=state.get("game_clock","10:00"), key="manual_clock")
        with adj2:
            new_shot = st.number_input("Set Shot Clock (s)", min_value=0, max_value=24, value=int(state.get("shot_clock", 24)), key="manual_shot")
        with adj3:
            st.write("")
            st.write("")
            if st.button("✅ Apply Clock", use_container_width=True, key="apply_clock"):
                state["game_clock"] = new_clock
                state["shot_clock"] = int(new_shot)
                log_event(state, "clock", f"Clock manually set to {new_clock}, shot={new_shot}s")
                save_state(state)
                st.rerun()

    # ── SCORING ──
    st.markdown('<div class="section-header">🏀 SCORING</div>', unsafe_allow_html=True)

    scol1, scol2 = st.columns(2)

    with scol1:
        st.markdown(f"<div style='color:#6699ff; font-weight:700; text-align:center; font-size:1rem; margin-bottom:6px;'>▲ {state['team_a']['name']}</div>", unsafe_allow_html=True)
        sc1, sc2, sc3, sc4 = st.columns(4)
        with sc1:
            if st.button("➕1 pt", use_container_width=True, key="a1"):
                state["team_a"]["score"] += 1
                state["shot_clock"] = 24
                state["possession"] = "B"
                log_event(state, "score", f"🔵 {state['team_a']['name']} +1 (FT) → {state['team_a']['score']}")
                save_state(state); st.rerun()
        with sc2:
            if st.button("➕2 pt", use_container_width=True, key="a2"):
                state["team_a"]["score"] += 2
                state["shot_clock"] = 24
                state["possession"] = "B"
                log_event(state, "score", f"🔵 {state['team_a']['name']} +2 → {state['team_a']['score']}")
                save_state(state); st.rerun()
        with sc3:
            if st.button("➕3 pt", use_container_width=True, key="a3"):
                state["team_a"]["score"] += 3
                state["shot_clock"] = 24
                state["possession"] = "B"
                log_event(state, "score", f"🔵 {state['team_a']['name']} +3 → {state['team_a']['score']}")
                save_state(state); st.rerun()
        with sc4:
            if st.button("➖1", use_container_width=True, key="am1"):
                if state["team_a"]["score"] > 0:
                    state["team_a"]["score"] -= 1
                    log_event(state, "score", f"⚠️ {state['team_a']['name']} score corrected -{1}")
                    save_state(state); st.rerun()

    with scol2:
        st.markdown(f"<div style='color:#ff8888; font-weight:700; text-align:center; font-size:1rem; margin-bottom:6px;'>▲ {state['team_b']['name']}</div>", unsafe_allow_html=True)
        sc1, sc2, sc3, sc4 = st.columns(4)
        with sc1:
            if st.button("➕1 pt", use_container_width=True, key="b1"):
                state["team_b"]["score"] += 1
                state["shot_clock"] = 24
                state["possession"] = "A"
                log_event(state, "score", f"🔴 {state['team_b']['name']} +1 (FT) → {state['team_b']['score']}")
                save_state(state); st.rerun()
        with sc2:
            if st.button("➕2 pt", use_container_width=True, key="b2"):
                state["team_b"]["score"] += 2
                state["shot_clock"] = 24
                state["possession"] = "A"
                log_event(state, "score", f"🔴 {state['team_b']['name']} +2 → {state['team_b']['score']}")
                save_state(state); st.rerun()
        with sc3:
            if st.button("➕3 pt", use_container_width=True, key="b3"):
                state["team_b"]["score"] += 3
                state["shot_clock"] = 24
                state["possession"] = "A"
                log_event(state, "score", f"🔴 {state['team_b']['name']} +3 → {state['team_b']['score']}")
                save_state(state); st.rerun()
        with sc4:
            if st.button("➖1", use_container_width=True, key="bm1"):
                if state["team_b"]["score"] > 0:
                    state["team_b"]["score"] -= 1
                    log_event(state, "score", f"⚠️ {state['team_b']['name']} score corrected -1")
                    save_state(state); st.rerun()

    # ── FOULS & TIMEOUTS ──
    st.markdown('<div class="section-header">🟡 FOULS & TIMEOUTS</div>', unsafe_allow_html=True)

    ft1, ft2 = st.columns(2)

    with ft1:
        st.markdown(f"<div style='color:#6699ff; font-size:0.85rem; font-weight:600; margin-bottom:4px;'>{state['team_a']['name']}</div>", unsafe_allow_html=True)
        fa1, fa2 = st.columns(2)
        with fa1:
            if st.button(f"🟡 Add Foul", use_container_width=True, key="af"):
                f = state["team_a"]["fouls"]
                if f < 5:
                    state["team_a"]["fouls"] += 1
                    if state["team_a"]["fouls"] >= 5:
                        log_event(state, "foul", f"⚠️ {state['team_a']['name']} player FOULED OUT ({state['team_a']['fouls']} fouls)")
                    else:
                        log_event(state, "foul", f"🟡 {state['team_a']['name']} foul #{state['team_a']['fouls']}")
                    save_state(state); st.rerun()
                else:
                    st.warning("Max fouls reached!")
            if st.button("↩ Remove Foul", use_container_width=True, key="afr"):
                if state["team_a"]["fouls"] > 0:
                    state["team_a"]["fouls"] -= 1
                    log_event(state, "foul", f"↩ {state['team_a']['name']} foul corrected")
                    save_state(state); st.rerun()
        with fa2:
            if st.button("⏸ USE TIMEOUT", use_container_width=True, key="ato"):
                if state["team_a"]["timeouts"] > 0:
                    state["team_a"]["timeouts"] -= 1
                    state["clock_running"] = False
                    log_event(state, "timeout", f"⏸ {state['team_a']['name']} timeout called ({state['team_a']['timeouts']} left)")
                    save_state(state); st.rerun()
                else:
                    st.error("No timeouts remaining!")
            if st.button("➕ Add Timeout", use_container_width=True, key="atoa"):
                state["team_a"]["timeouts"] = min(3, state["team_a"]["timeouts"] + 1)
                save_state(state); st.rerun()

    with ft2:
        st.markdown(f"<div style='color:#ff8888; font-size:0.85rem; font-weight:600; margin-bottom:4px;'>{state['team_b']['name']}</div>", unsafe_allow_html=True)
        fb1, fb2 = st.columns(2)
        with fb1:
            if st.button(f"🟡 Add Foul", use_container_width=True, key="bf"):
                f = state["team_b"]["fouls"]
                if f < 5:
                    state["team_b"]["fouls"] += 1
                    if state["team_b"]["fouls"] >= 5:
                        log_event(state, "foul", f"⚠️ {state['team_b']['name']} player FOULED OUT ({state['team_b']['fouls']} fouls)")
                    else:
                        log_event(state, "foul", f"🟡 {state['team_b']['name']} foul #{state['team_b']['fouls']}")
                    save_state(state); st.rerun()
                else:
                    st.warning("Max fouls reached!")
            if st.button("↩ Remove Foul", use_container_width=True, key="bfr"):
                if state["team_b"]["fouls"] > 0:
                    state["team_b"]["fouls"] -= 1
                    log_event(state, "foul", f"↩ {state['team_b']['name']} foul corrected")
                    save_state(state); st.rerun()
        with fb2:
            if st.button("⏸ USE TIMEOUT", use_container_width=True, key="bto"):
                if state["team_b"]["timeouts"] > 0:
                    state["team_b"]["timeouts"] -= 1
                    state["clock_running"] = False
                    log_event(state, "timeout", f"⏸ {state['team_b']['name']} timeout called ({state['team_b']['timeouts']} left)")
                    save_state(state); st.rerun()
                else:
                    st.error("No timeouts remaining!")
            if st.button("➕ Add Timeout", use_container_width=True, key="btoa"):
                state["team_b"]["timeouts"] = min(3, state["team_b"]["timeouts"] + 1)
                save_state(state); st.rerun()

    # ── VIOLATIONS ──
    st.markdown('<div class="section-header">🚫 VIOLATIONS & RULE ENFORCEMENT</div>', unsafe_allow_html=True)

    violations = [
        ("🏃 Traveling", "traveling"),
        ("🔢 Double Dribble", "double_dribble"),
        ("3-sec Lane", "3sec_lane"),
        ("5-sec Inbound", "5sec_inbound"),
        ("8-sec Half", "8sec_half"),
        ("24-sec Shot", "24sec_shot"),
        ("🔄 Ball OOB", "oob"),
        ("🔙 Backcourt", "backcourt"),
        ("🤝 Held Ball", "held_ball"),
        ("🦶 Kicking", "kicking"),
    ]

    team_for_violation = st.selectbox("Violation charged to:", [state["team_a"]["name"], state["team_b"]["name"]], key="viol_team")
    vcols = st.columns(5)
    for idx, (label, key) in enumerate(violations):
        with vcols[idx % 5]:
            if st.button(label, use_container_width=True, key=f"viol_{key}"):
                state["clock_running"] = False
                opp = "B" if team_for_violation == state["team_a"]["name"] else "A"
                state["possession"] = opp
                log_event(state, "foul", f"🚫 {team_for_violation}: {label.replace('🏃','').replace('🔢','').replace('🔄','').replace('🔙','').replace('🤝','').replace('🦶','').strip()} violation")
                save_state(state); st.rerun()

    # ── POSSESSION ──
    st.markdown('<div class="section-header">🏀 POSSESSION & GAME EVENTS</div>', unsafe_allow_html=True)
    p1, p2, p3, p4 = st.columns(4)
    with p1:
        if st.button(f"🏀 Poss → {state['team_a']['name']}", use_container_width=True, key="poss_a"):
            state["possession"] = "A"
            log_event(state, "clock", f"Possession: {state['team_a']['name']}")
            save_state(state); st.rerun()
    with p2:
        if st.button(f"🏀 Poss → {state['team_b']['name']}", use_container_width=True, key="poss_b"):
            state["possession"] = "B"
            log_event(state, "clock", f"Possession: {state['team_b']['name']}")
            save_state(state); st.rerun()
    with p3:
        if st.button("🎽 JUMP BALL", use_container_width=True, key="jumpball"):
            state["clock_running"] = False
            log_event(state, "clock", "Jump ball called")
            save_state(state); st.rerun()
    with p4:
        if st.button("📢 TECHNICAL FOUL", use_container_width=True, key="tech"):
            tf_team = st.session_state.get("tf_team", state["team_a"]["name"])
            log_event(state, "foul", f"🔴 TECHNICAL FOUL assessed")
            save_state(state); st.rerun()

    # ── FREE THROWS ──
    with st.expander("🎯 Free Throw Management", expanded=False):
        ft_team = st.selectbox("Shooting team:", [state["team_a"]["name"], state["team_b"]["name"]], key="ft_team")
        ft_num = st.selectbox("Free throws awarded:", [1, 2, 3], key="ft_num")
        ft_made = st.number_input("Free throws made:", min_value=0, max_value=3, value=0, key="ft_made")
        if st.button("✅ Record Free Throws", use_container_width=True, key="ft_record"):
            team_key = "team_a" if ft_team == state["team_a"]["name"] else "team_b"
            state[team_key]["score"] += int(ft_made)
            opp = "B" if team_key == "team_a" else "A"
            state["possession"] = opp
            state["shot_clock"] = 24
            log_event(state, "score", f"🎯 {ft_team} FT: {ft_made}/{ft_num} → {state[team_key]['score']}")
            save_state(state); st.rerun()

# ─────────────────────────────────────────────
# RIGHT PANEL
# ─────────────────────────────────────────────
with right_panel:

    # ── SETUP ──
    with st.expander("⚙️ GAME SETUP", expanded=True):
        ta_name = st.text_input("Team A Name", value=state["team_a"]["name"], key="ta_name_input")
        tb_name = st.text_input("Team B Name", value=state["team_b"]["name"], key="tb_name_input")
        period_mins = st.selectbox("Period Length", [5, 8, 10, 12, 15, 20], index=2, key="period_mins")
        if st.button("✅ Apply Setup", use_container_width=True, key="apply_setup"):
            state["team_a"]["name"] = ta_name
            state["team_b"]["name"] = tb_name
            state["period_minutes"] = int(period_mins)
            state["game_clock"] = format_clock(int(period_mins) * 60)
            log_event(state, "clock", f"Setup: {ta_name} vs {tb_name}, {period_mins}min periods")
            save_state(state); st.rerun()

    # ── QUICK STATS ──
    st.markdown('<div class="section-header" style="font-size:0.75rem;">📊 QUICK STATS</div>', unsafe_allow_html=True)

    diff = state["team_a"]["score"] - state["team_b"]["score"]
    diff_str = f"+{diff}" if diff > 0 else str(diff)
    diff_col = "#00ff88" if diff > 0 else ("#ff4444" if diff < 0 else "#888")

    st.markdown(f"""
    <div style="display:grid; grid-template-columns:1fr 1fr; gap:6px; margin:6px 0;">
      <div class="stat-box"><div class="stat-val">{state['team_a']['score']}</div><div class="stat-lbl">{state['team_a']['name'][:8]}</div></div>
      <div class="stat-box"><div class="stat-val">{state['team_b']['score']}</div><div class="stat-lbl">{state['team_b']['name'][:8]}</div></div>
      <div class="stat-box"><div class="stat-val" style="color:{diff_col};">{diff_str}</div><div class="stat-lbl">Margin</div></div>
      <div class="stat-box"><div class="stat-val" style="color:#88aaff;">{state.get('quarter',1)}</div><div class="stat-lbl">Period</div></div>
      <div class="stat-box"><div class="stat-val" style="color:#ff8888;">{state['team_a']['fouls']}</div><div class="stat-lbl">{state['team_a']['name'][:6]} Fouls</div></div>
      <div class="stat-box"><div class="stat-val" style="color:#ff8888;">{state['team_b']['fouls']}</div><div class="stat-lbl">{state['team_b']['name'][:6]} Fouls</div></div>
    </div>
    """, unsafe_allow_html=True)

    # ── FOUL INDICATOR ──
    st.markdown('<div class="section-header" style="font-size:0.75rem;">🟡 FOUL TRACKER</div>', unsafe_allow_html=True)
    for team_key, color in [("team_a", "#6699ff"), ("team_b", "#ff8888")]:
        fouls = state[team_key]["fouls"]
        dots = ""
        for i in range(5):
            cls = "foul-active" if i < fouls else "foul-inactive"
            dots += f'<span class="foul-dot {cls}"></span>'
        st.markdown(f'<div style="color:{color}; font-size:0.78rem; font-weight:600;">{state[team_key]["name"]}</div>{dots}', unsafe_allow_html=True)
        if fouls >= 5:
            st.markdown(f'<span style="color:#ff4444; font-size:0.7rem; font-weight:700;">⛔ PLAYER FOUL OUT</span>', unsafe_allow_html=True)
        elif fouls >= 3:
            st.markdown(f'<span style="color:#ffd700; font-size:0.7rem;">⚠️ Foul trouble</span>', unsafe_allow_html=True)

    # ── EVENT LOG ──
    st.markdown('<div class="section-header" style="font-size:0.75rem;">📋 EVENT LOG</div>', unsafe_allow_html=True)
    events = state.get("events", [])
    cat_class = {"score": "event-score", "foul": "event-foul", "timeout": "event-timeout",
                 "clock": "event-clock", "quarter": "event-quarter"}
    log_html = '<div class="event-log">'
    for ev in events[:40]:
        cls = cat_class.get(ev.get("cat",""), "event-item")
        log_html += f'<div class="event-item {cls}">[{ev.get("period","?")} {ev.get("time","--:--")}] {ev.get("msg","")}</div>'
    log_html += '</div>'
    st.markdown(log_html, unsafe_allow_html=True)

    if st.button("🗑 Clear Log", use_container_width=True, key="clear_log"):
        state["events"] = []
        save_state(state); st.rerun()

# ─────────────────────────────────────────────
# GAME OVER BANNER
# ─────────────────────────────────────────────
if state.get("game_over"):
    winner = state["team_a"]["name"] if state["team_a"]["score"] > state["team_b"]["score"] else state["team_b"]["name"]
    st.markdown(f"""
    <div style="background:linear-gradient(135deg,#1a5c1a,#0a2a0a); border:2px solid #00ff88;
         border-radius:16px; padding:2rem; text-align:center; margin-top:1rem;">
      <div style="font-family:Orbitron,monospace; font-size:2rem; font-weight:900; color:#00ff88;">
        🏆 GAME OVER
      </div>
      <div style="font-size:1.3rem; color:#fff; margin-top:0.5rem;">
        WINNER: <strong style="color:#f5a623;">{winner}</strong>
      </div>
      <div style="font-size:1.8rem; font-family:Orbitron,monospace; color:#f5a623; margin-top:0.3rem;">
        {state['team_a']['score']} — {state['team_b']['score']}
      </div>
    </div>
    """, unsafe_allow_html=True)

# Auto-refresh: 0.1s always for smooth per-second updates and tenths in last minute
if state.get("clock_running") and not state.get("game_over"):
    time.sleep(0.1)
    st.rerun()
