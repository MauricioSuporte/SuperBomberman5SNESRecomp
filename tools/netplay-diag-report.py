#!/usr/bin/env python3
"""Summarize Super Bomberman 5 netplay diagnostics from host and guest.

Reads what a match leaves in saves/netplay/ when SNES_NET_DIAG=1 is set
(tools/netplay-diag.cmd does that):

  net_diag_<date>_<time>_<host|guest>_slot<N>.jsonl   (older builds: net_diag.jsonl)
  netplay_log.txt                                      (stderr of the game)

Usage:
  python tools/netplay-diag-report.py <file-or-folder> [<file-or-folder> ...]

Pass the host's and the guest's files (or the folders/zips they sent). The
report lists, per match file, the connection path, frame rate and slow frames,
admission stalls, the host-authority backlog (how far the guest trailed the
host), resends/refused rows, catch-up frames, and the notable log lines.
Nothing here changes netplay; it only reads the files.
"""
from __future__ import annotations

import io
import json
import os
import re
import sys
import zipfile
from dataclasses import dataclass, field

SLOW_FRAME_MS = 25.0     # one admitted frame taking longer than ~1.5 frames
LOW_FPS = 55.0           # admitted frames per second below this is visible stutter
LOG_PATTERNS = [
    ("lost", re.compile(r"host_authority guest LOST")),
    ("backlog", re.compile(r"host_authority backlog")),
    ("starvation", re.compile(r"delay_sync_starvation")),
    ("buffer", re.compile(r"host_authority guest buffer")),
    ("slow", re.compile(r"local_slow")),
    ("disconnect", re.compile(r"peer_disconnect")),
    ("desync", re.compile(r"desync", re.I)),
    ("refused", re.compile(r"match refused")),
    ("ice", re.compile(r"ICE FAILED")),
    ("match", re.compile(r"===== netplay match|snes_netplay: started")),
]


@dataclass
class Source:
    name: str
    text: str


@dataclass
class Episode:
    start_ms: int
    end_ms: int
    max_ms: int
    reason: str


@dataclass
class Report:
    name: str
    summary: dict = field(default_factory=dict)
    samples: list = field(default_factory=list)


def collect(paths: list[str]) -> list[Source]:
    out: list[Source] = []

    def add_file(name: str, data: bytes) -> None:
        base = os.path.basename(name).lower()
        if base.endswith(".jsonl") or base.endswith(".txt") or base.endswith(".log"):
            out.append(Source(name, data.decode("utf-8", "replace")))

    for p in paths:
        if os.path.isdir(p):
            for root, _dirs, files in os.walk(p):
                for f in sorted(files):
                    full = os.path.join(root, f)
                    if f.lower().endswith(".zip"):
                        with zipfile.ZipFile(full) as z:
                            for n in z.namelist():
                                add_file(f + ":" + n, z.read(n))
                    else:
                        with open(full, "rb") as fh:
                            add_file(full, fh.read())
        elif p.lower().endswith(".zip"):
            with zipfile.ZipFile(p) as z:
                for n in z.namelist():
                    add_file(p + ":" + n, z.read(n))
        else:
            with open(p, "rb") as fh:
                add_file(p, fh.read())
    return out


def parse_jsonl(src: Source) -> Report:
    rep = Report(src.name)
    for line in io.StringIO(src.text):
        line = line.strip()
        if not line.startswith("{"):
            continue
        try:
            obj = json.loads(line)
        except json.JSONDecodeError:
            continue
        if obj.get("type") == "summary":
            rep.summary = obj
        else:
            rep.samples.append(obj)
    return rep


def stall_episodes(samples: list[dict]) -> list[Episode]:
    eps: list[Episode] = []
    cur: Episode | None = None
    for s in samples:
        stalled = s.get("stall") not in (None, "ok", "none") and s.get("stall_ms", 0) >= 100
        if stalled:
            if cur is None:
                cur = Episode(s["t_ms"], s["t_ms"], s.get("stall_ms", 0), s.get("stall", "?"))
            cur.end_ms = s["t_ms"]
            cur.max_ms = max(cur.max_ms, s.get("stall_ms", 0))
        elif cur is not None:
            eps.append(cur)
            cur = None
    if cur is not None:
        eps.append(cur)
    return eps


def fmt_t(ms: int, t0: int) -> str:
    sec = max(0, ms - t0) / 1000.0
    return f"{int(sec // 60):02d}:{sec % 60:05.2f}"


def report_jsonl(rep: Report) -> None:
    sm, ss = rep.summary, rep.samples
    print(f"\n=== {rep.name}")
    if sm:
        role = "HOST" if sm.get("is_host") else "GUEST"
        print(f"  role={role} slot={sm.get('slot')} mode={sm.get('match')} delay={sm.get('input_delay')} "
              f"transport={sm.get('transport')} path={sm.get('ice_path')} nat={sm.get('ice_nat')} "
              f"turn_configured={sm.get('turn_configured')} relay_forced={sm.get('force_input_relay')}")
    if not ss:
        print("  (no samples)")
        return
    t0 = ss[0]["t_ms"]
    dur = (ss[-1]["t_ms"] - t0) / 1000.0
    ha = ss[-1].get("host_authority")
    print(f"  samples={len(ss)} duration={dur:.1f}s sim_tick_end={ss[-1].get('sim_tick')} "
          f"host_authority={ha if ha is not None else '? (older build)'}")

    paths = sorted({s.get("ice_path", "?") for s in ss})
    if len(paths) > 1:
        print(f"  ICE path changed during the match: {paths}")

    if "frames" in ss[-1]:
        rows = []
        prev = None
        for s in ss:
            if prev is not None:
                dt = (s["t_ms"] - prev["t_ms"]) / 1000.0
                if dt > 0:
                    rows.append((s, s.get("frames", 0) / dt))
            prev = s
        if rows:
            fps = [r[1] for r in rows]
            low = [r for r in rows if r[1] < LOW_FPS]
            slow = [s for s in ss if s.get("frame_ms_max", 0) >= SLOW_FRAME_MS]
            print(f"  admitted fps: avg={sum(fps)/len(fps):.1f} min={min(fps):.1f} "
                  f"intervals below {LOW_FPS:.0f}: {len(low)}/{len(rows)}")
            print(f"  frame_ms: avg={sum(s.get('frame_ms_avg', 0) for s in ss)/len(ss):.2f} "
                  f"max={max(s.get('frame_ms_max', 0) for s in ss):.1f} "
                  f"samples with a frame >= {SLOW_FRAME_MS:.0f} ms: {len(slow)}")
            for s, f in low[:12]:
                print(f"    {fmt_t(s['t_ms'], t0)} fps={f:.0f} stall={s.get('stall')} "
                      f"stall_ms={s.get('stall_ms')} backlog={s.get('backlog')} "
                      f"frame_ms_max={s.get('frame_ms_max')} pump_gap_max={s.get('pump_gap_max_ms')} "
                      f"rx_age={s.get('peer_rx_age_ms')}")
            if len(low) > 12:
                print(f"    ... {len(low) - 12} more")
    else:
        print("  (no frame timing: build without the 2026-10-22 diagnostics)")

    eps = stall_episodes(ss)
    if eps:
        print(f"  stall episodes >= 100 ms: {len(eps)}; worst {max(e.max_ms for e in eps)} ms")
        for e in sorted(eps, key=lambda e: -e.max_ms)[:10]:
            print(f"    {fmt_t(e.start_ms, t0)}..{fmt_t(e.end_ms, t0)} {e.reason} max {e.max_ms} ms")
    else:
        print("  stall episodes >= 100 ms: none")

    if "backlog" in ss[-1]:
        bl = [s.get("backlog", -1) for s in ss]
        known = [b for b in bl if b >= 0]
        if known:
            worst = max(known)
            at = next(s for s in ss if s.get("backlog") == worst)
            print(f"  backlog (frames the guest trails the host): max={worst} at {fmt_t(at['t_ms'], t0)} "
                  f"end={bl[-1]} samples>=16: {sum(1 for b in known if b >= 16)}")
        print(f"  resend_rows={ss[-1].get('resend_rows')} rows_dropped={ss[-1].get('rows_dropped')} "
              f"catchup_total={ss[-1].get('catchup_total')}")
    if "auth_buffer" in ss[-1]:
        buf = [s["auth_buffer"] for s in ss if s.get("auth_buffer", -1) >= 0]
        if buf:
            # Guest only: host rows it holds beyond the one it plays next --
            # frames of input lag added on top of the round trip.
            leads = sorted(s.get("remote_lead", 0) for s in ss if s.get("auth_buffer", -1) >= 0)
            print(f"  guest jitter buffer (rows): min={min(buf)} max={max(buf)} end={buf[-1]}; "
                  f"queued rows median={leads[len(leads) // 2]} max={leads[-1]}")
        print(f"  inputs sent early while waiting (guest)={ss[-1].get('presend', 0)} "
              f"guest input queue jumps (host)={ss[-1].get('input_jumps', 0)}")
    lead = [s.get("remote_lead", 0) for s in ss]
    print(f"  remote_lead: min={min(lead)} max={max(lead)} end={lead[-1]} "
          f"peer_rx_age_max={max(s.get('peer_rx_age_ms', 0) for s in ss)} ms "
          f"desync={any(s.get('desync') for s in ss)}")
    last = ss[-1]
    if last.get("stall") not in (None, "ok", "none") and last.get("stall_ms", 0) > 1000:
        print(f"  !! the file ENDS stalled: {last.get('stall')} for {last.get('stall_ms')} ms "
              f"(backlog={last.get('backlog')}, remote_lead={last.get('remote_lead')}) -- the frozen screen")


def report_log(src: Source) -> None:
    hits: dict[str, list[str]] = {}
    for line in src.text.splitlines():
        for key, pat in LOG_PATTERNS:
            if pat.search(line):
                hits.setdefault(key, []).append(line.strip())
                break
    print(f"\n=== {src.name} (log)")
    if not hits:
        print("  no netplay events")
        return
    for key, _pat in LOG_PATTERNS:
        lines = hits.get(key)
        if not lines:
            continue
        print(f"  {key}: {len(lines)}")
        show = lines if len(lines) <= 6 else lines[:3] + ["..."] + lines[-3:]
        for ln in show:
            print(f"    {ln[:220]}")


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__)
        return 2
    sources = collect(argv[1:])
    if not sources:
        print("no .jsonl / .txt files found")
        return 1
    for src in sources:
        if src.name.lower().endswith(".jsonl"):
            report_jsonl(parse_jsonl(src))
        else:
            report_log(src)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
