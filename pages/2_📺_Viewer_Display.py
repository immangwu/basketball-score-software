import streamlit as st
import json
import os
import time

STATE_FILE = os.path.join(os.path.dirname(__file__), "..", "state.json")

st.set_page_config(
    page_title="🏀 Basketball Scoreboard",
    page_icon="🏀",
    layout="wide",
    initial_sidebar_state="collapsed"
)

st.markdown("""
<style>
@import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700;900&family=Orbitron:wght@700;900&display=swap');

html, body, [class*="css"] { font-family: 'Inter', sans-serif; background: #000 !important; }
.main { background: #000 !important; }
.block-container { padding: 0.5rem 1.5rem !important; max-width: 100% !important; background: #000; }

/* Main scoreboard */
.big-scoreboard {
    background: linear-gradient(180deg, #080818 0%, #0a0a1a 60%, #080810 100%);
    border: 3px solid #1a1a3a;
    border-radius: 24px;
    padding: 2.5rem 2rem;
    text-align: center;
    box-shadow: 0 0 60px rgba(100,120,255,0.08), inset 0 1px 0 rgba(255,255,255,0.05);
    position: relative;
    overflow: hidden;
}
.big-scoreboard::before {
    content: '';
    position: absolute;
    top: 0; left: 0; right: 0; height: 3px;
    background: linear-gradient(90deg, transparent, #f5a623, transparent);
}

/* Team scores */
.big-score {
    font-family: 'Orbitron', monospace;
    font-size: 9rem;
    font-weight: 900;
    line-height: 1;
    letter-spacing: -0.02em;
    text-shadow: 0 0 60px currentColor;
}
.score-a { color: #4d88ff; }
.score-b { color: #ff4d4d; }

.big-team-name {
    font-size: 2rem;
    font-weight: 900;
    text-transform: uppercase;
    letter-spacing: 0.2em;
    margin-bottom: 0.5rem;
}
.team-a-name { color: #6699ff; }
.team-b-name { color: #ff8888; }

/* VS divider */
.vs-divider {
    font-family: 'Orbitron', monospace;
    font-size: 2.5rem;
    font-weight: 900;
    color: #333;
    padding: 0 1rem;
    align-self: center;
}

/* Game clock */
.main-clock {
    font-family: 'Orbitron', monospace;
    font-size: 5rem;
    font-weight: 900;
    color: #00ff88;
    text-shadow: 0 0 30px rgba(0,255,136,0.4), 0 0 60px rgba(0,255,136,0.1);
    letter-spacing: 0.08em;
}
.clock-running { animation: clockPulse 1s ease-in-out infinite; }
@keyframes clockPulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.7; }
}

/* Period badge */
.period-badge {
    background: #f5a623;
    color: #000;
    font-family: 'Orbitron', monospace;
    font-weight: 900;
    font-size: 1.2rem;
    padding: 8px 28px;
    border-radius: 30px;
    display: inline-block;
    letter-spacing: 0.12em;
    text-transform: uppercase;
    box-shadow: 0 0 20px rgba(245,166,35,0.4);
}

/* Shot clock */
.shot-clock-display {
    font-family: 'Orbitron', monospace;
    font-size: 3rem;
    font-weight: 900;
    color: #ff4444;
    text-shadow: 0 0 20px rgba(255,68,68,0.5);
}
.shot-clock-urgent { color: #ff0000; animation: urgentPulse 0.4s ease-in-out infinite; }
@keyframes urgentPulse { 0%,100%{opacity:1;transform:scale(1)} 50%{opacity:0.6;transform:scale(1.05)} }

/* Info panels */
.info-panel {
    background: #080810;
    border: 1px solid #1a1a3a;
    border-radius: 16px;
    padding: 1.2rem 1rem;
    text-align: center;
}
.info-val {
    font-family: 'Orbitron', monospace;
    font-size: 2.5rem;
    font-weight: 900;
    color: #f5a623;
}
.info-lbl {
    font-size: 0.75rem;
    color: #555;
    text-transform: uppercase;
    letter-spacing: 0.15em;
    margin-top: 4px;
}

/* Foul indicator */
.foul-pip {
    display: inline-block;
    width: 18px; height: 18px;
    border-radius: 50%;
    margin: 3px;
    transition: all 0.3s;
}
.foul-on { background: #ff4444; box-shadow: 0 0 10px #ff4444; }
.foul-off { background: #1a1a2e; border: 1px solid #333; }
.foul-danger { background: #ff0000; box-shadow: 0 0 20px #ff0000; animation: dangerPulse 0.5s infinite; }
@keyframes dangerPulse { 0%,100%{box-shadow:0 0 10px #f00} 50%{box-shadow:0 0 25px #f00,0 0 40px #f00} }

/* Timeout pips */
.timeout-pip {
    display: inline-block;
    width: 14px; height: 14px;
    border-radius: 3px;
    margin: 2px;
}
.to-on { background: #ffd700; box-shadow: 0 0 6px #ffd700; }
.to-off { background: #2a2a2a; }

/* Possession arrow */
.possession-indicator {
    font-size: 3rem;
    line-height: 1;
    filter: drop-shadow(0 0 10px rgba(255,255,255,0.3));
}

/* Status bar */
.status-bar {
    background: #050510;
    border-top: 1px solid #1a1a2e;
    border-radius: 0 0 16px 16px;
    padding: 0.6rem 1.5rem;
    display: flex;
    justify-content: center;
    gap: 2rem;
    font-size: 0.8rem;
    color: #555;
}

/* Event ticker */
.ticker {
    background: #050508;
    border: 1px solid #1a1a2e;
    border-radius: 12px;
    padding: 0.8rem 1.2rem;
    font-size: 0.85rem;
    color: #888;
    font-family: monospace;
    overflow: hidden;
    position: relative;
}
.ticker-scroll {
    white-space: nowrap;
    animation: tickerScroll 30s linear infinite;
}
@keyframes tickerScroll {
  0% { transform: translateX(100%); }
  100% { transform: translateX(-100%); }
}

/* GAME OVER */
.game-over-banner {
    background: linear-gradient(135deg, #0a2a0a, #1a5c1a);
    border: 3px solid #00ff88;
    border-radius: 20px;
    padding: 3rem;
    text-align: center;
    animation: winnerGlow 2s ease-in-out infinite;
}
@keyframes winnerGlow {
  0%,100% { box-shadow: 0 0 30px rgba(0,255,136,0.3); }
  50% { box-shadow: 0 0 80px rgba(0,255,136,0.6); }
}

/* Grid layout */
.score-grid {
    display: grid;
    grid-template-columns: 1fr auto 1fr;
    align-items: center;
    gap: 1rem;
}

/* Responsive */
label, .stButton { display: none !important; }
section[data-testid="stSidebar"] { display: none !important; }
header[data-testid="stHeader"] { background: #000 !important; }

/* Stats row */
.stats-row {
    background: #04040c;
    border: 1px solid #111122;
    border-radius: 12px;
    padding: 1rem;
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 1rem;
    text-align: center;
}
.stat-cell { }
.stat-cell-val { font-family: 'Orbitron', monospace; font-size: 1.8rem; font-weight: 900; color: #f5a623; }
.stat-cell-lbl { font-size: 0.68rem; color: #444; text-transform: uppercase; letter-spacing: 0.1em; }
</style>
""", unsafe_allow_html=True)

# Load state
def load_state():
    try:
        with open(STATE_FILE, "r") as f:
            return json.load(f)
    except:
        return {}

state = load_state()
ta = state.get("team_a", {})
tb = state.get("team_b", {})
q = state.get("quarter", 1)
period_label = f"Q{q}" if q <= 4 else f"OT{q-4}"
full_period = f"QUARTER {q}" if q <= 4 else f"OVERTIME {q-4}"
clock_class = "main-clock clock-running" if state.get("clock_running") else "main-clock"
shot = int(state.get("shot_clock", 24))
shot_class = "shot-clock-urgent" if shot <= 5 else "shot-clock-display"
poss = state.get("possession", "A")

# ─────────────────────────────────────────────
# HEADER BAR
# ─────────────────────────────────────────────
st.markdown(f"""
<div style="display:flex; justify-content:space-between; align-items:center; padding:0.3rem 0 0.5rem 0;">
  <span style="font-family:Orbitron,monospace; font-size:1rem; font-weight:700; color:#f5a623; letter-spacing:0.15em;">
    🏀 BASKETBALL SCOREBOARD
  </span>
  <span style="color:#333; font-size:0.75rem; letter-spacing:0.1em;">
    OFFICIAL LIVE DISPLAY &nbsp;|&nbsp; AUTO-REFRESH EVERY 3s
  </span>
  <span style="font-size:0.75rem; color:#555;">
    {__import__('datetime').datetime.now().strftime('%H:%M:%S')}
  </span>
</div>
""", unsafe_allow_html=True)

# ─────────────────────────────────────────────
# MAIN SCOREBOARD
# ─────────────────────────────────────────────
st.markdown('<div class="big-scoreboard">', unsafe_allow_html=True)

# Possession arrows
poss_arrow_a = "🏀" if poss == "A" else ""
poss_arrow_b = "🏀" if poss == "B" else ""

# Scores row
sc1, sc_mid, sc2 = st.columns([2.5, 1, 2.5])

with sc1:
    st.markdown(f"""
    <div style="text-align:center;">
      <div class="big-team-name team-a-name">{ta.get('name','Team A')}</div>
      <div class="big-score score-a">{ta.get('score',0)}</div>
      <div style="margin-top:0.5rem; font-size:1.5rem;">{poss_arrow_a}</div>
    </div>
    """, unsafe_allow_html=True)

with sc_mid:
    st.markdown(f"""
    <div style="text-align:center; padding-top: 2rem;">
      <div class="vs-divider">VS</div>
    </div>
    """, unsafe_allow_html=True)

with sc2:
    st.markdown(f"""
    <div style="text-align:center;">
      <div class="big-team-name team-b-name">{tb.get('name','Team B')}</div>
      <div class="big-score score-b">{tb.get('score',0)}</div>
      <div style="margin-top:0.5rem; font-size:1.5rem;">{poss_arrow_b}</div>
    </div>
    """, unsafe_allow_html=True)

# Clock + Period row
st.markdown(f"""
<div style="text-align:center; margin: 1.5rem 0 1rem 0; border-top:1px solid #111; padding-top:1.5rem;">
  <div><span class="period-badge">{full_period}</span></div>
  <div class="{clock_class}" style="margin:0.8rem 0 0.3rem 0;">{state.get('game_clock','--:--')}</div>
  <div class="{shot_class}">SHOT CLOCK: {shot}s</div>
</div>
""", unsafe_allow_html=True)

st.markdown('</div>', unsafe_allow_html=True)

# ─────────────────────────────────────────────
# STATS ROW
# ─────────────────────────────────────────────
st.markdown("<div style='height:12px'></div>", unsafe_allow_html=True)

info_cols = st.columns(6)

# Team A fouls
with info_cols[0]:
    fouls_a = ta.get("fouls", 0)
    dots_a = "".join([f'<span class="foul-pip {"foul-danger" if i==4 else "foul-on"} "></span>' if i < fouls_a else f'<span class="foul-pip foul-off"></span>' for i in range(5)])
    foul_warn_a = '<div style="color:#ff4444;font-size:0.7rem;font-weight:700;margin-top:4px;">⛔ FOUL OUT</div>' if fouls_a >= 5 else ('<div style="color:#ffd700;font-size:0.7rem;margin-top:4px;">⚠️ Foul Trouble</div>' if fouls_a >= 3 else '')
    st.markdown(f"""
    <div class="info-panel">
      <div style="color:#6699ff;font-size:0.75rem;font-weight:700;text-transform:uppercase;letter-spacing:0.1em;margin-bottom:6px;">{ta.get('name','Team A')}</div>
      <div class="info-val">{fouls_a}</div>
      <div class="info-lbl">FOULS</div>
      <div style="margin-top:6px;">{dots_a}</div>
      {foul_warn_a}
    </div>
    """, unsafe_allow_html=True)

# Team A timeouts
with info_cols[1]:
    tos_a = ta.get("timeouts", 3)
    to_dots_a = "".join([f'<span class="timeout-pip {"to-on" if i < tos_a else "to-off"}"></span>' for i in range(3)])
    st.markdown(f"""
    <div class="info-panel">
      <div style="color:#6699ff;font-size:0.75rem;font-weight:700;text-transform:uppercase;letter-spacing:0.1em;margin-bottom:6px;">{ta.get('name','Team A')}</div>
      <div class="info-val" style="color:#ffd700;">{tos_a}</div>
      <div class="info-lbl">TIMEOUTS</div>
      <div style="margin-top:6px;">{to_dots_a}</div>
    </div>
    """, unsafe_allow_html=True)

# Score diff
with info_cols[2]:
    diff = ta.get("score",0) - tb.get("score",0)
    diff_str = f"+{diff}" if diff > 0 else (str(diff) if diff != 0 else "TIE")
    diff_col = "#00ff88" if diff > 0 else ("#ff4444" if diff < 0 else "#f5a623")
    lead_team = ta.get("name","A") if diff > 0 else (tb.get("name","B") if diff < 0 else "")
    st.markdown(f"""
    <div class="info-panel">
      <div class="info-val" style="color:{diff_col}; font-size:2rem;">{diff_str}</div>
      <div class="info-lbl">MARGIN</div>
      {f'<div style="color:{diff_col};font-size:0.75rem;margin-top:4px;font-weight:700;">{lead_team} leads</div>' if lead_team else ''}
    </div>
    """, unsafe_allow_html=True)

# Quarter
with info_cols[3]:
    running_status = '<span style="color:#00ff88;font-size:0.75rem;">▶ RUNNING</span>' if state.get("clock_running") else '<span style="color:#ff4444;font-size:0.75rem;">⏹ STOPPED</span>'
    st.markdown(f"""
    <div class="info-panel">
      <div class="info-val" style="color:#88aaff;">{q}</div>
      <div class="info-lbl">PERIOD</div>
      <div style="margin-top:4px;">{running_status}</div>
    </div>
    """, unsafe_allow_html=True)

# Team B timeouts
with info_cols[4]:
    tos_b = tb.get("timeouts", 3)
    to_dots_b = "".join([f'<span class="timeout-pip {"to-on" if i < tos_b else "to-off"}"></span>' for i in range(3)])
    st.markdown(f"""
    <div class="info-panel">
      <div style="color:#ff8888;font-size:0.75rem;font-weight:700;text-transform:uppercase;letter-spacing:0.1em;margin-bottom:6px;">{tb.get('name','Team B')}</div>
      <div class="info-val" style="color:#ffd700;">{tos_b}</div>
      <div class="info-lbl">TIMEOUTS</div>
      <div style="margin-top:6px;">{to_dots_b}</div>
    </div>
    """, unsafe_allow_html=True)

# Team B fouls
with info_cols[5]:
    fouls_b = tb.get("fouls", 0)
    dots_b = "".join([f'<span class="foul-pip {"foul-danger" if i==4 else "foul-on"}"></span>' if i < fouls_b else f'<span class="foul-pip foul-off"></span>' for i in range(5)])
    foul_warn_b = '<div style="color:#ff4444;font-size:0.7rem;font-weight:700;margin-top:4px;">⛔ FOUL OUT</div>' if fouls_b >= 5 else ('<div style="color:#ffd700;font-size:0.7rem;margin-top:4px;">⚠️ Foul Trouble</div>' if fouls_b >= 3 else '')
    st.markdown(f"""
    <div class="info-panel">
      <div style="color:#ff8888;font-size:0.75rem;font-weight:700;text-transform:uppercase;letter-spacing:0.1em;margin-bottom:6px;">{tb.get('name','Team B')}</div>
      <div class="info-val">{fouls_b}</div>
      <div class="info-lbl">FOULS</div>
      <div style="margin-top:6px;">{dots_b}</div>
      {foul_warn_b}
    </div>
    """, unsafe_allow_html=True)

# ─────────────────────────────────────────────
# EVENT TICKER
# ─────────────────────────────────────────────
st.markdown("<div style='height:8px'></div>", unsafe_allow_html=True)
events = state.get("events", [])
if events:
    ticker_items = " &nbsp;|&nbsp; ".join([f"[{e.get('period','?')} {e.get('time','--:--')}] {e.get('msg','')}" for e in events[:15]])
    st.markdown(f"""
    <div class="ticker">
      <span style="color:#f5a623;font-weight:700;font-size:0.8rem;">📋 LIVE EVENTS: </span>
      <span class="ticker-scroll" style="color:#666;">{ticker_items}</span>
    </div>
    """, unsafe_allow_html=True)

# ─────────────────────────────────────────────
# RECENT EVENTS LOG
# ─────────────────────────────────────────────
st.markdown("<div style='height:8px'></div>", unsafe_allow_html=True)
log_col, rules_col = st.columns([1.5, 1])

with log_col:
    st.markdown("""
    <div style="border:1px solid #111122; border-radius:12px; padding:0.8rem 1rem;">
      <div style="font-family:Orbitron,monospace; font-size:0.75rem; color:#f5a623; font-weight:700;
           letter-spacing:0.1em; text-transform:uppercase; margin-bottom:8px;">📋 Recent Events</div>
    """, unsafe_allow_html=True)
    cat_colors = {"score": "#f5a623", "foul": "#ff8888", "timeout": "#ffd700",
                  "clock": "#88aaff", "quarter": "#00ff88"}
    ev_html = ""
    for ev in events[:8]:
        col = cat_colors.get(ev.get("cat",""), "#666")
        ev_html += f'<div style="color:{col};font-size:0.78rem;padding:3px 0;border-bottom:1px solid #0a0a18;font-family:monospace;">[{ev.get("period","?")} {ev.get("time","--:--")}] {ev.get("msg","")}</div>'
    st.markdown(ev_html + "</div>", unsafe_allow_html=True)

with rules_col:
    st.markdown("""
    <div style="border:1px solid #111122; border-radius:12px; padding:0.8rem 1rem;">
      <div style="font-family:Orbitron,monospace; font-size:0.75rem; color:#f5a623; font-weight:700;
           letter-spacing:0.1em; text-transform:uppercase; margin-bottom:8px;">📖 FIBA Quick Rules</div>
      <div style="font-size:0.72rem; color:#555; line-height:1.7;">
        <span style="color:#888;">🏀 Shot clock:</span> <span style="color:#f5a623;">24 seconds</span> (reset on offensive rebound: 14s)<br>
        <span style="color:#888;">⏱ Period:</span> <span style="color:#88aaff;">4 × 10 minutes</span> (NBA: 4 × 12min)<br>
        <span style="color:#888;">🟡 Team fouls:</span> <span style="color:#ff8888;">4+ per period → bonus FTs</span><br>
        <span style="color:#888;">⛔ Personal fouls:</span> <span style="color:#ff8888;">5 fouls = fouled out</span><br>
        <span style="color:#888;">⏸ Timeouts:</span> <span style="color:#ffd700;">2 per half + 1 OT (FIBA)</span><br>
        <span style="color:#888;">🏃 3-sec rule:</span> Can't stay in paint &gt; 3 seconds<br>
        <span style="color:#888;">📐 Half-court:</span> Must advance within 8 seconds<br>
        <span style="color:#888;">🔄 Inbounding:</span> 5 seconds to pass the ball in<br>
        <span style="color:#888;">🔢 2 pts:</span> Inside arc &nbsp;|&nbsp; <span style="color:#888;">3 pts:</span> Outside arc<br>
        <span style="color:#888;">🎯 Free throw:</span> 1 pt each from the line
      </div>
    </div>
    """, unsafe_allow_html=True)

# ─────────────────────────────────────────────
# GAME OVER OVERLAY
# ─────────────────────────────────────────────
if state.get("game_over"):
    winner = ta.get("name","Team A") if ta.get("score",0) > tb.get("score",0) else tb.get("name","Team B")
    st.markdown(f"""
    <div class="game-over-banner" style="margin-top:1rem;">
      <div style="font-family:Orbitron,monospace; font-size:3rem; font-weight:900; color:#00ff88;">
        🏆 FINAL SCORE
      </div>
      <div style="font-size:5rem; font-family:Orbitron,monospace; font-weight:900; color:#fff; margin:0.5rem 0;">
        {ta.get('score',0)} — {tb.get('score',0)}
      </div>
      <div style="font-size:2rem; color:#f5a623; font-weight:900; text-transform:uppercase; letter-spacing:0.15em;">
        🥇 WINNER: {winner}
      </div>
    </div>
    """, unsafe_allow_html=True)

# Auto refresh
st.markdown("""
<script>
  setTimeout(function() { window.location.reload(); }, 3000);
</script>
""", unsafe_allow_html=True)
