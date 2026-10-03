"""Compile + jalankan simulasi.cpp (kode library asli), lalu render grafik ke ../gambar/.

Jalankan dari folder ini:  python gambar.py   (butuh g++ dan matplotlib)
"""
import os
import subprocess
import sys
import tempfile

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

plt.rcParams.update({
    "figure.figsize": (8, 3.6), "figure.dpi": 100, "savefig.bbox": "tight", "savefig.pad_inches": 0.15,
    "figure.facecolor": "white", "axes.facecolor": "white", "savefig.facecolor": "white",
    "font.size": 10, "axes.titlesize": 11, "axes.titleweight": "bold", "axes.titlelocation": "left",
    "axes.spines.top": False, "axes.spines.right": False, "axes.edgecolor": "#9ca3af",
    "axes.grid": True, "grid.color": "#e5e7eb", "grid.linewidth": 0.8,
    "legend.frameon": False, "svg.fonttype": "path", "svg.hashsalt": "nothinx",
    "lines.linewidth": 1.8,
})
WARNA = {"utama": "#2563eb", "pembanding": "#dc2626", "ketiga": "#16a34a", "keempat": "#9333ea",
         "kelima": "#ea580c", "mentah": "#9ca3af", "target": "#111827"}

SINI = os.path.dirname(os.path.abspath(__file__))
KELUAR = os.path.join(SINI, "..", "gambar")


def koma(x, d=2):
    return f"{x:.{d}f}".replace(".", ",")


def jalankan():
    """Hasil: {bagian: [baris sebagai dict]}. Library header saja, jadi tanpa src/*.cpp."""
    with tempfile.TemporaryDirectory() as tmp:
        exe = os.path.join(tmp, "sim")
        hasil = subprocess.run(["g++", "-std=c++11", "-O2", "-I../test", "-I../../src", "simulasi.cpp", "-o", exe],
                               cwd=SINI, capture_output=True, text=True)
        if hasil.returncode:
            sys.exit(hasil.stderr)
        teks = subprocess.run([exe], check=True, capture_output=True, text=True).stdout
    data, kolom = {}, None
    for baris in teks.splitlines():
        if baris.startswith("# "):
            nama, kolom = baris[2:], None
            data[nama] = []
        elif kolom is None:
            kolom = baris.split(";")
        else:
            nilai = [v if i == 0 and not v.replace(".", "").isdigit() else float(v)
                     for i, v in enumerate(baris.split(";"))]
            data[nama].append(dict(zip(kolom, nilai)))
    return data


def simpan(fig, nama):
    fig.savefig(os.path.join(KELUAR, nama), format="svg", metadata={"Date": None})
    plt.close(fig)


def sinyal(d):
    deret, tabel = d["deret"], {b["filter"]: b for b in d["tabel"]}
    x = [b["sampel"] for b in deret]
    panel = [
        ("rata", "RataRataBergerak<8>", "RataRataBergerak<8>"),
        ("median", "FilterMedian<5>", "FilterMedian<5>"),
        ("ema", "FilterEMA(0.2)", "FilterEMA(0.2)"),
        ("kalman", "FilterKalman(5, 1)", "FilterKalman(5, 1)"),
        ("gabungan", "Median<5> + EMA(0.3)", "Median<5> + EMA(0.3)"),
    ]
    fig, axs = plt.subplots(3, 2, figsize=(8, 7.2), sharex=True, sharey=True)
    axs = axs.ravel()
    for ax in axs:
        ax.plot(x, [b["mentah"] for b in deret], color=WARNA["mentah"], linewidth=1.0)
        ax.plot(x, [b["nyata"] for b in deret], color=WARNA["target"], linewidth=1.0, linestyle="--")
        ax.set_ylim(-40, 160)
        ax.set_yticks([0, 50, 100])
    axs[0].set_title("Bacaan mentah", fontsize=10)
    axs[0].text(2, -32, "lonjakan ±200 terpotong", color=WARNA["mentah"], fontsize=9)
    axs[0].text(100, 70, "nilai sebenarnya", color=WARNA["target"], fontsize=9)
    for ax, (kol, kunci, judul) in zip(axs[1:], panel):
        ax.plot(x, [b[kol] for b in deret], color=WARNA["utama"])
        ax.set_title(f"{judul}   error RMS {koma(tabel[kunci]['rms_lonjakan'])}", fontsize=10)
    for ax in axs[4:]:
        ax.set_xlabel("Sampel")
    for ax in axs[::2]:
        ax.set_ylabel("Nilai")
    m, g = tabel["FilterMedian<5>"], tabel["Median<5> + EMA(0.3)"]
    lain = [tabel[k]["rms_lonjakan"] for k in ("RataRataBergerak<8>", "FilterEMA(0.2)", "FilterKalman(5, 1)")]
    fig.suptitle(f"Median membuang lonjakan (error {koma(m['rms_lonjakan'])}); rata-rata, EMA, dan Kalman "
                 f"tertarik ({koma(min(lain))}–{koma(max(lain))})",
                 x=0.02, ha="left", fontsize=11, fontweight="bold")
    fig.tight_layout()
    simpan(fig, "sinyal-filter.svg")


def tradeoff(d):
    t = d["tabel"]
    fig, ax = plt.subplots()
    for b in t:
        w = WARNA["mentah"] if b["filter"] == "Tanpa filter" else WARNA["utama"]
        ax.plot([b["tunda"]] * 2, [b["rms_noise"], b["rms_lonjakan"]], color=w, linewidth=1.0, alpha=0.6)
        ax.plot(b["tunda"], b["rms_lonjakan"], "o", color=w, markersize=7)
        ax.plot(b["tunda"], b["rms_noise"], "o", color=w, markersize=6, markerfacecolor="white")
    # Label langsung; geser yang berdekatan (rata-rata, EMA, Kalman).
    geser = {"RataRataBergerak<8>": (-7, 0, "right"), "FilterEMA(0.2)": (0, 16, "center"),
             "Tanpa filter": (7, -4, "left")}
    for b in t:
        dx, dy, ha = geser.get(b["filter"], (7, 0, "left"))
        ax.annotate(f"{b['filter']}\n{koma(b['rms_lonjakan'])}", (b["tunda"], b["rms_lonjakan"]),
                    xytext=(dx, dy), textcoords="offset points", ha=ha, va="center", fontsize=8.5)
    ax.text(12.5, 38, "○ hanya noise\n● noise + lonjakan", color=WARNA["target"], fontsize=8.5, va="center")
    ax.set_xlabel("Tunda sampai 90% perubahan (sampel)")
    ax.set_ylabel("Error RMS")
    ax.set_xlim(0, 16)
    ax.set_ylim(0, 48)
    g = next(b for b in t if b["filter"].startswith("Median<5> +"))
    ax.set_title(f"Median lalu EMA: error {koma(g['rms_lonjakan'])} walau ada lonjakan, tunda {int(g['tunda'])} sampel")
    simpan(fig, "galat-vs-tunda.svg")


def parameter(d):
    fig, ax = plt.subplots()
    for kunci, w, awalan in [("ema", WARNA["utama"], "α "), ("rata", WARNA["ketiga"], "N ")]:
        t = d[kunci]
        ax.plot([b["tunda"] for b in t], [b["rms_noise"] for b in t], "o-", color=w, markersize=4)
        for b in t:
            if b["tunda"] == 1:
                continue  # alpha 1 dan N 1 = tanpa filter, diberi label sendiri
            nilai = b["filter"] if isinstance(b["filter"], str) else f"{b['filter']:g}"
            ax.annotate(awalan + nilai.replace(".", ","), (b["tunda"], b["rms_noise"]),
                        xytext=(4, 5) if kunci == "ema" else (-4, -11), textcoords="offset points",
                        ha="left" if kunci == "ema" else "right", fontsize=8.5, color=w)
    ax.annotate("tanpa filter", (1, d["ema"][-1]["rms_noise"]), xytext=(6, 0), textcoords="offset points",
                va="center", fontsize=8.5, color=WARNA["mentah"])
    ax.text(30, 2.9, "FilterEMA(α)", color=WARNA["utama"], fontweight="bold")
    ax.text(30, 2.4, "RataRataBergerak<N>", color=WARNA["ketiga"], fontweight="bold")
    ax.set_xlabel("Tunda sampai 90% perubahan (sampel)")
    ax.set_ylabel("Error RMS (hanya noise)")
    ax.set_ylim(0, 5.6)
    e = next(b for b in d["ema"] if abs(b["filter"] - 0.05) < 1e-6)
    ax.set_title(f"Makin halus, makin lambat: α 0,05 menekan error ke {koma(e['rms_noise'])}, "
                 f"tapi tunda {int(e['tunda'])} sampel")
    simpan(fig, "parameter.svg")


def main():
    os.makedirs(KELUAR, exist_ok=True)
    d = jalankan()
    sinyal(d)
    tradeoff(d)
    parameter(d)


if __name__ == "__main__":
    main()
